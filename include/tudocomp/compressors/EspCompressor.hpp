#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/compressors/esp/pre_header.hpp>

namespace tdc {

class EspCompressor: public Compressor {

public:
    inline static Meta meta() {
        Meta m("compressor", "esp", "ESP based grammar compression");
        //m.option("coder").templated<coder_t>();
        //m.option("min_run").dynamic("3");
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        using namespace esp;

        const size_t input_size = input.size();
        DCHECK(input_size > 0 /* 0-byte input not covered by paper */);

        auto in = input.as_stream();
        std::vector<size_t> work_buf;

        work_buf.reserve(input_size);

        for (auto b : in) {
            work_buf.push_back(b);
        }

        DCHECK_EQ(work_buf.size(), input_size);

        size_t alphabet_size = 256;
        size_t iteration = 0;
        while(work_buf.size() > 1) {
            std::cout << "\n>>> PARTITION "<< iteration <<" <<<\n";
            auto labeled_blocks = labeled_partition<size_t>(work_buf, alphabet_size);
            DCHECK(initial_labeled_blocks_debug(labeled_blocks.vec,
                                                ConstGenericView<size_t>(work_buf)));


            break;
            iteration++;
        }

        /*
        if (input.size() == 1) {
            std::cout << "done 0\n";
        } else {
            // initial
            std::cout << "\n>>> PARTITION 0 <<<\n";
            size_t alphabet_size = 256;
            auto labeled_blocks = labeled_partition(in, alphabet_size);
            DCHECK(initial_labeled_blocks_debug(labeled_blocks.vec, in));
            std::cout << "\n>>> PARTITION 1 <<<\n";

            std::vector<size_t> new_string;
            for (auto& e : labeled_blocks.vec) {
                new_string.push_back(e.type);
            }

            if (new_string.size() == 1) {
                std::cout << "done 1\n";
            } else {
                auto new_alphabet_size = labeled_blocks.alphabet_size;
                std::cout << "New alphabet size: " << new_alphabet_size << "\n";
                auto new_labeled_blocks = labeled_partition<size_t>(new_string, new_alphabet_size);
                print_labeled_blocks2(new_labeled_blocks.vec);
            }

            while (false) {

            }
        }*/
    }

    inline virtual void decompress(Input& input, Output& output) override {

    }
};

}
