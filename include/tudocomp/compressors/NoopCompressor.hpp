#pragma once

#include <tudocomp/Compressor.hpp>
#include <tudocomp/io.hpp>

namespace tdc {

class NoopCompressor: public Compressor {
public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "noop",
            "Simply forwards the unmodified input to the output.");
        m.param("mode",
            "The processing mode:\n"
            "\"stream\" - Streams the input\n"
            "\"buffer\" - Buffers the input\n"
        ).primitive("stream");
        m.param("debug", "Enables debugging").primitive(false);
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& i, Output& o) override final {
        auto os = o.as_stream();

        if (config().param("mode").as_string() == "stream") {
            auto is = i.as_stream();
            if (config().param("debug").as_bool()) {
                std::stringstream ss;
                ss << is.rdbuf();
                std::string txt = ss.str();
                DLOG(INFO) << vec_to_debug_string(txt);
                os << txt;
            } else {
                os << is.rdbuf();
            }
        } else {
            auto iv = i.as_view();
            if (config().param("debug").as_bool()) {
                DLOG(INFO) << vec_to_debug_string(iv);
                os << iv;
            } else {
                os << iv;
            }
        }
    }

    inline virtual void decompress(Input& i, Output& o) override final {
        auto os = o.as_stream();

        if (config().param("mode").as_string() == "stream") {
            auto is = i.as_stream();
            if (config().param("debug").as_bool()) {
                std::stringstream ss;
                ss << is.rdbuf();
                std::string txt = ss.str();
                DLOG(INFO) << vec_to_debug_string(txt);
                os << txt;
            } else {
                os << is.rdbuf();
            }
        } else {
            auto iv = i.as_view();
            if (config().param("debug").as_bool()) {
                DLOG(INFO) << vec_to_debug_string(iv);
                os << iv;
            } else {
                os << iv;
            }
        }
    }
};

}

