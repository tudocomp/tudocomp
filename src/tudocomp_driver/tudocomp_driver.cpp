#include <chrono>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <glog/logging.h>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp/version.hpp>

#include <tudocomp_driver/Options.hpp>
#include <tudocomp_driver/Registry.hpp>

#include <tudocomp_stat/Json.hpp>
#include <tudocomp_stat/StatPhase.hpp>

namespace tdc_driver {

using namespace tdc;
using namespace tdc_algorithms;

const std::string COMPRESSED_FILE_ENDING = "tdc";

static inline bool ternary_xor(bool a, bool b, bool c) {
    return (a ^ b ^ c) && !(a && b && c);
}

static void exit(std::string msg) {
    // TODO: Replace with more specific logic-error-exception
    throw std::runtime_error(msg);
}

static bool file_exists(const std::string& filename) {
    std::ifstream ifile(filename);
    return bool(ifile);
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

    // init logging
    google::InitGoogleLogging(cmd);

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

    try {
        // load registry
        const Registry<Compressor>& compressor_registry = COMPRESSOR_REGISTRY;
        const Registry<Generator>& generator_registry = GENERATOR_REGISTRY;

        if (options.list) {
            std::cout << "This build supports the following algorithms:\n";
            std::cout << std::endl;
            std::cout << compressor_registry.generate_doc_string();
            return 0;
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
        std::unique_ptr<Generator> generator;

        if(!options.stdin) {
            if(!options.generator.empty()) {
                generator = generator_registry.select(options.generator);
            } else if(!options.remaining.empty()) {
                // file
                file = options.remaining[0];
                if(!file_exists(file)) {
                    std::cerr << "input file not found: " << file << std::endl;
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
                return bad_usage(cmd, "missing output file or standard output");
            }

            if(file_exists(ofile) && !options.force) {
                std::cerr << "output file already exists: " << ofile << std::endl;
                return 1;
            }
        }

        // select compressor
        class Selection {
            std::string m_id_string;
            std::unique_ptr<Compressor> m_compressor;
            bool m_needs_sentinel_terminator;
            std::shared_ptr<EnvRoot> m_algorithm_env;
        public:
            Selection():
                m_id_string(),
                m_compressor(),
                m_needs_sentinel_terminator(false),
                m_algorithm_env() {}
            Selection(std::string&& id_string,
                      std::unique_ptr<Compressor>&& compressor,
                      bool needs_sentinel_terminator,
                      std::shared_ptr<EnvRoot>&& algorithm_env):
                m_id_string(std::move(id_string)),
                m_compressor(std::move(compressor)),
                m_needs_sentinel_terminator(needs_sentinel_terminator),
                m_algorithm_env(std::move(algorithm_env)) {}
            const std::string& id_string() const {
                return m_id_string;
            }
            Compressor& compressor() {
                return *m_compressor;
            }
            bool needs_sentinel_terminator() const {
                return m_needs_sentinel_terminator;
            }
            const std::shared_ptr<EnvRoot>& algorithm_env() const {
                return m_algorithm_env;
            }
            operator bool() const {
                return bool(m_compressor);
            }
        };
        Selection selection;

        if (!options.algorithm.empty()) {
            auto id_string = options.algorithm;

            auto av = compressor_registry.parse_algorithm_id(id_string);
            auto needs_sentinel_terminator = av.needs_sentinel_terminator();
            auto compressor = compressor_registry.select_algorithm(av);
            auto algorithm_env = compressor->env().root();

            selection = Selection {
                std::move(id_string),
                std::move(compressor),
                needs_sentinel_terminator,
                std::move(algorithm_env),
            };
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
                inp = Input(Input::Path{file});
                in_size = inp.size();
            }

            Output out;
            if (options.stdout) { // output to stdout
                out = Output(std::cout);
            } else { // output to file
                out = Output(ofile, true);
            }

            // do the due (or if you like sugar, the Dew is fine too)
            if (do_compress && selection) {
                if (!options.raw) {
                    CHECK(selection.id_string().find('%') == std::string::npos);

                    auto o_stream = out.as_stream();
                    o_stream << selection.id_string() << '%';
                }

                if (selection.needs_sentinel_terminator()) {
                    inp.escape_and_terminate();
                }

                //TODO: split?
                //selection.algorithm_env()->restart_stats("Compress");
                setup_time = clk::now();
                selection.compressor().compress(inp, out);
                comp_time = clk::now();
            } else if(options.decompress) {
                // 3 cases
                // --decompress                   : read and use header
                // --decompress --algorithm       : read but ignore header
                // --decompress --raw --algorithm : no header

                std::string algorithm_header;

                if (!options.raw) {
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

                if (!options.raw && !selection.id_string().empty()) {
                    DLOG(INFO) << "Ignoring header " << algorithm_header
                        << " and using manually given " << selection.id_string();
                } else if (!options.raw) {
                    DLOG(INFO) << "Using header id string " << algorithm_header;

                    auto id_string = std::move(algorithm_header);
                    auto av = compressor_registry.parse_algorithm_id(id_string);
                    auto compressor = compressor_registry.select_algorithm(av);
                    auto needs_sentinel_terminator = av.needs_sentinel_terminator();
                    auto algorithm_env = compressor->env().root();

                    selection = Selection {
                        std::move(id_string),
                        std::move(compressor),
                        needs_sentinel_terminator,
                        std::move(algorithm_env),
                    };
                } else {
                    DLOG(INFO) << "Using manually given " << selection.id_string();
                }

                if (selection.needs_sentinel_terminator()) {
                    out.unescape_and_trim();
                }

                //TODO: split?
                //selection.algorithm_env()->restart_stats("Decompress");
                setup_time = clk::now();
                selection.compressor().decompress(inp, out);
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

            meta.set("config", selection ? selection.id_string() : "<none>");
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

            stats.str(std::cout);
            std::cout << std::endl;
        }
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
