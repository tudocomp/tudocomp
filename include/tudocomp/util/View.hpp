#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <iomanip>
#include <cstring>
#include <glog/logging.h>
#include <tudocomp/def.hpp>

#include <tudocomp/util/GenericConstView.hpp>
#include <tudocomp/util/GenericView.hpp>
#include <tudocomp/util/GenericConstU8View.hpp>

namespace tdc {

using ByteView = ConstGenericView<uint8_t>;
using View = ByteView;
using string_ref = View;

}
