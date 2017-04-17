#pragma once

#include <map>
#include <memory>
#include <tuple>

#include <tudocomp/def.hpp>
#include <tudocomp/util/type_list.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/util/View.hpp>

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/DSProvider.hpp>
#include <tudocomp/ds/DSDependencyGraph.hpp>

#include <tudocomp/CreateAlgorithm.hpp>

namespace tdc {

static_assert(
    std::is_same<View::value_type, uliteral_t>::value,
    "View::value_type and uliteral_t must be the same");

/// Manages data structures and construction algorithms.
template<typename... provider_ts>
class DSManager : public Algorithm {
private:
    using this_t = DSManager<provider_ts...>;

    using provider_type_map = tl::multimix<typename provider_ts::provides...>;

    using provider_tuple_t = std::tuple<std::shared_ptr<provider_ts>...>;
    provider_tuple_t m_providers;

    // TODO: need source indices for compile-time lookup in tuple!

    View m_input; // TODO: when the changes to the I/O system are done,
                  // this can be an Input?
    CompressMode m_cm;

public:
    template<dsid_t dsid>
    using provider_type = tl::get<dsid, provider_type_map>;

    inline static Meta meta() {
        Meta m("ds", "ds");
        m.option("compress").dynamic("delayed");
        //TODO: m.option("providers").templated_array("provider")
        return m;
    }

    inline DSManager(Env&& env, const View& input)
        : Algorithm(std::move(env)), m_input(input) {

        if(!m_input.ends_with(uint8_t(0))){
             throw std::logic_error(
                 "Input has no sentinel! Please make sure you declare "
                 "the compressor calling this with "
                 "`m.needs_sentinel_terminator()` in its `meta()` function."
            );
        }

        // TODO: instantiate providers

        auto& cm_str = this->env().option("compress").as_string();
        if(cm_str == "delayed") {
            m_cm = CompressMode::delayed;
        } else if(cm_str == "compressed") {
            m_cm = CompressMode::compressed;
        } else {
            m_cm = CompressMode::plain;
        }
    }

public:
    inline void construct(const dsid_list_t& requested_ds) {
        /*DLOG(INFO) << "create dependency graph";

        DSDependencyGraph<this_t> g(*this);
        for(auto id : requested_ds) {
            g.request(id);
        }

        g.construct_requested();*/
    }

    const View& input = m_input;
};

} //ns
