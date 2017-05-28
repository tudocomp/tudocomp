#pragma once

#include <tudocomp/meta/DeclParam.hpp>

#include <sstream>
#include <unordered_map>
#include <vector>

namespace tdc {
namespace meta {
namespace decl {

/// \brief Represents an algorithm declaration.
class Algorithm {
private:
    std::string m_name;
    std::string m_type;
    std::string m_desc;
    std::vector<Param> m_params;

public:
    /// \brief Main constructor.
    inline Algorithm(
        const std::string& name,
        const std::string& type,
        const std::string& desc = "") : m_name(name),
                                        m_type(type),
                                        m_desc(desc) {
    }

    /// \brief Copy constructor.
    inline Algorithm(const Algorithm& other) : m_name(other.m_name),
                                               m_type(other.m_type),
                                               m_desc(other.m_desc),
                                               m_params(other.m_params) {
    }

    /// \brief Move constructor.
    inline Algorithm(Algorithm&& other) : m_name(std::move(other.m_name)),
                                          m_type(std::move(other.m_type)),
                                          m_desc(std::move(other.m_desc)),
                                          m_params(std::move(other.m_params)) {
    }

    inline void add_param(Param&& p) {
        m_params.emplace_back(std::move(p));
    }

    inline const std::string& name() const { return m_name; }
    inline const std::string& type() const { return m_type; }
    inline const std::string& desc() const { return m_desc; }

    inline std::string str() const {
        std::stringstream ss;
        ss << m_name << "(";

        size_t i = 0;
        for(auto& param : m_params) {
            ss << param.str();
            if(++i < m_params.size()) ss << ", ";
        }

        ss << ") : " << m_type;
        return ss.str();
    }
};

// name -> algorithm map
using AlgorithmMap = std::unordered_map<std::string, Algorithm>;

}}} //ns
