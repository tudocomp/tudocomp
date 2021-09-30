#pragma once

namespace tdc {
    template<typename... Ts> struct make_void { typedef void type;};
    /// C++ 14 implementation of void_t
    template<typename... Ts> using void_t = typename make_void<Ts...>::type;
}
