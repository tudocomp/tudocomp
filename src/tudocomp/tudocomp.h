#ifndef COMPRESSFRAMEWORK_H
#define COMPRESSFRAMEWORK_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <istream>
#include <streambuf>
#include <sstream>
#include <string>
#include <map>
#include <unordered_map>

#include "boost/utility/string_ref.hpp"
#include "glog/logging.h"

#include "rule.h"
// #include "rules.h"

namespace tudocomp {

/// Type of the input data to be compressed
using Input = std::vector<uint8_t>;

/// Type of the list of Rules the compression step produces
using Rules = std::vector<Rule>;

/// Interface for a compressor.
///
/// A (rule-based) compressor works by receiving the Input text, and
/// generating a list of substitution rules.
/// The rules in combination with the input parts not covered by them
/// can regenerate the full input.
class Compressor {
public:
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

/// Interface for a coder.
///
/// This takes a list of Rules and the input text, and outputs
/// an encoded form of them to a `ostream`. Also provided is a decoder,
/// that takes such an encoded stream and outputs the fully
/// decoded and decompressed original text.
class Coder {
public:
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

/// Create a `Input` value containing `s`.
///
/// This is useful for testing the implementations of
/// Compressor::Compress() and Coder::code() with small hardcoded strings.
Input input_from_string(std::string s);

/// A `streambuf` that can be used as a `istream` that points into the
/// contents of another string.
///
/// This is useful for testing the implementation of Coder::decode()
/// with small hardcoded strings.
struct StringRefStream: std::streambuf
{
    inline StringRefStream(const boost::string_ref vec) {
        this->setg((char*)vec.data(), (char*)vec.data(), (char*)vec.data() + vec.size());
    }
};

/// Temporary provides a `ostream` to write into, and returns it as a string.
///
/// This is useful for testing Coder::code() and Coder::decode().
///
/// \param f A callable type (like for example a C++ lambda expression)
///          that receives an std::ostream& as an argument so that its
///          body can write into it.
template<class Lambda>
std::string ostream_to_string(Lambda f) {
    std::stringstream ss;
    std::ostream& os = ss;
    f(os);
    return ss.str();
}

/// Temporary provides a `ostream` to write into, and returns it as a
/// byte vector.
///
/// This is useful for testing Coder::code() and Coder::decode().
///
/// \param f A callable type (like for example a C++ lambda expression)
///          that receives an std::ostream& as an argument so that its
///          body can write into it.
template<class Lambda>
std::vector<uint8_t> ostream_to_bytes(Lambda f) {
    auto s = ostream_to_string(f);
    return std::vector<uint8_t>(s.begin(), s.end());
}

/*
 Ziel ist es, ein Kompressionsframework zu erschaffen, das modular aufgebaut ist.
Module sollen sein:
  * Regelwerk
    - LZ77, LZ78, ESAComp, Tunstall, ESP
  * Codierer
    - Dictionary, Grammar, InPlace (ESAComp)
  * Decodierer

Eingaben:
  * Texte
  * Bit-Sequenzen

Die komprimierten Bit-Sequenzen sollen dann mit der SDSL Bibliothek bedienbar sein
(z.B. einen Wavelet-Tree auf einer komprimierten Bit-Sequenz;
Der Code dafür ist in hocbits bereits vorhanden)

*/

/// Convert a vector-like type into a string showing the element values.
///
/// Useful for logging output.
///
/// Example: [1, 2, 3] -> "[1, 2, 3]"
template<class T>
std::string vec_to_debug_string(const T& s) {
    using namespace std;

    stringstream ss;
    ss << "[";
    if (s.size() > 0) {
        for (size_t i = 0; i < s.size() - 1; i++) {
            // crazy cast needed to bring negative char values
            // into their representation as a unsigned one
            ss << uint((unsigned char) s[i]) << ", ";
        }
        ss << uint((unsigned char) s[s.size() - 1]);
    }
    ss << "]";
    return ss.str();
}

/// Convert a vector-like type into a string by inpterpreting printable ASCII
/// bytes as chars, and substituting others.
///
/// Useful for logging output.
///
/// Example: [97, 32, 97, 0] -> "a a?"
template<class T>
std::string vec_as_lossy_string(const T& s, size_t start = 0,
                                char replacement = '?') {
    using namespace std;

    stringstream ss;
    for (size_t i = start; i < s.size(); i++) {
        if (s[i] < 32 || s[i] > 127) {
            ss << replacement;
        } else {
            ss << char(s[i]);
        }
    }
    return ss.str();
}

// TODO: Exisitng implementation is probably very imperformant
/// Common helper class for decoders.
///
/// This type represents a buffer of partially decoded input.
/// The buffer gets filled by repeatedly either pushing a rule
/// or a decoded byte onto it, and automatically constructs the
/// fully decoded text in the process.
class DecodeBuffer {
    std::vector<uint8_t> text;
    std::vector<bool> byte_is_decoded;
    size_t text_pos;
    std::unordered_map<size_t, std::vector<size_t>> pointers;

    void decodeCallback(size_t source);

    inline void addPointer(size_t target, size_t source) {
        if (pointers.count(source) == 0) {
            std::vector<size_t> list;
            pointers[source] = list;
        }

        pointers[source].push_back(target);
    }

public:
    /// Create a new DecodeBuffer for a expected decoded text length of
    /// `length` bytes.
    ///
    /// \param length The expect length of the decoded text.
    inline DecodeBuffer(size_t length) {
        text.reserve(length);
        byte_is_decoded.reserve(length);
        text_pos = 0;
    }

    /// Push a decoded byte to the buffer.
    ///
    /// The byte will just be added to the internal buffer and
    /// be marked as already decoded.
    inline void push_decoded_byte(uint8_t byte) {
        // add new byte to text
        text.push_back(byte);
        byte_is_decoded.push_back(true);

        // decode all chars depending on this position
        decodeCallback(text_pos);

        // advance
        text_pos++;
        DLOG(INFO) << "text: " << vec_as_lossy_string(text);
    }

    /// Push a substitution rule to the buffer.
    ///
    /// The rule is described by the source position and substitution length,
    /// and will be targeted at the current location in the decode buffer.
    ///
    /// \param source The source position of the Rule, referring to a index into
    ///               the fully decoded text.
    /// \param length The length of the rule.
    inline void push_rule(size_t source, size_t length) {
        for (size_t k = 0; k < length; k++) {
            size_t source_k = source + k;

            if (source_k < text_pos && byte_is_decoded.at(source_k)) {
                // source already decoded, use it
                uint8_t byte = text.at(source_k);

                push_decoded_byte(byte);
            } else {
                // add pointer for source

                // add placeholder byte
                text.push_back(0);
                byte_is_decoded.push_back(false);
                addPointer(text_pos, source_k);

                // advance
                text_pos++;
                DLOG(INFO) << "text: " << vec_as_lossy_string(text);
            }
        }
    }

    /// Write the current state of the decode buffer into an ostream.
    inline void write_to(std::ostream& out) {
        out.write((const char*)text.data(), text.size());
    }

    /// Return the expected size of the decoded text.
    inline size_t size() {
        return text.size();
    }
};

// TODO: Error handling

/// Read a number of bytes and return them as an integer in big endian format.
///
/// If the number of bytes read is smaller than the number of bytes in the
/// return type, then all most significant bytes of that value will be filled
/// with zeroes.
template<class T>
T read_bytes(std::istream& inp, size_t bytes = sizeof(T)) {
    char c;
    T ret = 0;
    for(size_t i = 0; i < bytes; i++) {
        if (!inp.get(c)) {
            break;
        }
        ret <<= 8;
        ret |= T((unsigned char)c);
    }
    return ret;
}

/// Read a number of bytes and write them to a vector-like type.
template<class T>
void read_bytes_to_vec(std::istream& inp, T& vec, size_t bytes) {
    char c;
    for(size_t i = 0; i < bytes; i++) {
        if (!inp.get(c)) {
            break;
        }
        vec.push_back((unsigned char)c);
    }
}

/// Callable object, acts as a comparator of Rules, to order them
/// acording to target position.
struct rule_compare {
    inline bool operator() (const Rule& lhs, const Rule& rhs) const {
        return lhs.target < rhs.target;
    }
};

}

#endif
