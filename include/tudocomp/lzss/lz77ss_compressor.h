#ifndef LZ77RULE_H
#define LZ77RULE_H

#include <tudocomp/compressor.h>
#include <tudocomp/lzss/factors.h>
#include <tudocomp/lzss/coder.h>

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

    inline virtual void compress(Input& input, Output& output) override final;

    inline virtual void decompress(Input& input, Output& output) override final;
};

/// Interface for a compressor into LZ77-like substitution rules.
///
/// A (rule-based) compressor works by receiving the Input text, and
/// generating a list of substitution rules.
/// The rules in combination with the input parts not covered by them
/// can regenerate the full input.
class Lz77RuleCompressor {
public:
    Env& env;

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
    virtual Rules compress(Input& input, size_t threshold) = 0;
};

inline void Lz77Rule::compress(Input& input, Output& output) {
    uint64_t threshold = 0;

    if (env.has_option(THRESHOLD_OPTION)) {
        threshold = env.option_as<uint64_t>(THRESHOLD_OPTION);
    } else if (input.has_size()) {
        threshold = m_encoder->min_encoded_rule_length(input.size());
    } else {
        threshold = m_encoder->min_encoded_rule_length();
    }

    CHECK(threshold > 0);

    env.log_stat(THRESHOLD_LOG, threshold);
    auto rules = m_compressor->compress(input, threshold);
    env.log_stat(RULESET_SIZE_LOG, rules.size());
    m_encoder->code(std::move(rules), input, output);
}

inline void Lz77Rule::decompress(Input& inp, Output& out) {
    m_encoder->decode(inp, out);
}

}

#endif
