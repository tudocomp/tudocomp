
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
            cmd = "--raw --algorithm \"" + algo + "\""
                + " --output \"" + out + "\" \"" + in + "\"";
        } else {
            cmd = "--algorithm \"" + algo + "\""
                + " --output \"" + out + "\" \"" + in + "\"";
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
            cmd = "--raw --decompress --algorithm \"" + algo + "\""
                + " --output \"" + out + "\" \"" + in + "\"";
        } else {
            cmd = "--decompress --output \"" + out + "\" \"" + in + "\"";
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
