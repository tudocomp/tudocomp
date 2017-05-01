#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/SLP.hpp>
#include <tudocomp/compressors/esp/MonotoneSubsequences.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>
#include <tudocomp/compressors/esp/ArithmeticCoder.hpp>
#include <tudocomp/compressors/esp/HuffmanCoder.hpp>

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
        inline void encode(const rhs_t& rhs, std::shared_ptr<BitOStream>& out, size_t bit_width) const {
            HuffmanEncoder encoder { out, rhs };

            for (size_t i = 0; i < rhs.size(); i++) {
                encoder.encode(rhs[i]);
            }
        }
        template<typename rhs_t>
        inline void decode(rhs_t& rhs, std::shared_ptr<BitIStream>& in, size_t bit_width) const {
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
        inline void encode(const rhs_t& rhs, std::shared_ptr<BitOStream>& out, size_t bit_width) const {
            ArithmeticEncoder encoder { out, rhs };

            for (size_t i = 0; i < rhs.size(); i++) {
                encoder.encode(rhs[i]);
            }
        }
        template<typename rhs_t>
        inline void decode(rhs_t& rhs, std::shared_ptr<BitIStream>& in, size_t bit_width) const {
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
        inline void encode(const rhs_t& rhs, BitOStream& out, size_t bit_width) const {
            for(size_t i = 0; i < rhs.size(); i++) {
                out.write_int(rhs[i], bit_width);
            }
        }
        template<typename rhs_t>
        inline void decode(rhs_t& rhs, BitIStream& in, size_t bit_width) const {
            for(size_t i = 0; i < rhs.size(); i++) {
                rhs[i] = in.read_int<size_t>(bit_width);
            }
        }
        template<typename rhs_t>
        inline void encode(const rhs_t& rhs, std::shared_ptr<BitOStream>& out, size_t bit_width) const {
            encode(rhs, *out, bit_width);
        }
        template<typename rhs_t>
        inline void decode(rhs_t& rhs, std::shared_ptr<BitIStream>& in, size_t bit_width) const {
            decode(rhs, *in, bit_width);
        }
    };
    //template<typename d_coding_t>
    class DMonotonSubseq: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("d_coding", "optimal");
            //m.option("dx_coder").templated<d_coding_t, HuffmanCoder>("d_coding");
            return m;
        };

        using Algorithm::Algorithm;
        template<typename rhs_t>
        inline void encode(const rhs_t& rhs, BitOStream& bout, size_t bit_width) const {
            /*
            std::vector<size_t> TMP_D;
            for(size_t i = 0; i < rhs.size(); i++) {
                TMP_D.push_back(rhs[i]);
            }
            std::vector<size_t> TMP_Bde;
            std::vector<size_t> TMP_Dpi;
            std::vector<size_t> TMP_Dsi;
            IntVector<uint_t<1>> TMP_b;
            */

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
                auto tmp = esp::create_dpi_and_b_from_sorted_indices(sis);
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

            phase1.split("Create WT for Dpi, write, discard");
            // transform Dpi to WT and write WT and discard WT
            {
                auto Dpi_bvs = make_wt(Dpi, b_size - 1);

                // write WT depth
                bout.write_compressed_int(Dpi_bvs.size());
                //std::cout << "wt depth write: " << Dpi_bvs.size() << "\n";

                // write WT
                for(auto& bv : Dpi_bvs) {
                    for(uint8_t bit: bv) {
                        bout.write_bit(bit != 0);
                    }
                    //std::cout << "Dpi bv: " << vec_to_debug_string(bv) << "\n";
                }
            }

            phase1.split("Create Dsi, discard Dpi");

            // Create Dsi from Dpi, discard Dpi,
            auto Dsi = esp::create_dsigma_from_dpi_and_sorted_indices(sis, Dpi);
            //TMP_Dsi = Dsi;

            //std::cout << "Dsi: " << vec_to_debug_string(Dsi) << "\n";

            Dpi = std::vector<size_t>();

            phase1.split("Create WT for Dsi, write, discard WT and Dsi");
            // transform Dsi to WT and write WT and discard WT
            {
                auto Dsi_bvs = make_wt(Dsi, b_size - 1);

                // write WT
                for(auto& bv : Dsi_bvs) {
                    for(uint8_t bit: bv) {
                        bout.write_bit(bit != 0);
                    }
                    //std::cout << "Dsi bv: " << vec_to_debug_string(bv) << "\n";
                }
            }

            // Discard Dpi
            Dsi = std::vector<size_t>();

            /*
            std::cout << "Bde: " << vec_to_debug_string(TMP_Bde) << "\n";
            std::cout << "Dpi: " << vec_to_debug_string(TMP_Dpi) << "\n";
            std::cout << "Dsi: " << vec_to_debug_string(TMP_Dsi) << "\n";
            std::cout << "b:   " << vec_to_debug_string(TMP_b) << "\n";
            std::cout << "D:   " << vec_to_debug_string(TMP_D) << "\n";
            std::cout << "\nencode OK\n\n";
            */
        }
        template<typename rhs_t>
        inline void decode(rhs_t& D, BitIStream& bin, size_t bit_width) const {
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
            //std::cout << "decomp b: " << vec_to_debug_string(b) << "\n";
            //std::cout << "ok " << __LINE__ << "\n";

            // Read WT dept
            size_t wt_depth = bin.read_compressed_int<size_t>();

            //std::cout << "ok " << __LINE__ << "\n";
            //std::cout << wt_depth << "\n";

            // Read Dpi WT
            auto Dpi = std::vector<size_t>();
            {
                auto Dpi_bvs = std::vector<IntVector<uint_t<1>>>();
                Dpi_bvs.reserve(wt_depth);
                Dpi_bvs.resize(wt_depth);

                //std::cout << "ok " << __LINE__ << "\n";

                for(auto& bv : Dpi_bvs) {
                    bv.reserve(slp_size);

                    //std::cout << "ok " << __LINE__ << "\n";

                    for(size_t i = 0; i < slp_size; i++) {
                        bv.push_back(bin.read_bit());
                    }
                    //std::cout << "Dpi bv: " << vec_to_debug_string(bv) << "\n";
                }

                Dpi = esp::recover_Dxx(Dpi_bvs, slp_size);
            }

            //std::cout << "ok " << __LINE__ << "\n";
            //std::cout << "Dpi: " << vec_to_debug_string(Dpi) << "\n";

            // Read Dsi WT
            auto Dsi = std::vector<size_t>();
            {
                auto Dsi_bvs = std::vector<IntVector<uint_t<1>>>();
                Dsi_bvs.reserve(wt_depth);
                Dsi_bvs.resize(wt_depth);

                for(auto& bv : Dsi_bvs) {
                    bv.reserve(slp_size);
                    for(size_t i = 0; i < slp_size; i++) {
                        bv.push_back(bin.read_bit());
                    }
                    //std::cout << "Dsi bv: " << vec_to_debug_string(bv) << "\n";
                }

                Dsi = esp::recover_Dxx(Dsi_bvs, slp_size);
            }
            //std::cout << "Dsi: " << vec_to_debug_string(Dsi) << "\n";

            //std::cout << "ok " << __LINE__ << "\n";

            // Given sis, b, Dsi and Dpi, recover slp rhs

            //std::cout << "Bde: " << vec_to_debug_string(Bde) << "\n";
            //std::cout << "Dpi: " << vec_to_debug_string(Dpi) << "\n";
            //std::cout << "Dsi: " << vec_to_debug_string(Dsi) << "\n";
            //std::cout << "b:   " << vec_to_debug_string(b) << "\n";


            esp::recover_D_from_encoding(Dpi, Dsi, b, Bde, &D);

            //std::cout << "D:   " << vec_to_debug_string(D) << "\n";
        }
        template<typename rhs_t>
        inline void encode(const rhs_t& rhs, std::shared_ptr<BitOStream>& out, size_t bit_width) const {
            encode(rhs, *out, bit_width);
        }
        template<typename rhs_t>
        inline void decode(rhs_t& rhs, std::shared_ptr<BitIStream>& in, size_t bit_width) const {
            decode(rhs, *in, bit_width);
        }
    };
}}
