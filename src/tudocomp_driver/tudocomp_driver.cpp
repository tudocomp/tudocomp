#include <chrono>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/exception/exception.hpp>
#include <boost/program_options/parsers.hpp>

#include <glog/logging.h>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/io.h>
#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp_driver/registry.h>

namespace tudocomp_driver {

using namespace tudocomp;

namespace po = boost::program_options;

const std::string COMPRESSED_FILE_ENDING = "tdc";

static const std::string USAGE =
R"(TuDo Comp.

Usage:
    tudocomp [options] [-k]  -a <alg>  [-o <output>] [--] ( <input> | - )
    tudocomp [options]  -d  [-a <alg>] [-o <output>] [--] ( <input> | - )
    tudocomp --list
    tudocomp --help

Options:
    -h --help               Show this screen.
    -a --algorithm <alg>    Use algorithm <alg> for (de)compression.
                            <alg> can be a dot-separated chain of
                            sub-algorithms. See --list for a complete list
                            of them.
                            Example: -a lz77rule.esa.esa_code0
    -k --compress           Compress input instead of compressing it.
    -d --decompress         Decompress input instead of compressing it.
    -o --output <output>    Choose output filename instead the the default of
                            <input>.<compressor name>.<encoder name>.tdc
                            or stdout if reading from stdin.
    -s --stats              Print statistics to stdout.
    -f --force              Overwrite output even if it exists.
    -l --list               List all Compression algorithms supported
                            by this tool.
                            Algorithms may consist out of sub-algorithms,
                            which will be displayed in a hierarchical fashion.
    -O --option <option>    An additional option of the form key=value.
)";

static void exit(std::string msg) {
    throw std::runtime_error(msg);
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

int main(int argc, const char** argv)
{
    using namespace tudocomp_driver;

    google::InitGoogleLogging(argv[0]);

    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "")
        ("algorithm,a", po::value<std::string>(), "")
        ("compress,k", "")
        ("decompress,d", "")
        ("output,o", po::value<std::string>(), "")
        ("stats,s", "")
        ("force,f", "")
        ("list,l", "")
        ("option,O", po::value<std::vector<std::string>>(), "")
        ("input", po::value<std::string>(), "")
    ;
    po::positional_options_description pos_desc;
    pos_desc.add("input", 1);

    po::variables_map args;

    auto show_help = []() {
        std::cout << USAGE << std::endl;
    };

    try {
        po::store(
            boost::program_options::parse_environment(desc, "TUDOCOMP_"),
            args);

        po::store(po::command_line_parser(argc, argv)
                .positional(pos_desc)
                .options(desc)
                .run(),
            args);
        po::notify(args);

        auto arg2boost = [](std::string s) -> std::string {
            if (s.size() > 1 && s[0] == '-' && s[1] == '-') {
                return s.substr(2);
            } else if (s.size() > 0 && s[0] == '-') {
                return s;
            } else if (s.size() > 0 && s[0] == '<') {
                return s.substr(1, s.size() - 2);
            } else {
                std::cout << "error";
                return "";
            }
        };

        auto value_arg_exists = [&](std::string s) {
            return args.count(arg2boost(s)) > 0;
        };
        auto arg_exists = [&](std::string s) {
            return args.count(arg2boost(s)) > 0;
        };
        auto string_arg = [&](std::string s) {
            if (!value_arg_exists(s)) {
                exit("Argument not given: '" + s + "'");
            }
            return args[arg2boost(s)].as<std::string>();
        };
        auto string_args = [&](std::string s) {
            if (value_arg_exists(s)) {
                return args[arg2boost(s)].as<std::vector<std::string>>();
            } else {
                return std::vector<std::string>();
            }
        };

        std::map<std::string, const std::string> algorithm_options;

        for (auto& os : string_args("--option")) {
            std::vector<std::string> options;
            boost::split(options, os, boost::is_any_of(","));
            for (auto& o : options) {
                std::vector<std::string> key_value;
                boost::split(key_value, o, boost::is_any_of("="));
                CHECK(key_value.size() == 2);
                algorithm_options.emplace(key_value[0], key_value[1]);
            }
        }

        if (arg_exists("--help")) {
            show_help();
            return 0;
        }

        Env algorithm_env(algorithm_options, {});

        // Set up algorithms
        AlgorithmDb root;
        Registry registry {&root};
        register_algos(registry);

        if (arg_exists("--list")) {
            std::cout << "This build supports the following algorithms:\n";
            std::cout << std::endl;

            for (auto& e: registry.get_sub_algos()) {
                e.print_to(std::cout, 0);
            }

            return 0;
        }

        bool print_stats = arg_exists("--stats");
        int alphabet_size = 0;

        bool do_compress = !arg_exists("--decompress");

        /////////////////////////////////////////////////////////////////////////
        // Select algorithm

        std::string algorithm_id = string_arg("--algorithm");
        std::unique_ptr<Compressor> algo = select_algo_or_exit(registry,
                                                               algorithm_env,
                                                               algorithm_id);

        /////////////////////////////////////////////////////////////////////////
        // Select where the input comes from

        std::string file = string_arg("<input>");
        bool use_stdin = !arg_exists("--")
            && (file == "-" || arg_exists("-"));

        if (!use_stdin && !fexists(file)) {
            std::cerr << "input " << file << " does not exist\n";
            return 1;
        }

        /////////////////////////////////////////////////////////////////////////
        // Select where the output goes to

        std::string ofile;
        bool use_stdout = use_stdin;
        bool use_explict_output(value_arg_exists("--output"));

        if (use_explict_output) {
            // Output to manually specifed file
            ofile = string_arg("--output");
        } else if (!use_stdin && do_compress) {
            // Output to a automatically determined file
            ofile = file + "." + algorithm_id + "." + COMPRESSED_FILE_ENDING;
        } else if (!use_stdin && !do_compress) {
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
                bool force = bool(value_arg_exists("--force"));
                if (!check_for_file_already_exist(ofile, force)) {
                    return 1;
                } else {
                    out = Output::from_path(ofile);
                }
            }

            /////////////////////////////////////////////////////////////////////////
            // call into actual library

            if (do_compress) {
                if (print_stats) {
                    alphabet_size = 0; // count_alphabet_size(inp_vec);
                }

                setup_time = clk::now();

                algo->compress(inp, out);

                comp_time = clk::now();
            } else {
                setup_time = clk::now();

                // TODO: Optionally read encoding from file or header
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
        std::cout << '\n';
        std::cout << "This tool is used like this:\n";
        show_help();
        return 1;
    }

    return 0;
}
