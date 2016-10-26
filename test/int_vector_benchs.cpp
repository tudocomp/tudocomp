#include <string>
#include <vector>

#include <benchpress/benchpress.hpp>

#include <tudocomp/ds/GenericIntVector.hpp>
#include <tudocomp/util/IntegerBase.hpp>
#include <tudocomp/ds/uint_t.hpp>

using namespace tdc;

BENCHMARK("example", [](benchpress::context* ctx) {
    for (size_t i = 0; i < ctx->num_iterations(); ++i) {
        std::cout << "hello" << std::endl;
    }
})
