#ifndef _INCLUDED_LZSS_COMPRESSPR_HPP_
#define _INCLUDED_LZSS_COMPRESSPR_HPP_

#include <vector>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/io.h>
#include <tudocomp/lzss/LZSSCoderOpts.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

/// Base for Lempel-Ziv-Storer-Szymanski-based compressors.
///
/// Compressors inheriting this class will factorize the text, achieving
/// compression by replacing redundant phrases by references to their other
/// occurences.
///
/// \tparam C the coder to use for encoding factors.
template<typename C>
class LZSSCompressor : public Compressor {

protected:
    std::vector<LZSSFactor> m_factors;

private:
    C* m_coder;

public:
    /// Default constructor (not supported).
    inline LZSSCompressor() = delete;

    /// Constructor.
    ///
    /// \param env The environment.
    using Compressor::Compressor;

    /// \copydoc
    virtual void compress(Input& main_input, Output& output) override final {

        //init factorization (possibly factorize offline to buffer)
        Input input_copy_1(main_input);
        bool factorized = pre_factorize(input_copy_1);

        //instantiate coder
        Input input_copy_2(main_input);
        Input input_copy_3(main_input);
        auto out_guard = output.as_stream();
        m_coder = new C(env(), input_copy_2, out_guard, coder_opts(input_copy_3));

        //pass factor buffer (empty or filled)
        m_coder->set_buffer(m_factors);

        if(!factorized && !m_coder->uses_buffer()) {
            //init (online)
            m_coder->encode_init();
        }

        //factorize
        if(!factorized) {
            Input input_copy_4(main_input);
            factorize(input_copy_4);
            handle_eof();
        }

        //encode
        if(factorized || m_coder->uses_buffer()) {

            size_t len = main_input.size();

            auto in_guard = main_input.as_stream();
            std::istream& in_stream = *in_guard;

            //init (offline)
            m_coder->encode_init();

            //factors must be sorted by insert position!
            char c;
            size_t p = 0;

            for(auto f : m_factors) {
                while(p < f.pos) {
                    in_stream.get(c);
                    m_coder->encode_sym(uint8_t(c));
                    ++p;
                }

                m_coder->encode_sym_flush();
                m_coder->encode_fact(f);

                size_t skip = f.num;
                while(skip--) in_stream.get(c);
                p += f.num;
            }

            while(p < len) {
                in_stream.get(c);
                m_coder->encode_sym(uint8_t(c));
                ++p;
            }

            m_coder->encode_sym_flush();
        }

        //clean up
        delete m_coder;
        m_coder = nullptr;
    }

    /// \copydoc
    virtual void decompress(Input& input, Output& output) override {
        C::decode(env(), input, output);
    }

protected:
    /// Handles a freshly generated factor during factorization.
    ///
    /// Depending on how the encoder works, it will be encoded directly or
    /// buffered for later optimized encoding.
    void handle_fact(const LZSSFactor& f) {
        if(m_coder->uses_buffer()) {
            m_coder->buffer_fact(f);
        } else {
            m_coder->encode_sym_flush();
            m_coder->encode_fact(f);
        }
    }

    /// Handles a raw symbol during factorization.
    ///
    /// In case the coder opts to buffer factors instead of encoding directly,
    /// this will not do anything.
    void handle_sym(uint8_t sym) {
        if(!m_coder->uses_buffer()) {
            m_coder->encode_sym(sym);
        }
    }

private:
    void handle_eof() {
        if(!m_coder->uses_buffer()) {
            m_coder->encode_sym_flush();
        }
    }

protected:
    /// Allows the compressor to do work prior to the factorization stage.
    ///
    /// \return true if factorization has already been completed. The factors
    ///         are expected to be located in the buffer and sorted (!).
    virtual bool pre_factorize(Input& input) = 0;

    /// Determines the options passed to the encoder.
    virtual LZSSCoderOpts coder_opts(Input& input) = 0;

    /// Factorizes the input and uses the handlers (handle_fact and handle_sym)
    /// to pass them to the encoder.
    virtual void factorize(Input& input) = 0;
};

}}

#endif
