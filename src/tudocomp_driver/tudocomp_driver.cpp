#include <chrono>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/exception/exception.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/io.h>
#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp_driver/registry.h>

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
DEFINE_bool(stdin, false, "Read from stdin instead of trying to open a file.");
DEFINE_bool(stdout, false, "Output to stdout instead of writing to a file");

namespace tudocomp_driver {

using namespace tudocomp;

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

} // namespace tudocomp_driver

int main(int argc, char** argv)
{
    using namespace tudocomp_driver;

    google::InitGoogleLogging(argv[0]);
    int first_cmd_arg = google::ParseCommandLineFlags(&argc, &argv, true);

    try {
        validate_flag_combination();

        std::map<std::string, const std::string> algorithm_options;

        /*for (auto& os : string_args("--option")) {
            std::vector<std::string> options;
            boost::split(options, os, boost::is_any_of(","));
            for (auto& o : options) {
                std::vector<std::string> key_value;
                boost::split(key_value, o, boost::is_any_of("="));
                CHECK(key_value.size() == 2);
                algorithm_options.emplace(key_value[0], key_value[1]);
            }
        }*/

        /*if (arg_exists("--help")) {
            show_help();
            return 0;
        }*/

        Env algorithm_env(algorithm_options, {});

        // Set up algorithms
        AlgorithmDb root;
        Registry registry {&root};
        register_algos(registry);

        if (FLAGS_list) {
            std::cout << "This build supports the following algorithms:\n";
            std::cout << std::endl;

            for (auto& e: registry.get_sub_algos()) {
                e.print_to(std::cout, 0);
            }

            return 0;
        }

        bool print_stats = FLAGS_stats;
        int alphabet_size = 0;

        bool do_compress = !FLAGS_decompress;

        /////////////////////////////////////////////////////////////////////////
        // Select algorithm

        bool do_raw = FLAGS_raw;

        std::string algorithm_id;
        std::unique_ptr<Compressor> algo;

        if (do_raw || do_compress) {
            algorithm_id = FLAGS_algorithm;
            algo = select_algo_or_exit(registry, algorithm_env, algorithm_id);
        }

        /////////////////////////////////////////////////////////////////////////
        // Select where the input comes from

        std::string file;

        if ((!FLAGS_stdin) && (first_cmd_arg < argc)) {
            file = argv[first_cmd_arg];
            if (!fexists(file)) {
                std::cerr << "input file " << file << " does not exist\n";
                return 1;
            }
        } else if (!FLAGS_stdin) {
            std::cerr << "No input file given\n";
            return 1;
        }

        bool use_stdin = FLAGS_stdin;

        /////////////////////////////////////////////////////////////////////////
        // Select where the output goes to

        std::string ofile;
        bool use_stdout = FLAGS_stdout;
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
                stream_buffer = read_stream_to_stl_byte_container<
                    std::vector<uint8_t>
                >(std::cin);
                inp = Input::from_memory(stream_buffer);
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
                    CHECK(algorithm_id.find('%') == std::string::npos);

                    auto o_stream = out.as_stream();
                    (*o_stream) << algorithm_id << '%';
                }

                setup_time = clk::now();

                algo->compress(inp, out);

                comp_time = clk::now();
            } else {
                if (!do_raw) {
                    auto i_stream = inp.as_stream();
                    std::string algorithm_header;

                    char c;
                    size_t sanity_size_check = 0;
                    bool err = false;
                    while ((*i_stream).get(c)) {
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
                    algorithm_id = std::move(algorithm_header);
                    algo = select_algo_or_exit(registry, algorithm_env, algorithm_id);
                }

                setup_time = clk::now();

                algo->decompress(inp, out);

                comp_time = clk::now();
            }
        }

        end_time = clk::now();

        if (print_stats) {
            auto setup_duration = setup_time - start_time;
            auto comp_duration = comp_time - setup_time;
            auto end_duration = end_time - comp_time;
            std::cout << "---------------\n";
            std::cout << "Config: " << algorithm_id << std::endl;
            std::cout << "---------------\n";
            auto inp_size = 0;
            if (use_stdin) {
                std::cout << "input: <stdin>\n";
                std::cout << "input size: ? B\n";
            } else {
                inp_size = read_file_size(file);
                std::cout << "input: "<<file<<"\n";
                std::cout << "input size: "<<inp_size<<" B\n";
            }
            std::cout << "alphabet size: " << alphabet_size << "\n";

            auto out_size = 0;
            if (use_stdout) {
                std::cout << "output: <stdout>\n";
                std::cout << "output size: ? B\n";
            } else {
                out_size = read_file_size(ofile);
                std::cout << "output: "<<ofile<<"\n";
                std::cout << "output size: "<<out_size<<" B\n";
            }
            std::cout << "---------------\n";
            if (inp_size != 0) {
                std::cout << "compress ratio: "<<(double(out_size)/double(inp_size) * 100)<<"%\n";
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
            std::cout << "Algorithm Known Options:\n";
            for (auto& pair : algorithm_env.get_known_options()) {
                std::cout << "  " << pair << "\n";
            }
            std::cout << "Algorithm Given Options:\n";
            for (auto& pair : algorithm_env.get_options()) {
                std::cout << "  " << pair.first << " = " << pair.second << "\n";
            }

            std::cout << "---------------\n";
            std::cout << "Algorithm Stats:\n";
            for (auto& pair : algorithm_env.get_stats()) {
                std::cout << "  " << pair.first << ": " << pair.second << "\n";
            }

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
