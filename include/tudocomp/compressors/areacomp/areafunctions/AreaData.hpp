#pragma once

#include <queue>
#include <vector>
#include <cstdio>

namespace tdc::grammar::areacomp {
    
struct AreaData {

    size_t m_low;
    size_t m_high;
    size_t m_area;
    size_t m_len;

    AreaData(size_t low, size_t high, size_t area, size_t len) : m_low{low}, m_high{high}, m_area{area}, m_len{len} {}

    class DataCompare {
    public:
        bool operator() (AreaData &a, AreaData &b) {
            return a.area() < b.area();
        }  
    };

    size_t low() const {
        return m_low;
    }

    size_t high() const {
        return m_high;
    }

    size_t area() const {
        return m_area;
    }

    size_t len() const {
        return m_len;
    }
    
    static auto queue() {
        return std::priority_queue<AreaData, std::vector<AreaData>, DataCompare>{DataCompare{}};
    }

};

}