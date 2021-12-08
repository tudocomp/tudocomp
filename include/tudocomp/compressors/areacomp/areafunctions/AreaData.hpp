#pragma once

#include <queue>
#include <vector>
#include <cstdio>

namespace tdc::grammar::areacomp {
    
struct AreaData {

    const size_t low;
    const size_t high;
    const size_t area;
    const size_t len;

    AreaData(size_t low, size_t high, size_t area, size_t len) : low{low}, high{high}, area{area}, len{len} {}

    class DataCompare {
        static bool comp(AreaData &a, AreaData &b) {
            return a.area < b.area;
        }

        static bool equiv(AreaData &a, AreaData &b) {
            return a.area == b.area;
        }
    };
    
    static std::priority_queue<AreaData, std::vector<AreaData>, DataCompare> queue() {
        return {};
    }

};
    
}