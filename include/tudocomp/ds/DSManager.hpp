#pragma once

#include <memory>
#include <set>
#include <tuple>
#include <type_traits>

#include <tudocomp/def.hpp>
#include <tudocomp/util/type_list.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/util/View.hpp>

#include <tudocomp/Error.hpp>
#include <tudocomp/Tags.hpp>

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/DSDependencyGraph.hpp>

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
        construct_provider(0, std::get<Is>(m_providers)...);
    }

    template<typename Head, typename... Tail>
    inline void construct_provider(size_t i, Head& head, Tail&... tail) {
        // head_t is std::shared_ptr<provider_t>
        using provider_t = typename Head::element_type;

        // instantiate
        head = std::make_shared<provider_t>(
            Config(this->config().sub_configs("providers")[i]));

        // register in lookup table
        set_lookup(typename provider_t::provides(), head);

        // recurse
        construct_provider(i+1, tail...);
    }

    inline void construct_provider(size_t i) {
        // construction done
    }

    View m_input;      // TODO: use Input instead of View?
    CompressMode m_cm; // the compression mode

    std::set<dsid_t> m_protect; // used temporarily during construction

public:
    inline static Meta meta() {
        Meta m(ds::type(), "ds");
        m.param("providers").strategy_list<provider_ts...>(ds::provider_type());
        m.param("compress").primitive("delayed");
        m.inherit_tags_from_all(tl::type_list<provider_ts...>());
        return m;
    }

    inline DSManager(Config&& cfg, const View& input)
        : Algorithm(std::move(cfg)), m_input(input) {

        if(meta().has_tag(tags::require_sentinel)){
            MissingSentinelError::check(m_input);
        }

        construct_providers(std::make_index_sequence<
            std::tuple_size<provider_tuple_t>::value>());

        auto cm_str = this->config().param("compress").as_string();
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

    template<dsid_t dsid>
    inline const provider_type<dsid>& get_provider() const {
        return *std::get<dsid>(m_lookup);
    }

public:
    /// \cond INTERNAL

    using ds_types = tl::multimix<typename provider_ts::ds_types...>;

    inline void protect(const dsid_t ds) {
        //DLOG(INFO) << "protect: " << ds::name_for(ds);
        m_protect.emplace(ds);
    }

    inline void unprotect(const dsid_t ds) {
        //DLOG(INFO) << "unprotect: " << ds::name_for(ds);
        auto it = m_protect.find(ds);
        if(it != m_protect.end()) m_protect.erase(it);
    }

    inline bool is_protected(const dsid_t ds) {
        return (m_protect.find(ds) != m_protect.end());
    }
    /// \endcond

    /// \brief Constructs the specified data structures.
    ///
    /// \tparam ds the data structures to construct.
    template<dsid_t... ds>
    inline void construct() {
        // build dependency graph
        using depgraph_t = DSDependencyGraph<this_t, ds...>;

        // init protection (all requested data structures)
        m_protect = is::to_set(std::index_sequence<ds...>());
        //for(dsid_t x : m_protect) {
        //    DLOG(INFO) << "initially protect: " << ds::name_for(x);
        //}

        // construct
        depgraph_t(*this, m_cm);

        // revoke protection
        m_protect.clear();
    }

    /// \brief Gets the specified data structure for reading.
    ///
    /// The data structure must have been constructed beforehand.
    ///
    /// \tparam ds the requested data structure.
    /// \return a read-only reference to the data structure, the type of which
    ///         is determined by the respective provider
    template<dsid_t ds>
    inline const tl::get<ds, ds_types>& get() const {
        return get_provider<ds>().template get<ds>();
    }

    /// \brief Relinquishes the specified data structure for modification.
    ///
    /// The data structure must have been constructed beforehand and cannot
    /// be acquired again after this operation.
    ///
    /// \tparam ds the requested data structure.
    /// \return the data structure, the type of which is determined by the
    ///         respective provider
    template<dsid_t ds>
    inline tl::get<ds, ds_types> relinquish() {

        return get_provider<ds>().template relinquish<ds>();
    }

    /// \brief Relinquishes the specified data structure for modification or
    ///        creates a copy.
    ///
    /// This function is intended for construction time optimizations.
    /// Dependening on whether or not the requested data structure has been
    /// requested to be constructed (in \ref construct), the structure will
    /// either be relinquished or copied.
    ///
    /// \tparam ds the requested data structure.
    /// \return the data structure or a copy thereof, the type of which is
    ///         determined by the respective provider
    template<dsid_t ds>
    inline auto inplace() -> decltype(relinquish<ds>()) {
        if(is_protected(ds)) {
            // create a copy
            decltype(relinquish<ds>()) copy = get<ds>();
            return copy;
        } else {
            // relinquish
            return get_provider<ds>().template relinquish<ds>();
        }
    }

    const View& input = m_input;
};

} //ns
