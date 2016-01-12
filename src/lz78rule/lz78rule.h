#ifndef LZ78RULE_RULE_H
#define LZ78RULE_RULE_H

#include "tudocomp.h"
#include "lz78rule_rule.h"
#include "lz78rule_rules.h"

namespace lz78rule {

using namespace tudocomp;

struct Result {
    size_t size;
    Entry entry;
};

struct EdgeNode {
    std::vector<EdgeNode> children;
    size_t index;
    uint8_t chr;

    inline Entry insert(uint8_t chr, size_t* max_index) {
        size_t new_index = *max_index;
        (*max_index)++;

        Entry new_entry {
            index,
            chr
        };
        children.push_back(EdgeNode { {}, new_index, new_entry.chr });

        return new_entry;
    }

    inline Result find_or_insert(const boost::string_ref prefix,
                          size_t offset,
                          size_t* max_index,
                          size_t prev_index)
    {
        if (prefix.size() == 0) {
            return { offset, Entry { prev_index, chr } };
        } else {
            uint8_t c = prefix[0];

            for (auto& edge : children) {
                if (edge.chr == c) {
                    return edge.find_or_insert(
                        prefix.substr(1), offset + 1, max_index, index);
                }
            }

            Entry new_entry = insert(c, max_index);
            return { offset + 1, new_entry };
        }
    }

    inline void print(int ident) {
        DLOG(INFO) << std::setfill(' ') << std::setw(ident)
            << "[" << index << "] " << tudocomp::byte_to_nice_ascii_char(chr) << " [";
        for (auto& e: children) {
            e.print(ident + 4);
        }
        DLOG(INFO) << std::setfill(' ') << std::setw(ident) << "" "]";
    }
};

struct Trie {
    EdgeNode root = EdgeNode { {}, 0, 0 };
    size_t max_index = 1;

    inline Result find_or_insert(const boost::string_ref prefix) {
        Result r = root.find_or_insert(prefix, 0, &max_index, 0);
        return r;
    }

    inline Entry insert(uint8_t chr) {
        return root.insert(chr, &max_index);
    }
};

inline Entries compress_impl(const Env& env, Input& input) {
    auto guard = input.as_view();
    auto input_ref = *guard;

    Trie trie;
    Entries entries;

    for (size_t i = 0; i < input_ref.size(); i++) {
        auto s = input_ref.substr(i);

        Result phrase_and_size = trie.find_or_insert(s);

        DLOG(INFO) << "looking at " << input_ref.substr(0, i)
            << "|" << s << " -> " << phrase_and_size.size;

        i += phrase_and_size.size - 1;

        entries.push_back(phrase_and_size.entry);
    }

    trie.root.print(0);

    return entries;
}

class Lz78RuleCoder;
class Lz78RuleCompressor;

const std::string THRESHOLD_OPTION = "lz78rule.threshold";
const std::string THRESHOLD_LOG = "lz78rule.threshold";
const std::string RULESET_SIZE_LOG = "lz78rule.rule_count";

struct Lz78Rule: public Compressor {
    Lz78RuleCoder* m_encoder;

    inline Lz78Rule(Env& env,
                    Lz78RuleCoder* encoder):
        Compressor(env),
        m_encoder(encoder) {};

    inline virtual void compress(Input& input, Output& output) override final;

    inline virtual void decompress(Input& inp, Output& out) override final;
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
    auto entries = compress_impl(env, input);
    env.log_stat(RULESET_SIZE_LOG, entries.size());
    m_encoder->code(std::move(entries), out);
}

inline void Lz78Rule::decompress(Input& inp, Output& out) {
    m_encoder->decode(inp, out);
}

}

#endif
