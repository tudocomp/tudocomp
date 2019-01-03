#pragma once

#include <tudocomp/util/int_coder.hpp>
#include <tudocomp/Decompressor.hpp>

namespace tdc {

class DividingDecompressor : public Decompressor {
private:
    struct BitISink {
        std::istream* m_ptr;
        uint8_t m_byte = 0;
        int8_t m_cursor = -1;

        inline uint8_t read_bit() {
            if (m_cursor < 0) {
                char c;
                bool res = (bool)m_ptr->get(c);
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
            return ::tdc::read_int<T>(*this, amount);
        }
    };

public:
    inline static Meta meta() {
        Meta m(Decompressor::type_desc(), "dividing",
            "Decompresses a partitioned compression.");
        m.param("decompressor", "The decompressor.").complex();
        return m;
    }

    using Decompressor::Decompressor;

    virtual void decompress(Input& _input, Output& output) override {
        auto entry = Registry::of<Decompressor>().find(
            meta::ast::convert<meta::ast::Object>( // TODO: shorter syntax for conversion?
                config().param("decompressor").ast()));

        // FIXME (Marvin): Fix the special case in file slicing that requires this extra buffer here
        auto _view = _input.as_view();
        auto input = Input::from_memory(_view); // FIXME: ???

        size_t size = input.size();
        size_t cursor = 0;

        while (cursor < size) {
            size_t block_size;
            {
                auto local_input = Input(input, cursor);
                auto is = local_input.as_stream();
                block_size = ::tdc::read_int<size_t>(BitISink { &is });
            }
            cursor += sizeof(size_t);
            {
                auto block_slice = Input(input, cursor, cursor + block_size);
                entry.select()->decompress(block_slice, output);
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
};

}

