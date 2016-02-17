#ifndef COUNTER_H
#define COUNTER_H

#include "glog/logging.h"

#include <unordered_map>
#include <vector>
#include <utility>
#include <algorithm>
#include <cstdint>

namespace tudocomp {

/// A data structure for counting occurences of values of a given type T.
template<class T>
class Counter {
    std::unordered_map<T, size_t> map;
public:
    /// Increase the counter for the passed value by one, setting it to 1
    /// if it was not yet seen.
    void increase(const T& item) {
        // Indexing is defined as inserting and returning 0
        // in case of missing entry in the map.
        size_t prev_value = map[item];
        map[item] = prev_value + 1;
    }

    /// Set the count of an item to a specific value.
    void setCount(const T& item, size_t count) {
        map[item] = count;
    }

    /// Get the count associated with a value.
    size_t getCount(const T& item) {
        return map[item];
    }

    /// Return how many differnt values have been seen so far.
    size_t getNumItems() {
        return map.size();
    }

    /// Return a list of value-count pairs, sorted with the most-seen first.
    std::vector<std::pair<T, size_t>> getSorted() {
        std::vector<std::pair<T, size_t>> v(map.begin(), map.end());
        std::sort(v.begin(), v.end(), [] (
            std::pair<T, size_t> a, std::pair<T, size_t> b
        ) {
            // TODO: Only compare first in debug mode to get deterministic output
            if (a.second != b.second) {
                return a.second > b.second;
            } else {
                return a.first < b.first;
            }
        });

        DLOG(INFO) << "getSorted() [";
        for(auto e : v) {
            DLOG(INFO) << "  " << e.first << " " << e.second;
        }
        DLOG(INFO) << "]";

        return v;
    }

    /// Return a map of the `num`-most common values, keyed
    /// by their common-ness, with `map[0]` being hte most common one.
    ///
    /// This uses the same order as getSorted().
    std::unordered_map<T, size_t> createRanking(size_t num = SIZE_MAX) {
        std::unordered_map<T, size_t> r(num + 1);
        size_t i = 0;
        for (auto pair : getSorted()) {
            r[pair.first] = i++;
            if (i >= num) {
                break;
            }
        }
        return r;
    }
};

}

#endif
