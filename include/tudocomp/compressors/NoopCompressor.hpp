#pragma once

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/io.hpp>

namespace tdc {

class NoopCompressor: public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "noop");
        m.option("mode").dynamic("stream");
        m.option("debug").dynamic("false");
        return m;
    }

    inline NoopCompressor(Env&& env):
        Compressor(std::move(env)) {}


    inline virtual void compress(Input& i, Output& o) override final {
        auto os = o.as_stream();

        if (env().option("mode").as_string() == "stream") {
            auto is = i.as_stream();
            if (env().option("debug").as_bool()) {
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
            if (env().option("debug").as_bool()) {
                DLOG(INFO) << vec_to_debug_string(iv);
                os << iv;
            } else {
                os << iv;
            }
        }
    }

    inline virtual void decompress(Input& i, Output& o) override final {
        auto os = o.as_stream();

        if (env().option("mode").as_string() == "stream") {
            auto is = i.as_stream();
            if (env().option("debug").as_bool()) {
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
            if (env().option("debug").as_bool()) {
                DLOG(INFO) << vec_to_debug_string(iv);
                os << iv;
            } else {
                os << iv;
            }
        }
    }
};

}

