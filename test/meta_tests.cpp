#include <glog/logging.h>
#include <gtest/gtest.h>

#include <sstream>

#include <tudocomp/meta/Registry.hpp>
#include <tudocomp/Algorithm.hpp>

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

TEST(typedesc, properties) {
    TypeDesc invalid;
    ASSERT_FALSE(invalid.valid());
    ASSERT_TRUE(TypeDesc("a_name").valid());

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

TEST(algorithm_lib, insert_find) {
    // declare some types
    TypeDesc a_type("A");
    TypeDesc b_type("B", a_type); // B inherits from A
    TypeDesc c_type("C", a_type); // C inherits from A
    TypeDesc x_type("X");

    // construct declarations
    auto a = std::make_shared<AlgorithmDecl>("a", a_type);
    auto b = std::make_shared<AlgorithmDecl>("b", b_type);
    auto c = std::make_shared<AlgorithmDecl>("c", c_type);
    auto x = std::make_shared<AlgorithmDecl>("x", x_type);

    // a declaration named "a" but of type X
    auto a_x = std::make_shared<AlgorithmDecl>("a", x_type);
    // a declaration named "a" but of type B
    auto a_b = std::make_shared<AlgorithmDecl>("a", b_type);

    // construct library
    AlgorithmLib lib;
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

/*
    Test cases to cover:
    - AlgorithmLib
        - Merge
    - AlgorithmConfig
        - Value conversions
        - Decl defaults
        - Error for lack of defaults
        - Sub configs / nesting
    - Meta
        - Value options
        - Bindings (single and packs)
        - Signature
        - Unbounded strategies
        - Type issues
        - Default config
    - Registry
        - Registration (with minor differences)
        - Selection from AST with and w/o options
        - Selection using template type with and w/o options
*/
