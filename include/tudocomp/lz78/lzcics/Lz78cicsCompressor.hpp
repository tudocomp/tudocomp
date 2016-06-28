#ifndef _INCLUDED_LZ78_CICS_COMPRESSOR_HPP_
#define _INCLUDED_LZ78_CICS_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>
#include <tudocomp/lz78/lzcics/lz78.hpp>
#include <tudocomp/lz78/lzcics/st.hpp>
#include <tudocomp/lz78/lzcics/util.hpp>

namespace tudocomp {
namespace lz78 {
namespace lzcics {

template <typename C>
class Lz78cicsCompressor: public Compressor {
public:
    using Compressor::Compressor;

    inline static Meta meta() {
        Meta m("compressor", "lz78_cics");
        m.option("coder").templated<C, Lz78BitCoder>();
        return m;
    }

    inline virtual void compress(Input& in, Output& out) override final {
        auto i_view = in.as_view();

        C coder(env().env_for_option("coder"), out);

        // TODO: Hack to make the cics compressor work
        // with empty input
        if (i_view == "") {
            coder.encode_fact(Factor {
                0,
                '\0',
            });
            return;
        }

        std::string text(i_view);

        //Build ST
        cst_t cst;
        ST st = suffix_tree(text, cst);

        LZ78rule l78 = lz78(st);

        CHECK(l78.ref.size() == l78.cha.size());

        for (size_t i = 0; i < l78.ref.size(); i++) {
            auto ref = l78.ref[i];

            // want to have undef == 0
            // and idx starting at 1
            ref++;

            coder.encode_fact(Factor {
                uint32_t(ref),
                uint8_t(l78.cha[i]),
            });
        }
    }

    inline virtual void decompress(Input& in, Output& out) override final {
        // NB: The encoder adds a trailing 0 byte,
        // need to remove it when decoding.
        //
        // TODO: This does currently allocate a new buffer
        // but could be a streaming adapter that
        // remembers 1 or 2 chars.

        std::vector<uint8_t> buf;

        auto out_buf = Output::from_memory(buf);

        C::decode(in, out_buf);

        CHECK(buf[buf.size() - 1] == 0);

        buf.pop_back();

        auto o_guard = out.as_stream();

        o_guard.write((const char*)&buf[0], buf.size());
    }

};

}
}
}

#endif
