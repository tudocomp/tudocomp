#pragma once

#include <unordered_set>
#include <array>

#include <tudocomp/AlgorithmStringParser.hpp>

namespace tdc {

class Env;
class EnvRoot;
class WeakRegistry;

/// \cond INTERNAL
struct AlreadySeenPair {
    std::array<std::string, 2> pair;
};

inline bool operator==(const AlreadySeenPair& lhs, const AlreadySeenPair& rhs) {
    return lhs.pair[0] == rhs.pair[0] && lhs.pair[1] == rhs.pair[1];
}

template<typename algorithm_t>
class RegistryOf;

class RegistryOfVirtual {
public:
    virtual ~RegistryOfVirtual() = default;
    RegistryOfVirtual() = default;
    RegistryOfVirtual(RegistryOfVirtual const&) = default;
    RegistryOfVirtual(RegistryOfVirtual&&) = default;
    RegistryOfVirtual& operator=(RegistryOfVirtual const&) = default;
    RegistryOfVirtual& operator=(RegistryOfVirtual&&) = default;

    virtual string_ref root_type() const = 0;

    template<typename algorithm_t>
    inline RegistryOf<algorithm_t>& downcast() {
        DCHECK_EQ(algorithm_t::meta_type(), root_type());
        return *static_cast<RegistryOf<algorithm_t>*>(this);
    }
    template<typename algorithm_t>
    inline RegistryOf<algorithm_t> const& downcast() const {
        DCHECK_EQ(algorithm_t::meta_type(), root_type());
        return *static_cast<RegistryOf<algorithm_t> const*>(this);
    }
};
/// \endcond

class Registry;

class WeakRegistry {
    using map_t = std::unordered_map<std::string, std::unique_ptr<RegistryOfVirtual>>;
    std::weak_ptr<map_t> m_registries;

    friend class Registry;
public:
    inline WeakRegistry();
    inline ~WeakRegistry();
    inline WeakRegistry(Registry const& r);
    inline operator Registry() const;
};

/// \brief A registry for algorithms to be made available in the driver
///        application.
///
/// For algorithms to be made available in the driver application, they need
/// to be registered in the application's main registry. This is done in
/// the \ref register_algorithm step. Any registered algorithm will also
/// be listed in the utility's help message.
template<typename algorithm_t>
class RegistryOf: public RegistryOfVirtual {
    typedef std::function<std::unique_ptr<algorithm_t>(Env&&)> constructor_t;

    struct RegistryOfData {
        eval::AlgorithmTypes m_algorithms;
        std::map<pattern::Algorithm, constructor_t> m_registered;
        WeakRegistry m_registry;
    };

    std::shared_ptr<RegistryOfData> m_data;
public:
    inline RegistryOf():
        m_data(std::make_shared<RegistryOfData>()) {}

    /// \brief Registers an \ref tdc::Algorithm.
    ///
    /// Note that the algorithm type \c T needs to implement a static function
    /// called \c meta() that returns a \ref tdc::Meta information object.
    /// This meta information is used to automatically generate the
    /// documentation for the driver application's help message.
    ///
    /// \tparam T The algorithm to register.
    template<typename T>
    void register_algorithm();

    inline eval::AlgorithmTypes& algorithm_map();
    inline const eval::AlgorithmTypes& algorithm_map() const;

    inline std::unique_ptr<algorithm_t> create_algorithm(const AlgorithmValue& text) const;
    inline std::unique_ptr<algorithm_t> create_algorithm(const std::string& text) const;

    /// \cond INTERNAL
    friend class AlgorithmTypeBuilder;
    friend class Registry;
    friend class WeakRegistry;
    inline RegistryOf(WeakRegistry wr): m_data(std::make_shared<RegistryOfData>()) {
        m_data->m_registry = wr;
    }
    // Create the list of all possible static-argument-type combinations
    inline std::vector<pattern::Algorithm> all_algorithms_with_static(View type) const;
    inline std::vector<pattern::Algorithm> all_algorithms_with_static_internal(std::vector<AlreadySeenPair>& already_seen, View type) const;
    inline std::vector<pattern::Algorithm> check_for_undefined_algorithms();
    inline std::unique_ptr<algorithm_t> select_algorithm(EnvRoot, AlgorithmValue const&) const;
    inline AlgorithmValue parse_algorithm_id(string_ref text) const;
    inline static RegistryOf<algorithm_t> with_all_from(std::function<void(RegistryOf<algorithm_t>&)> f);
    inline std::string generate_doc_string(const std::string& title) const;
    inline string_ref root_type() const override {
        return algorithm_t::meta_type();
    }
    /// \endcond
};

}
namespace std {
    template<>
    struct hash<typename tdc::AlreadySeenPair>
    {
        size_t operator()(const tdc::AlreadySeenPair& x) const {
            return std::hash<tdc::ConstGenericView<std::string>>()(
                tdc::ConstGenericView<std::string> {
                    x.pair.data(),
                    x.pair.size()
                });
        }
    };
}
