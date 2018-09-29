#include <glog/logging.h>
#include <gtest/gtest.h>

#include <limits.h>
#include <sstream>

#include <tudocomp/meta/Decl.hpp>
#include <tudocomp/meta/DeclLib.hpp>
#include <tudocomp/meta/Config.hpp>
#include <tudocomp/meta/Meta.hpp>
#include <tudocomp/meta/RegistryOf.hpp>

using namespace tdc::meta;

template<typename F>
void test_with_whitespace(const std::string& input, F f) {
    const std::string whitespace = " \t \n \r ";

    f(input);
    f(whitespace + input);
    f(input + whitespace);
    f(whitespace + input + whitespace);
}

TEST(ast, parse_none) {
    test_with_whitespace("", [&](const std::string& input){
        ASSERT_EQ(nullptr, ast::Parser::parse(input));
    });
}

TEST(ast, parse_value) {
    test_with_whitespace("1", [&](const std::string& input){
        ASSERT_EQ("1",
            ast::convert<ast::Value>(ast::Parser::parse(input))->value());
    });
    test_with_whitespace("+1", [&](const std::string& input){
        ASSERT_EQ("+1",
            ast::convert<ast::Value>(ast::Parser::parse(input))->value());
    });
    test_with_whitespace("-1", [&](const std::string& input){
        ASSERT_EQ("-1",
            ast::convert<ast::Value>(ast::Parser::parse(input))->value());
    });
    test_with_whitespace("3.14159", [&](const std::string& input){
        ASSERT_EQ("3.14159",
            ast::convert<ast::Value>(ast::Parser::parse(input))->value());
    });
    test_with_whitespace("+3.14159", [&](const std::string& input){
        ASSERT_EQ("+3.14159",
            ast::convert<ast::Value>(ast::Parser::parse(input))->value());
    });
    test_with_whitespace("-3.14159", [&](const std::string& input){
        ASSERT_EQ("-3.14159",
            ast::convert<ast::Value>(ast::Parser::parse(input))->value());
    });
    test_with_whitespace("''", [&](const std::string& input){
        ASSERT_EQ("",
            ast::convert<ast::Value>(ast::Parser::parse(input))->value());
    });
    test_with_whitespace("'\t\r\n '", [&](const std::string& input){
        ASSERT_EQ("\t\r\n ",
            ast::convert<ast::Value>(ast::Parser::parse(input))->value());
    });
    test_with_whitespace("'abc'", [&](const std::string& input){
        ASSERT_EQ("abc",
            ast::convert<ast::Value>(ast::Parser::parse(input))->value());
    });
    test_with_whitespace("abc", [&](const std::string& input){
        // "abc" is parsed as a name and should not be convertible
        // to a value
        ASSERT_THROW(
            ast::convert<ast::Value>(ast::Parser::parse(input)),
            ast::TypeMismatchError);
    });
}

TEST(ast, parse_object) {
    test_with_whitespace("obj()", [&](const std::string& input){
        auto obj = ast::convert<ast::Object>(ast::Parser::parse(input));
        ASSERT_EQ("obj", obj->name());
        ASSERT_EQ(0, obj->params().size());
        ASSERT_FALSE(obj->has_param("void"));
    });
    test_with_whitespace("obj(\t\r\n )", [&](const std::string& input){
        auto obj = ast::convert<ast::Object>(ast::Parser::parse(input));
        ASSERT_EQ("obj", obj->name());
        ASSERT_EQ(0, obj->params().size());
        ASSERT_FALSE(obj->has_param("void"));
    });
    test_with_whitespace("obj", [&](const std::string& input){
        auto obj = ast::convert<ast::Object>(ast::Parser::parse(input));
        ASSERT_EQ("obj", obj->name());
        ASSERT_EQ(0, obj->params().size());
        ASSERT_FALSE(obj->has_param("void"));
        ASSERT_FALSE(obj->has_param(""));
    });
    test_with_whitespace("obj(x=1, y = '1',\r\n z='', 1 ,unnamed_nested(   ))",
    [&](const std::string& input){
        auto obj = ast::convert<ast::Object>(ast::Parser::parse(input));
        ASSERT_EQ("obj", obj->name());
        ASSERT_TRUE(obj->has_param("x"));
        ASSERT_TRUE(obj->has_param("y"));
        ASSERT_TRUE(obj->has_param("z"));
        ASSERT_TRUE(obj->has_param(""));
        ASSERT_FALSE(obj->has_param("void"));
        auto& params = obj->params();
        ASSERT_EQ(5, params.size());
        ASSERT_EQ("x", params[0].name());
        ASSERT_EQ("1", ast::convert<ast::Value>(params[0].value())->value());
        ASSERT_EQ("y", params[1].name());
        ASSERT_EQ("1", ast::convert<ast::Value>(params[1].value())->value());
        ASSERT_EQ("z", params[2].name());
        ASSERT_EQ("", ast::convert<ast::Value>(params[2].value())->value());
        ASSERT_EQ("", params[3].name());
        ASSERT_EQ("1", ast::convert<ast::Value>(params[3].value())->value());
        ASSERT_EQ("", params[4].name());
        ASSERT_EQ("unnamed_nested",
            ast::convert<ast::Object>(params[4].value())->name());
        ASSERT_EQ(0,
            ast::convert<ast::Object>(params[4].value())->params().size());
    });
}

TEST(ast, parse_list) {
    test_with_whitespace("[]", [&](const std::string& input){
        auto list = ast::convert<ast::List>(ast::Parser::parse(input));
        ASSERT_EQ(0, list->items().size());
    });
    test_with_whitespace("[\t\r\n ]", [&](const std::string& input){
        auto list = ast::convert<ast::List>(ast::Parser::parse(input));
        ASSERT_EQ(0, list->items().size());
    });
    test_with_whitespace("[ 1 ,'', -1,'xyz'\n, \r\n\t obj(a=[\n]), [1,2,3]]",
    [&](const std::string& input){
        auto list = ast::convert<ast::List>(ast::Parser::parse(input));
        auto& items = list->items();
        ASSERT_EQ(6, items.size());
        ASSERT_EQ("1", ast::convert<ast::Value>(items[0])->value());
        ASSERT_EQ("", ast::convert<ast::Value>(items[1])->value());
        ASSERT_EQ("-1", ast::convert<ast::Value>(items[2])->value());
        ASSERT_EQ("xyz", ast::convert<ast::Value>(items[3])->value());
        ASSERT_EQ("obj", ast::convert<ast::Object>(items[4])->name());
        ASSERT_EQ(1, ast::convert<ast::Object>(items[4])->params().size());
        ASSERT_EQ(3, ast::convert<ast::List>(items[5])->items().size());
    });
}

TEST(typedesc, basic) {
    TypeDesc invalid;
    ASSERT_FALSE(invalid.valid());
    ASSERT_TRUE(TypeDesc("any_name").valid());
}

TEST(typedesc, inheritance) {
    TypeDesc invalid;
    TypeDesc animal("animal");
    TypeDesc dog("dog", animal);
    TypeDesc bird("bird", animal);
    TypeDesc duck("duck", bird);
    TypeDesc goose("goose", bird);

    ASSERT_TRUE(dog.subtype_of(animal));
    ASSERT_TRUE(bird.subtype_of(animal));
    ASSERT_TRUE(duck.subtype_of(bird));
    ASSERT_TRUE(duck.subtype_of(animal));
    ASSERT_FALSE(duck.subtype_of(dog));
    ASSERT_FALSE(duck.subtype_of(goose));
    ASSERT_FALSE(duck.subtype_of(invalid));
    ASSERT_EQ(nullptr, animal.super());
    ASSERT_EQ(&animal, bird.super());
    ASSERT_EQ(&bird, duck.super());
    ASSERT_TRUE(duck == TypeDesc("duck"));
    ASSERT_TRUE(duck != goose);
}

TEST(lib, insert_find) {
    // declare some types
    TypeDesc a_type("A");
    TypeDesc b_type("B", a_type); // B inherits from A
    TypeDesc c_type("C", a_type); // C inherits from A
    TypeDesc x_type("X");

    // construct declarations
    auto a = std::make_shared<Decl>("a", a_type);
    auto b = std::make_shared<Decl>("b", b_type);
    auto c = std::make_shared<Decl>("c", c_type);
    auto x = std::make_shared<Decl>("x", x_type);

    // a declaration named "a" but of type X
    auto a_x = std::make_shared<Decl>("a", x_type);
    // a declaration named "a" but of type B
    auto a_b = std::make_shared<Decl>("a", b_type);

    // construct library
    DeclLib lib;
    lib.insert(a);
    lib.insert(b);
    lib.insert(c);
    lib.insert(x);
    lib.insert(a_x); // this works, because the already registered "a"
                     // is of type A, which is unrelated to X

    // this should fail, because there is already an "a" of type A,
    // which is a supertype of B
    ASSERT_THROW(lib.insert(a_b), LibError);

    // find
    ASSERT_EQ(a, lib.find("a", a_type));
    ASSERT_EQ(b, lib.find("b", b_type));
    ASSERT_EQ(b, lib.find("b", a_type));
    ASSERT_EQ(c, lib.find("c", c_type));
    ASSERT_EQ(c, lib.find("c", a_type));
    ASSERT_EQ(a_x, lib.find("a", x_type));
    ASSERT_EQ(nullptr, lib.find("a", b_type).get());
}

TEST(lib, merge) {
    // construct some declarations
    TypeDesc a_type("A");
    TypeDesc b_type("B", a_type); // B inherits from A
    TypeDesc x_type("X");
    TypeDesc y_type("Y", x_type); // Y inherits from X
    auto a = std::make_shared<Decl>("a", a_type);
    auto b = std::make_shared<Decl>("b", b_type);
    auto x = std::make_shared<Decl>("x", x_type);
    auto y = std::make_shared<Decl>("y", y_type);
    // a declaration named "a" but of type B
    auto a_b = std::make_shared<Decl>("a", b_type);

    // construct libraries
    DeclLib lib1; lib1.insert(a); lib1.insert(b);
    DeclLib lib2; lib2.insert(x); lib2.insert(y);
    DeclLib lib3; lib3.insert(a); lib3.insert(x);
    DeclLib lib4; lib4.insert(a_b); lib4.insert(x);

    // merge lib1 and lib2
    {
        auto merged = lib1 + lib2;
        ASSERT_EQ(a, merged.find("a", a_type));
        ASSERT_EQ(b, merged.find("b", b_type));
        ASSERT_EQ(x, merged.find("x", x_type));
        ASSERT_EQ(y, merged.find("y", y_type));
    }

    // attempt to merge with lib3
    // this should work, because the declarations' types match with
    // those of lib1 and lib2
    {
        auto merged = lib3 + (lib1 + lib2);
        ASSERT_EQ(a, merged.find("a", a_type));
        ASSERT_EQ(b, merged.find("b", b_type));
        ASSERT_EQ(x, merged.find("x", x_type));
        ASSERT_EQ(y, merged.find("y", y_type));
    }

    // attempt to merge with lib4
    // this should fail, because "a" is declared with type A in lib1 and B
    // in lib4
    ASSERT_THROW(lib4 + (lib1 + lib2), LibError);
}

class config : public ::testing::Test {
protected:
    static constexpr TypeDesc a_type() { return TypeDesc("A"); }
    static constexpr TypeDesc b_type() { return TypeDesc("B"); }
    static constexpr TypeDesc c_type() { return TypeDesc("C"); }

    std::shared_ptr<Decl> a, b, c;
    DeclLib lib;

    virtual void SetUp() override {
        a = std::make_shared<Decl>("a", a_type());
        b = std::make_shared<Decl>("b", b_type());
        c = std::make_shared<Decl>("c", c_type());

        // add parameters to a
        {
            // p1 is a primitive value with no default
            a->add_param(Decl::Param(
                "p1", "", Decl::Param::Kind::primitive,
                false, TypeDesc(), ast::NodePtr<>()));

            // p2 is a primitive value with default value "1"
            a->add_param(Decl::Param(
                "p2", "", Decl::Param::Kind::primitive,
                false, TypeDesc(), ast::Parser::parse("1")));

            // l1 is a list of primitive values with no default
            a->add_param(Decl::Param(
                "l1", "", Decl::Param::Kind::primitive,
                true, TypeDesc(), ast::NodePtr<>()));

            // l2 is a list of primitive values with default "[1,2,3]"
            a->add_param(Decl::Param(
                "l2", "", Decl::Param::Kind::primitive,
                true, TypeDesc(), ast::Parser::parse("[1,2,3]")));

            // b1 is an object of type B with no default
            // it is bounded and should occur in the signature
            a->add_param(Decl::Param(
                "b1", "", Decl::Param::Kind::bound,
                false, b_type(), ast::NodePtr<>()));

            // b2 is an object of type B with default "b()"
            // it is unbounded and should not occur in the signature
            a->add_param(Decl::Param(
                "b2", "", Decl::Param::Kind::unbound,
                false, b_type(), ast::Parser::parse("b()")));
        }

        // add parameters to b
        {
            // cl is a list of objects of type C with default "[]"
            // they are bounded and should occur in the signature
            b->add_param(Decl::Param(
                "cl", "", Decl::Param::Kind::bound,
                true, c_type(), ast::Parser::parse("[]")));
        }

        // insert a and b into lib
        lib.insert(a);
        lib.insert(b);
        lib.insert(c);
    }

    inline Config a_cfg(const std::string& str) {
        return Config(a, ast::Parser::parse(str), lib);
    }

    inline Config a_cfg(
        const std::string& str,
        const DeclLib& _lib) {

        return Config(a, ast::Parser::parse(str), _lib);
    }
};

TEST_F(config, sanity) {
    // null
    ASSERT_THROW(a_cfg(""),  ast::TypeMismatchError);
    // wrong node type
    ASSERT_THROW(a_cfg("1"), ast::TypeMismatchError);
    // wrong name
    ASSERT_THROW(a_cfg("x"), ConfigError);
    // missing values for p1, l1, b1
    ASSERT_THROW(a_cfg("a"), ConfigError);
    // missing values for p1, l1, b1
    ASSERT_THROW(a_cfg("a(p2=2,l2=[7,8,9],b1=b)"), ConfigError);
    // missing value for p1
    ASSERT_THROW(a_cfg("a(l1=[],b1=b)"), ConfigError);
    // missing value for l1
    ASSERT_THROW(a_cfg("a(p1=1,b1=b)"), ConfigError);
    // missing value for b1
    ASSERT_THROW(a_cfg("a(p1=1,l1=[])"), ConfigError);
    // OK
    ASSERT_NO_THROW(a_cfg("a(p1=2,l1=[],b1=b)"));
    // OK
    ASSERT_NO_THROW(a_cfg("a(p1=2,l1=[],b1=b(cl=[c,c,c]))"));
    // OK
    ASSERT_NO_THROW(a_cfg("a(p1=1,p2=2,l1=[],l2=[7,8,9],b1=b,b2=b)"));
    // b not known
    ASSERT_THROW(a_cfg("a(p1=2,l1=[],b1=b)", DeclLib()), ConfigError);
    // bad type for p1
    ASSERT_THROW(a_cfg("a(p1=[],l1=[],b1=b)"), ast::TypeMismatchError);
    // bad type for l1
    ASSERT_THROW(a_cfg("a(p1=1,l1=c,b1=b)"), ast::TypeMismatchError);
    // bad value type for b1
    ASSERT_THROW(a_cfg("a(p1=1,l1=[],b1=1)"), ast::TypeMismatchError);
    // bad algorithm type for b1
    ASSERT_THROW(a_cfg("a(p1=1,l1=[],b1=c)"), ConfigError);
    // bad algorithm type in cl
    ASSERT_THROW(a_cfg("a(p1=1,l1=[],b1=b(cl=[c,a]))"), ConfigError);
    // OK
    ASSERT_NO_THROW(a_cfg("a(1,2,[],[7,8,9],b(cl=[c]),b)"));
    // missing value for b1
    ASSERT_THROW(a_cfg("a(1,2,[],[7,8,9])"), ConfigError);
    // bad types
    ASSERT_THROW(a_cfg("a(1,[],b)"), ast::TypeMismatchError);

    auto cfg = a_cfg("a(p1=2,l1=[],b1=b)");
    // param x1 does not exist
    ASSERT_THROW(cfg.param("x1"), std::runtime_error);
    // param b2 does exist (default value)
    ASSERT_NO_THROW(cfg.param("b2"));
}

TEST_F(config, signature) {
    ASSERT_EQ("a(b1=b(cl=[]))",
        a_cfg("a(p1=2,l1=[],b1=b)").signature()->str());
    ASSERT_EQ("a(b1=b(cl=[c(), c(), c()]))",
        a_cfg("a(p1=7,l1=[1,2,3],b1=b(cl=[c,c,c]))").signature()->str());
}

TEST_F(config, primitive) {
    auto cfg = a_cfg("a(p1=-1.25,p2='true',l1=[],b1=b)");
    auto p1 = cfg.param("p1");
    ASSERT_EQ(-1, p1.as_int());
    ASSERT_EQ(UINT_MAX, p1.as_uint());
    ASSERT_EQ(-1.25f, p1.as_float());
    ASSERT_EQ(-1.25, p1.as_double());
    ASSERT_FALSE(p1.as_bool());
    ASSERT_THROW(p1.as_vector<int>(), ast::TypeMismatchError);

    auto p2 = cfg.param("p2");
    ASSERT_TRUE(p2.as_bool());
    ASSERT_EQ(0U, p2.as_uint());
    ASSERT_THROW(p2.as_vector<std::string>(), ast::TypeMismatchError);
}

TEST_F(config, primitive_list) {
    auto cfg = a_cfg("a(l1=[],l2=[+3,-1.5,'x'],p1=0,b1=b)");
    auto l1 = cfg.param("l1");
    ASSERT_THROW(l1.as_int(), ast::TypeMismatchError);
    ASSERT_EQ(0, l1.as_vector<int>().size());

    auto l2 = cfg.param("l2");
    ASSERT_EQ(3, l2.as_vector<int>().size());
    {
        auto vi = l2.as_vector<int>();
        ASSERT_EQ(3, vi[0]);
        ASSERT_EQ(-1, vi[1]);
        ASSERT_EQ(0, vi[2]);
    }
    {
        auto vf = l2.as_vector<float>();
        ASSERT_EQ(3.0f, vf[0]);
        ASSERT_EQ(-1.5f, vf[1]);
        ASSERT_EQ(0.0f, vf[2]);
    }
    {
        auto vs = l2.as_vector<std::string>();
        ASSERT_EQ("+3", vs[0]);
        ASSERT_EQ("-1.5", vs[1]);
        ASSERT_EQ("x", vs[2]);
    }
}

TEST_F(config, sub) {
    auto cfg = a_cfg("a(b1=b(cl=[c,c,c]),b2=b,p1=1,l1=[])");
    {
        auto b1 = cfg.param("b1");
        ASSERT_THROW(b1.as_int(), ast::TypeMismatchError);
        ASSERT_THROW(b1.as_vector<int>(), ast::TypeMismatchError);
    }
    {
        auto b1 = cfg.sub_config("b1");
        ASSERT_EQ(b, b1.decl());
        auto& cl = b1.sub_configs("cl");
        ASSERT_EQ(3, cl.size());
        ASSERT_EQ(c, cl[0].decl());
        ASSERT_EQ(c, cl[1].decl());
        ASSERT_EQ(c, cl[2].decl());
    }
    {
        auto b2 = cfg.sub_config("b2");
        ASSERT_EQ(b, b2.decl());
        auto& cl = b2.sub_configs("cl");
        ASSERT_EQ(0, cl.size());
    }
}

TEST_F(config, defaults) {
    auto cfg = a_cfg("a(p1=0,l1=[],b1=b)");
    {
        auto p2 = cfg.param("p2");
        ASSERT_EQ(1, p2.as_int());
    }
    {
        auto l2 = cfg.param("l2");
        auto vi = l2.as_vector<int>();
        ASSERT_EQ(3, vi.size());
        ASSERT_EQ(1, vi[0]);
        ASSERT_EQ(2, vi[1]);
        ASSERT_EQ(3, vi[2]);
    }
    {
        auto b2 = cfg.sub_config("b2");
        ASSERT_EQ(b, b2.decl());
        auto& cl = b2.sub_configs("cl");
        ASSERT_EQ(0, cl.size());
    }
}

#include <tudocomp/Algorithm.hpp>

using Algorithm = tdc::Algorithm;

class meta : public ::testing::Test {
protected:
    static constexpr TypeDesc a_type() { return TypeDesc("A"); }
    static constexpr TypeDesc b_type() { return TypeDesc("B"); }
    static constexpr TypeDesc c_type() { return TypeDesc("C"); }

    // first class with type C
    class C1 : public Algorithm {
    public:
        static inline Meta meta() { return Meta(c_type(), "c1"); }
        using Algorithm::Algorithm;
    };

    // second class with type C
    class C2 : public Algorithm {
    public:
        static inline Meta meta() { return Meta(c_type(), "c2"); }
        using Algorithm::Algorithm;
    };

    // B-type class
    template<typename... c_t>
    class B : public Algorithm {
    public:
        static inline Meta meta() {
            Meta m(b_type(), "b");
            m.param("cl").strategy_list<c_t...>(c_type());
            return m;
        }

        using Algorithm::Algorithm;
    };

    // A-type class
    template<typename b_t>
    class A : public Algorithm {
    public:
        static inline Meta meta() {
            Meta m(a_type(), "a");
            m.param("p1").primitive();
            m.param("p2").primitive(1);
            m.param("l1").primitive_list();
            m.param("l2").primitive_list({1,2,3});
            m.param("b1").strategy<b_t>(b_type());
            m.param("b2").unbound_strategy(
                b_type(), Meta::Default<B<C2,C1>>());
            return m;
        }

        using Algorithm::Algorithm;
    };
};

TEST_F(meta, signature) {
    using A_B_ = A<B<>>;
    using A_B_C1C2 = A<B<C1, C2>>;
    using A_B_C2C1 = A<B<C2, C1>>;

    ASSERT_EQ(
        "a(b1=b(cl=[]))", A_B_::meta().signature()->str());
    ASSERT_EQ(
        "a(b1=b(cl=[c1(), c2()]))", A_B_C1C2::meta().signature()->str());
    ASSERT_EQ(
        "a(b1=b(cl=[c2(), c1()]))", A_B_C2C1::meta().signature()->str());
}

TEST_F(meta, default_cfg) {
    using Subject = A<B<C1, C2>>;

    auto meta = Subject::meta();

    // no defaults for p1 and l1
    ASSERT_THROW(meta.config(), ConfigError);

    // OK (b1 and cl are bounded and therefore configured implicitly)
    ASSERT_NO_THROW(meta.config(ast::convert<ast::Object>(
        ast::Parser::parse("a(p1=777,l1=[])"))));

    // not default for cl of b2
    ASSERT_THROW(meta.config(ast::convert<ast::Object>(
        ast::Parser::parse("a(p1=777,l1=[],b2=b)"))),
        ConfigError);

    // OK
    ASSERT_NO_THROW(meta.config(ast::convert<ast::Object>(
        ast::Parser::parse("a(p1=777,l1=[],b2=b(cl=[]))"))));

    // config checks
    {
        auto cfg = meta.config(ast::convert<ast::Object>(
            ast::Parser::parse("a(p1=777,l1=[])")));

        ASSERT_EQ(777, cfg.param("p1").as_int());
        ASSERT_EQ(1, cfg.param("p2").as_int());
        ASSERT_EQ(0, cfg.param("l1").as_vector<int>().size());
        ASSERT_EQ(3, cfg.param("l2").as_vector<int>().size());

        auto b1 = cfg.sub_config("b1");
        ASSERT_EQ("b", b1.decl()->name());
        auto b2 = cfg.sub_config("b2");
        ASSERT_EQ("b", b1.decl()->name());

        auto& cl1 = b1.sub_configs("cl");
        ASSERT_EQ(2, cl1.size());
        ASSERT_EQ("c1", cl1[0].decl()->name());
        ASSERT_EQ("c2", cl1[1].decl()->name());
        auto& cl2 = b2.sub_configs("cl");
        ASSERT_EQ(2, cl2.size());
        ASSERT_EQ("c2", cl2[0].decl()->name());
        ASSERT_EQ("c1", cl2[1].decl()->name());
    }
}

class registry : public meta {
protected:
};

TEST_F(registry, _register) {
    using A_B_ = A<B<>>;
    using A_B_C1C2 = A<B<C1, C2>>;
    using A_B_C2C1 = A<B<C2, C1>>;

    // create a registry for type A
    RegistryOf<Algorithm> reg(a_type());

    // register
    ASSERT_NO_THROW(reg.register_algorithm<A_B_>());
    ASSERT_NO_THROW(reg.register_algorithm<A_B_C1C2>());
    ASSERT_NO_THROW(reg.register_algorithm<A_B_C2C1>());

    // already registered
    ASSERT_THROW(reg.register_algorithm<A_B_>(), RegistryError);

    // wrong type
    ASSERT_THROW(reg.register_algorithm<C1>(), RegistryError);
}

TEST_F(registry, select_ast) {
    // create a registry for type A
    RegistryOf<Algorithm> reg(a_type());
    reg.register_algorithm<A<B<>>>();
    reg.register_algorithm<A<B<C1, C2>>>();
    reg.register_algorithm<A<B<C2, C1>>>();

    // select unknown algorithm
    ASSERT_THROW(reg.select("x"), RegistryError);

    // no defaults for p1 and l1
    ASSERT_THROW(reg.select("a(b1=b([c1,c1]))"), ConfigError);

    // unregistered algorithm A<B<C1,C1>>
    ASSERT_THROW(reg.select("a(b1=b([c1,c1]),p1=1,l1=[])"), RegistryError);

    // OK
    using A_B_C1C2 = A<B<C1, C2>>;
    auto algo = reg.select("a(b1=b([c1,c2]),p1=1,l1=[])");
    ASSERT_EQ(
        A_B_C1C2::meta().signature()->str(),
        algo->config().signature()->str());
}

TEST_F(registry, select_type) {
    // create a registry for type A
    RegistryOf<Algorithm> reg(a_type());
    reg.register_algorithm<A<B<>>>();
    reg.register_algorithm<A<B<C1, C2>>>();
    reg.register_algorithm<A<B<C2, C1>>>();

    using A_B_C1C1 = A<B<C1, C1>>;

    // no defaults for p1 and l1
    ASSERT_THROW(reg.select<A_B_C1C1>(), ConfigError);

    // OK (implicit temporary registration)
    auto algo = reg.select<A_B_C1C1>("p1=1,l1=[]");
    ASSERT_EQ(
        A_B_C1C1::meta().signature()->str(),
        algo->config().signature()->str());
}
