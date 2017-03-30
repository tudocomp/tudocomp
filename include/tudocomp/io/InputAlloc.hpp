#pragma once

#include <tudocomp/io/InputRestrictedBuffer.hpp>

namespace tdc {namespace io {

    class InputAllocChunk;
    using InputAllocChunkHandle = std::shared_ptr<InputAllocChunk>;
    using InputAlloc = std::vector<InputAllocChunkHandle>;

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
    };

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
    };

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
    };

    class InputAllocHandle {
        std::shared_ptr<InputAlloc> m_ptr;

        template<typename F>
        inline InputAllocChunkHandle create_buffer(F f) const {
            DCHECK(m_ptr);
            auto new_alloc = std::make_shared<InputAllocChunkOwned>(f(m_ptr));

            m_ptr->push_back(new_alloc);
            return m_ptr->back();
        }

        inline InputAllocChunkHandle create_ref(InputAllocChunkReferenced&& v) const {
            auto new_alloc = std::make_shared<InputAllocChunkReferenced>(std::move(v));

            m_ptr->push_back(new_alloc);
            return m_ptr->back();
        }

        inline InputAllocChunkHandle create_stream(
            const InputSource& src,
            size_t from,
            size_t to,
            InputRestrictions restrictions,
            std::vector<InputAllocChunkHandle*>& selection
        ) const {

            InputAllocChunkHandle parent;
            bool needs_restriction_change = false;
            bool is_unique = false;

            // Check if there is a 0 ... npos parent already,
            // and try to pick the one with matching restrictions
            for (auto& eptr : selection) {
                auto& e = **eptr;
                if (e.from() == 0 && e.to() == RestrictedBuffer::npos) {
                    if (e.restrictions() == restrictions) {
                        needs_restriction_change = false;
                        is_unique = eptr->unique();
                        parent = *eptr;
                        break;
                    } else {
                        needs_restriction_change = true;
                        is_unique = eptr->unique();
                        parent = *eptr;
                    }
                }
            }

            if (!parent) {
                // Create a parent
                parent = create_buffer([&](std::weak_ptr<InputAlloc> ptr) {
                    return InputAllocChunkOwned {
                        RestrictedBuffer(src, 0, RestrictedBuffer::npos, restrictions),
                        0,
                        RestrictedBuffer::npos,
                        ptr,
                    };
                });
                needs_restriction_change = false;
                is_unique = true;
            }

            CHECK(is_unique) << "TODO: Copy non-unique cached value";

            // change restrictions if needed
            if (needs_restriction_change /* && is_unique */) {
                auto f = parent->from();
                auto t = parent->to();

                auto buf = std::move(*parent).unwrap();

                parent = create_buffer([&](std::weak_ptr<InputAlloc> ptr) {
                    return InputAllocChunkOwned {
                        std::move(buf).change_restrictions(restrictions),
                        f,
                        t,
                        ptr,
                    };
                });
            }

            // If this is all we need, return it directly
            if (from == 0 && to == RestrictedBuffer::npos) {
                return std::move(parent);
            }

            // TODO: Create sub object pointing into parent

            size_t escaped_from = 0;
            size_t escaped_to = 0;

            {
                if(parent->restrictions().has_no_restrictions()) {
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
            }

            std::cout << "Current from: " << from << ", to: " << to << "\n";
            std::cout << "Escaped from: " << escaped_from << ", to: " << escaped_to << "\n";

            return create_ref(InputAllocChunkReferenced {
                parent->view().slice(escaped_from, escaped_to),
                from,
                to,
                parent
            });
        }

    public:
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
                    std::cout << "#! Looked at cached value\n";
                    if (pred(e.source())) {
                        selection.push_back(&eptr);
                        std::cout << "#! Possible cached value\n";
                    }
                }
            }

            // Check for exact matches in the cache
            for (auto& eptr : selection) {
                auto& iac = **eptr;
                auto& e = iac;
                std::cout << "#! Finding existing allocation\n";
                if (e.from() == from && e.to() == to && e.restrictions() == restrictions) {
                    std::cout << "#! FOUND existing allocation\n";
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

        inline InputAllocHandle(): m_ptr(std::make_shared<InputAlloc>()) {}
        inline InputAllocHandle(std::weak_ptr<InputAlloc> weak) {
            DCHECK(!weak.expired());
            std::cerr << "All ok\n";
            m_ptr = weak.lock();
            DCHECK(m_ptr);
        }
    };
}}
