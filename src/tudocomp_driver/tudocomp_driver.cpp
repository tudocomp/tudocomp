#include <chrono>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp_driver/Registry.hpp>

/*namespace validate {
  static bool algorithm(const char* flagname, int32 value) {
    if (value > 0 && value < 32768)   // value is ok
      return true;
    printf("Invalid value for --%s: %d\n", flagname, (int)value);
    return false;
  }
  static const bool algorithm_dummy = RegisterFlagValidator(&FLAGS_algorithm, &algorithm);
}*/

DEFINE_string(algorithm, "", "Algorithm to use for (de)compression. See --list for a complete list of them.");
DEFINE_bool(decompress, false, "Decompress input instead of compressing it.");
DEFINE_string(output, "", "Choose output filename instead the the default generated one of <input>.tdc or stdout.");
DEFINE_bool(stats, false, "Print statistics to stdout.");
DEFINE_bool(force, false, "Overwrite output even if it exists.");
DEFINE_bool(list, false, "List all compression algorithms supported by this tool.");
DEFINE_bool(raw, false, "Do not emit an header into the output file when compressing.");
DEFINE_bool(usestdin, false, "Read from stdin instead of trying to open a file.");
DEFINE_bool(usestdout, false, "Output to stdout instead of writing to a file");

namespace tdc_driver {

using namespace tdc;
using namespace tdc_algorithms;

const std::string COMPRESSED_FILE_ENDING = "tdc";

static void exit(std::string msg) {
    // TODO: Replace with more specific logic-error-exception
    throw std::runtime_error(msg);
}

static void validate_flag_combination() {
    auto err = [](std::string s) {
        exit(s);
    };

    if (!FLAGS_list) {
        if (!FLAGS_decompress && (std::string(FLAGS_algorithm) == "")) {
            err("Need to give an algorithm for compression");
        }
        if (FLAGS_decompress && FLAGS_raw && (std::string(FLAGS_algorithm) == "")) {
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
    google::SetUsageMessage("This tool compresses and decompresses files");
    if(argc == 1 || strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0 || strcmp(argv[1],"-help") == 0) {
//        google::ShowUsageWithFlagsRestrict(argv[0], __FILE__); //shortcut
        std::vector<google::CommandLineFlagInfo> info;
        google::GetAllFlags(&info);

        std::cout << argv[0] << " [options] {file to compress/decompress}" << std::endl;
        std::cout << "You need to provide at least an algorithm and a source (a file) to compress/decompress." << std::endl << std::endl;
        std::cout
            << std::setw(20) << std::setiosflags(std::ios::left) << "Parameter"
            << std::setw(10) << "Type"
            << std::setw(20) << "Default"
            << "Description" << std::endl;
        std::cout << std::endl;
         for(auto it = info.cbegin(); it != info.cend(); ++it) {
             if(it->filename != __FILE__) continue;
                 std::cout
                    << std::setw(20) << std::setiosflags(std::ios::left)<< (std::string("--")+ it->name)
                    << std::setw(10) << it->type
                    << std::setw(20) << (std::string("(") + it->default_value + ")")
                    << it->description << std::endl;
         }
        return 0;
    }

    int first_cmd_arg = google::ParseCommandLineFlags(&argc, &argv, true);

    try {
        validate_flag_combination();

        // Set up algorithms
        Registry& registry = REGISTRY;

        if (FLAGS_list) {
            std::cout << "This build supports the following algorithms:\n";
            std::cout << std::endl;
            std::cout << registry.generate_doc_string();

            return 0;
        }

        bool print_stats = FLAGS_stats;
        int alphabet_size = 0;

        bool do_compress = !FLAGS_decompress;
        bool do_raw = FLAGS_raw;

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

        if (!FLAGS_algorithm.empty()) {
            auto id_string = FLAGS_algorithm;

            auto av = registry.parse_algorithm_id(id_string);
            auto needs_sentinel_terminator = av.needs_sentinel_terminator();
            auto compressor = registry.select_algorithm_or_exit(av);
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

        if ((!FLAGS_usestdin) && (first_cmd_arg < argc)) {
            file = argv[first_cmd_arg];
            if (!fexists(file)) {
                std::cerr << "input file " << file << " does not exist\n";
                return 1;
            }
        } else if (!FLAGS_usestdin) {
            std::cerr << "No input file given\n";
            return 1;
        }

        bool use_stdin = FLAGS_usestdin;

        /////////////////////////////////////////////////////////////////////////
        // Select where the output goes to

        std::string ofile;
        bool use_stdout = FLAGS_usestdout;
        bool use_explict_output = std::string(FLAGS_output) != "" ;

        if (use_explict_output) {
            // Output to manually specifed file
            ofile = FLAGS_output;
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
            std::vector<uint8_t> stream_buffer;
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
                bool force = FLAGS_force;
                if (!check_for_file_already_exist(ofile, force)) {
                    return 1;
                } else {
                    out = Output::from_path(ofile, true);
                }
            }

            /////////////////////////////////////////////////////////////////////////
            // call into actual library

            if (do_compress) {
                if (print_stats) {
                    alphabet_size = 0; // count_alphabet_size(inp_vec);
                }

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
                    auto av = registry.parse_algorithm_id(id_string);
                    auto compressor = registry.select_algorithm_or_exit(av);
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
            std::cout << "---------------\n";
            std::cout << "Config: " << selection.id_string() << std::endl;
            std::cout << "---------------\n";
            auto inp_size = 0;
            if (use_stdin) {
                std::cout << "input: <stdin>\n";
                std::cout << "input size: ? B\n";
            } else {
                inp_size = io::read_file_size(file);
                std::cout << "input: "<<file<<"\n";
                std::cout << "input size: "<<inp_size<<" B\n";
            }
            std::cout << "alphabet size: " << alphabet_size << "\n";

            auto out_size = 0;
            if (use_stdout) {
                std::cout << "output: <stdout>\n";
                std::cout << "output size: ? B\n";
            } else {
                out_size = io::read_file_size(ofile);
                std::cout << "output: "<<ofile<<"\n";
                std::cout << "output size: "<<out_size<<" B\n";
            }
            std::cout << "---------------\n";
            if (inp_size != 0) {
                std::cout << "compress rate: "<<(double(out_size)/double(inp_size) * 100)<<"%\n";
            }

            auto print_time = [] (std::string s, decltype(setup_duration)& t) {
                std::chrono::seconds sec(1);
                std::chrono::milliseconds milsec(1);
                std::chrono::microseconds micsec(1);
                using dur = std::chrono::duration<float>;
                using mildur = std::chrono::duration<float, std::milli>;
                using micdur = std::chrono::duration<float, std::micro>;

                if (t < milsec) {
                    auto ct = std::chrono::duration_cast<micdur>(t).count();
                    std::cout << s <<" time: "<< ct << " µs\n";
                } else if (t < sec) {
                    auto ct = std::chrono::duration_cast<mildur>(t).count();
                    std::cout << s <<" time: "<< ct << " ms\n";
                } else {
                    auto ct = std::chrono::duration_cast<dur>(t).count();
                    std::cout << s <<" time: "<< ct << " s\n";
                }
            };

            std::cout << "---------------\n";
            std::cout << "Algorithm Stats:\n";
            algo_stats.to_json(std::cout); //TODO text output
            std::cout << "\n";
            std::cout << "---------------\n";
            print_time("startup", setup_duration);
            print_time("compression", comp_duration);
            print_time("teardown", end_duration);
            std::cout << "---------------\n";
        }
    } catch (std::exception& e) {
        std::cout << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
