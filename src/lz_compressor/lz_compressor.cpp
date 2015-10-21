#include <vector>
#include <cmath>
#include <climits>
#include <iostream>
#include <algorithm>

#include "glog/logging.h"
#include "sdsl/int_vector.hpp"

#include "lz_compressor.h"

namespace lz_compressor {

Rules LZCompressor::compress(SdslVec sa, SdslVec lcp, size_t threshold) {
    size_t n = sa.size();
    CHECK_EQ(sa.size(), n);
    CHECK_EQ(sa.size(), lcp.size());

    size_t l = ceil(log2(n-1))+1;

    // Compute inverse SA.
    // TODO: Somehow pipe in the sdsl one
    // MAYBE COPY 7
    sdsl::int_vector<> saInv;
    saInv.width(l);
    saInv.resize(n);

    for (size_t i = 0; i < n; i++) {
        size_t tmp = sa[i];
        saInv[tmp] = i;
    }

    // TODO
    //if (printProgress) {
    //    std::cout << "Computing LZ factorization ...\n";
    //}

    Rules rules;

    /**
        * Process ESA and compute LZ factorization.
        */
    size_t i = 0;
    while (i < n) {
        //get SA position for suffix i
        size_t h = saInv[i];

        //search "upwards" in LCP array
        //include current, exclude last
        size_t p1 = lcp[h];
        ssize_t h1 = h - 1;
        if (p1 > 0) {
            while (h1 >= 0 && sa[h1] > sa[h]) {
                p1 = std::min(p1, size_t(lcp[h1--]));
            }
        }
        // ^
        // calculates the minimum lcp across more than adjacent positions
        // == longest common prefix among all string from h to h1
        // != longest common prefix among h and h1

        //search "downwards" in LCP array
        //exclude current, include last
        size_t p2 = 0;
        size_t h2 = h + 1;
        if (h2 < n) {
            p2 = SSIZE_MAX;
            do {
                p2 = std::min(p2, size_t(lcp[h2]));
                if (sa[h2] < sa[h]) {
                    break;
                }
            } while (++h2 < n);

            if (h2 >= n) {
                p2 = 0;
            }
        }

        //select maximum
        size_t p = std::max(p1, p2);
        if (p >= threshold) {
            //introduce rule
            auto rule = Rule {i, sa[p == p1 ? h1 : h2], p};
            rules.push_back(rule);
            i += p;
        } else {
            //next symbol
            i++;
        }
    }

    DLOG(INFO) << "lz exit";

    // target positions are already generated in order
    //std::sort(rules.begin(), rules.end(), rule_compare {});
    return std::move(rules);
}

}
