#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/compressors/ChainCompressor.hpp>

#include <tudocomp/io.hpp>
#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp/version.hpp>

#include <tudocomp/util/ASCIITable.hpp>

#include <tudocomp_driver/Options.hpp>
#include <tudocomp_driver/Registry.hpp>

#include <tudocomp_stat/json.hpp>
#include <tudocomp_stat/StatPhase.hpp>

#include <glog/logging.h>

namespace tdc_driver {

using namespace tdc;
using namespace tdc_algorithms;
using tdc::io::file_exists;

const std::string COMPRESSED_FILE_ENDING = "tdc";

static inline bool ternary_xor(bool a, bool b, bool c) {
    return (a ^ b ^ c) && !(a && b && c);
}

static void exit(std::string msg) {
    // TODO: Replace with more specific logic-error-exception
    throw std::runtime_error(msg);
}

static int bad_usage(const char* cmd, const std::string& message) {
    using namespace std;
    cerr << cmd << ": " << message << endl;
    cerr << "Try '" << cmd << " --help' for more information." << endl;
    return 2;
}

} // namespace tdc_driver

#include <iomanip>

int main(int argc, char** argv) {
    using namespace tdc_driver;

    // init registry
    tdc_algorithms::register_algorithms();

    const char* cmd = argv[0];

	FLAGS_logtostderr = 1;


    // no options
    if(argc == 1) {
        return bad_usage("missing options", cmd);
    }

    // parse command line options
    const Options options(argc, argv);

    // don't do anything when any unknown options have been passed
    if(options.unknown_options) {
        return bad_usage("", cmd);
    }

    // print usage when help is requested
    if(options.help) {
        Options::print_usage(cmd, std::cout);
        return 0;
    }

    if(options.version) {
        std::cout << tdc::VERSION << "\n";
        return 0;
    }

    // init logging
    google::InitGoogleLogging(cmd);

    // register chain compressor syntax in ast parser
    meta::ast::Parser::register_preprocessor<ChainSyntaxPreprocessor>();

    try {
        // load registries
        const auto& compressor_registry = Registry::of<Compressor>();
        const auto& decompressor_registry = Registry::of<Decompressor>();
        const auto& generator_registry = Registry::of<Generator>();

        if (options.list) {
            auto lib = compressor_registry.library() +
                       decompressor_registry.library() +
                       generator_registry.library();

            auto print_type_table = [&](const std::string& type){
                auto all = lib.type_entries(type);
                std::sort(all.begin(), all.end(),
                    [](std::shared_ptr<const meta::Decl> a,
                       std::shared_ptr<const meta::Decl> b) {

                    return (a->name() <= b->name());
                });

                ASCIITable table(80);
                size_t longest = 2;
                for(auto decl : all) {
                    longest = std::max(longest, decl->name().length());
                }

                table.add_column("ID", longest);
                table.add_column("Description", 0, true);
                for(auto decl : all) {
                    table.add_row({ decl->name(), decl->desc() });
                }
                table.print(std::cout, false, true, false);
            };

            if(options.list_algorithm.empty()) {
                // list all algorithms
                std::cout << "The following is a listing of algorithms "
                          << "supported by this build." << std::endl;
                std::cout << "Use " << cmd << " --list=<ID> to get detail "
                          << "information on the algorithm with the given ID."
                          << std::endl << std::endl;

                std::cout << "Compressors (for the -a option):";
                std::cout << std::endl;
                print_type_table(Compressor::type_desc().name());
                std::cout << std::endl;
                std::cout << "Decompressors (for -d -a):";
                std::cout << std::endl;
                print_type_table(Decompressor::type_desc().name());
                std::cout << std::endl;
                std::cout << "String generators (for the -g option):";
                std::cout << std::endl;
                print_type_table(Generator::type_desc().name());
                std::cout << std::endl;
                return 0;
            } else {
                std::shared_ptr<const meta::Decl> decl;

                // no type constraint given
                const size_t sep = options.list_algorithm.find(':');
                if(sep == std::string::npos) {
                    auto found = lib.name_entries(options.list_algorithm);
                    if(found.size() == 0) {
                        std::cout << "There is no algorithm named '"
                                  << options.list_algorithm << "'."
                                  << std::endl;
                    } else if(found.size() > 1) {
                        std::cout << "There is more than algorithm named '"
                                  << options.list_algorithm << "'. "
                                  << "Please specify using one of the "
                                  << "following canonical IDs:"
                                  << std::endl;

                        for(auto decl : found) {
                            std::cout << decl->name() << ":"
                                      << decl->type().name()
                                      << std::endl;
                        }
                    } else {
                        decl = found[0];
                    }
                } else {
                    auto name = options.list_algorithm.substr(0, sep);
                    auto type = options.list_algorithm.substr(sep + 1);
                    decl = lib.find(name, type, false);
                    if(!decl) {
                        std::cout << "There is no algorithm '"
                                  << options.list_algorithm << "'."
                                  << std::endl;
                    }
                }

                if(decl) {
                    std::cout << decl->name();
                    if(!decl->desc().empty()) {
                        std::cout << " -- " << decl->desc();
                    }
                    std::cout << std::endl << std::endl;
                    std::cout << "Declaration:" << std::endl;
                    std::cout << decl->str() << std::endl;
                    if(decl->type().super().valid()) {
                        std::cout << std::endl;
                        std::cout << "Type inheritance: ";
                        {
                            bool first = true;
                            TypeDesc type = decl->type();
                            while(type.valid()) {
                                if(!first) std::cout << " -> ";
                                    else first = false;

                                std::cout << type.name();
                                type = type.super();
                            }
                        }
                    }

                    if(decl->params().size() > 0) {
                        std::set<std::string> param_types;
                        {
                            size_t num_defaults = 0;
                            std::stringstream defaults;
                            defaults << decl->name() << "(";

                            ASCIITable table(80, " - ");
                            size_t longest_pname = 0;
                            for(auto& p : decl->params()) {
                                longest_pname = std::max(
                                    longest_pname, p.name().length());
                            }
                            table.add_column("", longest_pname);
                            table.add_column("", 0, true);

                            std::cout << std::endl;
                            std::cout << "Parameters:" << std::endl;
                            for(auto& p : decl->params()) {
                                std::stringstream desc;

                                if(p.is_primitive()) {
                                    desc << "primitive value";
                                } else {
                                    desc << "algorithm of type '"
                                         << p.type().name() << "'";

                                    param_types.emplace(p.type().name());
                                }

                                //TODO description
                                if(!p.desc().empty()) {
                                    desc << "\n" << p.desc();
                                }

                                if(p.default_value()) {
                                    if(num_defaults++ > 0) defaults << ", ";
                                    defaults << p.name() << "="
                                             << p.default_value()->str();
                                }

                                table.add_row({ p.name(), desc.str() });
                            }

                            table.print(std::cout, false, false, false);

                            defaults << ")";
                            std::cout << std::endl;
                            std::cout << "Default configuration:" << std::endl;
                            std::cout << defaults.str() << std::endl;
                        }

                        if(param_types.size() > 0) {
                            for(auto& type : param_types) {
                                std::cout << std::endl;
                                std::cout << std::endl;
                                std::cout << "Available algorithms for "
                                << "parameters of type '" << type << "':"
                                << std::endl;
                                print_type_table(type);
                            }
                        }
                    }
                    std::cout << std::endl;
                }
                return 0;
            }
        }

        // check mode
        const bool do_compress = !options.decompress;

        if(options.generator.empty() && options.algorithm.empty()) {
            // no compressor or generator given
            // this is allowed only with non-raw decompression

            if(do_compress) {
                return bad_usage(cmd, "missing compression algorithm.");
            }

            if(options.decompress && options.raw) {
                return bad_usage(cmd, "missing algorithm for raw decompression");
            }
        } else if(!options.generator.empty()) {
            if(options.decompress) {
                return bad_usage(cmd, "trying to decompress generated string");
            }
        }

        // select input
        if(!options.stdin && options.generator.empty() && options.remaining.empty()) {
            return bad_usage(cmd, "missing generator, input file or standard input");
        }

        if(!ternary_xor(
            options.stdin, !options.generator.empty(), !options.remaining.empty())) {

            return bad_usage(cmd, "trying to use multiple inputs");
        }

        std::string file;
        RegistryOf<Generator>::Selection generator;

        if(!options.stdin) {
            if(!options.generator.empty()) {
                generator = generator_registry.select(options.generator);
            } else if(!options.remaining.empty()) {
                // file
                file = options.remaining[0];
                if(!file_exists(file)) {
                    std::cerr << "input path not found or is not a file: " << file << std::endl;
                    return 1;
                }
            }
        }

        // determined later
        std::string generated;
        size_t in_size;

        // select output
        if(!options.output.empty() && options.stdout) {
            return bad_usage(cmd, "trying to use multiple outputs");
        }

        std::string ofile;

        if(!options.stdout) {
            if(!options.output.empty()) {
                ofile = options.output;
            } else if(do_compress && !options.remaining.empty()) {
                ofile = file + "." + COMPRESSED_FILE_ENDING;
            } else {
                return bad_usage(cmd, "either specify a filename (-o filename) or state that the output is standard output (--usestdout)");
            }

            if(file_exists(ofile) && !options.force) {
                std::cerr << "output file already exists: " << ofile << std::endl;
                return 1;
            }
        }

        RegistryOf<Compressor>::Selection compressor;
        if (!options.algorithm.empty() && !options.decompress) {
            compressor = compressor_registry.select(options.algorithm);
        }

        RegistryOf<Decompressor>::Selection decompressor;
        if (!options.algorithm.empty() && options.decompress) {
            decompressor = decompressor_registry.select(options.algorithm);
        }

        // open streams
        using clk = std::chrono::high_resolution_clock;

        clk::time_point start_time = clk::now();
        clk::time_point setup_time;
        clk::time_point comp_time;
        clk::time_point end_time;

        StatPhase root("root");
        {
            Input inp;
            if (options.stdin) { // input from stdin
                inp = Input(std::cin);
                in_size = 0;
            } else if(generator) { // input from generated string
                generated = generator->generate();
                inp = Input(generated);
                in_size = inp.size();
            } else { // input from file
                inp = Input(io::Path{file});
                in_size = inp.size();
            }

            const size_t prefix = options.prefix;
            if(prefix > 0 && (options.stdin || prefix < in_size)) {
                // limit input to prefix
                inp = Input(inp, 0, prefix);
                in_size = std::min(in_size, prefix);
            }

            Output out;
            if (options.stdout) { // output to stdout
                out = Output(std::cout);
            } else { // output to file
                out = Output(io::Path(ofile), true);
            }

            InputRestrictions restrictions(options.escape, options.sentinel);

            // do the due (or if you like sugar, the Dew is fine too)
            if (do_compress && compressor) {
                if (!options.raw) {
                    // write header
                    auto o_stream = out.as_stream();

                    // encode matching decompressor
                    auto dec_id = compressor->decompressor()->config().str();
                    CHECK(dec_id.find('%') == std::string::npos);

                    o_stream << dec_id << '%';
                }

                if(restrictions.has_restrictions()) {
                    inp = Input(inp, restrictions);
                }

                setup_time = clk::now();
                compressor->compress(inp, out);
                comp_time = clk::now();
            } else if(options.decompress) {
                // 3 cases
                // --decompress                   : read and use header
                // --decompress --algorithm       : read but ignore header
                // --decompress --raw --algorithm : no header

                std::string algorithm_header;
                {
                    auto i_stream = inp.as_stream();
                    if (!options.raw)
                    {
                        // read header
                        char c;
                        size_t sanity_size_check = 0;
                        bool err = false;
                        while (i_stream.get(c)) {
                            err = false;
                            if (sanity_size_check > 1023) {
                                err = true;
                                break;
                            } else if (c == '%') {
                                break;
                            } else {
                                algorithm_header.push_back(c);
                            }
                            sanity_size_check++;
                            err = true;
                        }

                        if (err) {
                            exit("Input did not have an algorithm header!");
                        }

                        // Slice off the header
                        inp = Input(inp, algorithm_header.size() + 1);
                    }
                }

                if (!options.raw && decompressor) {
                    DVLOG(1) << "Ignoring header " << algorithm_header
                        << " and using manually given "
                        << decompressor->config().str();
                } else if (!options.raw) {
                    DVLOG(1) << "Using header id string " << algorithm_header;

                    auto dec_id = std::move(algorithm_header);
                    decompressor = decompressor_registry.select(dec_id);
                } else {
                    DVLOG(1) << "Using manually given "
                               << decompressor->config().str();
                }

                if(restrictions.has_restrictions()) {
                    out = Output(out, restrictions);
                }

                setup_time = clk::now();
                decompressor->decompress(inp, out);
                comp_time = clk::now();
            } else {
                setup_time = clk::now();

                // echo input into output
                {
                    auto is = inp.as_stream();
                    auto os = out.as_stream();
                    os << is.rdbuf();
                }

                comp_time = clk::now();
            }
        }

        end_time = clk::now();

        if (options.stats) {
            auto algo_stats = root.to_json();

            auto setup_duration = setup_time - start_time;
            auto comp_duration = comp_time - setup_time;
            auto end_duration = end_time - comp_time;

            size_t out_size = options.stdout ? 0 : io::read_file_size(ofile);

            using json = nlohmann::json;
            json meta;
            meta["title"] = options.stats_title;
            meta["startTime"] =
                std::chrono::duration_cast<std::chrono::seconds>(
                    start_time.time_since_epoch()).count();
            meta["config"] = compressor ? compressor->config().str() : "<none>";
            meta["input"] = options.stdin ? "<stdin>" :
                (generator ? options.generator : file);
            meta["inputSize"] = in_size;
            meta["output"] = options.stdout ? "<stdin>" : ofile;
            meta["outputSize"] = out_size;
            meta["rate"] = (in_size == 0) ? 0.0 :
                double(out_size) / double(in_size);

            json stats = {{"meta", meta}, {"data", algo_stats}};
            if (options.stat_file == "") {
                std::cout << stats << std::endl;
            } else {
                std::ofstream stat_file;
                stat_file.open(options.stat_file);
                stat_file << stats << std::endl;
                stat_file.close();
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
