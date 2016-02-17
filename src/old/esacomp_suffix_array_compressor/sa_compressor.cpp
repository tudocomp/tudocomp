#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>

#include "sdsl/construct.hpp"
#include "sdsl/construct_sa.hpp"
#include "sdsl/suffix_arrays.hpp"
#include "sdsl/lcp_bitcompressed.hpp"
#include "glog/logging.h"

#include "tudocomp.h"
#include "sa_compressor.h"

namespace esacomp {

Rules SACompressor::compress(Input& input, size_t threshold) {
    SuffixData suffixData = computeESA(input);
    // TODO: Possible move location
    return this->compress(std::move(suffixData.sa), std::move(suffixData.lcp), threshold);
}

std::string esa_to_string(Input& inp, const SdslVec& sa, const SdslVec& lcp) {
    using namespace std;
    stringstream ss;

    auto guard = inp.as_view();
    auto input = *guard;

    size_t n = input.size();

    ss << endl << "Enhanced Suffix Array:\n";

    if (input.size() > 200) {
        ss << "<input too large, skipping>";
    } else {
        ss << setw(2) << "S" << " | "
            << setiosflags(ios::left) << setw(n) << "Suffix" << " | "
            << setw(3) << "LCP" << endl
            << resetiosflags(ios::left);

        ss << setfill('-') << setw(n + 16) << "" << setfill(' ') << endl;

        for (size_t i = 0; i < n + 1; i++) {
            ss << setw(2)
                << sa[i] + 1
                << " | "
                << setiosflags(ios::left)
                << setw(n)
                << vec_as_lossy_string(input, sa[i])
                << " | "
                << resetiosflags(ios::left)
                << setw(3)
                << lcp[i]
                << endl;
        }
    }

    ss << endl;

    return ss.str();
}

SuffixData SACompressor::computeESA(Input& input_handle) {
    using namespace sdsl;
    using namespace std;

    // TODO: Avoid copy if read from file
    auto guard = input_handle.as_view();
    auto input = *guard;

    // Check that the input data is valid
    // TODO: Automatically ensure this by encoding "0"
    // some other way.
    bool contains_null = false;
    for(size_t i = 0; i < input.size(); i++) {
        contains_null |= (input[i] == 0);
    }
    CHECK(!contains_null);

    // TODO: Understand sdsl enough to know wether this is a good solution...

    // Ensure all temporary files are RAM-only
    // MAYBE BAD COPY 8
    map<string, string> mem_files;
    mem_files[conf::KEY_TEXT] = "_@ram.text";
    mem_files[conf::KEY_LCP] = "_@ram.lcp";
    mem_files[conf::KEY_SA] = "_@ram.sa";
    mem_files[conf::KEY_ISA] = "_@ram.isa";
    mem_files[conf::KEY_BWT] = "_@ram.bwt";
    cache_config config(false, ".", "MYID", mem_files);

    DLOG(INFO) << "conf::KEY_TEXT: " << cache_file_name(conf::KEY_TEXT, config) << endl;
    DLOG(INFO) << "conf::KEY_LCP: " << cache_file_name(conf::KEY_LCP, config) << endl;
    DLOG(INFO) << "conf::KEY_SA: " << cache_file_name(conf::KEY_SA, config) << endl;
    DLOG(INFO) << "conf::KEY_ISA: " << cache_file_name(conf::KEY_ISA, config) << endl;
    DLOG(INFO) << "conf::KEY_BWT: " << cache_file_name(conf::KEY_BWT, config) << endl;

    // Create copy of text so that we can append the zero symbol
    // TODO: If input is file path call load_vector_from_file instead
    size_t size = input.size();
    // COPY 1
    int_vector<8> text(size + 1);
    for (size_t i = 0; i < size; i++) {
        text[i] = input[i];
    }
    // fold appending of 0 symbol directly into the copy
    // append_zero_symbol(text);
    text[size] = 0;

    // BAD COPY 2
    // DLOG(INFO) << "text:     " << vec_as_lossy_string(text);
    // BAD COPY 3
    // DLOG(INFO) << "text + 0: " << vec_as_lossy_string(text);

    store_to_cache(text, conf::KEY_TEXT, config);
    text.bit_resize(0);
    sdsl::util::clear(text);

    DLOG(INFO) << "calculating sa";
    construct_sa<8>(config);
    SdslVec sa;
    load_from_cache(sa, conf::KEY_SA, config);
    // BAD COPY 4
    // DLOG(INFO) << "sa:  " << vec_to_debug_string(sa);

    DLOG(INFO) << "calculating lcp";
    //construct_lcp_kasai<8>(config);
    construct_lcp_PHI<8>(config);
    SdslVec lcp;
    load_from_cache(lcp, conf::KEY_LCP, config);
    // BAD COPY 5
    // DLOG(INFO) << "lcp: " << vec_to_debug_string(lcp);

    // BAD COPY 6 XXX
    // DLOG(INFO) << esa_to_string(input, sa, lcp);

    sdsl::remove(cache_file_name(conf::KEY_TEXT, config));
    sdsl::remove(cache_file_name(conf::KEY_LCP, config));
    sdsl::remove(cache_file_name(conf::KEY_SA, config));
    sdsl::remove(cache_file_name(conf::KEY_ISA, config));
    sdsl::remove(cache_file_name(conf::KEY_BWT, config));

    SuffixData sd { std::move(sa), std::move(lcp) };
    return std::move(sd);
}

}
