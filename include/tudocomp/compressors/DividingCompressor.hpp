#pragma once

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/CreateAlgorithm.hpp>

namespace tdc {

struct DivisionDividingStrategy: public Algorithm {
    inline static Meta meta() {
        Meta m("dividing_strategy", "division");
        m.option("n").dynamic(4);
        return m;
    }

    using Algorithm::Algorithm;

    inline std::vector<size_t> split_at(Input& inp) const {
        auto size = inp.size();
        auto n = env().option("n").as_integer();
        size_t delta = size / n;
        if (size != 0 && delta == 0) {
            delta = 1;
        }

        std::vector<size_t> ret;
        ret.push_back(0);

        while(ret.back() < size) {
            ret.push_back(ret.back() + delta);
        }
        if(ret.back() >= size) {
            ret.pop_back();
        }
        ret.push_back(size);

        return ret;
    }
};

struct BlockedDividingStrategy: public Algorithm {
    inline static Meta meta() {
        Meta m("dividing_strategy", "blocked");
        m.option("n").dynamic(1024); // TODO: Change this to something reasonable like 1024
        return m;
    }

    using Algorithm::Algorithm;

    inline std::vector<size_t> split_at(Input& inp) const {
        auto size = inp.size();
        auto delta = env().option("n").as_integer();

        std::vector<size_t> ret;
        ret.push_back(0);

        while(ret.back() < size) {
            ret.push_back(ret.back() + delta);
        }
        if(ret.back() >= size) {
            ret.pop_back();
        }
        ret.push_back(size);

        return ret;
    }
};

template<typename dividing_t>
class DividingCompressor: public Compressor {
    struct BitISink {
        std::istream* m_ptr;
        uint8_t m_byte = 0;
        int8_t m_cursor = -1;

        inline uint8_t read_bit() {
            if (m_cursor < 0) {
                char c;
                bool res = m_ptr->get(c);
                DCHECK(res);
                m_byte = c;
                m_cursor = 7;
            }

            bool ret = (m_byte & (1 << m_cursor)) != 0;
            m_cursor--;
            return ret;
        }

        template<class T>
        inline T read_int(size_t amount = sizeof(T) * CHAR_BIT) {
            return ::tdc::io::read_int<T>(*this, amount);
        }
    };

    struct BitOSink {
        std::ostream* m_ptr;
        uint8_t m_byte = 0;
        int8_t m_cursor = 7;

        inline void write_bit(bool set) {
            if (set) {
                m_byte |= (1 << m_cursor);
            }
            m_cursor--;

            if(m_cursor < 0) {
                m_ptr->put(m_byte);
                m_byte = 0;
                m_cursor = 7;
            }
        }

        template<typename T>
        inline void write_int(T value, size_t bits = sizeof(T) * CHAR_BIT) {
            ::tdc::io::write_int<T>(*this, value, bits);
        }
    };

public:
    inline static Meta meta() {
        Meta m("compressor", "dividing");
        m.option("strategy").templated<dividing_t>("dividing_strategy");
        m.option("compressor").dynamic<Compressor>();
        return m;
    }

    inline DividingCompressor(Env&& env) : Compressor(std::move(env)) {}

    template<typename F>
    inline void compress_for_each_block(Input& _input, Output& output, F f) const {
        // TODO: Fix the special case in file slicing that requires this extra buffer here
        auto _view = _input.as_view();
        auto input = Input::from_memory(_view);

        const dividing_t strategy { this->env().env_for_option("strategy") };
        auto offsets = strategy.split_at(input);

        for (size_t i = 0; i < offsets.size() - 1; i++) {
            auto buffer = std::vector<uint8_t>();
            {
                auto slice = Input(input, offsets[i], offsets[i + 1]);
                auto tmp_o = Output(buffer);
                f(slice, tmp_o);
            }
            {
                auto os = output.as_stream();
                io::write_int<size_t>(BitOSink { &os }, buffer.size());
                os << View(buffer);
                os.flush();
            }
        }

        {
            // This zero-length write happens just to trigger the creation of an
            // empty output file in case of a empty input
            auto os = output.as_stream();
            os << ""_v;
        }
    }

    template<typename F>
    inline void decompress_for_each_block(Input& _input, Output& output, F f) const {
        // TODO: Fix the special case in file slicing that requires this extra buffer here
        auto _view = _input.as_view();
        auto input = Input::from_memory(_view);

        size_t size = input.size();
        size_t cursor = 0;

        while (cursor < size) {
            size_t block_size;
            {
                auto local_input = Input(input, cursor);
                auto is = local_input.as_stream();
                block_size = io::read_int<size_t>(BitISink { &is });
            }
            cursor += sizeof(size_t);
            {
                auto block_slice = Input(input, cursor, cursor + block_size);
                f(block_slice, output);
            }
            cursor += block_size;
        }
        DCHECK_EQ(cursor, size);

        {
            // This zero-length write happens just to trigger the creation of an
            // empty output file in case of a empty input
            auto os = output.as_stream();
            os << ""_v;
        }
    }

    inline virtual void compress(Input& input, Output& output) override final {
        auto& option_value = env().option("compressor");
        auto av = option_value.as_algorithm();
        auto textds_flags = av.textds_flags();
        auto env_root = env().root();

        // Make sure null termination and escaping happens
        auto input2 = Input(input, textds_flags);

        compress_for_each_block(input2, output, [&](auto& input, auto& output){
            // TODO: If compressors ever got changed to not store runtime state,
            // then this init could happen once
            auto compressor = env_root.select_algorithm<Compressor>(av);

            compressor->compress(input, output);
        });
    }

    inline virtual void decompress(Input& input, Output& output) override final {
        auto& option_value = env().option("compressor");
        auto av = option_value.as_algorithm();
        auto textds_flags = av.textds_flags();
        auto env_root = env().root();

        // Make sure null termination and escaping gets reverted
        auto output2 = Output(output, textds_flags);

        decompress_for_each_block(input, output2, [&](auto& input, auto& output){
            // TODO: If compressors ever got changed to not store runtime state,
            // then this init could happen once
            auto compressor = env_root.select_algorithm<Compressor>(av);

            compressor->decompress(input, output);
        });
    }
};

}
