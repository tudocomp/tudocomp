#pragma once

#include <unordered_set>
#include <tudocomp/AlgorithmStringParser.hpp>

namespace tdc {

class Env;

/// \cond INTERNAL
struct AlreadySeenPair {
    std::array<std::string, 2> pair;
};

inline bool operator==(const AlreadySeenPair& lhs, const AlreadySeenPair& rhs) {
    return lhs.pair[0] == rhs.pair[0] && lhs.pair[1] == rhs.pair[1];
}
/// \endcond

/// \brief A registry for algorithms to be made available in the driver
///        application.
///
/// For algorithms to be made available in the driver application, they need
/// to be registered in the application's main registry. This is done in
/// the \ref register_algorithms step. Any registered algorithm will also
/// be listed in the utility's help message.
template<typename algorithm_t>
class Registry {
    typedef std::function<std::unique_ptr<algorithm_t>(Env&&)> constructor_t;

    struct RegistryData {
        eval::AlgorithmTypes m_algorithms;
        std::map<pattern::Algorithm, constructor_t> m_registered;
    };

    std::shared_ptr<RegistryData> m_data;
    std::string m_root_type;

    /// \cond INTERNAL
    friend class AlgorithmTypeBuilder;
    friend class GlobalRegistry;
    /// \endcond

public:
    inline Registry(const std::string& root_type = "any"):
        m_data(std::make_shared<RegistryData>()), m_root_type(root_type) {}

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

    /// \cond INTERNAL
    // Create the list of all possible static-argument-type combinations
    inline std::vector<pattern::Algorithm> all_algorithms_with_static(View type) const;
    inline std::vector<pattern::Algorithm> all_algorithms_with_static_internal(std::vector<AlreadySeenPair>& already_seen, View type) const;
    inline std::vector<pattern::Algorithm> check_for_undefined_algorithms();
    inline std::unique_ptr<algorithm_t> select_algorithm(const AlgorithmValue& algo) const;
    inline AlgorithmValue parse_algorithm_id(string_ref text) const;
    inline std::unique_ptr<algorithm_t> select(const std::string& text) const;
    inline static Registry<algorithm_t> with_all_from(std::function<void(Registry<algorithm_t>&)> f, const std::string& root_type);
    inline std::string generate_doc_string(const std::string& title) const;
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

