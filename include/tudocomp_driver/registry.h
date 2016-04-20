#ifndef TUDOCOMP_REGISTRY_H
#define TUDOCOMP_REGISTRY_H

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <istream>
#include <map>
#include <sstream>
#include <streambuf>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <memory>

#include <boost/utility/string_ref.hpp>
#include <glog/logging.h>

#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/util.h>
#include <tudocomp_driver/AlgorithmStringParser.hpp>

namespace tudocomp_driver {

using namespace tudocomp;

struct AlgorithmInfo;

struct AlgorithmDb {
    std::string kind;
    std::vector<AlgorithmInfo> sub_algo_info;

    inline void print_to(std::ostream& out, int indent);

    inline std::vector<std::string> id_product();
};

struct AlgorithmInfo {
    /// Human readable name
    std::string name;
    /// Used as id string for command line and output filenames
    std::string shortname;
    /// Description text
    std::string description;
    std::vector<AlgorithmDb> sub_algo_info;

    inline void print_to(std::ostream& out, int indent);

};

template<class T>
std::vector<T> algo_cross_product(AlgorithmDb& subalgo,
                                  std::function<T(T, T&)> f
) {
    std::vector<T> r;
    for (auto& algo : subalgo.sub_algo_info) {

        auto& name = algo.shortname;
        std::vector<std::vector<T>> subalgos;
        for (auto& subalgo : algo.sub_algo_info) {
            subalgos.push_back(algo_cross_product(subalgo, f));
        }

        if (subalgos.size() == 0) {
            r.push_back(name);
        } else {
            for (auto x : cross<T>(std::move(subalgos), f)) {
                r.push_back(f(name, x));
            }
        }
    }
    return r;
}

inline std::vector<std::string> AlgorithmDb::id_product() {
    return algo_cross_product<std::string>(*this, [](std::string x, std::string& y) {
        if (x == "") {
            return y;
        } else {
            return x + "." + y;
        }
    });
}

const int ALGO_IDENT = 2;

inline std::string kind_str(int indent, std::string kind) {
    std::stringstream out;
    out << std::setw(indent) << "" << "[" << kind << "]";
    return out.str();
}
inline void AlgorithmDb::print_to(std::ostream& out, int indent) {
    out << kind_str(indent, kind) << "\n";
    for (auto& e : sub_algo_info) {
        e.print_to(out, indent + ALGO_IDENT);
    }
}

inline std::string shortname_str(int indent, std::string shortname) {
    std::stringstream out;
    out << std::setw(indent) << "" << shortname;
    return out.str();
}
inline void AlgorithmInfo::print_to(std::ostream& out, int indent) {
    out << shortname_str(indent, shortname) << " \t | "
        << name << ": "
        << description << "\n";
    for (auto& e : sub_algo_info) {
        e.print_to(out, indent + ALGO_IDENT);
    }
}

inline boost::string_ref pop_algorithm_id(boost::string_ref& algorithm_id) {
    auto idx = algorithm_id.find('.');
    int dot_size = 1;
    if (idx == boost::string_ref::npos) {
        idx = algorithm_id.size();
        dot_size = 0;
    }
    boost::string_ref r = algorithm_id.substr(0, idx);
    algorithm_id.remove_prefix(idx + dot_size);
    return r;
}

struct SubRegistry;

struct Registry {
    AlgorithmDb* m_algorithms;

    std::unordered_map<std::string, CompressorConstructor> m_compressors;

    // AlgorithmInfo

    void compressor(std::string id, CompressorConstructor f) {
        m_compressors[id] = f;
    }

    inline SubRegistry algo(std::string id, std::string title, std::string desc);

    std::vector<std::string> check_for_undefined_compressors() {
        std::vector<std::string> r;
        for (auto& s : m_algorithms->id_product()) {
            if (m_compressors.count(s) == 0) {
                r.push_back(s);
            }
        }
        return r;
    }

    std::vector<AlgorithmInfo> get_sub_algos() {
        return m_algorithms->sub_algo_info;
    }
};

struct SubRegistry {
    std::vector<AlgorithmInfo>* m_vector;
    size_t m_index;

    template<class G>
    inline void sub_algo(std::string name, G g) {
        auto& x = (*m_vector)[m_index].sub_algo_info;
        x.push_back({name});
        Registry y { &x[x.size() - 1] };
        g(y);
    }

};

inline SubRegistry Registry::algo(std::string id, std::string title, std::string desc) {
    m_algorithms->sub_algo_info.push_back({title, id, desc});

    return SubRegistry {
        &m_algorithms->sub_algo_info,
        m_algorithms->sub_algo_info.size() - 1
    };
}

inline std::unique_ptr<Compressor> select_algo_or_exit(Registry& reg,
                                                       Env& env,
                                                       std::string a_id) {

    if (reg.m_compressors.count(a_id) == 0) {
        throw std::runtime_error("Unknown algorithm: " + a_id);
    } else {
        return reg.m_compressors[a_id](env);
    }
}

void register_algos(Registry& registry);

// New unified algorithm spec registry

class RegistryV3;

struct ArgTypeBuilder {
    std::string m_id;
    size_t m_idx;
    RegistryV3& m_registry;

    ArgTypeBuilder& arg_id(std::string arg, std::string id, bool is_static = true);
    ArgTypeBuilder& doc(std::string doc);
};

struct AlgorithmSpecBuilder {
    AlgorithmSpec m_spec;
    std::string m_doc;
    std::unordered_map<std::string, std::pair<std::string, bool>> m_arg_ids;

    AlgorithmSpecBuilder(AlgorithmSpec spec): m_spec(spec) {}
};

class RegistryV3 {
public:
    std::unordered_map<std::string,
                       std::vector<AlgorithmSpecBuilder>> m_algorithms;

    std::map<AlgorithmSpec, CompressorConstructor> m_compressors;

    friend class ArgTypeBuilder;
public:
    inline ArgTypeBuilder register_spec(
        std::string id,
        std::string spec
    ) {
        Parser p { spec };
        AlgorithmSpecBuilder s { p.parse().unwrap() };

        m_algorithms[id].push_back(s);

        return ArgTypeBuilder { id, m_algorithms[id].size() - 1, *this };
    }

    AlgorithmSpec eval(std::string id, AlgorithmSpec s) {
        AlgorithmSpec r;

        // Find s in the options given with id
        for (auto& x : m_algorithms[id]) {
            if (x.m_spec.name == s.name) {
                // Found s, initialize r
                r.name = x.m_spec.name;

                // Evaluate the arguments, allowing positional
                // arguments followed by keyword arguments
                int i = 0;
                bool positional_ok = true;
                for (auto& pat_arg : s.args) {
                    AlgorithmArg* spec_arg;

                    if (pat_arg.keyword.size() == 0) {
                        // positional argument
                        CHECK(positional_ok);

                        spec_arg = &x.m_spec.args[i];
                    } else {
                        // keyword argument
                        positional_ok = false;

                        bool found = false;
                        for(uint j = 0; j < x.m_spec.args.size(); j++) {
                            if (x.m_spec.args[j].keyword == pat_arg.keyword) {
                                spec_arg = &x.m_spec.args[j];
                                found = true;
                                break;
                            }
                        }
                        CHECK(found);
                    }

                    // Got a reference to the corresponding spec Arg,
                    // extract keyword name from it
                    AlgorithmArg& arg = *spec_arg;

                    std::string arg_name;
                    if (arg.keyword.size() == 0) {
                        arg_name = arg.get<std::string>();
                    } else {
                        arg_name = arg.keyword;
                    }
                    CHECK(arg_name.size() > 0);
                    CHECK(x.m_arg_ids.count(arg_name) > 0);

                    // Use keyword/argument name to find out type
                    auto arg_id = x.m_arg_ids[arg_name].first;

                    // Combine argument name with argument
                    r.args.push_back(AlgorithmArg {
                        arg_name,
                        eval(arg_id, pat_arg.get<AlgorithmSpec>())
                    });

                    i++;
                }

                break;
            }
        }

        return r;
    }

    void compressor(std::string id, CompressorConstructor f) {
        Parser p { id };
        AlgorithmSpec s = p.parse().unwrap();

        // evaluate it to full keyword=value form
        s = eval("compressor", s);

        m_compressors[s] = f;
    }

    std::vector<AlgorithmSpec> all_algorithms_with_static(std::string id) {
        std::vector<AlgorithmSpec> r;

        using AlgorithmArgs = std::vector<AlgorithmArg>;

        for (auto& c : m_algorithms[id]) {
            std::vector<std::vector<AlgorithmArgs>> args_variations;

            for (auto& arg : c.m_spec.args) {
                std::string arg_name;
                if (arg.keyword.size() == 0) {
                    arg_name = arg.get<std::string>();
                } else {
                    arg_name = arg.keyword;
                }
                CHECK(arg_name.size() > 0);
                CHECK(c.m_arg_ids.count(arg_name) > 0);

                auto arg_id = c.m_arg_ids[arg_name].first;
                bool is_static = c.m_arg_ids[arg_name].second;
                if (is_static) {
                    std::vector<AlgorithmArgs> arg_variations;
                    for(auto arg : all_algorithms_with_static(arg_id)) {
                        arg_variations.push_back(AlgorithmArgs {
                            AlgorithmArg {
                                arg_name,
                                arg
                            }
                        });
                    }
                    args_variations.push_back(arg_variations);
                }
            }

            AlgorithmSpec x;
            x.name = c.m_spec.name;
            std::vector<AlgorithmArgs> r_;
            if (args_variations.size() == 0) {
                r.push_back(x);
            } else {
                r_ = cross<AlgorithmArgs>(
                    std::move(args_variations), [](AlgorithmArgs s,
                                                   AlgorithmArgs& t) {
                    for (auto& e : t) {
                        s.push_back(e);
                    }
                    return s;
                });
            }

            for (auto& elem : r_) {
                r.push_back(AlgorithmSpec {
                    c.m_spec.name,
                    elem
                });
            }
        }

        return r;
    }

    std::vector<AlgorithmSpec> check_for_undefined_compressors() {
        std::cout << "Check!" << "\n";

        std::vector<AlgorithmSpec> r;
        for (auto& s : all_algorithms_with_static("compressor")) {
            if (m_compressors.count(s) == 0) {
                r.push_back(s);
                std::cout << "MISSING " << s << "\n";
            } else {
                std::cout << "COVERED " << s << "\n";
            }
        }
        return r;
    }
};

inline ArgTypeBuilder& ArgTypeBuilder::arg_id(std::string arg, std::string id, bool is_static)  {
    m_registry.m_algorithms[m_id][m_idx].m_arg_ids[arg]
        = std::pair<std::string, bool>(id, is_static);
    return *this;
}

inline ArgTypeBuilder& ArgTypeBuilder::doc(std::string doc) {
    m_registry.m_algorithms[m_id][m_idx].m_doc = doc;
    return *this;
}

void register2(RegistryV3& registry);

}

#endif
