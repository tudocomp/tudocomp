#pragma once

#include <tudocomp/Range.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {
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
        tmpl.option("ref").templated<ref_coder_t>("coder");
        tmpl.option("len").templated<len_coder_t>("coder");
        tmpl.option("lit").templated<lit_coder_t>("coder");
        return tmpl;
    }

    using Algorithm::Algorithm;

    template<
        template<typename,typename,typename> typename lzss_encoder_t,
        typename literals_t
    >
    inline auto encoder(Output& output, literals_t&& literals) {
        auto out = std::make_shared<BitOStream>(output);
        return lzss_encoder_t<
            typename ref_coder_t::Encoder,
            typename len_coder_t::Encoder,
            typename lit_coder_t::Encoder>
        (
            this->env(),
            std::make_unique<typename ref_coder_t::Encoder>(
                this->env().env_for_option("ref"), out, NoLiterals()),
            std::make_unique<typename len_coder_t::Encoder>(
                this->env().env_for_option("len"), out, NoLiterals()),
            std::make_unique<typename lit_coder_t::Encoder>(
                this->env().env_for_option("lit"), out, std::move(literals))
        );
    }

    template<template<typename,typename,typename> typename lzss_decoder_t>
    inline auto decoder(Input& input) {
        auto in = std::make_shared<BitIStream>(input);
        return lzss_decoder_t<
            typename ref_coder_t::Decoder,
            typename len_coder_t::Decoder,
            typename lit_coder_t::Decoder>
        (
            this->env(),
            std::make_unique<typename ref_coder_t::Decoder>(
                this->env().env_for_option("ref"), in),
            std::make_unique<typename len_coder_t::Decoder>(
                this->env().env_for_option("len"), in),
            std::make_unique<typename lit_coder_t::Decoder>(
                this->env().env_for_option("lit"), in)
        );
    }
};

}}
