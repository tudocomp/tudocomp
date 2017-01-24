#pragma once

#include <tudocomp/Generator.hpp>

namespace tdc {

/// \brief Generates strings according to A Series of Run-Rich Strings
/// (Wataru Matsubara et al.)
class RunRichGenerator : public Generator {

public:
    inline static Meta meta() {
        Meta m("generator", "run_rich", "Generates run-rich strings.");
        m.option("n").dynamic();
        return m;
    }

    using Generator::Generator;

    inline virtual std::string generate() override {
        size_t n = env().option("n").as_integer();

        std::string t0 = "0110101101001011010",
            t1 = "0110101101001",
            t2 = "01101011010010110101101",
            t3 = t2+t1;

        if(n==0) return t0;
        if(n==1) return t1;
        if(n==2) return t2;

        for(size_t i = 4; i < n; ++i) {
            std::string tmp = (i % 3 == 0) ? (t3+t2) : (t3+t0);
            t0=t1;
            t1=t2;
            t2=t3;
            t3=tmp;
        }

        return t3;
    }
};

} //ns

