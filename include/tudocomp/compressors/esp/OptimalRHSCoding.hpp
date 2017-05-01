#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/SLP.hpp>
#include <tudocomp/compressors/esp/MonotoneSubsequences.hpp>
#include <tudocomp/compressors/esp/ArithmeticCoder.hpp>
#include <tudocomp/compressors/esp/HuffmanCoder.hpp>
#include <tudocomp/compressors/esp/SubseqStrategy.hpp>

namespace tdc {namespace esp {
    //template<typename coder_t>
    class DHuffman: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("d_coding", "huffman");
            //m.option("coder").templated<coder_t, HuffmanCoder>("coder");
            return m;
        };

        using Algorithm::Algorithm;

        template<typename rhs_t>
        inline void encode(const rhs_t& rhs, std::shared_ptr<BitOStream>& out, size_t bit_width, size_t max_value) const {
            HuffmanEncoder encoder { out, rhs };

            for (size_t i = 0; i < rhs.size(); i++) {
                encoder.encode(rhs[i]);
            }
        }
        template<typename rhs_t>
        inline void decode(rhs_t& rhs, std::shared_ptr<BitIStream>& in, size_t bit_width, size_t max_value) const {
            HuffmanDecoder decoder { in };

            for (size_t i = 0; i < rhs.size(); i++) {
                rhs[i] = decoder.decode();
            }
        }
    };
    class DArithmetic: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("d_coding", "arithmetic");
            //m.option("coder").templated<coder_t, HuffmanCoder>("coder");
            return m;
        };

        using Algorithm::Algorithm;

        template<typename rhs_t>
        inline void encode(const rhs_t& rhs, std::shared_ptr<BitOStream>& out, size_t bit_width, size_t max_value) const {
            ArithmeticEncoder encoder { out, rhs };

            for (size_t i = 0; i < rhs.size(); i++) {
                encoder.encode(rhs[i]);
            }
        }
        template<typename rhs_t>
        inline void decode(rhs_t& rhs, std::shared_ptr<BitIStream>& in, size_t bit_width, size_t max_value) const {
            ArithmeticDecoder decoder { in };

            for (size_t i = 0; i < rhs.size(); i++) {
                rhs[i] = decoder.decode();
            }
        }
    };
    class DPlain: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("d_coding", "plain");
            return m;
        };

        using Algorithm::Algorithm;

        template<typename rhs_t>
        inline void encode(const rhs_t& rhs, BitOStream& out, size_t bit_width, size_t max_value) const {
            for(size_t i = 0; i < rhs.size(); i++) {
                out.write_int(rhs[i], bit_width);
            }
        }
        template<typename rhs_t>
        inline void decode(rhs_t& rhs, BitIStream& in, size_t bit_width, size_t max_value) const {
            for(size_t i = 0; i < rhs.size(); i++) {
                rhs[i] = in.read_int<size_t>(bit_width);
            }
        }
        template<typename rhs_t>
        inline void encode(const rhs_t& rhs, std::shared_ptr<BitOStream>& out, size_t bit_width, size_t max_value) const {
            encode(rhs, *out, bit_width, max_value);
        }
        template<typename rhs_t>
        inline void decode(rhs_t& rhs, std::shared_ptr<BitIStream>& in, size_t bit_width, size_t max_value) const {
            decode(rhs, *in, bit_width, max_value);
        }
    };
    class DWaveletTree: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("d_coding", "wavelet_tree");
            return m;
        };

        using Algorithm::Algorithm;

        template<typename rhs_t>
        inline void encode(const rhs_t& rhs, BitOStream& bout, size_t bit_width, size_t max_value) const {
            auto bvs = make_wt(rhs, max_value);

            // write WT depth
            bout.write_compressed_int(bvs.size());

            {
                // write WT
                for(auto& bv : bvs) {
                    for(uint8_t bit: bv) {
                        bout.write_bit(bit != 0);
                    }
                }
            }
        }
        template<typename rhs_t>
        inline void decode(rhs_t& rhs, BitIStream& bin, size_t bit_width, size_t max_value) const {
            // Read WT dept
            size_t wt_depth = bin.read_compressed_int<size_t>();

            auto Dcombined_bvs = std::vector<IntVector<uint_t<1>>>();
            Dcombined_bvs.reserve(wt_depth);
            Dcombined_bvs.resize(wt_depth);
            {
                for(auto& bv : Dcombined_bvs) {
                    bv.reserve(rhs.size());

                    for(size_t i = 0; i < rhs.size(); i++) {
                        bv.push_back(bin.read_bit());
                    }
                }

                auto Dcombined = esp::recover_Dxx(Dcombined_bvs, rhs.size());
                for (size_t i = 0; i < rhs.size(); i++) {
                    rhs[i] = Dcombined[i];
                }
            }
        }
        template<typename rhs_t>
        inline void encode(const rhs_t& rhs, std::shared_ptr<BitOStream>& out, size_t bit_width, size_t max_value) const {
            encode(rhs, *out, bit_width, max_value);
        }
        template<typename rhs_t>
        inline void decode(rhs_t& rhs, std::shared_ptr<BitIStream>& in, size_t bit_width, size_t max_value) const {
            decode(rhs, *in, bit_width, max_value);
        }
    };
    template<typename subseq_t = SubSeqOptimal, typename d_coding_t = DWaveletTree>
    class DMonotonSubseq: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("d_coding", "succinct");
            m.option("subseq").templated<subseq_t, SubSeqOptimal>("subseq");
            m.option("dx_coder").templated<d_coding_t, DWaveletTree>("d_coding");
            return m;
        };

        using Algorithm::Algorithm;
        template<typename rhs_t>
        inline void encode(const rhs_t& rhs, std::shared_ptr<BitOStream>& out, size_t bit_width, size_t max_value) const {
            BitOStream& bout = *out;

            auto phase1 = StatPhase("Sorting D");

            // Sort rhs and create sorted indice array O(n log n)
            const auto sis = sorted_indices(rhs);

            //std::cout << "sis: " << vec_to_debug_string(sis) << "\n";

            phase1.split("Write out B array");

            // Write rules rhs in sorted order (B array of encoding)
            {
                size_t last = 0;
                for (size_t i = 0; i < rhs.size(); i++) {
                    auto v = rhs[sis[i]];
                    //TMP_Bde.push_back(v);
                    DCHECK_LE(last, v);
                    size_t diff = v - last;
                    bout.write_unary(diff);
                    last = v;
                }
            }

            phase1.split("Create Dpi and b arrays");

            // Create Dpi and b
            std::vector<size_t> Dpi;
            auto b = IntVector<uint_t<1>> {};
            {
                const subseq_t subseq { this->env().env_for_option("subseq") };
                auto tmp = subseq.create_dpi_and_b_from_sorted_indices(sis);
                Dpi = std::move(tmp.Dpi);
                b = std::move(tmp.b);
            }
            size_t b_size = b.size();
            //TMP_b = b;
            //TMP_Dpi = Dpi;
            DCHECK_GE(b_size, 1);

            //std::cout << "Dpi: " << vec_to_debug_string(Dpi) << "\n";
            //std::cout << "b:   " << vec_to_debug_string(b) << "\n";

            phase1.split("Write out b, discard");

            // Write out b and discard it
            bout.write_compressed_int(b_size);
            for(uint8_t e : b) {
                bout.write_bit(e != 0);
            }
            b = IntVector<uint_t<1>> {};

            phase1.split("Create Dsi from Dpi");
            auto Dsi = esp::create_dsigma_from_dpi_and_sorted_indices(sis, Dpi);

            // TODO: change the two function to write in same vector
            phase1.split("Combine Dsi and Dpi arrays");

            auto Dcombined = std::move(Dpi);
            Dcombined.reserve(Dcombined.size() * 2);
            for (auto x : Dsi) {
                Dcombined.push_back(x);
            }
            Dsi = std::vector<size_t>();

            phase1.split("Encode Dcombined");

            const d_coding_t coder { this->env().env_for_option("dx_coder") };

            auto d_max_value = b_size - 1;
            auto d_bit_width = bits_for(d_max_value);

            coder.encode(Dcombined, out, d_bit_width, d_max_value);
        }
        template<typename rhs_t>
        inline void decode(rhs_t& D, std::shared_ptr<BitIStream>& in, size_t bit_width, size_t max_value) const {
            BitIStream& bin = *in;
            auto slp_size = D.size();

            // TODO: Read later, requires splitting up input a bit
            // Read rules rhs in sorted order (B array)
            std::vector<size_t> Bde;
            Bde.reserve(slp_size);
            {
                size_t last = 0;
                for(size_t i = 0; i < slp_size && !bin.eof(); i++) {
                    // ...
                    auto diff = bin.read_unary<size_t>();
                    last += diff;
                    Bde.push_back(last);
                }
            }
            //std::cout << vec_to_debug_string(Bde) << "\n";

            // Read b
            size_t b_size = bin.read_compressed_int<size_t>();
            auto b = IntVector<uint_t<1>> {};
            b.reserve(b_size);
            b.resize(b_size);
            for(size_t i = 0; i < b_size; i++) {
                b[i] = bin.read_bit();
            }

            // Read Dpi WT
            // Read Dsi WT
            auto Dcombined = std::vector<size_t>();
            Dcombined.reserve(slp_size * 2);
            Dcombined.resize(slp_size * 2);

            const d_coding_t coder { this->env().env_for_option("dx_coder") };

            auto d_max_value = b_size - 1;
            auto d_bit_width = bits_for(d_max_value);

            coder.decode(Dcombined, in, d_bit_width, d_max_value);

            auto Dpi = ConstGenericView<size_t>(Dcombined).slice(0, slp_size);
            auto Dsi = ConstGenericView<size_t>(Dcombined).slice(slp_size, slp_size * 2);

            esp::recover_D_from_encoding(Dpi, Dsi, b, Bde, &D);
        }
    };
}}
