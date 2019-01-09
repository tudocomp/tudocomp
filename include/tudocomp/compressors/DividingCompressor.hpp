#pragma once

#include <tudocomp/util/int_coder.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/decompressors/DividingDecompressor.hpp>

namespace tdc {

constexpr TypeDesc dividing_strategy_td() {
    return TypeDesc("dividing_strategy");
}

struct DivisionDividingStrategy: public Algorithm {
    inline static Meta meta() {
        Meta m(dividing_strategy_td(), "division",
            "Partitions the input into a fixed amount of blocks.");
        m.param("n", "The amount of blocks").primitive(4);
        return m;
    }

    using Algorithm::Algorithm;

    inline std::vector<size_t> split_at(Input& inp) const {
        auto size = inp.size();
        auto n = config().param("n").as_uint();
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
        Meta m(dividing_strategy_td(), "blocked",
            "Partitions the input into blocks of fixed size.");
        m.param("n", "the size of each block (in bytes)").primitive(1024);
        return m;
    }

    using Algorithm::Algorithm;

    inline std::vector<size_t> split_at(Input& inp) const {
        auto size = inp.size();
        auto delta = config().param("n").as_uint();

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
            ::tdc::write_int<T>(*this, value, bits);
        }
    };

    inline auto find_compressor() const {
        return Registry::of<Compressor>().find(
            meta::ast::convert<meta::ast::Object>( // TODO: shorter syntax for conversion?
                config().param("compressor").ast()));
    }

public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "dividing",
            "Partitions the input into blocks and compresses each block "
            "individually.");
        m.param("strategy").strategy<dividing_t>(dividing_strategy_td());
        m.param("compressor").unbound_strategy(Compressor::type_desc());
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& _input, Output& output) override final {
        auto entry = find_compressor();
        const dividing_t strategy { config().sub_config("strategy") };

        // TODO: Fix the special case in file slicing that requires this extra buffer here
        auto _view = _input.as_view();
        auto input = Input::from_memory(_view);

        auto offsets = strategy.split_at(input);

        for (size_t i = 0; i < offsets.size() - 1; i++) {
            auto buffer = std::vector<uint8_t>();
            {
                auto slice = Input(input, offsets[i], offsets[i + 1]);
                auto tmp_o = Output(buffer);
                entry.select()->compress(slice, tmp_o);
            }
            {
                auto os = output.as_stream();
                ::tdc::write_int<size_t>(BitOSink { &os }, buffer.size());
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

    inline virtual std::unique_ptr<Decompressor> decompressor() const override {
        // FIXME: construct AST and pass it
        std::stringstream cfg;
        auto c = find_compressor().select(); // TODO: ugh
        cfg << "decompressor=" << c->decompressor()->config().str() << ",";
        return Algorithm::instance<DividingDecompressor>(cfg.str());
    }
};

}
