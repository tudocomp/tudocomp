#pragma once

#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>

#include "st.hpp"

//read file fully
inline std::string file_get_contents(const std::string& filename) {
    std::ifstream stream(filename);
    return std::string(
            (std::istreambuf_iterator<char>(stream)),
            std::istreambuf_iterator<char>());
}

//get suffix tree for text
inline tdc::SuffixTree suffix_tree(const std::string& text, tdc::SuffixTree::cst_t& cst) {
	construct_im(cst, text, 1);
    return tdc::SuffixTree(cst);
}
