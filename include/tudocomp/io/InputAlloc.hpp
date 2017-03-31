#pragma once

#include <tudocomp/io/InputRestrictedBuffer.hpp>

/// \cond INTERNAL
namespace tdc {namespace io {
    /// Collection of types for storing all needed allocations
    /// of input data done by a collection of related Input Instances.
    ///
    /// The basic idea is that of a pool of existing allocations
    /// shared through a shared_ptr in the background,
    /// and used for lookups each time an allocations needs to be accessed.

    class InputAllocChunk;
    using InputAllocChunkHandle = std::shared_ptr<InputAllocChunk>;
    using InputAlloc = std::vector<InputAllocChunkHandle>;

    /// Abstract interface for an allocation
    /// from input positions `from` to `to`.
    ///
    /// The data contained in it is not the original one,
    /// but modified according to the InputRestrictions instance used.
    class InputAllocChunk {
        size_t m_from;
        size_t m_to;
        std::weak_ptr<InputAlloc> m_alloc;
    public:
        inline InputAllocChunk(size_t from, size_t to, std::weak_ptr<InputAlloc> alloc):
            m_from(from), m_to(to), m_alloc(alloc) {
                DCHECK(m_alloc.lock());
            }
        inline size_t from() const {
            return m_from;
        }
        inline size_t to() const {
            return m_to;
        }
        inline std::weak_ptr<InputAlloc> alloc() const {
            return m_alloc;
        }

        inline virtual ~InputAllocChunk() {}
        virtual const InputRestrictions& restrictions() const = 0;
        virtual const InputSource& source() const = 0;
        virtual View view() const = 0;
        virtual RestrictedBuffer unwrap() && = 0;
        virtual void debug_print_content() const = 0;
    };

    /// Allocation where the data is backed by an actual owned buffer.
    class InputAllocChunkOwned: public InputAllocChunk {
        RestrictedBuffer m_buffer;

    public:
        InputAllocChunkOwned(RestrictedBuffer&& buffer,
                             size_t from,
                             size_t to,
                             std::weak_ptr<InputAlloc> alloc):
            InputAllocChunk(from, to, alloc),
            m_buffer(std::move(buffer)) {}
        inline virtual const InputRestrictions& restrictions() const {
            return m_buffer.restrictions();
        }
        inline virtual const InputSource& source() const {
            return m_buffer.source();
        }
        inline virtual View view() const {
            return m_buffer.view();
        }
        inline virtual RestrictedBuffer unwrap() && {
            return std::move(m_buffer);
        }
        inline virtual void debug_print_content() const {
            DVLOG(2) << "  buf:  " << m_buffer.view().size() << "\n";
        }
    };

    /// Allocation where the data is just a view into
    /// another allocation
    class InputAllocChunkReferenced: public InputAllocChunk {
        View m_view;
        std::shared_ptr<InputAllocChunk> m_parent;
    public:
        inline InputAllocChunkReferenced(
            View view,
            size_t from,
            size_t to,
            const std::shared_ptr<InputAllocChunk>& parent
        ):
            InputAllocChunk(from, to, parent->alloc()),
            m_view(view),
            m_parent(parent) {}
        inline virtual const InputRestrictions& restrictions() const {
            return m_parent->restrictions();
        }
        inline virtual const InputSource& source() const {
            return m_parent->source();
        }
        inline virtual View view() const {
            return m_view;
        }
        inline virtual RestrictedBuffer unwrap() && {
            throw std::runtime_error("This is only a view");
        }
        inline virtual void debug_print_content() const {
            DVLOG(2) << "  parent: " << std::hex << size_t(&*m_parent) << std::dec << "\n";
        }
    };

    /// Copyable handle for a list of active allocations.
    class InputAllocHandle {
        std::shared_ptr<InputAlloc> m_ptr;

        inline void debug_print_content() const {
            auto& vec = *m_ptr;

            DVLOG(2) << "Alloc registry:\n";
            for (auto& e : vec) {
                DCHECK(e);
                auto& ee = *e;
                DVLOG(2) << "  self: " << std::hex << size_t(&ee) << std::dec << "\n";
                DVLOG(2) << "  kind: " << ee.source() << "\n";
                DVLOG(2) << "  from: " << ee.from() << "\n";
                if (ee.to() == RestrictedBuffer::npos) {
                    DVLOG(2) << "  to:   <npos>" << "\n";
                } else {
                    DVLOG(2) << "  to:   " << ee.to() << "\n";
                }
                DVLOG(2) << "  rest: " << ee.restrictions() << "\n";
                DVLOG(2) << "  refs: " << e.use_count() << "\n";
                ee.debug_print_content();
                DVLOG(2) << "\n";
            }
        }

        template<typename F>
        inline InputAllocChunkHandle create_buffer(F f) const {
            DCHECK(m_ptr);
            {
                auto new_alloc = std::make_shared<InputAllocChunkOwned>(f(m_ptr));
                m_ptr->push_back(new_alloc);
            }

            debug_print_content();
            return m_ptr->back();
        }

        inline InputAllocChunkHandle create_ref(InputAllocChunkReferenced&& v) const {
            {
                auto new_alloc = std::make_shared<InputAllocChunkReferenced>(std::move(v));
                m_ptr->push_back(new_alloc);
            }

            debug_print_content();
            return m_ptr->back();
        }

        inline InputAllocChunkHandle create_stream(
            const InputSource& src,
            size_t from,
            size_t to,
            InputRestrictions restrictions,
            std::vector<InputAllocChunkHandle*>& selection
        ) const {
            InputAllocChunkHandle* parent_ptr = nullptr;

            // Try to find the one full copy of the stream we should maximally have
            for (auto ch_ptr : selection)  {
                auto& ch = *ch_ptr;
                if (ch->from() == 0 && ch->to() == RestrictedBuffer::npos) {
                    parent_ptr = ch_ptr;
                    break;
                }
            }

            // If there isn't one yet, create it.
            if (parent_ptr == nullptr) {
                create_buffer([&](std::weak_ptr<InputAlloc> ptr) {
                    return InputAllocChunkOwned {
                        RestrictedBuffer(src,
                                         0,
                                         RestrictedBuffer::npos,
                                         restrictions),
                        0,
                        RestrictedBuffer::npos,
                        ptr,
                    };
                });
                parent_ptr = &(m_ptr->back());
            }

            DVLOG(2) << "After parent create:\n";
            debug_print_content();

            // If there is one, but it differs in restrictions,
            // ensure its unique and change the restrictions
            auto& parent = *parent_ptr;
            if (!(parent->restrictions() == restrictions)) {
                CHECK(parent.unique())
                    << "Attempt to access stream `Input` in a way "
                    << "that would need to create a copy of the data.";

                auto buf = std::move(*parent).unwrap();
                parent = std::make_unique<InputAllocChunkOwned>(
                    InputAllocChunkOwned {
                        std::move(buf).change_restrictions(restrictions),
                        0,
                        RestrictedBuffer::npos,
                        m_ptr,
                    }
                );
            }

            DVLOG(2) << "After parent restrict:\n";
            debug_print_content();

            // If the whole range is requested, return parent directly
            if (from == 0 && to == RestrictedBuffer::npos) {
                return parent;
            }

            // Else create a slice into it.

            // First, calculate offset in escaoed buffer:
            size_t escaped_from = 0;
            size_t escaped_to = 0;

            {
                if(parent->restrictions().has_no_escape_restrictions()) {
                    escaped_from = from;
                    escaped_to = to;
                } else {
                    uint8_t escape_byte = EscapeMap(parent->restrictions()).escape_byte();
                    auto v = parent->view();

                    size_t i = 0;
                    size_t unescaped_i = 0;

                    for (; i < v.size(); i++) {
                        if (unescaped_i == from) {
                            escaped_from = i;
                            break;
                        }
                        if (v[i] != escape_byte) {
                            unescaped_i++;
                        }
                    }

                    if (to != RestrictedBuffer::npos) {
                        for (; i < v.size(); i++) {
                            if (unescaped_i == to) {
                                escaped_to = i;
                                break;
                            }
                            if (v[i] != escape_byte) {
                                unescaped_i++;
                            }
                        }
                    } else {
                        escaped_to = RestrictedBuffer::npos;
                    }
                }

                if (restrictions.null_terminate()) {
                    CHECK(escaped_to == RestrictedBuffer::npos)
                        << "Can not yet slice while adding a null terminator";
                }
            }

            DVLOG(2) << "After ref create:\n";
            return create_ref(InputAllocChunkReferenced {
                parent->view().slice(escaped_from, escaped_to),
                from,
                to,
                parent
            });
        }

        inline void cleanup_empty() {
            auto& vec = *m_ptr;

            InputAlloc new_vec;

            for (auto p : vec) {
                if (p) {
                    new_vec.push_back(p);
                }
            }

            vec = new_vec;
        }

        inline void remove(InputAllocChunkHandle handle) {
            for (auto& ptr : *m_ptr) {
                if (ptr == handle) {
                    ptr.reset();
                }
            }
            cleanup_empty();
        }
    public:
        /// Lookup or create a allocation
        inline InputAllocChunkHandle find_or_construct(
            const InputSource& src,
            size_t from,
            size_t to,
            InputRestrictions restrictions) const
        {
            auto pred = [&](const InputSource& e) -> bool {
                return e == src;
            };

            std::vector<InputAllocChunkHandle*> selection;
            for (auto& eptr : *m_ptr) {
                if (eptr) {
                    auto& e = *eptr;
                    if (pred(e.source())) {
                        selection.push_back(&eptr);
                    }
                }
            }

            // Check for exact matches in the cache
            for (auto& eptr : selection) {
                auto& iac = **eptr;
                auto& e = iac;
                if (e.from() == from && e.to() == to && e.restrictions() == restrictions) {
                    return *eptr;
                }
            }

            // Else we need to allocate a buffer:
            if (src.is_stream()) {
                // Streams source are complicated,
                // because wen need to rember the first allocation rather
                // that creating them anew as needed

                return create_stream(src, from, to, restrictions, selection);
            } else {
                // File or View sources can be created arbitrarily:

                return create_buffer([&](std::weak_ptr<InputAlloc> ptr) {
                    return InputAllocChunkOwned {
                        RestrictedBuffer(src, from, to, restrictions),
                        from,
                        to,
                        ptr,
                    };
                });
            }
        }

        inline InputAllocHandle(): m_ptr(std::make_shared<InputAlloc>()) {}
        inline InputAllocHandle(std::weak_ptr<InputAlloc> weak) {
            DCHECK(!weak.expired());
            m_ptr = weak.lock();
            DCHECK(m_ptr);
        }

        friend void unregister_alloc_chunk_handle(InputAllocChunkHandle handle);
    };

    /// Unregister a allocation from the pool again to free memory.
    ///
    /// This will leave the buffer for a input stream intact,
    /// as it is the only one that can not be recreated at a latter point
    /// in time.
    inline void unregister_alloc_chunk_handle(InputAllocChunkHandle handle) {
        if (handle) {
            // If its a stream root handle then we keep it

            if (handle->source().is_stream()) {
                if (handle->from() == 0 && handle->to() == RestrictedBuffer::npos) {
                    return;
                }
            }

            auto tmp = InputAllocHandle(handle->alloc());
            tmp.remove(handle);
        }
    }
}}
/// \endcond
