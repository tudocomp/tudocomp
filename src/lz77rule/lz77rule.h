#ifndef LZ77RULE_H
#define LZ77RULE_H

#include "tudocomp.h"
#include "rule.h"
#include "rules.h"

namespace lz77rule {

using namespace tudocomp;

// /// Type of the list of Rules the compression step produces
// using Rules = std::vector<Rule>;

class Lz77RuleCoder;
class Lz77RuleCompressor;

const std::string THRESHOLD_OPTION = "lz77rule.threshold";
const std::string THRESHOLD_LOG = "lz77rule.threshold";
const std::string RULESET_SIZE_LOG = "lz77rule.rule_count";

struct Lz77Rule: public Compressor {
    Lz77RuleCompressor* m_compressor;
    Lz77RuleCoder* m_encoder;

    inline Lz77Rule(Env& env,
                    Lz77RuleCompressor* compressor,
                    Lz77RuleCoder* encoder):
        Compressor(env),
        m_compressor(compressor),
        m_encoder(encoder) {};

    inline virtual void compress(Input input, std::ostream& out) override final;

    inline virtual void decompress(std::istream& inp, std::ostream& out) override final;
};

/// Interface for a compressor into LZ77-like substitution rules.
///
/// A (rule-based) compressor works by receiving the Input text, and
/// generating a list of substitution rules.
/// The rules in combination with the input parts not covered by them
/// can regenerate the full input.
class Lz77RuleCompressor {
public:
    const Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline Lz77RuleCompressor() = delete;

    /// Construct the class with an environment.
    inline Lz77RuleCompressor(Env& env_): env(env_) {}

    /// Compress the input.
    ///
    /// \param input The input to be compressed.
    /// \param threshold The threshold in bytes that limits how small an area a
    ///                  substitution rule may maximally
    ///                  cover. For example, a threshold of 3 means no
    ///                  rules for substitutions of length 2 should be generated.
    /// \return The list of rules.
    virtual Rules compress(const Input& input, size_t threshold) = 0;
};

/// Interface for a coder from LZ77-like substitution rules.
///
/// This takes a list of Rules and the input text, and outputs
/// an encoded form of them to a `ostream`. Also provided is a decoder,
/// that takes such an encoded stream and outputs the fully
/// decoded and decompressed original text.
class Lz77RuleCoder {
public:
    const Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline Lz77RuleCoder() = delete;

    /// Construct the class with an environment.
    inline Lz77RuleCoder(Env& env_): env(env_) {}

    /// Encode a list or Rules and the input text.
    ///
    /// \param rules The list of substitution rules
    /// \param input The input text
    /// \param out `ostream` where the encoded output will be written to.
    virtual void code(Rules rules, Input input, std::ostream& out) = 0;

    /// Decode and decompress `inp` into `out`.
    ///
    /// This method expects `inp` to be encoded with the same encoding
    /// that the `code()` method emits.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decode(std::istream& inp, std::ostream& out) = 0;

    /// Return the expected minimum encoded
    /// length in bytes of a single rule if encoded with this encoder.
    ///
    /// This can be used by compressors to directly filter
    /// out rules that would not be beneficial in the encoded output.
    ///
    /// \param input_size The length of the input in bytes
    virtual size_t min_encoded_rule_length(size_t input_size) = 0;
};

inline void Lz77Rule::compress(Input input, std::ostream& out) {
    uint64_t threshold = 0;

    if (env.has_option(THRESHOLD_OPTION)) {
        threshold = env.option_as<uint64_t>(THRESHOLD_OPTION);
    } else {
        threshold = m_encoder->min_encoded_rule_length(input.size());
    }

    env.log_stat(THRESHOLD_LOG, threshold);
    auto rules = m_compressor->compress(input, threshold);
    env.log_stat(RULESET_SIZE_LOG, rules.size());
    m_encoder->code(rules, std::move(input), out);
}

inline void Lz77Rule::decompress(std::istream& inp, std::ostream& out) {
    m_encoder->decode(inp, out);
}

}

#endif
