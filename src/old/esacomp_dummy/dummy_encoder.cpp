#include "dummy_encoder.h"

// Put every C++ code in this project into a common namespace
// in order to not pollute the global one
namespace esacomp {

void DummyCoder::code(Rules&& rules, Input& inp, Output& out) {
    // TODO: stream-to-stream direct transfer?
    auto i_guard = inp.as_view();
    auto o_guard = out.as_stream();
    auto input = *i_guard;
    auto& output = *o_guard;

    output.write((const char*) input.data(), input.size());
}

void DummyCoder::decode(Input& inp, Output& out) {
    // TODO: stream-to-stream direct transfer?
    auto i_guard = inp.as_stream();
    auto o_guard = out.as_stream();

    char c;
    while((*i_guard).get(c)) {
        (*o_guard).put(c);
    }
}

size_t DummyCoder::min_encoded_rule_length(size_t input_size) {
    return 1;
}

}
