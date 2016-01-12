#ifndef TUDOCOMP_UTIL_H
#define TUDOCOMP_UTIL_H

#include "tudocomp.h"

namespace tudocomp {

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

inline std::string byte_to_nice_ascii_char(uint64_t byte) {
    using namespace std;

    stringstream out;
    if (byte >= 32 && byte <= 127) {
        out << "'" << char(byte) << "'";
    } else {
        out << byte;
    }
    return out.str();
}

/// Convert a vector-like type into a string by interpreting printable ASCII
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

// TODO: Existing implementation is probably very inperformant
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

inline bool parse_number_until_other(std::istream& inp, char& last, size_t& out) {
    size_t n = 0;
    char c;
    bool more = true;
    while ((more = bool(inp.get(c)))) {
        if (c >= '0' && c <= '9') {
            n *= 10;
            n += (c - '0');
        } else {
            last = c;
            break;
        }
    }
    out = n;
    return more;
}

/// Returns number of bits needed to store the integer value n
///
/// The returned value will always be greater than 0
///
/// Example:
/// - `bitsFor(0b0) == 1`
/// - `bitsFor(0b1) == 1`
/// - `bitsFor(0b10) == 2`
/// - `bitsFor(0b11) == 2`
/// - `bitsFor(0b100) == 3`
inline size_t bitsFor(size_t n) {
    // TODO: Maybe use nice bit ops?
    return size_t(ceil(log2(std::max(size_t(1), n) + 1)));
}

/// Returns number of bytes needed to store the amount of bits.
///
/// Example:
/// - `bytesFor(0) == 0`
/// - `bytesFor(1) == 1`
/// - `bytesFor(8) == 1`
/// - `bytesFor(9) == 2`
inline size_t bytesFor(size_t bits) {
    // TODO: Maybe use nice bit ops?
    return (size_t) ceil(bits / 8.0);
}

/// Read contents of a file to a T.
/// T needs to behave like a vector of bytes,
/// eg it should be either std::string, std::vector<char>
/// or std::vector<uint8_t>
template<class T>
T read_file_to_stl_byte_container(std::string& filename) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in) {
        T contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read((char*)&contents[0], contents.size());
        in.close();
        return(contents);
    }
    throw(errno);
}

/// Read contents of a stream S to a T.
/// T needs to behave like a vector of bytes,
/// eg it should be either std::string, std::vector<char>
/// or std::vector<uint8_t>
template<class T, class S>
T read_stream_to_stl_byte_container(S& stream) {
    T vector;
    char c;
    while (stream.get(c)) {
        vector.push_back(typename T::value_type(c));
    }
    return(vector);
}

/// A wrapper around a istream that reads from
/// a existing memory buffer.
class ViewStream {
    struct membuf : std::streambuf {
        membuf(char* begin, size_t size) {
            this->setg(begin, begin, begin + size);
        }
    };

    std::unique_ptr<membuf> mb;
    std::unique_ptr<std::istream> m_stream;

public:
    ViewStream(char* begin, size_t size) {
        mb = std::unique_ptr<ViewStream::membuf>(
            new ViewStream::membuf { begin, size }
        );
        m_stream = std::unique_ptr<std::istream> {
            new std::istream(&*mb)
        };
    }

    std::istream& stream() {
        return *m_stream;
    }
};

/// A ostream that writes bytes into a stl byte container.
class BackInsertStream {
    // NB: Very hacky implementation right now...

    std::vector<uint8_t>* buffer;
    std::unique_ptr<
        boost::iostreams::stream_buffer<
            boost::iostreams::back_insert_device<
                std::vector<char>>>> outBuf;
    std::unique_ptr<std::stringstream> ss;
    std::ostream* o;

public:
    BackInsertStream(std::vector<uint8_t>& buf) {
        buffer = &buf;
        outBuf = std::unique_ptr<
            boost::iostreams::stream_buffer<
                boost::iostreams::back_insert_device<
                    std::vector<char>>>> {
            new boost::iostreams::stream_buffer<
                    boost::iostreams::back_insert_device<
                        std::vector<char>>> {
                // TODO: Very iffy cast here...
                *((std::vector<char>*) buffer)
            }
        };
        ss = std::unique_ptr<std::stringstream> {
            new std::stringstream()
        };
        o = &*ss;
        o->rdbuf(&*outBuf);
    }

    std::ostream& stream() {
        return *o;
    }
};

inline size_t read_file_size(std::string& file) {
    std::ifstream t(file);
    if (t) {
        t.seekg(0, std::ios::end);
        return t.tellg();
    }
    throw(errno);
}

}

#endif
