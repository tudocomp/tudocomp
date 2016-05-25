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
        bool is_spec;


        inline AlgorithmSpec(std::string&& name_,
                             std::vector<AlgorithmArg>&& args_):
            name(std::move(name_)),
            args(std::move(args_)),
            is_spec(true) {}

        inline AlgorithmSpec(std::string&& value):
            name(std::move(value)),
            is_spec(false) {}

        //inline AlgorithmSpec(): AlgorithmSpec("") {}

        inline std::string to_string() const;

        inline bool operator==(const AlgorithmSpec &rhs) const {
            const auto& lhs = *this;

            return (lhs.name == rhs.name)
                && (lhs.args == rhs.args)
                && (lhs.is_spec == rhs.is_spec);
        }

        inline bool operator!=(const AlgorithmSpec &rhs) const {
            const auto& lhs = *this;

            return !(lhs == rhs);
        }

        inline bool operator<(const AlgorithmSpec &rhs) const {
            const auto& lhs = *this;

            if (lhs.name != rhs.name) return lhs.name < rhs.name;
            if (lhs.args != rhs.args) return lhs.args < rhs.args;
            if (lhs.is_spec != rhs.is_spec) return lhs.is_spec < rhs.is_spec;
            return false;
        }

    };

    struct AlgorithmArg {
        std::string keyword;
        AlgorithmSpec arg;

        AlgorithmArg(std::string&& keyword_, AlgorithmSpec&& arg_):
            keyword(keyword_), arg(arg_) {}

        inline std::string to_string() const;

        inline const std::string& get_as_string() const {
            if (arg.is_spec) {
                throw std::runtime_error("AlgorithmArg did not contain a string value");
            } else {
                return arg.name;
            }
        }

        inline const AlgorithmSpec& get_as_spec() const {
            if (!arg.is_spec) {
                throw std::runtime_error("AlgorithmArg did not contain a AlgorithmSpec value");
            } else {
                return arg;
            }
        }

        inline bool operator==(const AlgorithmArg &other) const {
            return (keyword == other.keyword)
                && (arg == other.arg);
        }

        inline bool operator<(const AlgorithmArg &other) const {
            if (keyword != other.keyword) return keyword < other.keyword;
            if (arg != other.arg) return arg < other.arg;
            return false;
        }
    };

    inline std::ostream& operator<<(std::ostream& os,
                                    const AlgorithmSpec& s) {
        os << s.to_string();
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os,
                                    const AlgorithmArg& s) {
        os << s.to_string();
        return os;
    }

    inline std::string AlgorithmSpec::to_string() const {
        auto& s = *this;
        std::stringstream os;
        os << s.name;

        if (is_spec) {
            os << "(";
            bool first = true;
            for(auto& arg : s.args) {
                if (!first) {
                    os << ", ";
                }
                os << arg;
                first = false;
            }
            os << ")";
        }
        return os.str();
    }

    inline std::string AlgorithmArg::to_string() const {
        auto& s = *this;
        std::stringstream os;
        if (s.keyword.size() > 0) {
            os << s.keyword << " = ";
        }
        os << s.arg;
        return os.str();
    }

    class Err {
        std::string m_reason;
        std::shared_ptr<Err> m_prev;
    public:
        inline Err(std::string reason): m_reason(reason) {
        }
        inline Err(const Err& prev, std::string reason): Err(reason) {
            m_prev = std::make_shared<Err>(prev);
        }
        inline std::string reason() {
            return m_reason;
        }
    };

    class Parser;

    template<class T>
    class Result {
        Parser* m_trail;

        struct Variant {
            virtual ~Variant() {}
            virtual bool is_ok() = 0;
            virtual T unwrap_or_else(std::function<T(Err&& err)>) = 0;
            virtual void visit(std::function<void(T& ok)>,
                               std::function<void(Err& err)>) = 0;
        };
        struct Ok: Variant {
            T m_data;
            inline Ok(T&& data): m_data(std::move(data)) {}

            inline virtual bool is_ok() override {
                return true;
            }
            inline virtual T unwrap_or_else(std::function<T(Err&& err)> f) override {
                return std::move(m_data);
            }
            inline virtual void visit(std::function<void(T& ok)> f_ok,
                              std::function<void(Err& err)> f_err) override {
                f_ok(m_data);
            }
        };
        struct Error: Variant {
            Err m_data;
            inline Error(Err&& data): m_data(std::move(data)) {}

            inline virtual bool is_ok() override {
                return false;
            }
            inline virtual T unwrap_or_else(std::function<T(Err&& err)> f) {
                return f(std::move(m_data));
            }
            inline virtual void visit(std::function<void(T& ok)> f_ok,
                              std::function<void(Err& err)> f_err) override {
                f_err(m_data);
            }
        };

        std::unique_ptr<Variant> m_data;

    public:
        Result(Parser& parser):
            m_trail(&parser) {}

        Result(Parser& parser, T&& data):
            m_trail(&parser),
            m_data(std::make_unique<Ok> (std::move(data))) {}

        Result(Parser& parser, Err&& data):
            m_trail(&parser),
            m_data(std::make_unique<Error> (std::move(data))) {}

        template<class A, class R>
        using Fun = std::function<Result<R> (A)>;

        template<class U>
        inline Result<U> and_then(Fun<T, U> f);

        inline Result<T> or_else(Fun<Err, T> f);

        inline T unwrap();

        inline Result<T> end_parse();

        inline bool is_ok() {
            return m_data->is_ok();
        }

        inline bool is_err() {
            return !is_ok();
        }
    };

    class Parser {
        std::string m_input;
        size_t m_current_pos;

    public:
        inline Parser(const std::string& input): m_input(input), m_current_pos(0) {}
        inline Parser(const Parser& other) = delete;
        inline Parser(Parser&& other) = delete;

        inline std::string cursor() const {
            return m_input.substr(m_current_pos);
        }

        //inline void end_parse_or_error()

        inline void skip_whitespace() {
            for (; m_current_pos < m_input.size(); m_current_pos++) {
                if (m_input[m_current_pos] != ' ') {
                    return;
                }
            }
            return;
        }

        inline void skip(size_t i) {
            m_current_pos += i;
        }

        inline size_t cursor_pos() const {
            return m_current_pos;
        }

        inline const std::string& input() const {
            return m_input;
        }

        inline Result<std::string> parse_ident() {
            Parser& s = *this;

            s.skip_whitespace();

            auto valid_first = [](uint8_t c) {
                return (c == '_')
                    || (c >= 'a' && c <= 'z')
                    || (c >= 'A' && c <= 'Z');
            };
            auto valid_middle = [=](uint8_t c) {
                return valid_first(c)
                    || (c >= '0' && c <= '9');
            };

            size_t i = 0;
            if (i < s.cursor().size() && valid_first(s.cursor()[i])) {
                for (i = 1; i < s.cursor().size() && valid_middle(s.cursor()[i]); i++) {
                }
                auto r = s.cursor().substr(0, i);
                s.skip(i);
                return ok<std::string>(std::move(r));
            } else {
                return err<std::string>("Expected an identifier");
            }
        }

        inline Result<uint8_t> parse_char(uint8_t chr) {
            Parser& s = *this;

            s.skip_whitespace();

            if (s.cursor().size() > 0 && uint8_t(s.cursor()[0]) == chr) {
                s.skip(1);
                return ok<uint8_t>(std::move(chr));
            } else {
                return err<uint8_t>(std::string("Expected char '")
                    + char(chr) + "'" + ", found '"
                    + s.cursor()[0] + "'");
            }
        }

        inline Result<std::string> parse_number() {
            Parser& s = *this;

            s.skip_whitespace();

            auto valid = [=](uint8_t c) {
                return c >= '0' && c <= '9';
            };

            size_t i = 0;
            if (i < s.cursor().size() && valid(s.cursor()[i])) {
                for (i = 1; i < s.cursor().size() && valid(s.cursor()[i]); i++) {
                }
                auto r = s.cursor().substr(0, i);
                s.skip(i);
                return ok<std::string>(std::move(r));
            } else {
                return err<std::string>("Expected an number");
            }
        }

        template<class T>
        inline Result<T> ok(T&& t) {
            return Result<T> {
                *this,
                std::move(t),
            };
        }

        template<class T>
        inline Result<T> err(std::string&& msg) {
            return Result<T> {
                *this,
                Err { std::move(msg) },
            };
        }

        template<class T>
        inline Result<T> err(Err&& msg) {
            return Result<T> {
                *this,
                std::move(msg),
            };
        }

        inline Result<AlgorithmArg> parse_arg(const std::string& keyword = "") {
            Parser& p = *this;

            return p.parse_ident().and_then<AlgorithmArg>([&](const std::string& arg_ident) {
                // "ident ..." case

                if (keyword == "") {
                    auto r = p.parse_char('=').and_then<AlgorithmArg>([&](uint8_t chr) {
                        // "ident = ..." case
                        return p.parse_arg(std::string(arg_ident));
                    });

                    if (r.is_ok()) {
                        return r;
                    }
                }

                return p.parse_args().and_then<AlgorithmArg>([&](std::vector<AlgorithmArg> args) {
                    // "ident(...) ..." case
                    return p.ok(AlgorithmArg {
                        std::string(keyword),
                        AlgorithmSpec {
                            std::string(arg_ident),
                            std::move(args)
                        }
                    });
                }).or_else([&](Err err) {
                    // "ident ..." fallback case
                    return p.ok<AlgorithmArg>(AlgorithmArg {
                        std::string(keyword),
                        std::string(arg_ident)
                    });
                });
            }).or_else([&](Err err) {
                return p.parse_number().and_then<AlgorithmArg>([&](const std::string& n) {
                    // "num ..." case
                    return p.ok<AlgorithmArg>(AlgorithmArg {
                        std::string(keyword),
                        std::string(n)
                    });
                });
            });
        }

        inline Result<std::vector<AlgorithmArg>> parse_args() {
            Parser& p = *this;

            return p.parse_char('(').and_then<std::vector<AlgorithmArg>>([&](uint8_t chr) {
                // Parse arguments here

                std::vector<AlgorithmArg> args;

                while (true) {
                    auto arg = p.parse_arg();
                    if (arg.is_ok()) {
                        args.push_back(arg.unwrap());
                        if (p.parse_char(',').is_err()) {
                            break;
                        }
                    } else {
                        break;
                    }
                }

                return p.parse_char(')').and_then<std::vector<AlgorithmArg>>([&](uint8_t chr) {
                    return p.ok(std::move(args));
                });
            });
        }

        inline Result<AlgorithmSpec> parse() {
            Parser& p = *this;

            return p.parse_ident().and_then<AlgorithmSpec>([&](const std::string& ident) {
                return p.parse_args().and_then<AlgorithmSpec>([&](std::vector<AlgorithmArg> args) {
                    return p.ok(AlgorithmSpec {
                        std::string(ident),
                        std::move(args)
                    });
                });
            }).end_parse();
        }

    };

    template<class T>
    inline T Result<T>::unwrap() {
        /*T ret;

        m_data.visit(
            [&](T& ok) {
                ret = m_f(ok);
            },
            [&](Err& err) {
                ret = m_trail->err<U>(err);
            }
        );

        return ret;*/

        return m_data->unwrap_or_else([&](Err&& err) -> T {
            std::stringstream ss;

            ss << "\nParse error at #" << int(m_trail->cursor_pos()) << ":\n";
            ss << m_trail->input() << "\n";
            ss << std::setw(m_trail->cursor_pos()) << "";
            ss << "^\n";
            ss << err.reason() << "\n";

            throw std::runtime_error(ss.str());
        });
    }

    template<class T>
    template<class U>
    inline Result<U> Result<T>::and_then(Fun<T, U> f) {
        Result<U> ret(*m_trail);

        m_data->visit(
            [&](T& ok) {
                ret = f(ok);
            },
            [&](Err& err) {
                ret = m_trail->err<U>(std::move(err));
            }
        );

        return ret;
    }

    template<class T>
    inline Result<T> Result<T>::or_else(Fun<Err, T> f) {
        Result<T> ret(*m_trail);

        m_data->visit(
            [&](T& ok) {
                ret = m_trail->ok<T>(std::move(ok));
            },
            [&](Err& err) {
                ret =  f(err);
            }
        );

        return ret;
    }

    template<class T>
    inline Result<T> Result<T>::end_parse() {
        return this->and_then<T>([&](T _t) {
            Parser& p = *m_trail;

            p.skip_whitespace();

            if (p.cursor() == "") {
                return std::move(*this);
            } else {
                return p.err<T>("Expected end of input");
            }
        });
    }
}

#endif
