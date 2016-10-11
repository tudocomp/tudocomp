#ifndef _INCLUDED_LZW_DECODE_HPP_
#define _INCLUDED_LZW_DECODE_HPP_

#include <tudocomp/lzw/Factor.hpp>
#include <tudocomp/lz78/dictionary.hpp>
#include <tudocomp/util.hpp>

namespace tdc {

namespace lzw {

using lz78_dictionary::CodeType;

template<class F>
void decode_step(F next_code_callback,
                 std::ostream& out,
                 const CodeType dms,
                 const CodeType reserve_dms) {
    std::vector<std::pair<CodeType, uint8_t>> dictionary;

    // "named" lambda function, used to reset the dictionary to its initial contents
    const auto reset_dictionary = [&] {
        dictionary.clear();
        dictionary.reserve(reserve_dms);

        const long int minc = std::numeric_limits<uint8_t>::min();
        const long int maxc = std::numeric_limits<uint8_t>::max();

        for (long int c = minc; c <= maxc; ++c)
            dictionary.push_back({dms, static_cast<uint8_t> (c)});
    };

    const auto rebuild_string = [&](CodeType k) -> const std::vector<uint8_t> * {
        static std::vector<uint8_t> s; // String

        s.clear();

        // the length of a string cannot exceed the dictionary's number of entries
        s.reserve(reserve_dms);

        while (k != dms)
        {
            s.push_back(dictionary[k].second);
            k = dictionary[k].first;
        }

        std::reverse(s.begin(), s.end());
        return &s;
    };

    reset_dictionary();

    CodeType i {dms}; // Index
    CodeType k; // Key

    bool corrupted = false;

    while (true)
    {
        bool dictionary_reset = false;

        // dictionary's maximum size was reached
        if (dictionary.size() == dms)
        {
            reset_dictionary();
            dictionary_reset = true;
        }

        if (!next_code_callback(k, dictionary_reset, corrupted))
            break;

        //std::cout << byte_to_nice_ascii_char(k) << "\n";

        if (k > dictionary.size()) {
            std::stringstream s;
            s << "invalid compressed code " << k;
            throw std::runtime_error(s.str());
        }

        const std::vector<uint8_t> *s; // String

        if (k == dictionary.size())
        {
            dictionary.push_back({i, rebuild_string(i)->front()});
            s = rebuild_string(k);
        }
        else
        {
            s = rebuild_string(k);

            if (i != dms)
                dictionary.push_back({i, s->front()});
        }

        out.write((char*) &s->front(), s->size());
        i = k;
    }

    if (corrupted)
        throw std::runtime_error("corrupted compressed file");
}

}

}

#endif
