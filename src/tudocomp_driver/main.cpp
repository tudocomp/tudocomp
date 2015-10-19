#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <boost/program_options.hpp>
#include <boost/exception/exception.hpp>

#include "glog/logging.h"
#include "docopt/docopt.h"
#include "tudocomp.h"

#include "tudocomp_algorithms.h"

namespace tudocomp_driver {

using namespace tudocomp;
namespace po = boost::program_options;

const std::string COMPRESSED_FILE_ENDING = "tdc";

static const std::string USAGE =
R"(TuDo Comp.

Usage:
    tudocomp [-fs] [-k] -c <name>  -e <name>  [-o <output>] [--] ( <input> | - )
    tudocomp [-fs]  -d            [-e <name>] [-o <output>] [--] ( <input> | - )
    tudocomp --list
    tudocomp --help

Options:
    -h --help               Show this screen.
    -c --compressor <name>  Use compressor <name> for generating the Ruleset.
    -e --encoder <name>     Use encoder <name> for generating the Output.
    -k --compress           Compress input instead of compressing it.
    -d --decompress         Decompress input instead of compressing it.
    -o --output <output>    Choose output filename instead the the default of
                            <input>.<compressor name>.<encoder name>.tdc
                            or stdout if reading from stdin.
    -s --stats              Print statistics to stdout.
    -f --force              Overwrite output even if it exists.
    -l --list               List all Compression and Encoding algorithms
                            supported by this tool.
)";

static void exit(std::string msg) {
    throw std::runtime_error(msg);
}

static bool fexists(std::string filename)
{
  std::ifstream ifile(filename);
  return bool(ifile);
}

static size_t fsize(std::string file) {
    std::ifstream t(file);
    t.seekg(0, std::ios::end);
    return t.tellg();
}

static bool open_output(std::ostream*& out, std::string& ofile, bool allow_overwrite) {
    // Don't accidentially overwrite files
    if (!allow_overwrite && fexists(ofile)) {
        std::cerr << "Outputfile already exists\n";
        return false;
    }
    out = new std::ofstream(ofile);
    return true;
}

struct FileNameComponents {
    bool found;
    std::string base_name;
    std::string enc_shortname;
};
static FileNameComponents extract_from_file(const std::string& file) {
    // TODO: Ugly way to get the last two dot seperated components
    // of filename
    size_t dot2 = 0;
    size_t dot1 = 0;
    size_t dot0 = 0;
    size_t dots = 0;
    for (size_t i_ = 0; i_ < file.size(); i_++) {
        size_t i = file.size() - i_;
        if (file[i] == '.' && dots == 0) {
            dot2 = i;
            dots++;
        } else if (file[i] == '.' && dots == 1) {
            dot1 = i;
            dots++;
        } else if (file[i] == '.' && dots == 2) {
            dot0 = i;
            dots++;
            break;
        }
    }

    if (dots == 3) {
        auto ending = file.substr(dot2 + 1, file.size() - dot2 - 1);
        auto shortname = file.substr(dot1 + 1, dot2 - dot1 - 1);

        if (ending == COMPRESSED_FILE_ENDING) {
            return { true, file.substr(0, dot0), shortname };
        }
    }
    return { false, "", "" };
}

uint8_t count_alphabet_size(Input& input) {
    uint64_t table[256] = {};

    for (auto& e : table) {
        CHECK(e == 0);
    }

    uint8_t counter = 0;

    for (uint8_t byte : input) {
        table[byte]++;
    }

    for (uint64_t count : table) {
        if (count > 0) {
            counter++;
        }
    }

    return counter;
}

} // namespace tudocomp_driver

int main(int argc, const char** argv)
{
    using namespace tudocomp_driver;

    google::InitGoogleLogging(argv[0]);

    // Set up environment for algorithms.
    Env algorithm_env;

    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "")
        ("compressor,c", po::value<std::string>(), "")
        ("encoder,e", po::value<std::string>(), "")
        ("compress,k", "")
        ("decompress,d", "")
        ("output,o", po::value<std::string>(), "")
        ("stats,s", "")
        ("force,f", "")
        ("list,l", "")
        ("input", po::value<std::string>(), "")
    ;
    po::positional_options_description pos_desc;
    pos_desc.add("input", 1);

    po::variables_map args;

    auto show_help = []() {
        std::cout << USAGE << std::endl;
    };

    try {
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

        if (arg_exists("--help")) {
            show_help();
            return 0;
        }

        if (arg_exists("--list")) {
            std::cout << "This build supports the following algorithms:\n";
            std::cout << std::endl;

            auto sep = "  |  ";

            {
                std::cout << "Compression:\n";
                uint w = 0;
                uint dw = 0;
                for (auto algo : COMPRESSION_ALGORITHM) {
                    w = w < algo.shortname.size() ? algo.shortname.size() : w;
                    dw = dw < algo.name.size() ? algo.name.size() : dw;
                }
                for (auto algo : COMPRESSION_ALGORITHM) {
                    std::cout << "  "
                        << std::setw(w) << std::left << algo.shortname << sep
                        << std::setw(dw) << std::left << algo.name << sep
                        << algo.description << std::endl;
                }
                std::cout << std::endl;
            }

            {
                std::cout << "Encoding:\n";
                uint w = 0;
                uint dw = 0;
                for (auto algo : CODING_ALGORITHM) {
                    w = w < algo.shortname.size() ? algo.shortname.size() : w;
                    dw = dw < algo.name.size() ? algo.name.size() : dw;
                }
                for (auto algo : CODING_ALGORITHM) {
                    std::cout << "  "
                        << std::setw(w) << std::left << algo.shortname << sep
                        << std::setw(dw) << std::left << algo.name << sep
                        << algo.description << std::endl;
                }
                std::cout << std::endl;
            }

            return 0;
        }

        bool print_stats = arg_exists("--stats");
        int alphabet_size = 0;

        bool do_compress = !arg_exists("--decompress");

        /////////////////////////////////////////////////////////////////////////
        // Select where the input comes from

        std::string file = string_arg("<input>");
        bool use_stdin = !arg_exists("--")
            && (file == "-" || arg_exists("-"));

        if (!use_stdin && !fexists(file)) {
            std::cerr << "input " << file << " does not exist\n";
            return 1;
        }

        // Handle selection of encoder
        CodingAlgorithm enc;
        bool use_explict_encoder(value_arg_exists("--encoder"));
        auto decode_meta_from_file = extract_from_file(file);

        if (use_explict_encoder) {
            enc = getCodingByShortname(string_arg("--encoder"));
        } else if (!use_stdin && decode_meta_from_file.found) {
            enc = getCodingByShortname(decode_meta_from_file.enc_shortname);
        } else {
            std::cerr << "Need to either specify a encoder or have it encoded in filename\n";
            return 1;
        }

        if (enc.coder == nullptr) {
            std::cerr << "Unknown encoder '" << enc.shortname << "'.\n";
            std::cerr << "Use --list for a list of all implemented algorithms.\n";
            return 1;
        }

        Coder* enc_instance = enc.coder(algorithm_env);

        CompressionAlgorithm comp;
        Compressor* comp_instance;
        if (do_compress) {
            comp = getCompressionByShortname(string_arg("--compressor"));
            if (comp.compressor == nullptr) {
                std::cerr << "Unknown compressor '" << comp.shortname << "'.\n";
                std::cerr << "Use --list for a list of all implemented algorithms.\n";
                return 1;
            }
            comp_instance = comp.compressor(algorithm_env);
        } else {
            comp = { "", "", "", nullptr };
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
            ofile = file + "." + comp.shortname + "." + enc.shortname + "." + COMPRESSED_FILE_ENDING;
        } else if (!use_stdin && !do_compress) {
            if (decode_meta_from_file.found) {
                ofile = decode_meta_from_file.base_name;
            } else {
                std::cerr << "Need to either specify a output filename or have it endcoded in input filename\n";
                return 1;
            }
        }

        /////////////////////////////////////////////////////////////////////////
        // open streams

        using clk = std::chrono::high_resolution_clock;

        clk::time_point start_time = clk::now();
        clk::time_point setup_time;
        clk::time_point comp_time;
        clk::time_point enc_time;
        clk::time_point end_time;

        {
            std::istream* inp;
            if (use_stdin) {
                // Input from stdin
                inp = &std::cin;
            } else {
                // Input from specified file
                inp = new std::ifstream(file);
            }

            std::ostream* out;
            if (use_stdout) {
                // Output to stdout
                out = &std::cout;
            } else {
                // Output to specified file
                if (!open_output(out, ofile, bool(value_arg_exists("--force")))) {
                    return 1;
                }
            }

            /////////////////////////////////////////////////////////////////////////
            // call into actual library

            if (do_compress) {
                // TODO: Solve better, eg by making compress take a istream
                char c;
                // COPY 0
                std::vector<uint8_t> inp_vec;

                if (!use_stdin) {
                    // TODO: Repair?
                    //inp_vec.reserve(fsize(file));
                    // HACK to get exact file size
                }

                while (inp->get(c)) {
                    inp_vec.push_back(c);
                }

                if (print_stats) {
                    alphabet_size = count_alphabet_size(inp_vec);
                }

                setup_time = clk::now();

                auto threshold = enc_instance->min_encoded_rule_length(
                    inp_vec.size());

                auto rules = comp_instance->compress(inp_vec, threshold);

                comp_time = clk::now();

                enc_instance->code(rules, std::move(inp_vec), *out);

                enc_time = clk::now();
            } else {
                setup_time = clk::now();
                comp_time = clk::now();

                // TODO: Optionally read encoding from file or header
                enc_instance->decode(*inp, *out);

                enc_time = clk::now();
            }

            out->flush();
        }

        end_time = clk::now();

        if (print_stats) {
            auto setup_duration = setup_time - start_time;
            auto comp_duration = comp_time - setup_time;
            auto enc_duration = enc_time - comp_time;
            auto end_duration = end_time - enc_time;
            std::cout << "---------------\n";
            std::cout << "Compressor: " << comp.name << std::endl;
            std::cout << "Coder: " << enc.name << std::endl;
            std::cout << "---------------\n";
            auto inp_size = 0;
            if (use_stdin) {
                std::cout << "input: <stdin>\n";
                std::cout << "input size: ? B\n";
            } else {
                inp_size = fsize(file);
                std::cout << "input: "<<file<<"\n";
                std::cout << "input size: "<<inp_size<<" B\n";
            }
            std::cout << "alphabet size: " << alphabet_size << "\n";

            auto out_size = 0;
            if (use_stdout) {
                std::cout << "output: <stdout>\n";
                std::cout << "output size: ? B\n";
            } else {
                out_size = fsize(ofile);
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
            print_time("startup", setup_duration);
            print_time("compression", comp_duration);
            print_time("encoding", enc_duration);
            print_time("teardown", end_duration);
            std::cout << "---------------\n";
        }
    } catch (std::exception& e) {
        std::cout << "Error: " << e.what() << '\n';
        std::cout << '\n';
        std::cout << "The tool is used like this:\n";
        show_help();
        return 1;
    }

    return 0;
}
