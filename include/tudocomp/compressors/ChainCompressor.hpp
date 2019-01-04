#pragma once

#include <memory>
#include <vector>

#include <sstream>

#include <tudocomp/io.hpp>
#include <tudocomp/meta/Registry.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/decompressors/ChainDecompressor.hpp>

namespace tdc {

class ChainSyntaxPreprocessor {
public:
    static inline std::string preprocess(const std::string& str) {
        // replace substrings separated by ':' by chain invokations
        size_t pos = str.find(':');
        if(pos != std::string::npos) {
            return std::string("chain(") +
                    str.substr(0, pos) + ", " +
                    preprocess(str.substr(pos+1)) + ")"; // FIXME: call global preprocessor!
        } else {
            return str;
        }
    }
};

class ChainCompressor: public Compressor {
public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "chain",
            "Executes two compressors consecutively, passing the first "
            "compressor's output to the input of the second.");
        m.param("first", "The first compressor.")
            .unbound_strategy(Compressor::type_desc());
        m.param("second", "The second compressor.")
            .unbound_strategy(Compressor::type_desc());
        return m;
    }

private:
    std::unique_ptr<Compressor> m_first, m_second;

    std::unique_ptr<Compressor> get_compressor(const std::string& option) {
        return Registry::of<Compressor>().select(
            meta::ast::convert<meta::ast::Object>( // TODO: shorter syntax for conversion?
                config().param(option).ast())).move_instance();
    }

public:
    inline ChainCompressor(Config&& cfg) : Compressor(std::move(cfg)) {
        m_first = get_compressor("first");
        m_second = get_compressor("second");
    }

    inline virtual void compress(Input& input, Output& output) override final {
        std::vector<uint8_t> between_buf;
        {
            Output between(between_buf);
            m_first->compress(input, between);
            DVLOG(1) << "Buffer between chain: "
                     << vec_to_debug_string(between_buf);
        }
        {
            Input between(between_buf);
            m_second->compress(between, output);
            // FIXME: between_buf should be freed as soon as possible to avoid
            //        wasting memory in longer chains!
        }
    }

    inline virtual std::unique_ptr<Decompressor> decompressor() const override {
        // FIXME: construct AST and pass it
        std::stringstream cfg;
        cfg << "first=" << m_second->decompressor()->config().str() << ",";
        cfg << "second=" << m_first->decompressor()->config().str();
        return Algorithm::unique_instance<ChainDecompressor>(cfg.str());
    }
};

}
