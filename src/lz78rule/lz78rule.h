#ifndef LZ78RULE_RULE_H
#define LZ78RULE_RULE_H

#include "tudocomp.h"
#include "lz78rule_rule.h"
#include "lz78rule_rules.h"

namespace lz78rule {

using namespace tudocomp;

class Lz78RuleCoder;
class Lz78RuleCompressor;

const std::string THRESHOLD_OPTION = "lz78rule.threshold";
const std::string THRESHOLD_LOG = "lz78rule.threshold";
const std::string RULESET_SIZE_LOG = "lz78rule.rule_count";

struct Lz78Rule: public Compressor {
    Lz78RuleCompressor* m_compressor;
    Lz78RuleCoder* m_encoder;

    inline Lz78Rule(Env& env,
                    Lz78RuleCompressor* compressor,
                    Lz78RuleCoder* encoder):
        Compressor(env),
        m_compressor(compressor),
        m_encoder(encoder) {};

    inline virtual void compress(Input& input, Output& output) override final;

    inline virtual void decompress(Input& inp, Output& out) override final;
};

/// Interface for a compressor into LZ77-like substitution rules.
///
/// A (rule-based) compressor works by receiving the Input text, and
/// generating a list of substitution rules.
/// The rules in combination with the input parts not covered by them
/// can regenerate the full input.
class Lz78RuleCompressor {
public:
    const Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline Lz78RuleCompressor() = delete;

    /// Construct the class with an environment.
    inline Lz78RuleCompressor(Env& env_): env(env_) {}

    /// Compress the input.
    ///
    /// \param input The input to be compressed.
    /// \return The list of rules.
    virtual Entries compress(Input& input) = 0;
};

/// Interface for a coder from LZ77-like substitution rules.
///
/// This takes a list of Entries and the input text, and outputs
/// an encoded form of them to a `ostream`. Also provided is a decoder,
/// that takes such an encoded stream and outputs the fully
/// decoded and decompressed original text.
class Lz78RuleCoder {
public:
    const Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline Lz78RuleCoder() = delete;

    /// Construct the class with an environment.
    inline Lz78RuleCoder(Env& env_): env(env_) {}

    /// Encode a list or Entries and the input text.
    ///
    /// \param rules The list of substitution rules
    /// \param input The input text
    /// \param out `ostream` where the encoded output will be written to.
    virtual void code(Entries&& rules, Output& out) = 0;

    /// Decode and decompress `inp` into `out`.
    ///
    /// This method expects `inp` to be encoded with the same encoding
    /// that the `code()` method emits.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decode(Input& inp, Output& out) = 0;
};

inline void Lz78Rule::compress(Input& input, Output& out) {
    auto rules = m_compressor->compress(input);
    env.log_stat(RULESET_SIZE_LOG, rules.size());
    m_encoder->code(std::move(rules), out);
}

inline void Lz78Rule::decompress(Input& inp, Output& out) {
    m_encoder->decode(inp, out);
}

}

#endif
