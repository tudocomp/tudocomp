#pragma once

#include <map>
#include <memory>
#include <tuple>
#include <type_traits>

#include <tudocomp/def.hpp>
#include <tudocomp/util/type_list.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/util/View.hpp>

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/DSDependencyGraph.hpp>

#include <tudocomp/CreateAlgorithm.hpp>

namespace tdc {

static_assert(
    std::is_same<View::value_type, uliteral_t>::value,
    "View::value_type and uliteral_t must be the same");

/// \cond INTERNAL
namespace internal {
    // helper to construct provider type map
    template<typename... Ts> struct _make_type_map;

    template<typename Head, typename... Tail>
    struct _make_type_map<Head, Tail...> {
        using type_map = tl::mix<
            tl::set_all<typename Head::provides, Head>,
            typename _make_type_map<Tail...>::type_map>;
    };

    template<> struct _make_type_map<> {
        using type_map = tl::mt;
    };

    template<typename... Ts>
    using make_type_map = typename _make_type_map<Ts...>::type_map;

    // helper to construct matching tuple for runtime lookup
    template<typename Tl> struct _make_lookup_tuple;

    template<typename... Ts>
    struct _make_lookup_tuple<tl::type_list<Ts...>> {
        using tuple = std::tuple<std::shared_ptr<Ts>...>;
    };

    template<typename Tl>
    using make_lookup_tuple = typename _make_lookup_tuple<Tl>::tuple;
}
/// \endcond

/// Manages data structures and construction algorithms.
template<typename... provider_ts>
class DSManager : public Algorithm {
private:
    using this_t = DSManager<provider_ts...>;

    // a type_list that contains the provider type for data structure i at
    // index i
    using provider_type_map_t =
            internal::make_type_map<provider_ts...>;

    // the lookup tuple that contains a pointer to the instance of provider
    // for data structure i at index i
    using provider_lookup_tuple_t =
            internal::make_lookup_tuple<provider_type_map_t>;

    provider_lookup_tuple_t m_lookup;

    // the provider tuple, containing pointers to the provider instances
    using provider_tuple_t = std::tuple<std::shared_ptr<provider_ts>...>;
    provider_tuple_t m_providers;

    // construct providers
    template<size_t... Is, typename T>
    inline void set_lookup(std::index_sequence<Is...>, std::shared_ptr<T>& ptr) {
        set_lookup(ptr, std::get<Is>(m_lookup)...);
    }

    template<typename T, typename Head, typename... Tail>
    inline void set_lookup(std::shared_ptr<T>& ptr, Head& head, Tail&... tail) {
        // register T
        head = ptr;

        // recurse
        set_lookup(ptr, tail...);
    }

    template<typename T>
    inline void set_lookup(std::shared_ptr<T>& ptr) {
        // lookup registration done for T
    }

    template<size_t... Is>
    inline void construct_providers(std::index_sequence<Is...>) {
        construct_provider(std::get<Is>(m_providers)...);
    }

    template<typename Head, typename... Tail>
    inline void construct_provider(Head& head, Tail&... tail) {
        // head_t is std::shared_ptr<provider_t>
        using provider_t = typename Head::element_type;

        // instantiate
        //TODO: head = std::make_shared<provider_t>(env().env_for_option("providers", i));
        head = std::make_shared<provider_t>(create_env(provider_t::meta()));

        // register in lookup table
        set_lookup(typename provider_t::provides(), head);

        // recurse
        construct_provider(tail...);
    }

    inline void construct_provider() {
        // construction done
    }

    View m_input; // TODO: when the changes to the I/O system are done,
                  // this can be an Input?

    CompressMode m_cm;

public:
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

        construct_providers(std::make_index_sequence<
            std::tuple_size<provider_tuple_t>::value>());

        auto& cm_str = this->env().option("compress").as_string();
        if(cm_str == "delayed") {
            m_cm = CompressMode::delayed;
        } else if(cm_str == "compressed") {
            m_cm = CompressMode::compressed;
        } else {
            m_cm = CompressMode::plain;
        }
    }

    template<dsid_t dsid>
    using provider_type = tl::get<dsid, provider_type_map_t>;

    template<dsid_t dsid>
    inline provider_type<dsid>& get_provider() {
        return *std::get<dsid>(m_lookup);
    }

public:
    template<dsid_t... ds>
    inline void construct() {
        // build dependency graph
        using depgraph_t = DSDependencyGraph<this_t, ds...>;

        // construct
        depgraph_t(*this, m_cm);
    }

    const View& input = m_input;
};

} //ns
