#pragma once

#include <iostream>
#include <getopt.h>

/// \cond INTERNAL
namespace tdc_driver {

// getopt data
constexpr int OPT_HELP   = 1000;
constexpr int OPT_RAW    = 1001;
constexpr int OPT_STDIN  = 1002;
constexpr int OPT_STDOUT = 1003;

constexpr option OPTIONS[] = {
    {"algorithm",  required_argument, nullptr, 'a'},
    {"decompress", no_argument,       nullptr, 'd'},
    {"force",      no_argument,       nullptr, 'f'},
    {"generator",  required_argument, nullptr, 'g'},
    {"help",       no_argument,       nullptr, OPT_HELP},
    {"list",       no_argument,       nullptr, 'l'},
    {"output",     required_argument, nullptr, 'o'},
    {"stats",      optional_argument, nullptr, 's'},
    {"version",    no_argument,       nullptr, 'v'},
    {"raw",        no_argument,       nullptr, OPT_RAW},
    {"usestdin",   no_argument,       nullptr, OPT_STDIN},
    {"usestdout",  no_argument,       nullptr, OPT_STDOUT},
    {"logdir",     required_argument, nullptr, 'L'},
    {"loglevel",   required_argument, nullptr, 'O'},
    {"logverbosity",   required_argument, nullptr, 'V'},
    {0, 0, 0, 0} // termination (required last entry!!)
};

class Options {
public:
    static inline void print_usage(const std::string& cmd, std::ostream& out) {
        using namespace std;

        // Usage
        out << left;
        out << setw(7) << "Usage: " << cmd << " [OPTION] "
            << setw(11) << "FILE" << "(1)" << endl;
        out << setw(7) << "or: " << cmd << " [OPTION] "
            << setw(11) << "--usestdin" << "(2)" << endl;
        out << setw(7) << "or: " << cmd << " [OPTION] "
            << setw(11) << "-g GENERATOR" << "(3)" << endl;

        // Brief description
        out << endl;
        out << "Compresses or decompresses a file (1), an input received via stdin (2) or a" << endl;
        out << "generated string (3). Depending on the selected input, an output (either a" << endl;
        out << "file or stdout) may need to be specified." << endl;

        // Options
        out << endl;
        out << "Options:" << endl;

        constexpr int W_SF = 4;
        constexpr int W_NOSF = 6;
        constexpr int W_LF = 24;
        constexpr int W_INDENT = 30;

        // -a, --algorithm
        out << right << setw(W_SF) << "-a" << ", "
            << left << setw(W_LF) << "--algorithm=ALGORITHM"
            << "use ALGORITHM for (de-)compression"
            << endl << setw(W_INDENT) << "" << "(use -l for more information)"
            << endl;

        // -d, --decompress
        out << right << setw(W_SF) << "-d" << ", "
            << left << setw(W_LF) << "--decompress"
            << "decompress the input (instead of compressing it)"
            << endl;

        // -f, --force
        out << right << setw(W_SF) << "-f" << ", "
            << left << setw(W_LF) << "--force"
            << "overwrite output file if it exists"
            << endl;

        // -g, --generator
        out << right << setw(W_SF) << "-g" << ", "
            << left << setw(W_LF) << "--generator=GENERATOR"
            << "generate the input using GENERATOR"
            << endl << setw(W_INDENT) << "" << "(use -l for more information)"
            << endl;

        // -l, --list
        out << right << setw(W_SF) << "-l" << ", "
            << left << setw(W_LF) << "--list"
            << "list available (de-)compression algorithms"
            << endl;

        // -o, --output=FILE
        out << right << setw(W_SF) << "-o" << ", "
            << left << setw(W_LF) << "--output=FILE"
            << "write output to FILE."
            << endl;

        // -s, --stats
        out << right << setw(W_SF) << "-s" << ", "
            << left << setw(W_LF) << "--stats[=TITLE]"
            << "print (de-)compression statistics in JSON format"
            << endl;

        // --help
        out << right << setw(W_NOSF) << ""
            << left << setw(W_LF) << "--help"
            << "display this help"
            << endl;

        // --raw
        out << right << setw(W_NOSF) << ""
            << left << setw(W_LF) << "--raw"
            << "(de-)compress without writing/reading a header"
            << endl;

        // --usestdin
        out << right << setw(W_NOSF) << ""
            << left << setw(W_LF) << "--usestdin"
            << "use stdin for input"
            << endl;

        // --usestdout
        out << right << setw(W_NOSF) << ""
            << left << setw(W_LF) << "--usestdout"
            << "use stdout for input"
            << endl;

        // -v, --version
        out << right << setw(W_SF) << "-v" << ", "
            << left << setw(W_LF) << "--version"
            << "print the version number of this build"
            << endl;

        // -L, --logdir
        out << right << setw(W_SF) << "-L" << ", "
            << left << setw(W_LF) << "--logdir=path"
            << "instead of logging to stdout use a log dir"
            << endl << setw(W_INDENT) << "" << "in which log files are saved"
            << endl;

        // -O, --lOglevel
        out << right << setw(W_SF) << "-O" << ", "
            << left << setw(W_LF) << "--loglevel=[0|1|2|3]"
            << "log messages at or above this level"
            << endl << setw(W_INDENT) << "" << "levels are INFO, WARNING, ERROR, and FATAL, "
            << endl << setw(W_INDENT) << "" << "enumerated by 0, 1, 2, and 3, respectively."
            << endl;

        // -V, --logverbosity
        out << right << setw(W_SF) << "-V" << ", "
            << left << setw(W_LF) << "--logverbosity=[0|1|2]"
            << "show all VLOG(m) messages for m less or equal the value of this flag"
            << endl;
    }

private:
    // fields
    bool m_unknown_options;

    bool m_help;
    bool m_version;
    bool m_list;

    std::string m_algorithm;

    std::string m_output;
    bool m_force;
    bool m_stdin, m_stdout;
    std::string m_generator;

    bool m_raw;
    bool m_decompress;

    bool m_stats;
    std::string m_stats_title;

    std::vector<std::string> m_remaining;

public:
    // The reference-based accessors will
    // get invalidated in case of a move or copy, so forbid them
    Options(const Options& other) = delete;
    Options(Options&& other) = delete;

    inline Options(int argc, char **argv) :
        m_unknown_options(false),
        m_help(false),
        m_version(false),
        m_list(false),
        m_force(false),
        m_stdin(false),
        m_stdout(false),
        m_raw(false),
        m_decompress(false),
        m_stats(false)
    {
        int c, option_index = 0;
        while((c = getopt_long(argc, argv, "O:V:L:a:dfg:lo:s::v",
            OPTIONS, &option_index)) != -1) {

            switch(c) {
                case 'a': // --algorithm=<optarg>
                    m_algorithm = std::string(optarg);
                    break;

                case 'd': // --decompress
                    m_decompress = true;
                    break;

                case 'f': // --force
                    m_force = true;
                    break;

                case 'g': // --generator=<optarg>
                    m_generator = std::string(optarg);
                    break;

                case 'l': // --list
                    m_list = true;
                    break;


                case 'v': // --version
                    m_version = true;
                    break;

                case 'o': // --output=<optarg>
                    m_output = std::string(optarg);
                    break;

                case 's': // --stats=[optarg]
                    m_stats = true;
                    if(optarg) m_stats_title = std::string(optarg);
                    break;

				//logging
                case 'L': // --logdir
					FLAGS_log_dir = std::string(optarg);
					FLAGS_logtostderr = 0;
                    break;
                case 'V': // --logVerbosity
					FLAGS_v = std::stoi(std::string(optarg));
                    break;
                case 'O': // --lOglevel
					FLAGS_minloglevel = std::stoi(std::string(optarg));
                    break;

                case OPT_HELP: // --help
                    m_help = true;
                    break;

                case OPT_RAW: // --raw
                    m_raw = true;
                    break;

                case OPT_STDIN: // --usestdin
                    m_stdin = true;
                    break;

                case OPT_STDOUT: // --usestdout
                    m_stdout = true;
                    break;

                case '?': // unknown option
                    m_unknown_options = true;
                    break;

                default: // declared, but unhandled
                    std::cerr << "Unhandled option \"" <<
                        OPTIONS[option_index].name << "\"";
                    break;
            }
        }

        // remaining options (e.g. filename)
        while(optind < argc) {
            m_remaining.emplace_back(argv[optind++]);
        }
    }

    // public accessors
    const bool& unknown_options = m_unknown_options;

    const bool& help = m_help;
    const bool& version = m_version;
    const bool& list = m_list;

    const std::string& algorithm = m_algorithm;

    const std::string& output = m_output;
    const bool& force = m_force;
    const bool& stdin = m_stdin;
    const bool& stdout = m_stdout;
    const std::string& generator = m_generator;

    const bool& raw = m_raw;
    const bool& decompress = m_decompress;

    const bool& stats = m_stats;
    const std::string& stats_title = m_stats_title;

    const std::vector<std::string>& remaining = m_remaining;
};

}
/// \endcond

