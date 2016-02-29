#ifndef _INCLUDED_LZW_DECODE_HPP_
#define _INCLUDED_LZW_DECODE_HPP_

#include <tudocomp/lzw/factor.h>

namespace tudocomp {

namespace lzw {

using ::lzw::LzwEntry;

template<class F>
void decode_step(F ReadIndex, std::ostream& out) {
    std::vector<std::tuple<LzwEntry, uint8_t>> dict;
    for (size_t i = 0; i <= 0xff; i++) {
        dict.push_back(std::tuple<LzwEntry, uint8_t> { 0, i });
    }

    auto AddDict = [&](LzwEntry C, uint8_t x) -> LzwEntry {
        dict.push_back(std::tuple<LzwEntry, uint8_t> { C, x });
        return dict.size() - 1;
    };

    auto GetString = [&](LzwEntry C) -> std::string {
        std::stringstream s;
        while (true) {
            s << std::get<1>(dict[C]);
            C = std::get<0>(dict[C]);
            if (C == 0) {
                break;
            }
        }
        std::string r = s.str();
        return std::string(r.rbegin(), r.rend());
    };

    auto IsIndexInDict = [&](LzwEntry C) -> bool {
        return C < dict.size();
    };

    LzwEntry C = ReadIndex();
    if (C == LzwEntry(-1)) {
        return;
    }
    std::string W = GetString(C);
    out << W;
    //std::cout << W << "\n";
    /*auto f = [&](uint C,
                 uint C_,
                 std::string W,
                 bool indict,
                 uint adddict) {
        std::cout
            << " |\t" << byte_to_nice_ascii_char(C)
            << " |\t" << byte_to_nice_ascii_char(C_)
            << " |\t" << W
            << " |\t" << indict
            << " |\t" << byte_to_nice_ascii_char(adddict)
                << "(" << byte_to_nice_ascii_char(C) << ","
                << W[0] << ")"
            << " |\t" << W << "\n";
    };

    f(C, 0, W, false, 0);*/

    while (C != LzwEntry(-1)) {
        LzwEntry C_ = ReadIndex();

        if (C_ == LzwEntry(-1)) {
            break;
        }

        //LzwEntry new_dict;
        bool isindict;

        if ((isindict = IsIndexInDict(C_))) {
            W = GetString(C_);
            //new_dict = AddDict(C, W[0]);
            AddDict(C, W[0]);
        } else {
            //new_dict = C_ = AddDict(C, W[0]);
            C_ = AddDict(C, W[0]);
            W = GetString(C_);
        }
        out << W;
        //f(C, C_, W, isindict, new_dict);

        C = C_;
    }
}

}

}

#endif
