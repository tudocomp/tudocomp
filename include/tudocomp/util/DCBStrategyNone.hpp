#ifndef _INCLUDED_DCB_STRATEGY_NONE_HPP_
#define _INCLUDED_DCB_STRATEGY_NONE_HPP_

namespace tudocomp {

class DCBStrategyNone {

public:
    inline DCBStrategyNone() {
    }

    inline void wait(size_t pos, size_t src) {
        //
    }
    
    inline bool next_waiting_for(size_t pos, size_t& out_waiting) {
        return false;
    }
};

}

#endif
