#ifndef _INCLUDED_LITERAL_HPP_
#define _INCLUDED_LITERAL_HPP_

#include <cassert>
#include <tudocomp/util/View.hpp>

namespace tdc {

struct Literal {
    uint8_t c;
    size_t pos;
};

class NoLiterals {
public:
    inline NoLiterals() {}
    inline bool has_next() const { return false; }
    inline Literal next() { assert(false); }
};

class ViewLiterals {
private:
    const View* m_view;
    size_t m_index;

public:
    inline ViewLiterals(const View& view) : m_view(&view), m_index(0) {
    }

    inline bool has_next() const {
        return m_index < m_view->size();
    }

    inline Literal next() {
        assert(has_next());

        auto i = m_index++;
        return Literal { uint8_t((*m_view)[i]), i };
    }
};

}

#endif

