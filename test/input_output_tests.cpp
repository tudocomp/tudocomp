#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>
#include <glog/logging.h>

#include <tudocomp/io/Input.hpp>
#include <tudocomp/io/Output.hpp>

#include "test/util.hpp"

using namespace tdc;
using namespace tdc::io;

TEST(AAAMmap, test) {
    auto ps = pagesize();

    std::vector<uint8_t> test_vec;
    test_vec.reserve(ps * 2);
    test_vec.resize(ps * 2);

    for(auto& e : test_vec) {
        e = 42;
    }

    std::vector<uint8_t> test_vec2;
    test_vec2.reserve(ps * 1);
    test_vec2.resize(ps * 1);

    auto basename = "mmap_overalloc_test";

    test::write_test_file(basename, test_vec);
    auto path = test::test_file_path(basename);

    // Given file [..42|..42], create [____[..42| ..0] buffer view into it

    {
        MMap map { path, MMap::Mode::ReadWrite, ps * 2, ps };

        DCHECK_EQ(map.view().slice(0, ps), View(test_vec).slice(ps));
        DCHECK_EQ(map.view().slice(ps), View(test_vec2));
        std::cout << "ReadWrite mode OK\n";
    }

    {
        const MMap map { path, MMap::Mode::Read, ps * 2, ps };

        DCHECK_EQ(map.view().slice(0, ps), View(test_vec).slice(ps));
        DCHECK_EQ(map.view().slice(ps), View(test_vec2));
        std::cout << "Read mode OK\n";
    }
}

const View STREAMBUF_ORIGINAL    = "test\x00\x00\xff\xfe""abcd"_v;
const View STREAMBUF_NTE         = "test\x00\x00\xff\xfe""abcd\0"_v;
const View STREAMBUF_ESCAPED_NTE = "test\xfe\xc0\xfe\xc0\xfe\xc1\xfe\xfe""abcd\0"_v;
const View STREAMBUF_ESCAPED     = "test\xfe\xc0\xfe\xc0\xfe\xc1\xfe\xfe""abcd"_v;

void check(View a, View b) {
    ASSERT_EQ(vec_to_debug_string(a), vec_to_debug_string(b));
}

TEST(AAViewSanity, test) {
    ViewStream vs { STREAMBUF_ORIGINAL };
    std::stringstream ss;
    ss << vs.stream().rdbuf();
    auto sss = ss.str();
    check(View(sss), STREAMBUF_ORIGINAL);
}

TEST(ARestrictedStreamBuf, Input_nte_only) {
    std::stringstream ssi;
    ssi << STREAMBUF_ORIGINAL;

    RestrictedIStreamBuf rb(ssi, InputRestrictions({}, true));

    std::stringstream ss;
    ss << &rb;
    auto sss = ss.str();
    check(View(sss), STREAMBUF_NTE);
}

TEST(ARestrictedStreamBuf, Output_nte_only) {
    std::stringstream sso;
    {
        RestrictedOStreamBuf rb(sso, InputRestrictions({}, true));
        std::ostream os { &rb };
        os << STREAMBUF_NTE;
        os.flush();
    }
    auto sss = sso.str();
    check(View(sss), STREAMBUF_ORIGINAL);
}

TEST(ARestrictedStreamBuf, Input_escaped_nte) {
    std::stringstream ssi;
    ssi << STREAMBUF_ORIGINAL;

    RestrictedIStreamBuf rb(ssi, InputRestrictions({0, 0xff}, true));

    std::stringstream ss;
    ss << &rb;
    auto sss = ss.str();
    check(View(sss), STREAMBUF_ESCAPED_NTE);
}

TEST(ARestrictedStreamBuf, Output_escaped_nte) {
    std::stringstream sso;
    {
        RestrictedOStreamBuf rb(sso, InputRestrictions({0, 0xff}, true));
        std::ostream os { &rb };
        os << STREAMBUF_ESCAPED_NTE;
        os.flush();
    }
    auto sss = sso.str();
    ASSERT_EQ(View(sss), STREAMBUF_ORIGINAL);
}

TEST(ARestrictedStreamBuf, Input_escaped) {
    std::stringstream ssi;
    ssi << STREAMBUF_ORIGINAL;

    RestrictedIStreamBuf rb(ssi, InputRestrictions({0, 0xff}, false));

    std::stringstream ss;
    ss << &rb;
    auto sss = ss.str();
    check(View(sss), STREAMBUF_ESCAPED);
}

TEST(ARestrictedStreamBuf, Output_escaped) {
    std::stringstream sso;
    {
        RestrictedOStreamBuf rb(sso, InputRestrictions({0, 0xff}, false));
        std::ostream os { &rb };
        os << STREAMBUF_ESCAPED;
        os.flush();
    }
    auto sss = sso.str();
    ASSERT_EQ(View(sss), STREAMBUF_ORIGINAL);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct TestString {
    View in_str;
    View escaped_str;
    InputRestrictions restrictions;
};

static const std::vector<TestString> direct_cases {
    TestString {
        "yasdvat\0rav\xffsds\xfevvssca"_v,
        "yasdvat\0rav\xffsds\xfevvssca"_v,
        InputRestrictions(),
    },
    TestString {
        "yasdvat\0rav\xffsds\xfevvssca"_v,
        "yasdvat\0rav\xffsds\xfevvssca\0"_v,
        InputRestrictions { {}, true },
    },
    TestString {
        "yasdvat\0rav\xffsds\xfevvssca"_v,
        "yasdvat\xff\xferav\xff\xffsds\xfevvssca\0"_v,
        InputRestrictions { { 0 }, true },
    },
    TestString {
        "yasdvat\0rav\xffsds\xfevvssca"_v,
        "yasdvat\xfe\xc0rav\xfe\xc1sds\xfe\xfevvssca\0"_v,
        InputRestrictions { { 0, 0xff }, true },
    },
    TestString {
        STREAMBUF_ORIGINAL,
        STREAMBUF_ORIGINAL,
        InputRestrictions { { }, false },
    },
    TestString {
        STREAMBUF_ORIGINAL,
        STREAMBUF_NTE,
        InputRestrictions { { }, true },
    },
    TestString {
        STREAMBUF_ORIGINAL,
        STREAMBUF_ESCAPED,
        InputRestrictions { { 0, 0xff }, false },
    },
    TestString {
        STREAMBUF_ORIGINAL,
        STREAMBUF_ESCAPED_NTE,
        InputRestrictions { { 0, 0xff }, true },
    },
    TestString {
        ""_v,
        ""_v,
        InputRestrictions { { }, false },
    },
    TestString {
        ""_v,
        "\0"_v,
        InputRestrictions { { }, true },
    },
    TestString {
        ""_v,
        "\0"_v,
        InputRestrictions { { 0, 0xff }, true },
    },
};

struct SplitTestString {
    View outer_str;
    View prefix_str;
    size_t prefix;
    TestString inner;
};

static const std::vector<SplitTestString> driver_split_cases {
    SplitTestString {
        "yasdvat\0rav\xffsds\xfevvssca"_v,
        "yasdvat\0r"_v,
        9,
        TestString {
            "av\xffsds\xfevvssca"_v,
            "av\xffsds\xfevvssca"_v,
            InputRestrictions(),
        },
    },
    SplitTestString {
        "yasdvat\0rav\xffsds\xfevvssca"_v,
        "yasdvat\0r"_v,
        9,
        TestString {
            "av\xffsds\xfevvssca"_v,
            "av\xffsds\xfevvssca\0"_v,
            InputRestrictions { {}, true },
        },
    },
    SplitTestString {
        "yasdvat\0rav\xffsds\xfevvssca"_v,
        "yasdvat\0r"_v,
        9,
        TestString {
            "av\xffsds\xfevvssca"_v,
            "av\xff\xffsds\xfevvssca\0"_v,
            InputRestrictions { { 0 }, true },
        },
    },
    SplitTestString {
        "yasdvat\0rav\xffsds\xfevvssca"_v,
        "yasdvat\0r"_v,
        9,
        TestString {
            "av\xffsds\xfevvssca"_v,
            "av\xfe\xc1sds\xfe\xfevvssca\0"_v,
            InputRestrictions { { 0, 0xff }, true },
        },
    },
    SplitTestString {
        "yasdvat\0r"_v,
        "yasdvat\0r"_v,
        9,
        TestString {
            ""_v,
            ""_v,
            InputRestrictions(),
        },
    },
    SplitTestString {
        "yasdvat\0r"_v,
        "yasdvat\0r"_v,
        9,
        TestString {
            ""_v,
            "\0"_v,
            InputRestrictions { {}, true },
        },
    },
    SplitTestString {
        "yasdvat\0r"_v,
        "yasdvat\0r"_v,
        9,
        TestString {
            ""_v,
            "\0"_v,
            InputRestrictions { { 0 }, true },
        },
    },
    SplitTestString {
        "yasdvat\0r"_v,
        "yasdvat\0r"_v,
        9,
        TestString {
            ""_v,
            "\0"_v,
            InputRestrictions { { 0, 0xff }, true },
        },
    },
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct ViewSrc {
    View m_view;
    ViewSrc(View v): m_view(v) {}

    View view() { return m_view; }

    Input input() {
        return Input(view());
    }
};

struct FileSrc {
    View m_view;
    FileSrc(View v): m_view(v) {}

    std::string file() {
        std::hash<std::string> hasher;

        std::string prefix = "io_test_matrix_3_";

        std::string basename = prefix + std::string(m_view);
        std::stringstream ss;
        ss << hasher(basename);
        basename = prefix + ss.str() + ".txt";

        test::write_test_file(basename, m_view);
        return test::test_file_path(basename);
    }

    Input input() {
        return Input(Path { file() });
    }
};

struct StreamSrc {
    View m_view;
    std::stringstream m_ss;
    StreamSrc(View v): m_view(v) {}

    std::istream& stream() {
        m_ss = std::stringstream();
        m_ss << m_view;
        return m_ss;
    }

    Input input() {
        return Input(stream());
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

TEST(Input, from_view_sanity) {
    ViewSrc x { direct_cases[0].in_str };
    ASSERT_EQ(x.view(), direct_cases[0].escaped_str);
}

TEST(Input, from_file_sanity) {
    FileSrc x { direct_cases[0].in_str };
    ASSERT_EQ(read_file_to_stl_byte_container<std::string>(x.file()), direct_cases[0].escaped_str);
}

TEST(Input, from_stream_sanity) {
    StreamSrc x { direct_cases[0].in_str };
    std::stringstream ss;
    ss << x.stream().rdbuf();
    ASSERT_EQ(ss.str(), direct_cases[0].escaped_str);
}

void input_equal(const Input& i, const View& str) {
    {
        auto x = i.as_view();

        auto is = vec_to_debug_string(View(x), 3);
        auto should_be = vec_to_debug_string(str, 3);
        ASSERT_EQ(is, should_be);
        //std::cout << "    View Ok\n";
    }
    {
        auto x = i.as_stream();
        std::stringstream ss;
        ss << x.rdbuf();

        auto is = vec_to_debug_string(ss.str(), 3);
        auto should_be = vec_to_debug_string(str, 3);
        ASSERT_EQ(is, should_be);
        //std::cout << "    Stream Ok\n";
    }
}

struct Direct {
    template<typename InpSrc>
    static void doit() {
        for (auto& tests : direct_cases) {
            std::cout << "Case: " << vec_to_debug_string(tests.in_str) << "\n";
            InpSrc x { tests.in_str };
            Input i = x.input();

            i = Input(i, tests.restrictions);

            input_equal(i, tests.escaped_str);
            std::cout << "Ok:   " << vec_to_debug_string(tests.escaped_str) << "\n\n";
        }
    }
};

struct DriverSplit {
    template<typename InpSrc>
    static void doit() {
        for (auto& tests : driver_split_cases) {
            std::cout << "Case: " << vec_to_debug_string(tests.outer_str) << "\n";
            InpSrc x { tests.outer_str };
            Input i = x.input();

            size_t actual_p;
            {
                auto s = i.as_stream();
                size_t c = 0;
                auto b = s.begin();
                auto e = s.end();
                std::vector<uint8_t> p;
                while(c < tests.prefix && b != e) {
                    p.push_back(*b);
                    ++b;
                    ++c;
                }
                actual_p = c;
                ASSERT_EQ(View(p), tests.prefix_str);
                std::cout << "Ok prefix: " << vec_to_debug_string(p) << "\n";
            }

            {
                Input sliced = Input(i, actual_p);
                Input restricted = Input(sliced, tests.inner.restrictions);
                input_equal(restricted, tests.inner.escaped_str);
            }

            std::cout << "Ok slice: " << vec_to_debug_string(tests.inner.escaped_str) << "\n\n";
        }
    }
};

template<typename InpSrc, typename Splitting>
void i_matrix_test() {
    Splitting::template doit<InpSrc>();
}

TEST(InputMatrix, ViewSrc_Direct) {
    i_matrix_test<ViewSrc, Direct>();
}
TEST(InputMatrix, FileSrc_Direct) {
    i_matrix_test<FileSrc, Direct>();
}
TEST(InputMatrix, StreamSrc_Direct) {
    i_matrix_test<StreamSrc, Direct>();
}
TEST(InputMatrix, ViewSrc_DriverSplit) {
    i_matrix_test<ViewSrc, DriverSplit>();
}
TEST(InputMatrix, FileSrc_DriverSplit) {
    i_matrix_test<FileSrc, DriverSplit>();
}
TEST(InputMatrix, StreamSrc_DriverSplit) {
    i_matrix_test<StreamSrc, DriverSplit>();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct MemTrgt {
    View m_view;
    std::vector<uint8_t> m_mem;
    MemTrgt(View v): m_view(v) {}

    std::vector<uint8_t>& mem() { return m_mem; }

    Output output() {
        return Output(mem());
    }

    std::vector<uint8_t> result() {
        return m_mem;
    }
};

struct FileTrgt {
    View m_view;
    std::string m_basename;
    FileTrgt(View v): m_view(v) {
        std::hash<std::string> hasher;

        std::string prefix = "io_test_matrix_3_out_";

        std::string basename = prefix + std::string(m_view);
        std::stringstream ss;
        ss << hasher(basename);
        basename = prefix + ss.str() + ".txt";

        test::write_test_file(basename, m_view);
        m_basename = basename;
    }

    std::string file() {
        return test::test_file_path(m_basename);
    }

    Output output() {
        return Output(Path { file() }, true);
    }

    std::vector<uint8_t> result() {
        auto s = test::read_test_file(m_basename);
        std::vector<uint8_t> r = View(s);
        return r;
    }
};

struct StreamTrgt {
    View m_view;
    std::stringstream m_ss;
    StreamTrgt(View v): m_view(v) {}

    std::ostream& stream() {
        return m_ss;
    }

    Output output() {
        return Output(stream());
    }

    std::vector<uint8_t> result() {
        auto s = m_ss.str();
        std::vector<uint8_t> r = View(s);
        return r;
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

struct OutDirect {
    template<typename OutTrgt>
    static void doit() {
        for (const auto& tests: direct_cases) {
            OutTrgt out0 { tests.escaped_str };
            {
                Output out1 = out0.output();

                out1 = Output(out1, tests.restrictions);

                {
                    auto out2 = out1.as_stream();
                    out2 << tests.escaped_str;
                }
            }

            auto res = out0.result();

            ASSERT_EQ(vec_to_debug_string(res),
                      vec_to_debug_string(tests.in_str));

            std::cout << "OK: "
                << "   " << vec_to_debug_string(tests.escaped_str) << "\n    -> "
                << vec_to_debug_string(tests.in_str)
                << "\n";
        }
    }
};

struct OutDriverSplit {
    template<typename OutTrgt>
    static void doit() {
        for (const auto& tests: driver_split_cases) {
            OutTrgt out0 { tests.outer_str };
            Output out1 = out0.output();
            {
                auto out2 = out1.as_stream();
                out2 << tests.prefix_str;
            }
            out1 = Output(out1, tests.inner.restrictions);
            {
                auto out2 = out1.as_stream();
                out2 << tests.inner.escaped_str;
            }

            auto res = out0.result();

            ASSERT_EQ(vec_to_debug_string(res),
                      vec_to_debug_string(tests.outer_str));

            std::cout << "OK: "
                << vec_to_debug_string(tests.outer_str)
                << "\n";
        }
    }
};

template<typename OutTrgt, typename Splitting>
void o_matrix_test() {
    Splitting::template doit<OutTrgt>();
}

TEST(OnputMatrix, MemTrgt_OutDirect) {
    o_matrix_test<MemTrgt, OutDirect>();
}
TEST(OnputMatrix, FileTrgt_OutDirect) {
    o_matrix_test<FileTrgt, OutDirect>();
}
TEST(OnputMatrix, StreamTrgt_OutDirect) {
    o_matrix_test<StreamTrgt, OutDirect>();
}
TEST(OnputMatrix, MemTrgt_OutDriverSplit) {
    o_matrix_test<MemTrgt, OutDriverSplit>();
}
TEST(OnputMatrix, FileTrgt_OutDriverSplit) {
    o_matrix_test<FileTrgt, OutDriverSplit>();
}
TEST(OnputMatrix, StreamTrgt_OutDriverSplit) {
    o_matrix_test<StreamTrgt, OutDriverSplit>();
}
