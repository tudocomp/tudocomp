#ifndef _INCLUDED_INNER_NULL_COMPRESSOR_HPP_
#define _INCLUDED_INNER_NULL_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/CreateAlgorithm.hpp>
#include <vector>
#include <memory>

namespace tudocomp {

class InnerNullCompressor: public Compressor {
    std::unique_ptr<Compressor> m_compressor;
    bool m_ensure_null_terminator;
    bool m_check_inner_nulls;
    bool m_escape_inner_nulls;
    bool m_escape_byte;

public:
    inline static Meta meta() {
        Meta m("compressor", "inner_null");
        m.option("compressor").dynamic_compressor();
        m.option("check_inner_nulls").dynamic("true");
        m.option("escape_inner_nulls").dynamic("true");
        m.option("escape_byte").dynamic("255");
        return m;
    }

    inline InnerNullCompressor(Env&& env):
        Compressor(std::move(env)),
        m_compressor(create_algo_with_registry_dynamic(
            this->env().registry(),
            this->env().option("compressor").as_algorithm())),
        m_ensure_null_terminator(this->env().option("ensure_null_terminator").as_bool()),
        m_check_inner_nulls(this->env().option("check_inner_nulls").as_bool()),
        m_escape_inner_nulls(this->env().option("escape_inner_nulls").as_bool()),
        m_escape_byte(this->env().option("escape_byte").as_integer()) {}

    inline virtual void compress(Input& input, Output& output) override final {
        m_compressor->compress(input, output);
    }

    inline virtual void decompress(Input& input, Output& output) override final {
        m_compressor->decompress(input, output);
    }
};

}

#endif
