#pragma once

#include <cassert>
#include <tudocomp/def.hpp>
#include <tudocomp/util/View.hpp>

namespace tdc {

/// \brief Contains a literal and its location in the input.
///
/// This structure is used by encoders to get a glimpse at the literals it will
/// receive. The encoder is given a stream of literals and their occurences in
/// the compressed text. This information can be used to initialize data
/// structures that can be used for efficient encoding, e.g. a Huffman tree
/// or a k-mer dictionary.
struct Literal {
    /// The literal.
    uliteral_t c;

    /// The position of the literal in the input.
    len_t pos;
};

/// \brief An empty literal iterator that yields no literals whatsoever.
class NoLiterals {
public:
    /// \brief Constructor.
    inline NoLiterals() {}

    /// \brief Tests whether there are more literals in the stream.
    /// \return \e true if there are more literals in the stream, \e false
    ///         otherwise.
    inline bool has_next() const { return false; }

    /// \brief Yields the next literal from the stream.
    /// \return The next literal from the stream.
    inline Literal next() { assert(false); return Literal{0, 0}; }
};

/// \brief A literal iterator that yields every character from a \ref View.
class ViewLiterals {
private:
    View m_view;
    len_t m_index;

public:
    /// \brief Constructor.
    ///
    /// \param view The view to iterate over.
    inline ViewLiterals(View view) : m_view(view), m_index(0) {
    }

    /// \brief Tests whether there are more literals in the stream.
    /// \return \e true if there are more literals in the stream, \e false
    ///         otherwise.
    inline bool has_next() const {
        return m_index < m_view.size();
    }

    /// \brief Yields the next literal from the stream.
    /// \return The next literal from the stream.
    inline Literal next() {
        assert(has_next());

        auto i = m_index++;
        return Literal { uliteral_t(m_view[i]), i };
    }
};

}

