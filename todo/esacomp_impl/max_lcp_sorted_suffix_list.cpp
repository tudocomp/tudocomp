#include "max_lcp_sorted_suffix_list.h"

namespace esacomp {

MaxLCPSortedSuffixList::MaxLCPSortedSuffixList(SdslVec sa_, SdslVec lcp_, size_t threshold_): threshold(threshold_), sa(std::move(sa_)), lcp(std::move(lcp_)) {

    // TODO: fix this
    size_t inputLength = sa.size();

    // Find maximum LCP
    size_t maxLCP = 0;
    for (size_t i = 0; i < lcp.size(); i++) {
        size_t p = lcp[i];
        if (p > maxLCP) {
            maxLCP = p;
        }
    }

    // TODO: Make this less memory hungry
    //Allocate
    this->prev = std::vector<ssize_t>(inputLength, -1);
    this->next = std::vector<ssize_t>(inputLength, -1);

    this->lcpIndex = std::vector<ssize_t>(maxLCP + 1, -1);
    this->suffixRef = std::vector<ssize_t>(inputLength, -1);

    for (size_t i = 0; i < inputLength; i++) {
        if (lcp[i] >= threshold) {
            insert(i);
        }
    }
}

void MaxLCPSortedSuffixList::insert(size_t i) {
    size_t lcp = getLCP(i);

    ssize_t pos = floorIndexEntry(lcp);
    if (pos == -1) {
        //link as last
        if (last != -1) {
            next[last] = i;
        }

        prev[i] = last;
        last = i;
    } else {
        //link at pos
        ssize_t oldPrev = prev[pos];

        prev[pos] = i;
        if (oldPrev >= 0) {
            next[oldPrev] = i;
        }

        prev[i] = oldPrev;
        next[i] = pos;
    }

    //update index
    lcpIndex[lcp] = i;

    //update maximum
    if (max == -1 || getLCP(max) <= lcp) {
        max = i;
    }

    //update suffix reference
    suffixRef[getSuffix(i)] = i;

    //update num entries
    numEntries++;
}

ssize_t MaxLCPSortedSuffixList::floorIndexEntry(size_t lcp_) {
    ssize_t lcp = lcp_;
    while (lcpIndex[lcp] == -1) {
        lcp--;

        if (lcp < 0) {
            return -1;
        }
    }

    return lcpIndex[lcp];
}

size_t MaxLCPSortedSuffixList::getMaxEntry() {
    return max;
}

size_t MaxLCPSortedSuffixList::getEntryForSuffix(size_t suffix) {
    return suffixRef[suffix];
}

size_t MaxLCPSortedSuffixList::getSize() {
    return numEntries;
}

size_t MaxLCPSortedSuffixList::getSuffix(size_t i) {
    return sa[i];
}

size_t MaxLCPSortedSuffixList::getSharedSuffix(size_t i) {
    return sa[i - 1];
}

size_t MaxLCPSortedSuffixList::getLCP(size_t i) {
    return lcp[i];
}

void MaxLCPSortedSuffixList::remove(size_t i) {
    //unlink
    if (prev[i] >= 0) {
        next[prev[i]] = next[i];
    }

    if (next[i] >= 0) {
        prev[next[i]] = prev[i];
    }

    //update head and tail
    if (ssize_t(i) == last) {
        last = prev[i];
    }

    if (ssize_t(i) == max) {
        max = next[i];
    }

    //update index
    size_t lcp = getLCP(i);
    if (ssize_t(i) == lcpIndex[lcp]) {
        ssize_t k = next[i];
        if (k >= 0 && getLCP(k) == lcp) {
            lcpIndex[lcp] = k; //move to next entry with same LCP
        } else {
            lcpIndex[lcp] = -1; //invalidate
        }
    }

    //update suffix ref map
    suffixRef[getSuffix(i)] = -1;

    //update num entries
    numEntries--;
}

void MaxLCPSortedSuffixList::updateLCP(size_t i, size_t newLCP) {
    remove(i);
    lcp[i] = newLCP;

    if (newLCP >= threshold) {
        insert(i);
    }
}

std::string MaxLCPSortedSuffixList::toString() {
    std::stringstream stream;

    stream << "[";
    for (size_t i = max; i >= 0; i = next[i]) {
        stream << (getSuffix(i) + 1) << ", ";
    }
    stream << "]";

    return stream.str();
}

}
