#ifndef _INCLUDED_DECODE_BUFFER_HPP_
#define _INCLUDED_DECODE_BUFFER_HPP_

#include <unordered_map>
#include <vector>

#include <glog/logging.h>

#include <tudocomp/util.h>

namespace tudocomp {

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

    void decodeCallback(size_t source) {
        if (pointers.count(source) > 0) {
            std::vector<size_t> list = pointers[source];
            for (size_t target : list) {
                // decode this char
                text.at(target) = text.at(source);
                byte_is_decoded.at(target) = true;

                // decode all chars depending on this location
                decodeCallback(target);
            }

            pointers.erase(source);
        }
    }

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

}

#endif

