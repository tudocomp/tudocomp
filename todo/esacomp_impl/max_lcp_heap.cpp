#include "max_lcp_heap.h"

namespace esacomp {

MaxLCPHeap::MaxLCPHeap(SdslVec sa_, SdslVec lcp_, size_t threshold) {
    this->threshold = threshold;

    sa = sa_;
    lcp = lcp_;

    // This code assumes a suffix and lcp array to
    // text without null terminator, so skip entry 0
    // for all access to sa and lcp
    CHECK(sa.size() > 0);
    size_t inputLength = sa.size() - 1;
    offset = 1;

    suffixRef = std::vector<ssize_t>(inputLength);

    this->size = 0;

    CHECK(inputLength >= 0);
    heap = std::vector<size_t>((inputLength > 0) ? (inputLength - 1 ) : 0);

    //first suffix can never be on the left side in the LCP heap
    if (inputLength > 0) {
        suffixRef.at(sa[0 + offset]) = -1;
    }

    for (size_t i = 1; i < inputLength; i++) {
        if (lcp[i + offset] >= threshold) {
            //insert into heap
            heap.at(size) = i;

            //create reference
            suffixRef.at(sa[i + offset]) = size;

            //increase size
            size++;
        } else {
            //do not insert into heap
            suffixRef.at(sa[i + offset]) = -1;
        }
    }

    DLOG(INFO) << "suffixref " << vec_to_debug_string(suffixRef);
    DLOG(INFO) << "heap " << vec_to_debug_string(heap);

    //Make heap
    makeHeap();

    DLOG(INFO) << "initial sorted suffix list " << vec_to_debug_string(heap);
}

size_t MaxLCPHeap::getMaxEntry() {
    return 0;
}

size_t MaxLCPHeap::getEntryForSuffix(size_t suf) {
    DLOG(INFO) << "getEntryForSuffix suffixRef["<<suf<<"] " << suffixRef.at(suf);
    return suffixRef.at(suf);
}

size_t MaxLCPHeap::getSize() {
    return size;
}

size_t MaxLCPHeap::getSuffix(size_t i) {
    DLOG(INFO) << "getSuffix heap["<<i<<"] " << heap.at(i);
    size_t x = sa[heap.at(i) + offset];
    return x;
}

size_t MaxLCPHeap::getSharedSuffix(size_t i) {
    DLOG(INFO) << "getSharedSuffix heap["<<i<<"] - 1 " << heap.at(i) - 1;
    size_t x = sa[heap.at(i) - 1 + offset];
    return x;
}

size_t MaxLCPHeap::getLCP(size_t i) {
    DLOG(INFO) << "getLCP heap["<<i<<"] " << heap.at(i);
    size_t x = lcp[heap.at(i) + offset];
    return x;
}

void MaxLCPHeap::remove(size_t i) {
    removeFromHeap(i);
}

void MaxLCPHeap::updateLCP(size_t i, size_t newLCP) {
    DLOG(INFO) << "updateLCP("<<i<<", "<<newLCP<<") ";
    if (newLCP > getLCP(i)) {
        // TODO
        //throw new IllegalArgumentException("MaxLCPHeap does not support the increment of LCPs");
        return;
    }

    this->decreaseKey(i, newLCP);
}

std::string MaxLCPHeap::toString() {
    // TODO
    return "";
}

void MaxLCPHeap::setSize(size_t size) {
    this->size = size;
}

size_t MaxLCPHeap::getKey(size_t i) {
    return getLCP(i);
}

void MaxLCPHeap::setKey(size_t i, size_t newKey) {
    lcp[heap.at(i) + offset] = newKey;
}

void MaxLCPHeap::swap(size_t a, size_t b) {
    //swap entries
    size_t t = heap.at(a);
    heap.at(a) = heap.at(b);
    heap.at(b) = t;

    //update references
    suffixRef.at(sa[heap.at(a) + offset]) = a;
    suffixRef.at(sa[heap.at(b) + offset]) = b;
}

size_t MaxLCPHeap::get(size_t i) {
    return heap.at(i);
}

void MaxLCPHeap::decreaseKey(size_t i, size_t newKey) {
    DLOG(INFO) << "decreaseKey("<<i<<", "<<newKey<<")\n---";
    if (newKey < threshold) {
        removeFromHeap(i);
    } else {
        MaxHeap<size_t>::decreaseKey(i, newKey);
    }
}

size_t MaxLCPHeap::removeFromHeap(size_t i) {
    size_t x = MaxHeap<size_t>::removeFromHeap(i);
    suffixRef.at(sa[x + offset]) = -999;
    return x;
}

}
