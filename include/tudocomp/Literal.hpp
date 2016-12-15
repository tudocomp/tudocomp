#pragma once

#include <cassert>
#include <tudocomp/def.hpp>
#include <tudocomp/util/View.hpp>

namespace tdc {

struct Literal {
    uliteral_t c;
    len_t pos;
};

class NoLiterals {
public:
    inline NoLiterals() {}
    inline bool has_next() const { return false; }
    inline Literal next() { assert(false); return Literal{0, 0}; }
};

class ViewLiterals {
private:
    View m_view;
    len_t m_index;

public:
    inline ViewLiterals(View view) : m_view(view), m_index(0) {
    }

    inline bool has_next() const {
        return m_index < m_view.size();
    }

    inline Literal next() {
        assert(has_next());

        auto i = m_index++;
        return Literal { uliteral_t(m_view[i]), i };
    }
};

}

