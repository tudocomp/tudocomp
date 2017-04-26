#pragma once

#include <memory>

#include <tudocomp/compressors/esp/TypedBlock.hpp>

namespace tdc {namespace esp {
    template<typename T>
    class DebugContextBase {
        struct Data {
            std::ostream* m_out;
            bool print_enabled = true;
            bool print_early = true;
            std::vector<std::function<void(std::ostream&)>> m_print_instructions;
            T m_child_data;
        };
        std::shared_ptr<Data> m_data;

        template<typename U>
        friend class DebugContextBase;
    protected:
        template<typename F>
        void with_child(F f) {
            if (m_data) {
                f(m_data->m_child_data);
            }
        }

        template<typename U>
        static std::vector<size_t> cast_vec(const U& v) {
            std::vector<size_t> r;
            r.reserve(v.size());
            for (auto e : v) {
                r.push_back(e);
            }
            return r;
        }

        template<typename F>
        void print(F f) {
            m_data->m_print_instructions.push_back(f); // TODO: Could add wrappers here
            if (m_data->print_enabled && m_data->print_early) {
                m_data->m_print_instructions.back()(*(m_data->m_out));
            }

            /*
            print([m_data](std::ostream& o) {
            })
             */
        }
        DebugContextBase(std::ostream& o, bool p_en, bool p_ea):
            m_data(std::make_shared<Data>(Data { &o, p_en, p_ea })) {}
        template<typename U>
        DebugContextBase(DebugContextBase<U>& parent):
            DebugContextBase(*(parent.m_data->m_out),
                             parent.m_data->print_enabled,
                             parent.m_data->print_early) {}
    public:
        DebugContextBase(const DebugContextBase& other):
            m_data(other.m_data) {}

        void print_all() const {
            if (m_data->print_enabled & !m_data->print_early) {
                for (auto& f : m_data->m_print_instructions) {
                    f(*(m_data->m_out));
                }
            }
        }
    };

////////////////////////////////////////////////////////////////////////////////

    struct DebugMetablockContextData {
        size_t alphabet_size;
        std::vector<size_t> metablock;
        size_t type;
        size_t offset;
        std::vector<TypedBlock> unadjusted_blocks;

        std::vector<size_t> mb2_initial;
        std::vector<std::shared_ptr<std::vector<size_t>>> mb2_reduce_to_6_steps;
        std::vector<std::shared_ptr<std::vector<size_t>>> mb2_reduce_to_3_steps;

        std::vector<size_t> mb2_high_landmarks;
        std::vector<size_t> mb2_high_and_low_landmarks;
    };
    class DebugMetablockContext: public DebugContextBase<DebugMetablockContextData> {
    public:
        DebugMetablockContext(std::ostream& o, bool p_en, bool p_ea, size_t alphabet_size):
            DebugContextBase(o, p_en, p_ea)
        {
            with_child([&] (auto& m_data) {
                m_data.alphabet_size = alphabet_size;
            });
        }
        template<typename U>
        DebugMetablockContext(DebugContextBase<U>& parent, size_t alphabet_size):
            DebugContextBase(parent)
        {
            with_child([&] (auto& m_data) {
                m_data.alphabet_size = alphabet_size;
            });
        }

        template<typename T>
        void init(size_t type, const T& string, size_t offset) {
            with_child([&] (auto& m_data) {
                m_data.type = type;
                m_data.offset = offset;
                m_data.metablock = cast_vec(string);

                this->print([m_data = &m_data](std::ostream& o) {
                    o << "    type: " << m_data->type
                    << ", offset: " << m_data->offset
                    << ", metablock: " << vec_to_debug_string(m_data->metablock)
                    << "\n";
                });
            });
        }

        void block(size_t width, size_t type) {
            with_child([&] (auto& m_data) {
                auto b = TypedBlock { uint8_t(width), uint8_t(type) };
                m_data.unadjusted_blocks.push_back(b);

                this->print([m_data = &m_data, b](std::ostream& o) {
                    o << "      block: " << b << "\n";
                });
            });
        }

        template<typename T>
        void mb2_initial(const T& buf) {
            with_child([&] (auto& m_data) {
                m_data.mb2_initial = cast_vec(buf);

                this->print([m_data = &m_data](std::ostream& o) {
                    o << "      Alphabet reduction initial:\n"
                    << "        " << vec_to_debug_string(m_data->mb2_initial) << "\n";
                });
            });
        }

        void mb2_reduce_to_6_start() {
            with_child([&] (auto& m_data) {
                this->print([m_data = &m_data](std::ostream& o) {
                    o << "      Reduce to 6:\n";
                });
            });
        }

        template<typename T>
        void mb2_reduce_to_6_step(const T& buf) {
            with_child([&] (auto& m_data) {
                auto p = std::make_shared<std::vector<size_t>>(cast_vec(buf));
                m_data.mb2_reduce_to_6_steps.push_back(p);

                this->print([m_data = &m_data, p](std::ostream& o) {
                    o << "        " << vec_to_debug_string(*p) << "\n";
                });
            });
        }

        void mb2_reduce_to_3_start() {
            with_child([&] (auto& m_data) {
                this->print([m_data = &m_data](std::ostream& o) {
                    o << "      Reduce to 3:\n";
                });
            });
        }

        template<typename T>
        void mb2_reduce_to_3_step(const T& buf) {
            with_child([&] (auto& m_data) {
                auto p = std::make_shared<std::vector<size_t>>(cast_vec(buf));
                m_data.mb2_reduce_to_3_steps.push_back(p);

                this->print([m_data = &m_data, p](std::ostream& o) {
                    o << "        " << vec_to_debug_string(*p) << "\n";
                });
            });
        }

        template<typename T>
        void mb2_high_landmarks(const T& buf) {
            with_child([&] (auto& m_data) {
                m_data.mb2_high_landmarks = cast_vec(buf);

                this->print([m_data = &m_data](std::ostream& o) {
                    o << "      High landmarks:\n"
                    << "        " << vec_to_debug_string(m_data->mb2_high_landmarks) << "\n";
                });
            });
        }

        template<typename T>
        void mb2_high_and_low_landmarks(const T& buf) {
            with_child([&] (auto& m_data) {
                m_data.mb2_high_and_low_landmarks = cast_vec(buf);

                this->print([m_data = &m_data](std::ostream& o) {
                    o << "      High and low landmarks:\n"
                    << "        " << vec_to_debug_string(m_data->mb2_high_and_low_landmarks) << "\n";
                });
            });
        }
    };

    struct DebugRoundContextData {
        size_t number;
        std::vector<size_t> string;
        size_t root_node = 0;
        bool empty = false;
        std::vector<DebugMetablockContext> metablocks;
        size_t alphabet_size;
        std::vector<TypedBlock> adjusted_blocks;
        std::vector<std::shared_ptr<std::pair<std::vector<size_t>, size_t>>> slice_symbol_map;
    };
    class DebugRoundContext: public DebugContextBase<DebugRoundContextData> {
        using Data = DebugRoundContextData;
        std::shared_ptr<Data> m_data;
    public:
        DebugRoundContext(std::ostream& o, bool p_en, bool p_ea):
            DebugContextBase(o, p_en, p_ea),
            m_data(std::make_shared<Data>(Data {})) {}
        template<typename U>
        DebugRoundContext(DebugContextBase<U>& parent):
            DebugContextBase(parent),
            m_data(std::make_shared<Data>(Data {})) {}

        void init(size_t number,
                  const std::vector<size_t>& string,
                  size_t alphabet_size) {
            m_data->number = number;
            m_data->string = string;
            m_data->alphabet_size = alphabet_size;

            print([m_data = m_data](std::ostream& o) {
                o << "\n[Round #" << m_data->number << "]:\n"
                  << "  " << vec_to_debug_string(m_data->string) << "\n";
                o << "  Alphabet size: " << m_data->alphabet_size << "\n";
            });
        }

        void last_round(size_t rn, bool empty) {
            m_data->root_node = rn;
            m_data->empty = empty;
            print([m_data = m_data](std::ostream& o) {
                if (m_data->empty) {
                    o << "  DONE, empty input\n";
                } else {
                    o << "  DONE, root node: " << m_data->root_node << "\n";
                }
            });
        }

        DebugMetablockContext metablock() {
            DebugMetablockContext m_data_child(*this, m_data->alphabet_size);
            m_data->metablocks.push_back(m_data_child);

            print([m_data_child](std::ostream& o) {
                m_data_child.print_all();
            });

            return m_data_child;
        }

        void adjusted_blocks(const ConstGenericView<TypedBlock>& buf) {
            m_data->adjusted_blocks = buf;

            print([m_data = m_data](std::ostream& o) {
                o << "  Adjusted blocks:\n";
                for (auto b : m_data->adjusted_blocks) {
                    o << "    block: " << b << "\n";
                }
            });
        }

        void slice_symbol_map_start() {
            print([m_data = m_data](std::ostream& o) {
                o << "  Slice-Symbol map:\n";
            });
        }

        template<typename T>
        void slice_symbol_map(const T& slice, size_t symbol) {
            auto p = std::make_shared<std::pair<std::vector<size_t>, size_t>>(
                std::pair<std::vector<size_t>, size_t> {
                    cast_vec(slice),
                    symbol,
                }
            );
            m_data->slice_symbol_map.push_back(p);

            print([m_data = m_data, p](std::ostream& o) {
                o << "    "
                  << vec_to_debug_string(p->first) << " -> "
                  << p->second << "\n";
            });
        }
    };

    struct DebugContextData {
        std::string input;
        std::vector<DebugRoundContext> rounds;
        bool empty;
        size_t root_node;
        size_t encode_max_value;
        size_t encode_max_value_bits;
        size_t encode_root_node;
        std::vector<std::shared_ptr<std::vector<size_t>>> encode_slp;
    };
    class DebugContext: public DebugContextBase<DebugContextData> {
        using Data = DebugContextData;
        std::shared_ptr<Data> m_data;
    public:
        DebugContext(std::ostream& o, bool p_en, bool p_ea):
            DebugContextBase(o, p_en, p_ea),
            m_data(std::make_shared<Data>(Data {})) {}

        void input_string(string_ref s) {
            m_data->input = s;

            print([m_data = m_data](std::ostream& o) {
                o << "\n[Input]:\n\"" << m_data->input << "\"\n";
            });
        }

        void generate_grammar(bool empty, size_t root_node) {
            m_data->empty = empty;
            m_data->root_node = root_node;

            print([m_data = m_data](std::ostream& o) {
                o << "\n[Grammar]:\n"
                  << "  Is empty: " << (m_data->empty? "yes" : "no") << "\n"
                  << "  Root node: " << m_data->root_node << "\n";
            });
        }

        void encode_start() {
            print([m_data = m_data](std::ostream& o) {
                o << "\n[Encode]:\n";
            });
        }

        void encode_max_value(size_t value, size_t bits) {
            m_data->encode_max_value = value;
            m_data->encode_max_value_bits = bits;

            print([m_data = m_data](std::ostream& o) {
                o << "  Max value: " << m_data->encode_max_value << "\n";
                o << "  Bits: " << m_data->encode_max_value_bits << "\n";
            });
        }

        void encode_root_node(size_t node) {
            m_data->encode_root_node = node;

            print([m_data = m_data](std::ostream& o) {
                o << "  Root node: " << m_data->encode_root_node << "\n";
            });
        }

        void encode_rule_start() {
            print([m_data = m_data](std::ostream& o) {
                o << "\n  [SLP]:\n";
            });
        }

        template<typename T>
        void encode_rule(const T& rule) {
            auto p = std::make_shared<std::vector<size_t>>(cast_vec(rule));
            m_data->encode_slp.push_back(p);

            print([m_data = m_data, p](std::ostream& o) {
                o << "    " << vec_to_debug_string(*p) << "\n";
            });
        }

        DebugRoundContext round() {
            DebugRoundContext m_data_child(*this);
            m_data->rounds.push_back(m_data_child);

            print([m_data_child](std::ostream& o) {
                m_data_child.print_all();
            });

            return m_data_child;
        }
    };
}}
