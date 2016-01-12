#ifndef LZWRULE_RULE_H
#define LZWRULE_RULE_H

#include "tudocomp.h"
#include "sdsl_extension.h"
#include "glog/logging.h"

namespace lzwrule {

using namespace tudocomp;

using LzwEntries = sdsl_extension::GrowableIntVector;
using LzwEntry = uint64_t;

class LzwRuleCoder;
class LzwRuleCompressor;

const std::string RULESET_SIZE_LOG = "lzwrule.rule_count";

struct LzwRule: public Compressor {
    LzwRuleCompressor* m_compressor;
    LzwRuleCoder* m_encoder;

    inline LzwRule(Env& env,
                   LzwRuleCompressor* compressor,
                   LzwRuleCoder* encoder):
        Compressor(env),
        m_compressor(compressor),
        m_encoder(encoder) {};

    inline virtual void compress(Input input, std::ostream& out) override final;

    inline virtual void decompress(std::istream& inp, std::ostream& out) override final;
};

/// Interface for a compressor into LZW-like dictionaries.
class LzwRuleCompressor {
public:
    const Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline LzwRuleCompressor() = delete;

    /// Construct the class with an environment.
    inline LzwRuleCompressor(Env& env_): env(env_) {}

    /// Compress the input.
    ///
    /// \param input The input to be compressed.
    /// \return The list of rules.
    virtual LzwEntries compress(const Input& input) = 0;
};

/// Interface for a coder from LZW-like substitution rules.
class LzwRuleCoder {
public:
    const Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline LzwRuleCoder() = delete;

    /// Construct the class with an environment.
    inline LzwRuleCoder(Env& env_): env(env_) {}

    /// Encode a list or LzwEntries and the input text.
    ///
    /// \param rules The list of substitution rules
    /// \param input The input text
    /// \param out `ostream` where the encoded output will be written to.
    virtual void code(LzwEntries rules, Input input, std::ostream& out) = 0;

    /// Decode and decompress `inp` into `out`.
    ///
    /// This method expects `inp` to be encoded with the same encoding
    /// that the `code()` method emits.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decode(std::istream& inp, std::ostream& out) = 0;
};

inline void LzwRule::compress(Input input, std::ostream& out) {
    auto rules = m_compressor->compress(input);
    env.log_stat(RULESET_SIZE_LOG, rules.size());
    m_encoder->code(rules, std::move(input), out);
}

inline void LzwRule::decompress(std::istream& inp, std::ostream& out) {
    m_encoder->decode(inp, out);
}

}

#endif
