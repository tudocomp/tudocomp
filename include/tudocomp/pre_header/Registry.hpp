#ifndef _INCLUDED_FORWARD_REGISTRY_HPP_
#define _INCLUDED_FORWARD_REGISTRY_HPP_

#include <tudocomp/AlgorithmStringParser.hpp>

namespace tdc {

class Env;
class Compressor;
using CompressorConstructor = std::function<std::unique_ptr<Compressor>(Env&&)>;


/// \brief A registry for algorithms to be made available in the driver
///        application.
///
/// For algorithms to be made available in the driver application, they need
/// to be registered in the application's main registry. This is done in
/// the \ref register_algorithms step. Any registered algorithm will also
/// be listed in the utility's help message.
class Registry {
    struct RegistryData {
        eval::AlgorithmTypes m_algorithms;
        std::map<pattern::Algorithm, CompressorConstructor> m_compressors;
    };

    std::shared_ptr<RegistryData> m_data;

    /// \cond INTERNAL
    friend class AlgorithmTypeBuilder;
    friend class GlobalRegistry;
    /// \endcond

public:
    inline Registry():
        m_data(std::make_shared<RegistryData>()) {}

    /// \brief Registers a \ref tdc::Compressor.
    ///
    /// Note that the compressor type \c T needs to implement a static function
    /// called \c meta() that returns a \ref tdc::Meta information object.
    /// This meta information is used to automatically generate the
    /// documentation for the driver application's help message.
    ///
    /// \tparam T The compressor to register.
    template<class T>
    void register_compressor();

    inline eval::AlgorithmTypes& algorithm_map();
    inline const eval::AlgorithmTypes& algorithm_map() const;

    /// \cond INTERNAL
    // Create the list of all possible static-argument-type combinations
    inline std::vector<pattern::Algorithm> all_algorithms_with_static(View type) const;
    inline std::vector<pattern::Algorithm> check_for_undefined_compressors();
    inline std::unique_ptr<Compressor> select_algorithm_or_exit(const AlgorithmValue& algo) const;
    inline AlgorithmValue parse_algorithm_id(string_ref text) const;
    inline static Registry with_all_from(std::function<void(Registry&)> f);
    inline std::string generate_doc_string() const;
    /// \endcond
};

inline std::unique_ptr<Registry> make_ptr_copy_of_registry(const Registry& registry);

}

#endif
