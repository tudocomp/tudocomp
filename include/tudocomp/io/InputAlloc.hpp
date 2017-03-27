#pragma once

#include <tudocomp/io/InputRestrictedBuffer.hpp>

namespace tdc {namespace io {
    using InputAllocChunk = RestrictedBuffer;
    using InputAllocChunkHandle = std::shared_ptr<InputAllocChunk>;
    using InputAlloc = std::vector<InputAllocChunkHandle>;

    class InputAllocHandle {
        std::shared_ptr<InputAlloc> m_ptr;

        template<typename F>
        inline InputAllocChunkHandle create_buffer(F f) const {
            auto new_alloc = std::make_shared<InputAllocChunk>(
                InputAllocChunk { f() });

            m_ptr->push_back(new_alloc);
            return m_ptr->back();
        }

        class InputAllocChunkHandleKeepalive:
            public RestrictedBufferSourceKeepalive
        {
        public:
            InputAllocChunkHandleKeepalive(const InputAllocChunkHandle& handle):
                m_handle(handle) {}
            InputAllocChunkHandle m_handle;
        };

        inline InputAllocChunkHandle create_stream(
            const InputSource& src,
            size_t from,
            size_t to,
            InputRestrictions restrictions,
            std::vector<InputAllocChunkHandle*>& selection
        ) const {
            auto full_buffer = create_buffer([&]() {
                return RestrictedBuffer(src, 0, RestrictedBuffer::npos, restrictions);
            });

            if (from == 0 && to == RestrictedBuffer::npos) {
                return std::move(full_buffer);
            }

            DCHECK(false) << "early oops";

            // If src is a stream that got already created once, we need to
            // do something more complicted.
            if (src.is_stream() && !selection.empty()) {
                // Check for partial matches in the cache that can be sliced
                // into
                for (auto& eptr : selection) {
                    auto& e = **eptr;
                    if (e.restrictions() == restrictions) {
                        if (e.from() <= from && e.to() >= to) {
                            View v = e.view();

                            auto from_diff = from - e.from();
                            auto len = to - from;

                            v = v.substr(from_diff, len);

                            std::cout << "#! Reslicing existing allocation\n";
                            return create_buffer([&]() {
                                return RestrictedBuffer(
                                    InputSource(v),
                                    0,
                                    RestrictedBuffer::npos,
                                    restrictions,
                                    std::make_shared<InputAllocChunkHandleKeepalive>(
                                        *eptr
                                    )
                                );
                            });
                        }
                    }
                }

                // Finally check for unowned allocations that can be reused
                // instead of creating a new allocation
                for (auto& eptr : selection) {
                    auto& e = **eptr;
                    if (eptr->unique() && !(e.restrictions() == restrictions)) {
                        if (e.from() <= from && e.to() >= to) {
                            auto removed_eptr = InputAllocChunkHandle();
                            eptr->swap(removed_eptr);

                            RestrictedBuffer buffer = std::move(*removed_eptr);

                            std::cout << "Existing restriction: " << buffer.restrictions() << "\n";
                            std::cout << "New      restriction: " << restrictions << "\n";

                            auto buffer2 = RestrictedBuffer::restrict(
                                std::move(buffer), from, to, restrictions);

                            std::cout << "#! Reusing existing allocation\n";
                            return create_buffer([&]() {
                                return std::move(buffer2);
                            });
                        }
                    }
                }

                if (!selection.empty()) {
                    std::cerr << "[Err]: This should not be reached in normal situations\n";
                    std::cerr << "This from/to:     " << from << ", " << to << "\n";
                    std::cerr << "This restriction: " << restrictions << "\n";

                    std::cerr << "Existing allocations:\n";
                    for (auto& eptr : selection) {
                        if (*eptr) {
                            auto& e = **eptr;
                            std::cerr << "  from/to:     " << e.from() << ", " << e.to() << "\n";
                            std::cerr << "  restriction: " << e.restrictions() << "\n";
                            std::cerr << "  use count:   " << eptr->use_count() << "\n";
                        }
                    }

                    DCHECK(false) << "needs to do special handling";
                }
            }

            DCHECK(false) << "Whoops";
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

                return create_buffer([&]() {
                    return RestrictedBuffer(src, from, to, restrictions);
                });
            }
        }

        inline InputAllocHandle(): m_ptr(std::make_shared<InputAlloc>()) {}
    };
}}
