#pragma once

#include <tudocomp/Generator.hpp>

namespace tdc {

class FibonacciWordGenerator : public Generator {

public:
    inline static Meta meta() {
        Meta m("generator", "fib", "Generates the n-th Fibonacci word.");
        m.option("n").dynamic();
        return m;
    }

    using Generator::Generator;

    inline virtual std::string generate() override {
        size_t n = env().option("n").as_integer();

	    if(n == 1) return "b";
	    if(n == 2) return "a";

	    std::string vold = "b";
	    std::string old = "a";

	    vold.reserve(std::pow(1.62, n-1));
	    old.reserve(std::pow(1.62, n));

	    for(size_t i = 2; i < n; ++i) {
		    std::string tmp = old + vold;
		    vold = old;
		    old = tmp;
	    }

	    DCHECK_LE(old.length(), std::pow(1.62, n));
	    DCHECK_LE(vold.length(), std::pow(1.62, n-1));
	    return old;
    }
};

} //ns

