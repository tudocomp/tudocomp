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

class ViewLiterals {
private:
    const View* m_view;

    class end_iterator {};

    class iterator {
    private:
        const View* m_view;
        size_t m_index;

    public:
        inline iterator(const View& view) : m_view(&view), m_index(0) {
        }

        inline Literal operator*() const {
            return Literal { uint8_t((*m_view)[m_index]), m_index };
        }

        inline bool operator!= (const end_iterator& other) const {
            return m_index >= m_view->size();
        }

        inline iterator& operator++() {
            ++m_index;
            return *this;
        }
    };

public:
    inline ViewLiterals(const View& view) : m_view(&view) {}

    inline iterator begin() const {
        return iterator(*m_view);
    }

    inline end_iterator end() const {
        return end_iterator();
    }
};

}

#endif

