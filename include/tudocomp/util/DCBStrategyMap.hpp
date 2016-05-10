#ifndef _INCLUDED_DCB_STRATEGY_MAP_HPP_
#define _INCLUDED_DCB_STRATEGY_MAP_HPP_

#include <map>
#include <list>

#include <glog/logging.h>

namespace tudocomp {

class DCBStrategyMap {

private:
    using list_type = std::list<size_t>;
    
    std::map<size_t, list_type> m_map;

public:
    inline DCBStrategyMap(size_t len) {
    }

    inline void wait(size_t pos, size_t src) {
        auto it = m_map.find(src);
        if(it != m_map.end()) {
            //append
            auto& q = it->second;
            q.push_back(pos);
        } else {
            //create new list
            list_type q;
            q.push_back(pos);
            m_map.emplace(src, q);
        }
    }
    
    inline bool next_waiting_for(size_t pos, size_t& out_waiting) {
        auto it = m_map.find(pos);
        if(it != m_map.end()) {
            auto& q = it->second;
            if(!q.empty()) {
                out_waiting = q.front();
                q.pop_front();
                return true;
            }
        }
        
        return false;
    }
};

}

#endif

