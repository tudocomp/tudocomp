#ifndef INCLUDED_util_hpp
#define INCLUDED_util_hpp

#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>

#include "st.hpp"

//read file fully
std::string file_get_contents(const std::string& filename) {
    std::ifstream stream(filename);
    return std::string(
            (std::istreambuf_iterator<char>(stream)),
            std::istreambuf_iterator<char>());
}

//get suffix tree for text
ST suffix_tree(const std::string& text, cst_t& cst) {
	construct_im(cst, text, 1);
    return ST(cst);
}

#endif

