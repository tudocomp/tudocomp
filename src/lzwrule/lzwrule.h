#ifndef LZWRULE_RULE_H
#define LZWRULE_RULE_H

#include "tudocomp.h"
#include "sdsl_extension.h"
#include "glog/logging.h"
#include "lz78rule.h"

namespace lzwrule {

using namespace tudocomp;

using LzwEntries = sdsl_extension::GrowableIntVector;
using LzwEntry = uint64_t;

class LzwRuleCoder;
class LzwRuleCompressor;

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

/// Interface for a coder from LZW-like substitution rules.
class LzwRuleCoder {
public:
    Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline LzwRuleCoder() = delete;

    /// Construct the class with an environment.
    inline LzwRuleCoder(Env& env_): env(env_) {}

    /// Encode a list or LzwEntries and the input text.
    ///
    /// \param rules The list of substitution rules
    /// \param out `ostream` where the encoded output will be written to.
    virtual void code(LzwEntries&& rules, Output& out) = 0;

    /// Decode and decompress `inp` into `out`.
    ///
    /// This method expects `inp` to be encoded with the same encoding
    /// that the `code()` method emits.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decode(Input& inp, Output& out) = 0;
};

inline LzwEntries _compress(Input& input) {
    using namespace lz78rule;

    LzwEntries entries;

    auto guard = input.as_view();
    boost::string_ref input_ref = *guard;

    Trie trie;

    for (uint32_t i = 0; i <= 0xff; i++) {
        trie.insert(i);
    }

    for (size_t i = 0; i < input.size(); i++) {
        auto s = input_ref.substr(i);

        Result phrase_and_size = trie.find_or_insert(s);

        DLOG(INFO) << "looking at " << input_ref.substr(0, i)
            << " | " << s << " -> " << phrase_and_size.size;

        if (phrase_and_size.size > 1) {
            i += phrase_and_size.size - 2;
        } else if (phrase_and_size.size == 1) {
            i += 1;
        }

        LzwEntry e;
        if (phrase_and_size.entry.index != 0) {
            e = phrase_and_size.entry.index - 1;
        } else {
            e = phrase_and_size.entry.chr;
        }

        entries.push_back(e);
    }

    trie.root.print(0);

    return std::move(entries);
}

inline void LzwRule::compress(Input& input, Output& out) {
    auto entries = _compress(input);
    DLOG(INFO) << "entries size: " << entries.size();
    env.log_stat(RULESET_SIZE_LOG, entries.size());
    m_encoder->code(std::move(entries), out);
}

inline void LzwRule::decompress(Input& inp, Output& out) {
    m_encoder->decode(inp, out);
}

}

#endif
