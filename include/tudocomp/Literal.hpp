#ifndef _INCLUDED_LITERAL_HPP_
#define _INCLUDED_LITERAL_HPP_

#include <cassert>

namespace tdc {

struct Literal {
    uint8_t c;
    size_t pos;
};

class NoLiterals {
public:
    inline NoLiterals() {}

    inline Literal operator*() const {
        assert(false);
    }

    inline bool operator!= (const NoLiterals& other) const {
        return false; //always equal
    }

    inline NoLiterals& operator++() {
        assert(false);
    }

    inline const NoLiterals& begin() const { return *this; }
    inline const NoLiterals& end() const { return *this; }
};

}

#endif

