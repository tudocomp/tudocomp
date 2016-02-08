#include "glog/logging.h"

#include "tudocomp.h"
#include "dummy_compressor.h"

// Put every C++ code in this project into a common namespace
// in order to not pollute the global one
namespace esacomp {

Rules DummyCompressor::compress(Input& input, size_t threshold) {
    Rules rules = {};

    // You can use various macros from the glog library to assert
    // invariants of your code
    CHECK(rules.size() == 0);

    // This is a debug log statement. It will output to the console
    // if enabled with a environment variable, and will not be compiled
    // if compiled in release mode.
    DLOG(INFO) << "Compressed with Dummy compressor, 0 rules generated";

    return rules;
}

}
