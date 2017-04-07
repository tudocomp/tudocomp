#pragma once

#include <list>
#include <map>

#include <tudocomp/def.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/util/View.hpp>

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/DSProvider.hpp>
#include <tudocomp/ds/DSDependencyGraph.hpp>

namespace tdc {

static_assert(
    std::is_same<View::value_type, uliteral_t>::value,
    "View::value_type and uliteral_t must be the same");

/// Manages data structures and construction algorithms.
class DSManager : public Algorithm {
    using this_t = DSManager;

    View m_input; // TODO: when the changes to the I/O are done, this can be an Input?
    CompressMode m_cm;

    std::map<dsid_t, DSProvider*> m_providers;

public:
    inline static Meta meta() {
        Meta m("ds", "ds");
        m.option("compress").dynamic("delayed");
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

        auto& cm_str = this->env().option("compress").as_string();
        if(cm_str == "delayed") {
            m_cm = CompressMode::delayed;
        } else if(cm_str == "compressed") {
            m_cm = CompressMode::compressed;
        } else {
            m_cm = CompressMode::plain;
        }
    }

public: //private:
    void register_provider(DSProvider& p) {
        DLOG(INFO) << "register_provider";
        for(auto prod : p.products()) {
            DLOG(INFO) << "    produces " << ds::name_for(prod);
            auto it = m_providers.find(prod);
            if(it != m_providers.end()) {
                throw std::logic_error(
                    std::string("There is already a provider for text ds ") +
                    ds::name_for(prod));
            } else {
                m_providers.emplace(prod, &p);
            }
        }
    }

public:
    inline DSProvider& get_provider(dsid_t id) {
        auto it = m_providers.find(id);
        if(it != m_providers.end()) {
            return *(it->second);
        } else {
            throw std::logic_error(
                std::string("No provider available for text ds ") +
                ds::name_for(id));
        }
    }

    inline void construct(const dsid_list_t& requested_ds) {
        DLOG(INFO) << "create dependency graph";
        
        DSDependencyGraph<this_t> g(*this);
        for(auto id : requested_ds) {
            g.insert_requested(id);
        }
    }

    const View& input = m_input;
};

} //ns
