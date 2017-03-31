#pragma once

namespace tdc {namespace io {
    inline size_t Input::Variant::size() const {
        if (source().is_view()) {

            auto actual_to = to();
            if (to_unknown()) {
                actual_to = source().view().size();
            }
            if(escaped_size_unknown()) {
                if(restrictions().has_no_restrictions()) {
                    set_escaped_size(actual_to - from());
                } else {
                    auto strm = as_stream();
                    size_t i = 0;
                    char c;
                    while (strm.get(c)) {
                        ++i;
                    }

                    set_escaped_size(i);
                }
            }

        } else if (source().is_file()) {

            auto actual_to = to();
            if (to_unknown()) {
                actual_to = read_file_size(source().file());
            }
            if(escaped_size_unknown()) {
                if(restrictions().has_no_restrictions()) {
                    set_escaped_size(actual_to - from());
                } else {
                    auto strm = as_stream();
                    size_t i = 0;
                    char c;
                    while (strm.get(c)) {
                        ++i;
                    }

                    set_escaped_size(i);
                }
            }

        } else if (source().is_stream()) {
            if(escaped_size_unknown()) {
                auto p = alloc().find_or_construct(
                    InputSource(source().stream()), from(), to(), restrictions());
                set_escaped_size(p->view().size());
                unregister_alloc_chunk_handle(p);
            }

        }

        return escaped_size();
    }

    inline std::shared_ptr<Input::Variant> Input::Variant::slice(
        size_t from, size_t to = npos) const
    {
        size_t new_from = this->from() + from;
        size_t new_to;
        if (to == npos) {
            new_to = npos;
        } else {
            new_to = this->from() + to;
        }

        return std::shared_ptr<Variant>(new Variant(*this, new_from, new_to));
    }

    inline std::shared_ptr<Input::Variant> Input::Variant::restrict(
        const InputRestrictions& rest) const
    {
        return std::shared_ptr<Variant>(new Variant(*this, restrictions() | rest));
    }

}}
