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
#include <tudocomp/io.hpp>
#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp/version.hpp>

#include <tudocomp/util/ASCIITable.hpp>

#include <tudocomp_driver/Options.hpp>
#include <tudocomp_driver/Registry.hpp>

#include <tudocomp_stat/Json.hpp>
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
    using namespace tdc_algorithms;

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

    try {
        // load registry
        const Registry<Compressor>& compressor_registry = COMPRESSOR_REGISTRY;
        const Registry<Generator>& generator_registry = GENERATOR_REGISTRY;

        if (options.list) {
            auto lib = compressor_registry.library() +
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
                    if(decl->type().super()) {
                        std::cout << std::endl;
                        std::cout << "Type inheritance: ";
                        {
                            bool first = true;
                            const TypeDesc* type = &decl->type();
                            while(type) {
                                if(!first) std::cout << " -> ";
                                    else first = false;

                                std::cout << type->name();
                                type = type->super();
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
        Registry<Generator>::Selection generator;

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

        Registry<Compressor>::Selection compressor;
        if (!options.algorithm.empty()) {
            compressor = compressor_registry.select(options.algorithm);
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

            Output out;
            if (options.stdout) { // output to stdout
                out = Output(std::cout);
            } else { // output to file
                out = Output(io::Path(ofile), true);
            }

            // do the due (or if you like sugar, the Dew is fine too)
            if (do_compress && compressor) {
                if (!options.raw) {
                    auto id_string = compressor->env().str();
                    CHECK(id_string.find('%') == std::string::npos);

                    auto o_stream = out.as_stream();
                    o_stream << id_string << '%';
                }

                auto is = compressor.decl()->input_restrictions();
                if (is.has_restrictions()) {
                    inp = Input(inp, is);
                }

                //TODO: split?
                //selection.algorithm_env()->restart_stats("Compress");
                setup_time = clk::now();
                compressor->compress(inp, out);
                comp_time = clk::now();
            } else if(options.decompress) {
                // 3 cases
                // --decompress                   : read and use header
                // --decompress --algorithm       : read but ignore header
                // --decompress --raw --algorithm : no header

                std::string algorithm_header;

                if (!options.raw) {
                    {
                        // read header
                        auto i_stream = inp.as_stream();

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
                    }
                    // Slice off the header
                    inp = Input(inp, algorithm_header.size() + 1);
                }

                if (!options.raw && compressor) {
                    DLOG(INFO) << "Ignoring header " << algorithm_header
                        << " and using manually given "
                        << compressor->env().str();
                } else if (!options.raw) {
                    DLOG(INFO) << "Using header id string " << algorithm_header;

                    auto id_string = std::move(algorithm_header);
                    compressor = compressor_registry.select(id_string);
                } else {
                    DLOG(INFO) << "Using manually given "
                               << compressor->env().str();
                }

                auto is = compressor.decl()->input_restrictions();
                if (is.has_restrictions()) {
                    out = Output(out, is);
                }

                //TODO: split?
                //selection.algorithm_env()->restart_stats("Decompress");
                setup_time = clk::now();
                compressor->decompress(inp, out);
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

            json::Object meta;
            meta.set("title", options.stats_title);
            meta.set("startTime",
                std::chrono::duration_cast<std::chrono::seconds>(
                    start_time.time_since_epoch()).count());

            meta.set("config", compressor ? compressor->env().str() : "<none>");
            meta.set("input", options.stdin ? "<stdin>" :
                              (generator ? options.generator : file));
            meta.set("inputSize", in_size);
            meta.set("output", options.stdout ? "<stdin>" : ofile);
            meta.set("outputSize", out_size);
            meta.set("rate", (in_size == 0) ? 0.0 :
                double(out_size) / double(in_size));

            json::Object stats;
            stats.set("meta", meta);
            stats.set("data", algo_stats);

            if (options.stat_file == "") {
                stats.str(std::cout);
                std::cout << std::endl;
            } else {
                std::ofstream stat_file;
                stat_file.open(options.stat_file);

                stats.str(stat_file);
                stat_file << std::endl;

                stat_file.close();
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
