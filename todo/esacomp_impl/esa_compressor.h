#ifndef ESACOMPRESSOR_H
#define ESACOMPRESSOR_H

#include "tudocomp.h"
#include "esacomp/esacomp_rule_compressor.h"
#include "esacomp/rule.h"
#include "esacomp/rules.h"
#include "sa_compressor.h"
#include "max_lcp.h"
#include "max_lcp_sorted_suffix_list.h"
#include "max_lcp_heap.h"

namespace esacomp {

template<class Strategy = MaxLCPSortedSuffixList>
class ESACompressor: public SACompressor {
public:
    inline ESACompressor(Env& env): SACompressor(env) {}

    virtual Rules compress(SdslVec sa, SdslVec lcp, size_t threshold) override final
    {
        using namespace std;

        Strategy list_(std::move(sa), std::move(lcp), threshold);
        // TODO: This is silly
        Strategy* list = &list_;

        /*if (printSufList) {
            cout << endl;
            cout << "Initial LCP-sorted suffix list (" << list->getSize() << "):\n";
            cout << list->toString() << endl;
        }*/

        Rules rules;

        //Process while there are entries in the heap
        while (list->getSize() > 0) {
            DLOG(INFO) << "list size: " << list->getSize();
            // TODO progresslistenere

            //Extract maximum
            size_t maxEntry = list->getMaxEntry();
            DLOG(INFO) << "getMaxEntry: " << maxEntry;
            Rule rule {
                list->getSuffix(maxEntry),
                list->getSharedSuffix(maxEntry),
                list->getLCP(maxEntry)
            };

            rules.push_back(rule);

            DLOG(INFO) << "... adding rule: " << rule << endl;

            DLOG(INFO) << "eliminate overlapped suffixes from heap";

            //Eliminate overlapped suffixes from heap
            for (size_t k = 0; k < rule.num; k++) {
                ssize_t i = list->getEntryForSuffix(rule.target + k);
                if (i >= 0) { //is in list?
                    list->remove(i);
                }
            }

            DLOG(INFO) << "correct intersecting prefixes";

            //Correct intersecting prefixes
            ssize_t suffix;
            for (size_t k = 0; k < rule.num && (suffix = rule.target - k - 1) >= 0; k++) {
                ssize_t i = list->getEntryForSuffix(suffix);
                if (i >= 0) { //is in list?
                    if (suffix + list->getLCP(i) > rule.target) {
                        list->updateLCP(i, rule.target - suffix);
                    }
                }
            }
        }

        /*if (printTime) {
            // TODO
        }*/

        DLOG(INFO) << "sort start!";
        std::sort(rules.begin(), rules.end(), rule_compare {});
        DLOG(INFO) << "sorted!";
        return std::move(rules);
    }

    // If you wonder why this is here, google "C++ name hiding"
    using SACompressor::compress;
};

}

#endif
