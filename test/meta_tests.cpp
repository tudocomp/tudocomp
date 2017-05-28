#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/util/meta/DeclAlgorithm.hpp>
#include <tudocomp/util/meta/DeclParam.hpp>

#include <tudocomp/util/meta/ASTDef.hpp>
#include <tudocomp/util/meta/ASTParser.hpp>

using namespace tdc::meta;

TEST(Sandbox, example) {
    /*auto lz77 = decl::Algorithm("lz77", "compressor", "LZ77 online compressor.");
    lz77.add_param(decl::Param("window"));
    lz77.add_param(decl::Param("coder", false, false, "coder"));
    //DLOG(INFO) << lz77.str();

    auto binary = decl::Algorithm("binary", "coder", "Binary coder.");
    //DLOG(INFO) << binary.str();

    // registry
    decl::AlgorithmMap map;
    map.emplace("lz77", std::move(lz77));
    map.emplace("binary", std::move(binary));*/

    // parse
    DLOG(INFO) << "parse...";
    auto v = ast::Parser::parse("lz77(coder=binary,window=10,values=[1,2,3,4,5])");
    DLOG(INFO) << v->str();
}

