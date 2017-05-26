#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace tdc {
namespace meta {

/// \brief Represents an algorithm parameter.
class Param {
private:
    std::string m_name;
    bool m_primitive;
    bool m_list;
    std::string m_type; // non-primitive or "$"

public:
    Param(
        const std::string& name,
        bool primitive = true,
        bool list = false,
        const std::string& type = "$")
        : m_name(name), m_primitive(false), m_list(list), m_type(type) {
    }

    Param(const Param& other) : m_name(other.m_name),
                                m_primitive(other.m_primitive),
                                m_list(other.m_list),
                                m_type(other.m_type) {
    }

    Param(Param&& other) : m_name(std::move(other.m_name)),
                           m_primitive(std::move(other.m_primitive)),
                           m_list(std::move(other.m_list)),
                           m_type(std::move(other.m_type)) {
    }

    const std::string str() const {
        return m_name + " : " + (m_list ? "[" + m_type + "]" : m_type);
    }
};

/// \brief Represents an algorithm declaration.
class Algorithm {
private:
    std::string m_name;
    std::string m_type;
    std::string m_desc;
    std::vector<Param> m_params;

public:
    Algorithm(
        const std::string& name,
        const std::string& type,
        const std::string& desc = "")
        : m_name(name), m_type(type), m_desc(desc) {
    }

    Algorithm(const Algorithm& other) : m_name(other.m_name),
                                        m_type(other.m_type),
                                        m_desc(other.m_desc),
                                        m_params(other.m_params) {
    }


    Algorithm(Algorithm&& other) : m_name(std::move(other.m_name)),
                                   m_type(std::move(other.m_type)),
                                   m_desc(std::move(other.m_desc)),
                                   m_params(std::move(other.m_params)) {
    }

    void add_param(Param&& p) {
        m_params.push_back(std::move(p));
    }

    const std::string& name() const { return m_name; }
    const std::string& type() const { return m_type; }
    const std::string& desc() const { return m_desc; }

    std::string str() const {
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

// TODO return type
void parse(const std::string& str, const AlgorithmMap& map) {
    // TODO
}

}} //ns

