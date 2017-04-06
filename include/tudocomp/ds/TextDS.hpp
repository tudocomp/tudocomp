#pragma once

#include <list>
#include <map>

#include <tudocomp/def.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/util/View.hpp>

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/TextDSProvider.hpp>

namespace tdc {

static_assert(
    std::is_same<View::value_type, uliteral_t>::value,
    "View::value_type and uliteral_t must be the same");

/// Manages text related data structures.
class TextDS : public Algorithm {
public:
    using dsid_t = TextDSProvider::dsid_t;
    using dsid_list_t = TextDSProvider::dsid_list_t;

    static constexpr dsid_t SA  = 0x7DC01;
    static constexpr dsid_t ISA = 0x7DC02;
    static constexpr dsid_t LCP = 0x7DC03;
    static constexpr dsid_t PHI = 0x7DC04;
    static constexpr dsid_t PLCP = 0x7DC05;

private:
    using this_t = TextDS;

    View m_text;
    CompressMode m_cm;

    std::map<dsid_t, TextDSProvider*> m_providers;

public: //private:
    void register_provider(TextDSProvider& p) {
        DLOG(INFO) << "register_provider";
        for(auto prod : p.products()) {
            DLOG(INFO) << "  produces " << prod;
            auto it = m_providers.find(prod);
            if(it != m_providers.end()) {
                throw std::logic_error(
                    std::string("There is already a provider for text ds ") +
                    std::to_string(prod));
            } else {
                m_providers.emplace(prod, &p);
            }
        }
    }

public:
    inline static Meta meta() {
        Meta m("textds", "textds");
        m.option("compress").dynamic("delayed");
        return m;
    }

    inline TextDS(Env&& env, const View& text)
        : Algorithm(std::move(env)), m_text(text) {

        if(!m_text.ends_with(uint8_t(0))){
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

private:
    TextDSProvider& get_provider(dsid_t id) {
        auto it = m_providers.find(id);
        if(it != m_providers.end()) {
            return *(it->second);
        } else {
            throw std::logic_error(
                std::string("No provider available for text ds ") +
                std::to_string(id));
        }
    }

public:
    void construct(const dsid_list_t& ds) {
    }

    const View& text = m_text;
};

} //ns
