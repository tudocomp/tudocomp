#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>

#include "glog/logging.h"
#include "docopt/docopt.h"
#include "tudocomp.h"

#include "tudocomp_algorithms.h"

namespace tudocomp_driver {

using namespace tudocomp;

const std::string COMPRESSED_FILE_ENDING = "tdc";

static const std::string USAGE =
R"(TuDo Comp.

Usage:
    tudocomp [-fs] [-k] -c <name>  -e <name>  [-o <file>] [--] ( <file> | - )
    tudocomp [-fs]  -d            [-e <name>] [-o <file>] [--] ( <file> | - )
    tudocomp --list

Options:
    -h --help               Show this screen.
    --version               Show version.
    -c --compressor <name>  Use compressor <name> for generating the Ruleset.
    -e --encoder <name>     Use encoder <name> for generating the Output.
    -k --compress           Compress input instead of compressing it.
    -d --decompress         Decompress input instead of compressing it.
    -o --output <file>      Choose output filename instead the the default of
                            <input file>.<compressor name>.<encoder name>.tdc
                            or stdout if reading from stdin.
    -s --stats              Print statistics to stdout.
    -f --force              Overwrite output even if it exists.
    -l --list               List all Compression and Encoding algorithms
                            supported by this tool.
)";

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

    std::vector<std::string> debug_args;

    for (int i = 0; i < argc; i++) {
        debug_args.push_back(argv[i]);
    }

    for (auto& elem : debug_args) {
        std::cout << vec_as_lossy_string(elem) << std::endl;
    }

    std::map<std::string, docopt::value> args
        = docopt::docopt(USAGE,
                         { argv + 1, argv + argc },
                         true,                // show help if requested
                         "TuDoComp 0.1.0");  // version string

    if (args["--list"].asBool()) {
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

    bool print_stats = args["--stats"].asBool();
    int alphabet_size = 0;

    bool do_compress = !args["--decompress"].asBool();

    /////////////////////////////////////////////////////////////////////////
    // Select where the input comes from

    std::string file = args["<file>"].asString();
    bool use_stdin = !args["--"].asBool()
        && (file == "-" || args["-"].asBool());

    if (!use_stdin && !fexists(file)) {
        std::cerr << "input " << file << " does not exist\n";
        return 1;
    }

    // Handle selection of encoder
    CodingAlgorithm enc;
    bool use_explict_encoder(args["--encoder"]);
    auto decode_meta_from_file = extract_from_file(file);

    if (use_explict_encoder) {
        enc = getCodingByShortname(args["--encoder"].asString());
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

    CompressionAlgorithm comp;
    if (do_compress) {
        comp = getCompressionByShortname(args["--compressor"].asString());
        if (comp.compressor == nullptr) {
            std::cerr << "Unknown compressor '" << comp.shortname << "'.\n";
            std::cerr << "Use --list for a list of all implemented algorithms.\n";
            return 1;
        }
    } else {
        comp = { "", "", nullptr };
    }

    /////////////////////////////////////////////////////////////////////////
    // Select where the output goes to

    std::string ofile;
    bool use_stdout = use_stdin;
    bool use_explict_output(args["--output"]);

    if (use_explict_output) {
        // Output to manually specifed file
        ofile = args["--output"].asString();
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
            if (!open_output(out, ofile, bool(args["--force"]))) {
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

            auto threshold = enc.coder->min_encoded_rule_length(inp_vec.size());

            auto rules = comp.compressor->compress(inp_vec, threshold);

            comp_time = clk::now();

            enc.coder->code(rules, std::move(inp_vec), *out);

            enc_time = clk::now();
        } else {
            setup_time = clk::now();
            comp_time = clk::now();

            // TODO: Optionally read encoding from file or header
            enc.coder->decode(*inp, *out);

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

    return 0;
}
