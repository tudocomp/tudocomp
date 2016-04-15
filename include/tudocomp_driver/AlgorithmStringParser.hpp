#ifndef TUDOCOMP_DRIVER_ALGORITHM_PARSER
#define TUDOCOMP_DRIVER_ALGORITHM_PARSER

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

namespace tudocomp_driver {
using namespace tudocomp;

    /*
        AlgorithmSpec ::= IDENT '(' [AlgorithmArg,]* [AlgorithmArg] ')'
        AlgorithmArg  ::= IDENT '=' string | string
    */

    struct AlgorithmArg;
    struct AlgorithmSpec {
        std::string name;
        std::vector<AlgorithmArg> args;
    };
    struct AlgorithmArg {
        std::string keyword;
        std::string arg;
    };

    struct Err {
        std::string reason;
    };

    template<class T>
    struct Result {
        boost::string_ref trail;
        boost::variant<T, Err> data;

        template<class U>
        using Fun = std::function<Result<U> (boost::string_ref, T)>;

        template<class U>
        inline Result<U> and_then(Fun<U> f) {
            struct visitor: public boost::static_visitor<Result<U>> {
                // insert constructor here
                const Fun<U> m_f;
                const boost::string_ref m_trail;

                visitor(Fun<U> f, boost::string_ref trail): m_f(f), m_trail(trail) {
                }

                Result<U> operator()(T& ok) const {
                    return m_f(m_trail, ok);
                }
                Result<U> operator()(Err& err) const {
                    return Result<U> { m_trail, err };
                }
            };
            return boost::apply_visitor(visitor(f, trail), data);
        }

        inline T unwrap() {
            struct visitor: public boost::static_visitor<T> {
                const boost::string_ref m_trail;

                visitor(boost::string_ref trail): m_trail(trail) {
                }

                T operator()(T& ok) const {
                    return ok;
                }
                T operator()(Err& err) const {


                    throw std::runtime_error("Parse error: " + err.reason);
                }
            };
            return boost::apply_visitor(visitor(trail), data);
        }
    };

    template<class T>
    inline Result<T> ok(boost::string_ref trail, T t) {
        return Result<T> {
            trail,
            t,
        };
    }

    inline boost::string_ref skip_whitespace(boost::string_ref s) {
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] != ' ') {
                return s.substr(i);
            }
        }
        return s.substr(s.size());
    }

    inline Result<boost::string_ref> parse_ident(boost::string_ref s) {
        s = skip_whitespace(s);

        auto valid_first = [](uint8_t c) {
            return (c == '_')
                | (c >= 'a' && c <= 'z')
                | (c >= 'A' && c <= 'Z');
        };
        auto valid_middle = [=](uint8_t c) {
            return valid_first(c)
                | (c >= '0' && c <= '9');
        };

        size_t i = 0;
        if (i < s.size() && valid_first(s[i])) {
            for (i = 1; i < s.size() && valid_middle(s[i]); i++) {
            }
            return Result<boost::string_ref> {
                s.substr(i),
                s.substr(0, i),
            };
        } else {
            return Result<boost::string_ref> {
                s,
                Err { "Expected an identificator" },
            };
        }
    }

    inline Result<uint8_t> parse_char(boost::string_ref s, uint8_t chr) {
        s = skip_whitespace(s);

        if (s.size() > 0 && uint8_t(s[0]) == chr) {
            return Result<uint8_t> {
                s.substr(1),
                chr,
            };
        } else {
            return Result<uint8_t> {
                s,
                Err { std::string("Expected char '") + char(chr) + "'" },
            };
        }
    }

    inline Result<AlgorithmSpec> parse(boost::string_ref s) {
        return parse_ident(s).and_then<AlgorithmSpec>([=](boost::string_ref s,
                                                          boost::string_ref ident) {
            return parse_char(s, '(').and_then<AlgorithmSpec>([=](boost::string_ref s,
                                                                  uint8_t chr) {
                // Parse arguments here



                return parse_char(s, ')').and_then<AlgorithmSpec>([=](boost::string_ref s,
                                                                      uint8_t chr) {
                    s = skip_whitespace(s);

                    if (s == "") {
                        return ok(s, AlgorithmSpec {
                            std::string(ident),
                            std::vector<AlgorithmArg> {}
                        });
                    } else {
                        return Result<AlgorithmSpec> {
                            s, Err { "Expected end of input" }
                        };
                    }
                });
            });
        });
    }


}

#endif
