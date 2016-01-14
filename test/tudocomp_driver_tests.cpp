#include <stdio.h>
#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "glog/logging.h"

#include "test_util.h"
#include "tudocomp.h"
#include "tudocomp_algorithms.h"

std::string driver(std::string args) {
    using namespace std;

    FILE *in;
    char buff[512];
    std::stringstream ss;

    const std::string cmd = "./src/tudocomp_driver/tudocomp_driver " + args;

    if(!(in = popen(cmd.data(), "r"))) {
        throw std::runtime_error("Error executing " + cmd);
    }

    while(fgets(buff, sizeof(buff), in)!=NULL){
        ss << buff;
    }
    pclose(in);

    return ss.str();
}

// TODO: Find out how to make change to driver re-run this tests case

TEST(TudocompDriver, help) {
    ASSERT_EQ(driver("--help"),
"TuDo Comp.\n"
"\n"
"Usage:\n"
"    tudocomp [options] [-k]  -a <alg>  [-o <output>] [--] ( <input> | - )\n"
"    tudocomp [options]  -d  [-a <alg>] [-o <output>] [--] ( <input> | - )\n"
"    tudocomp --list\n"
"    tudocomp --help\n"
"\n"
"Options:\n"
"    -h --help               Show this screen.\n"
"    -a --algorithm <alg>    Use algorithm <alg> for (de)compression.\n"
"                            <alg> can be a dot-separated chain of\n"
"                            sub-algorithms. See --list for a complete list\n"
"                            of them.\n"
"                            Example: -a lz77rule.esa.esa_code0\n"
"    -k --compress           Compress input instead of compressing it.\n"
"    -d --decompress         Decompress input instead of compressing it.\n"
"    -o --output <output>    Choose output filename instead the the default of\n"
"                            <input>.<compressor name>.<encoder name>.tdc\n"
"                            or stdout if reading from stdin.\n"
"    -s --stats              Print statistics to stdout.\n"
"    -f --force              Overwrite output even if it exists.\n"
"    -l --list               List all Compression algorithms supported\n"
"                            by this tool.\n"
"                            Algorithms may consist out of sub-algorithms,\n"
"                            which will be displayed in a hierarchical fashion.\n"
"    -O --option <option>    An additional option of the form key=value.\n"
"\n"
    );
}

// A hacky parser for the command line `--list` output
// of all algorithms
namespace list {
    struct SubAlgo;
    struct Algo {
        std::string id_name;
        std::vector<SubAlgo> subalgos;
        inline std::string print(std::string i);
    };
    struct SubAlgo {
        std::string kind;
        std::vector<Algo> algos;
        inline std::string print(std::string i);
    };
    inline std::string Algo::print(std::string i) {
        std::stringstream s;
        s << i << id_name << "\n";
        for (auto& x : subalgos) {
            s << x.print(i + "  ");
        }
        return s.str();
    }
    inline std::string SubAlgo::print(std::string i) {
        std::stringstream s;
        s << i << kind << "\n";
        for (auto& x : algos) {
            s << x.print(i + "  ");
        }
        return s.str();
    }

    SubAlgo parse_list(std::string& headerless_list_raw) {
        SubAlgo root { "<root>", {} };

        {
            struct Stack { size_t ident; SubAlgo* ptr; };
            std::vector<Stack> stack { { 0, &root } };
            auto top = [&] () {
                return stack[stack.size() - 1];
            };
            size_t prev_ident = 0;

            std::istringstream ss(headerless_list_raw);
            std::string line;
            while (getline(ss, line)) {
                //std::cout << "### " << line << "\n";

                size_t ident = 0;

                while (ident < line.size()) {
                    if (line.at(ident) == '[') {
                        std::string algo_kind = line.substr(ident);
                        auto end = algo_kind.find_first_of(" |");
                        if (end != std::string::npos) {
                            algo_kind = algo_kind.substr(0, end);
                        }

                        if (ident < prev_ident) {
                            while (top().ident > ident) {
                                stack.pop_back();
                            }
                            stack.pop_back();
                        }
                        auto& a = top().ptr->algos;
                        a[a.size() - 1].subalgos.push_back(
                            SubAlgo {
                                algo_kind,
                                {}
                            }
                        );
                        auto& sa = a[a.size() - 1].subalgos;
                        stack.push_back(Stack {
                            ident,
                            &sa[sa.size() - 1]});

                        break;
                    } else if (line.at(ident) != ' ') {
                        std::string algo = line.substr(ident);
                        auto end = algo.find_first_of(" |");
                        if (end != std::string::npos) {
                            algo = algo.substr(0, end);
                        }

                        if (ident < prev_ident) {
                            while (top().ident > ident) {
                                stack.pop_back();
                            }
                        }

                        top().ptr->algos.push_back(Algo {
                            algo, {}
                        });

                        break;
                    }
                    ident++;
                }

                prev_ident = ident;
            }
        }

        return root;
    }

    struct DriverListOutput {
        std::string header;
        SubAlgo root;
    };

    DriverListOutput tudocomp_list() {
        std::string list_raw = driver("--list");

        auto header_end = list_raw.find("\n\n");
        if (header_end == std::string::npos) {
            return { "", { "", {} } };
        }
        std::string header = list_raw.substr(0, header_end);
        std::string headerless_list_raw = list_raw.substr(header_end + 2);
        auto root = parse_list(headerless_list_raw);
        //std::cout << root.print("") << std::endl;
        return { header, root };
    }
}

TEST(TudocompDriver, list) {
    auto list = list::tudocomp_list();

    ASSERT_EQ(list.header, "This build supports the following algorithms:");

    auto& root = list.root;

    // Test that we got at least the amount of algorithms
    // we had when writing this test.
    ASSERT_GE(root.algos.size(), 2u);
    {
        auto& r0 = root.algos[0];
        ASSERT_GE(r0.subalgos.size(), 2u);
        {
            auto& r00 = r0.subalgos[0];
            ASSERT_GE(r00.algos.size(), 5u);
        }
        {
            auto& r01 = r0.subalgos[1];
            ASSERT_GE(r01.algos.size(), 4u);
        }
    }
    {
        auto& r1 = root.algos[1];
        ASSERT_GE(r1.subalgos.size(), 1u);
        {
            auto& r10 = r1.subalgos[0];
            ASSERT_GE(r10.algos.size(), 1u);
        }
    }
}

std::vector<std::string> cross(std::vector<std::vector<std::string>>&& vs) {
    auto remaining = vs;
    if (remaining.size() == 0) {
        return {};
    }

    std::vector<std::string> first = std::move(remaining[0]);
    remaining.erase(remaining.begin());

    auto next = cross(std::move(remaining));

    if (next.size() == 0) {
        return first;
    } else {
        std::vector<std::string> r;
        for (auto& x : first) {
            for (auto& y : next) {
                r.push_back(x + "." + y);
            }
        }
        return r;
    }
}

std::vector<std::string> algo_cross_product(list::SubAlgo& subalgo) {
    std::vector<std::string> r;
    for (auto& algo : subalgo.algos) {

        auto& name = algo.id_name;
        std::vector<std::vector<std::string>> subalgos;
        for (auto& subalgo : algo.subalgos) {
            subalgos.push_back(algo_cross_product(subalgo));
        }

        if (subalgos.size() == 0) {
            r.push_back(name);
        } else {
            for (auto x : cross(std::move(subalgos))) {
                r.push_back(name + "." + x);
            }
        }
    }
    return r;
}

TEST(TudocompDriver, roundtrip_matrix) {
    std::cout << "[ Parsing Algorithm list from executable ]\n";
    auto list = list::tudocomp_list().root;
    std::cout << list.print("") << std::endl;
    std::cout << "[ Generating cross product of all algorithms ]\n";
    std::vector<std::string> algo_lines = algo_cross_product(list);
    for (auto& e : algo_lines) {
        std::cout << "  " << e << "\n";
    }
    std::cout << "[ Start roundtrip tests ]\n";

    for (auto& algo : algo_lines) {
        int counter = 0;
        bool abort = false;
        test_roundtrip_batch([&](std::string text) {
            if (abort) {
                return;
            }
            std::stringstream ss;
            ss << counter;
            std::string n = ss.str();
            counter++;

            std::string in_file   = algo + "_" + n + ".txt";
            std::string comp_file = algo + "_" + n + ".tdc";
            std::string decomp_file  = algo + "_" + n + ".decomp.txt";

            //std::cout << "Roundtrip with\n";
            std::cout << in_file << " -> ";
            std::cout << comp_file << " -> ";
            std::cout << decomp_file << "...";

            remove_test_file(in_file);
            remove_test_file(comp_file);
            remove_test_file(decomp_file);

            write_test_file(in_file, text);

            std::string comp_out;
            std::string decomp_out;

            // Compress
            {
                std::string in = test_file_path(in_file);
                std::string out = test_file_path(comp_file);
                std::string cmd = "-k -a " + algo + " -o " + out + " " + in;
                comp_out = driver(cmd);
            }

            // Decompress
            {
                std::string in = test_file_path(comp_file);
                std::string out = test_file_path(decomp_file);
                std::string cmd = "-d -a " + algo + " -o " + out + " " + in;
                decomp_out = driver(cmd);
            }

            std::string read_text = read_test_file(decomp_file);
            if (read_text != text) {
                std::cout << "\n";
                abort = true;

                assert_eq_strings(text, read_text);
                std::string diff;
                for(int i = 0; i < std::max(text.size(), read_text.size()); i++) {
                    if (i < std::min(text.size(), read_text.size())
                        && text[i] == read_text[i]
                    ) {
                        diff.push_back('-');
                    } else {
                        diff.push_back('#');
                    }
                }
                std::cout << "Diff:     \"" << diff << "\"\n";

                return;
            }

            std::cout << " OK\n";
            std::cout << comp_out;
            std::cout << decomp_out;
        });
        if (abort) {
            break;
        }
    }
}
