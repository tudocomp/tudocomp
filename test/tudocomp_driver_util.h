
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


std::string roundtrip_in_file_name(std::string algo,
                                   std::string name_addition) {
    return algo + name_addition + ".txt";
}
std::string roundtrip_comp_file_name(std::string algo,
                                   std::string name_addition) {
    return algo + name_addition + ".tdc";
}
std::string roundtrip_decomp_file_name(std::string algo,
                                   std::string name_addition) {
    return algo + name_addition + ".decomp.txt";
}

void roundtrip(std::string algo,
               std::string name_addition,
               std::string text,
               bool use_raw,
               bool& abort)
{
    std::string in_file   = roundtrip_in_file_name(algo, name_addition);
    std::string comp_file = roundtrip_comp_file_name(algo, name_addition);
    std::string decomp_file  = roundtrip_decomp_file_name(algo, name_addition);

    //std::cout << "Roundtrip with\n";
    std::cout << in_file << " -> ";
    std::cout.flush();

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
        std::string cmd;
        if (use_raw) {
            cmd = "--raw --compress --algorithm " + algo
                + " --output " + out + " " + in;
        } else {
            cmd = "--compress --algorithm " + algo
                + " --output " + out + " " + in;
        }
        comp_out = driver(cmd);
    }

    std::cout << comp_file << " -> ";
    std::cout.flush();

    bool compressed_file_exists = test_file_exists(comp_file);

    if (!compressed_file_exists) {
        std::cout << "ERR\n";
        std::cout << "---\n";
        std::cout << comp_out;
        std::cout << "---\n";
        EXPECT_TRUE(compressed_file_exists);
        return;
    }

    // Decompress
    {
        std::string in = test_file_path(comp_file);
        std::string out = test_file_path(decomp_file);
        std::string cmd;
        if (use_raw) {
            cmd = "--raw --decompress --algorithm " + algo
                + " --output " + out + " " + in;
        } else {
            cmd = "--decompress --output " + out + " " + in;
        }
        decomp_out = driver(cmd);
    }

    std::cout << decomp_file << " ... ";
    std::cout.flush();

    bool decompressed_file_exists = test_file_exists(decomp_file);
    if (!decompressed_file_exists) {
        std::cout << "ERR\n";
        std::cout << "---\n";
        std::cout << comp_out;
        std::cout << "---\n";
        std::cout << decomp_out;
        std::cout << "---\n";
        EXPECT_TRUE(decompressed_file_exists);
        return;
    } else {
        std::string read_text = read_test_file(decomp_file);
        if (read_text != text) {
            std::cout << "ERR\n";
            std::cout << "---\n";
            std::cout << comp_out;
            std::cout << "---\n";
            std::cout << decomp_out;
            std::cout << "---\n";

            assert_eq_strings(text, read_text);
            std::string diff;
            for(size_t i = 0; i < std::max(text.size(), read_text.size()); i++) {
                if (i < std::min(text.size(), read_text.size())
                    && text[i] == read_text[i]
                ) {
                    diff.push_back('-');
                } else {
                    diff.push_back('#');
                }
            }
            std::cout << "Diff:     \"" << diff << "\"\n";

            abort = true;
            return;
        } else {
            std::cout << "OK\n";
            std::cout << comp_out;
            std::cout << decomp_out;
        }
    }
}
