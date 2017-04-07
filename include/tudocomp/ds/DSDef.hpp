#pragma once

#include <vector>

namespace tdc {

using dsid_t = int;
using dsid_list_t = std::vector<dsid_t>;

namespace ds {
    static constexpr dsid_t SUFFIX_ARRAY  = 0x7DC01;
    static constexpr dsid_t INVERSE_SUFFIX_ARRAY = 0x7DC02;
    static constexpr dsid_t LCP_ARRAY = 0x7DC03;
    static constexpr dsid_t PHI_ARRAY = 0x7DC04;
    static constexpr dsid_t PLCP_ARRAY = 0x7DC05;
}

}

