#pragma once

#include <tudocomp/io.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/Range.hpp>

namespace tdc {

/// \brief Base for data encoders.
///
/// Used for encoding integer values to a certain bit representation.
class Encoder : public Algorithm {

protected:
    /// The underlying bit output stream.
    std::shared_ptr<BitOStream> m_out;

public:
    /// \brief Constructor.
    ///
    /// \tparam literals_t The literal iterator type.
    ///
    /// \param env The algorithm's environment.
    /// \param out The bit stream to write to.
    /// \param literals The literal iterator.
    template<typename literals_t>
    inline Encoder(
        Env&& env,
        std::shared_ptr<BitOStream> out,
        literals_t&& literals)
        : Algorithm(std::move(env)), m_out(out) {
    }

    /// \brief Convenience constructor.
    ///
    /// \tparam literals_t The literal iterator type.
    ///
    /// \param env The algorithm's environment.
    /// \param out The output to write to.
    /// \param literals The literal iterator.
    template<typename literals_t>
    inline Encoder(Env&& env, Output& out, literals_t&& literals)
        : Encoder(
            std::move(env),
            std::make_shared<BitOStream>(out),
            literals) {
    }

    /// \brief Encodes an arbitrary-range integer value.
    ///
    /// This default implementation computes the amount of bits required to
    /// store an integer value of the given range and writes its binary
    /// representation to the output.
    ///
    /// \tparam value_t The input value type.
    /// \param v The integer value to encode.
    /// \param r The value range.
    template<typename value_t>
    inline void encode(value_t v, const Range& r) {
        m_out->write_int(v - r.min(), bits_for(r.max() - r.min()));
    }

    /// \brief Encodes a bit.
    ///
    /// This default implementation will write exactly one bit to the output.
    /// A zero bit is written if the given value is zero, a one bit is written
    /// otherwise.
    ///
    /// \tparam value_t The input value type.
    /// \param v The value to encode.
    /// \param r Unused.
    template<typename value_t>
    inline void encode(value_t v, const BitRange& r) {
        m_out->write_bit(v);
    }

    inline const std::shared_ptr<BitOStream>& stream() {
        return m_out;
    }
};

/// \brief Base for data decoders.
///
/// Used for decoding integer values from a certain bit representation.
class Decoder : public Algorithm {

protected:
    /// The underlying bit input stream.
    std::shared_ptr<BitIStream> m_in;

public:
    /// \brief Constructor.
    ///
    /// \param env The algorithm's environment.
    /// \param in The bit stream to read from.
    inline Decoder(Env&& env, std::shared_ptr<BitIStream> in)
        : Algorithm(std::move(env)), m_in(in) {
    }

    /// \brief Convenience constructor.
    ///
    /// \param env The algorithm's environment.
    /// \param in The input to read from.
    inline Decoder(Env&& env, Input& in)
        : Decoder(std::move(env), std::make_shared<BitIStream>(in)) {
    }

    /// \brief Tests whether the end of the bit input stream has been reached.
    /// \return \e true if the end of the underlying bit stream has been
    ///         reached, \e false otherwise.
    inline bool eof() const {
        return m_in->eof();
    }

    /// \brief Decodes an arbitrary-range integer value.
    ///
    /// This default implementation computes the amount of bits required to
    /// store an integer value of the given range and reads its binary
    /// representation from the input.
    ///
    /// \tparam value_t The return value type.
    /// \param r The value range.
    /// \return The decoded integer value.
    template<typename value_t>
    inline value_t decode(const Range& r) {
        return value_t(r.min()) +
               m_in->read_int<value_t>(bits_for(r.max() - r.min()));
    }

    /// \brief Decodes a bit.
    ///
    /// This default implementation will read exactly one bit from the input.
    ///
    /// \tparam value_t The return value type.
    /// \param r Unused.
    /// \return The decoded bit value (zero or one).
    template<typename value_t>
    inline value_t decode(const BitRange& r) {
        return value_t(m_in->read_bit());
    }

    inline const std::shared_ptr<BitIStream>& stream() {
        return m_in;
    }
};

/// \brief Defines constructors for clases inheriting from \ref tdc::Decoder.
///
/// This includes a convenience constructor that automatically opens a
/// bit input stream on an \ref tdc::Input.
///
/// \param env The variable name for the environment.
/// \param in The variable name for the bit input stream.
#define DECODER_CTOR(env, in)                                \
        inline Decoder(Env&& env, Input& in)                 \
             : Decoder(std::move(env),                       \
                       std::make_shared<BitIStream>(in)) {}  \
        inline Decoder(Env&& env, std::shared_ptr<BitIStream> in) \
            : tdc::Decoder(std::move(env), in)

}

