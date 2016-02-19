#ifndef MAXHEAP_H
#define MAXHEAP_H

#include "tudocomp.h"

namespace esacomp {

/// A generic max heap.
template<class T>
class MaxHeap {
protected:
    static inline size_t left(size_t i) {
        return 2 * i + 1;
    }

    static inline size_t right(size_t i) {
        return 2 * i + 2;
    }

    static inline size_t parent(size_t i) {
        return ((ssize_t(i) - 1) / 2);
    }

    virtual void setSize(size_t size) = 0;
    virtual size_t getKey(size_t i) = 0;
    virtual void setKey(size_t i, size_t newKey) = 0;
    virtual void swap(size_t a, size_t b) = 0;

public:
    virtual size_t getSize() = 0;
    virtual T get(size_t i) = 0;

    void makeHeap() {
        for (ssize_t i = (getSize() / 2) - 1; i >= 0; i--) {
            heapify(i);
        }
    }

    void heapify(size_t i) {
        size_t sz = getSize();
        size_t l = left(i);
        size_t r = right(i);

        size_t max = i;
        if (l < sz && getKey(l) > getKey(max)) {
            max = l;
        }

        if (r < sz && getKey(r) > getKey(max)) {
            max = r;
        }

        if (max != i) {
            swap(i, max);
            heapify(max);
        }
    }

    virtual void decreaseKey(size_t i, size_t newKey) {
        DLOG(INFO) << "decreaseKey("<<i<<", "<<newKey<<")\n---";
        size_t oldKey = getKey(i);
        if (newKey > oldKey) {
            // TODO error
            //throw new IllegalArgumentException("decreaseKey cannot increase a key!");
        } else if (newKey < oldKey) {
            setKey(i, newKey);
            heapify(i);
        }
    }

    virtual T removeFromHeap(size_t i) {
        T removed = get(i);

        size_t last = getSize() - 1;
        if (i != last) {
            //find correct insert position
            size_t lastKey = getKey(last);
            size_t x = i;
            while (x > 0 && lastKey > getKey(parent(x))) {
                x = parent(x);
            }

            //swap removed element with last
            swap(i, last);

            //possibly correct position
            if(x != i) {
                swap(x, i);
            }

            //update size and heapify
            setSize(last);
            heapify(x);
        } else {
            setSize(last);
        }

        return removed;
    }
};

}

#endif
