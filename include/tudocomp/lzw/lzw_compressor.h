#ifndef LZWRULE_RULE_H
#define LZWRULE_RULE_H

#include <glog/logging.h>

#include <tudocomp/Compressor.hpp>

#include <tudocomp/lz78/trie.h>

#include <tudocomp/lzw/factor.h>
#include <tudocomp/lzw/coder.h>

namespace lzw {

using namespace tudocomp;

using ::lz78::PrefixBuffer;

const std::string RULESET_SIZE_LOG = "lzwrule.rule_count";

struct LzwRule: public Compressor {
    LzwRuleCoder* m_encoder;

    inline LzwRule(Env& env,
                   LzwRuleCoder* encoder):
        Compressor(env),
        m_encoder(encoder) {};

    inline virtual void compress(Input& input, Output& output) override final;
    inline virtual void decompress(Input& inp, Output& out) override final;
};

inline LzwEntries _compress(Input& input) {
    LzwEntries entries;

    auto guard = input.as_stream();
    PrefixBuffer buf(*guard);

    lz78::Trie trie(lz78::Trie::Lzw);

    for (uint32_t i = 0; i <= 0xff; i++) {
        trie.insert(i);
    }

    while (!buf.is_empty()) {
        lz78::Result phrase_and_size = trie.find_or_insert(buf);

        LzwEntry e;
        if (phrase_and_size.entry.index != 0) {
            e = phrase_and_size.entry.index - 1;
        } else {
            e = phrase_and_size.entry.chr;
        }

        entries.push_back(e);
        DLOG(INFO) << "factor: " << e << "\n";
    }

    trie.print(0);

    return std::move(entries);
}

inline void LzwRule::compress(Input& input, Output& out) {
    auto entries = _compress(input);
    DLOG(INFO) << "entries size: " << entries.size();
    m_env->log_stat(RULESET_SIZE_LOG, entries.size());
    m_encoder->code(std::move(entries), out);
}

inline void LzwRule::decompress(Input& inp, Output& out) {
    m_encoder->decode(inp, out);
}

}

#endif
