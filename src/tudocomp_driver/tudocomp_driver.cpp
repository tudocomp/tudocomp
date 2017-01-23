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
#include <tudocomp/util/Json.hpp>

#include <tudocomp_driver/Options.hpp>
#include <tudocomp_driver/Registry.hpp>

namespace tdc_driver {

using namespace tdc;
using namespace tdc_algorithms;

const std::string COMPRESSED_FILE_ENDING = "tdc";

static void exit(std::string msg) {
    // TODO: Replace with more specific logic-error-exception
    throw std::runtime_error(msg);
}

static void validate_flag_combination(const Options& options) {
    auto err = [](std::string s) {
        exit(s);
    };

    if (!options.list) {
        if (!options.decompress && options.algorithm.empty()) {
            err("Need to give an algorithm for compression");
        }

        if (options.decompress && options.raw && options.algorithm.empty()) {
            err("Need to give an algorithm for raw decompression");
        }
    }
}

static bool fexists(std::string filename)
{
  std::ifstream ifile(filename);
  return bool(ifile);
}

static bool check_for_file_already_exist(std::string& ofile,
                                         bool allow_overwrite) {
    // Don't accidentially overwrite files
    if (!allow_overwrite && fexists(ofile)) {
        std::cerr << "Outputfile already exists\n";
        return false;
    }
    return true;
}

} // namespace tdc_driver

#include <iomanip>

int main(int argc, char** argv)
{
    using namespace tdc_driver;
    using namespace tdc_algorithms;

    google::InitGoogleLogging(argv[0]);

    // Parse command line options
    if(argc == 1) {
        // don't do anything when any unknown options have been passed
        std::cerr << "No options given." << std::endl;
        std::cerr <<
            "Try '" << argv[0] << " --help' for more information." << std::endl;

        return 1;
    }

    Options options(argc, argv);

    if(options.unknown_options) {
        // don't do anything when any unknown options have been passed
        std::cerr <<
            "Try '" << argv[0] << " --help' for more information." << std::endl;

        return 1;
    }

    if(options.help) {
        Options::print_usage(std::string(argv[0]), std::cout);
        return 0;
    }

    try {
        validate_flag_combination(options);

        // Set up algorithms
        Registry& registry = REGISTRY;

        if (options.list) {
            std::cout << "This build supports the following algorithms:\n";
            std::cout << std::endl;
            std::cout << registry.generate_doc_string();
            return 0;
        }

        bool print_stats = options.stats;
        bool do_compress = !options.decompress;
        bool do_raw = options.raw;

        /////////////////////////////////////////////////////////////////////////
        // Select algorithm

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
        };
        Selection selection;

        if (!options.algorithm.empty()) {
            auto id_string = options.algorithm;

            auto av = registry.parse_algorithm_id(id_string, "compressor");
            auto needs_sentinel_terminator = av.needs_sentinel_terminator();
            auto compressor = registry.select_compressor_or_exit(av);
            auto algorithm_env = compressor->env().root();

            selection = Selection {
                std::move(id_string),
                std::move(compressor),
                needs_sentinel_terminator,
                std::move(algorithm_env),
            };
        }

        /////////////////////////////////////////////////////////////////////////
        // Select where the input comes from

        std::string file;

        if(!options.generator.empty()) {
            auto av = registry.parse_algorithm_id(options.generator, "generator");
            auto generator = registry.select_generator_or_exit(av);
            //auto algorithm_env = generator->env().root();

            std::cout << generator->generate() << std::endl;
            return 0;
        } else if(!options.stdin) {
            if (!options.remaining.empty()) {
                file = options.remaining[0];
                if (!fexists(file)) {
                    std::cerr << "input file " << file << " does not exist\n";
                    return 1;
                }
            } else {
                std::cerr << "No input file given\n";
                return 1;
            }
        }

        bool use_stdin = options.stdin;

        /////////////////////////////////////////////////////////////////////////
        // Select where the output goes to

        std::string ofile;
        bool use_stdout = options.stdout;
        bool use_explict_output = !options.output.empty();

        if (use_explict_output) {
            // Output to manually specifed file
            ofile = options.output;
        } else if (!use_stdin && do_compress) {
            // Output to a automatically determined file
            ofile = file + "." + COMPRESSED_FILE_ENDING;
        } else if (!use_stdin && !do_compress && !use_stdout) {
            std::cerr << "Need to specify a output filename\n";
            return 1;
        }

        /////////////////////////////////////////////////////////////////////////
        // open streams

        using clk = std::chrono::high_resolution_clock;

        clk::time_point start_time = clk::now();
        clk::time_point setup_time;
        clk::time_point comp_time;
        clk::time_point end_time;

        {
            Input inp;

            if (use_stdin) {
                // Input from stdin
                inp = Input(std::cin);
            } else {
                // Input from specified file
                inp = Input::from_path(file);
            }

            Output out;
            if (use_stdout) {
                // Output to stdout
                out = Output::from_stream(std::cout);
            } else {
                // Output to specified file
                bool force = options.force;
                if (!check_for_file_already_exist(ofile, force)) {
                    return 1;
                } else {
                    out = Output::from_path(ofile, true);
                }
            }

            /////////////////////////////////////////////////////////////////////////
            // call into actual library

            if (do_compress) {
                if (!do_raw) {
                    CHECK(selection.id_string().find('%') == std::string::npos);

                    auto o_stream = out.as_stream();
                    o_stream << selection.id_string() << '%';
                }

                if (selection.needs_sentinel_terminator()) {
                    inp.escape_and_terminate();
                }

                selection.algorithm_env()->restart_stats("Compress");
                setup_time = clk::now();
                selection.compressor().compress(inp, out);
                comp_time = clk::now();
            } else {
                // 3 cases
                // --decompress                   : read and use header
                // --decompress --algorithm       : read but ignore header
                // --decompress --raw --algorithm : no header

                std::string algorithm_header;

                if (!do_raw) {
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

                if (!do_raw && !selection.id_string().empty()) {
                    DLOG(INFO) << "Ignoring header " << algorithm_header
                        << " and using manually given " << selection.id_string();
                } else if (!do_raw) {
                    DLOG(INFO) << "Using header id string " << algorithm_header;

                    auto id_string = std::move(algorithm_header);
                    auto av = registry.parse_algorithm_id(id_string, "compressor");
                    auto compressor = registry.select_compressor_or_exit(av);
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

                selection.algorithm_env()->restart_stats("Decompress");
                setup_time = clk::now();
                selection.compressor().decompress(inp, out);
                comp_time = clk::now();
            }
        }

        auto algo_stats = selection.algorithm_env()->finish_stats();
        end_time = clk::now();

        if (print_stats) {
            auto setup_duration = setup_time - start_time;
            auto comp_duration = comp_time - setup_time;
            auto end_duration = end_time - comp_time;

            size_t in_size  = use_stdin  ? 0 : io::read_file_size(file);
            size_t out_size = use_stdout ? 0 : io::read_file_size(ofile);

            json::Object meta;
            meta.set("title", options.stats_title);
            meta.set("startTime",
                std::chrono::duration_cast<std::chrono::seconds>(
                    start_time.time_since_epoch()).count());

            meta.set("config", selection.id_string());
            meta.set("input", use_stdin ? "<stdin>" : file);
            meta.set("inputSize", in_size);
            meta.set("output", use_stdout ? "<stdin>" : ofile);
            meta.set("outputSize", out_size);
            meta.set("rate", (use_stdin || use_stdout) ? 0.0 :
                double(out_size) / double(in_size));

            json::Object stats;
            stats.set("meta", meta);
            stats.set("data", algo_stats.to_json());

            stats.str(std::cout);
            std::cout << std::endl;
        }
    } catch (std::exception& e) {
        std::cout << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
