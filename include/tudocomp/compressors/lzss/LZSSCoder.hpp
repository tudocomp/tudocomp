#pragma once

#include <tudocomp/io.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {

static inline constexpr TypeDesc lzss_coder_type() {
    return TypeDesc("lzss_coder");
}

static inline constexpr TypeDesc lzss_bidirectional_coder_type() {
    return TypeDesc("lzss_bidirectional_coder", lzss_coder_type());
}

namespace lzss {

/// \brief Base coder for LZ77-style factorizations in a
///        Storer-Symanszki manner (references and literal symbols).
///
/// \tparam ref_coder_t coder type for referred positions of factors
/// \tparam len_coder_t coder type for factor lengths
/// \tparam lit_coder_t coder type for literal symbols
template<typename ref_coder_t, typename len_coder_t, typename lit_coder_t>
class LZSSCoder : public Algorithm {
public:
    static inline Meta meta(Meta tmpl) {
        tmpl.param("ref").strategy<ref_coder_t>(TypeDesc("coder"));
        tmpl.param("len").strategy<len_coder_t>(TypeDesc("coder"));
        tmpl.param("lit").strategy<lit_coder_t>(TypeDesc("coder"));
        return tmpl;
    }

    using Algorithm::Algorithm;

    template<
        template<typename,typename,typename> class lzss_encoder_t,
        typename literals_t
    >
    inline auto encoder(Output& output, literals_t&& literals) {
        auto out = std::make_shared<BitOStream>(output);
        return lzss_encoder_t<
            typename ref_coder_t::Encoder,
            typename len_coder_t::Encoder,
            typename lit_coder_t::Encoder>
        (
            this->config(),
            std::make_unique<typename ref_coder_t::Encoder>(
                this->config().sub_config("ref"), out, NoLiterals()),
            std::make_unique<typename len_coder_t::Encoder>(
                this->config().sub_config("len"), out, NoLiterals()),
            std::make_unique<typename lit_coder_t::Encoder>(
                this->config().sub_config("lit"), out, std::move(literals))
        );
    }

    template<template<typename,typename,typename> class lzss_decoder_t>
    inline auto decoder(Input& input) {
        auto in = std::make_shared<BitIStream>(input);
        return lzss_decoder_t<
            typename ref_coder_t::Decoder,
            typename len_coder_t::Decoder,
            typename lit_coder_t::Decoder>
        (
            this->config(),
            std::make_unique<typename ref_coder_t::Decoder>(
                this->config().sub_config("ref"), in),
            std::make_unique<typename len_coder_t::Decoder>(
                this->config().sub_config("len"), in),
            std::make_unique<typename lit_coder_t::Decoder>(
                this->config().sub_config("lit"), in)
        );
    }
};

}}
