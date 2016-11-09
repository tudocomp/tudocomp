#include <sdsl/int_vector.hpp>
#include <sdsl/cst_sada.hpp>
#include <sdsl/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/io/BitOStream.hpp>
#include <tudocomp/io/BitIStream.hpp>
#include <tudocomp/util.hpp>

namespace tdc {

class LCPSuffixLinkCompressor : public Compressor {

public:
    inline static Meta meta() {
        Meta m("compressor", "suflink", "Suffix Link Compressor");
        return m;
    }

private:
    // This is the suffix tree used to store all the symbols for a given text
    sdsl::cst_sada<> cst;
    struct cstSymbol {
        char c = '\0';
        size_t length = 0;
        ssize_t sl = -1;

        bool operator==(const cstSymbol& rhs) const {
            return (c == rhs.c) && (length == rhs.length);
        }
    };
    std::vector<cstSymbol> symbols;
    size_t longest;
    std::map<size_t, size_t> slMap;

    // This method finds all the symbols and assigns them in the cstSymbols vector
    void cstGetSymbols() {
        std::string edge;

        // This first run through the suffix tree maps the node elements to the elements in the symbol table
        for(auto it = cst.begin(); it != cst.end(); it++) {
            if(it.visit()==1) {
                if(*it != cst.root()) {
                        edge = extract(cst, *it);

                        // We want to check all inner nodes
                        if(edge.back() != '\0') {
                            symbols.push_back(cstSymbol());
                            slMap[*it] = symbols.size()-1;
                        }
                }
            }
        }

        // This second run through the suffix tree fills the symbols in the symbol table
        // We need two runs since it could happen that a symbol needs to link to another symbol that had not yet been mapped
        for(auto it = cst.begin(); it != cst.end(); it++) {
            if(it.visit()==1) {
                if(*it != cst.root()) {
                        edge = extract(cst, *it);

                        // We want to check all inner nodes
                        if(edge.back() != '\0') {
                            visit(*it);
                        }
                }
            }
        }
    }

    // This method visits a node, adding its contents (first char, length, and suffix link) to the symbol table
    void visit(size_t n) {
        std::string s = extract(cst,n);
        size_t mapn = slMap[n];

        // If the node's parent is the root, we have a symbol to be added to the table
        if(cst.parent(n) == cst.root() && s.length() == 1) {
            symbols.at(mapn).c = s[0];
            symbols.at(mapn).length = 1;
        }
        // If the edge is longer than one character or if its parent is not the root, we point the symbol to its suffix link
        else {
            symbols.at(mapn).c = s.front();
            symbols.at(mapn).length = s.length();
            symbols.at(mapn).sl = slMap[cst.sl(n)];
        }
    }

    void outputSymbolTable() {
        // DLOG(INFO) << "Outputting symbols";
        for(size_t i = 0; i < symbols.size(); i++) {
            cstSymbol csts = symbols[i];
            DLOG(INFO) << "Symbol " << i << ", " << csts.c << " has length " << csts.length << " and links to " << csts.sl;
        }
    }

    // This method takes the input string and encodes it to the output
    void encode(std::string input_text, BitOStream* bito) {
        std::string output_text = "";
        auto csa = cst.csa;
        size_t n;
        for(size_t i=0;i<input_text.length();i++) {
            // Here we use the inverse suffix array and inverse id to get the node corresponding to the current char index
            n = cst.inv_id(csa.isa[i]);

            // If the node we are looking at is a child of the root, then it does not link to any other node
            // and can't be compressed. The first char in this suffix is encoded directly
            if(cst.parent(n) == cst.root()) {
                // DLOG(INFO) << n << "'s parent was the root";
                output_text += extract(cst,n).front();

                // A 0 bit indicates that a char is being encoded directly
                bito->write_bit(0);
                bito->write_int(input_text[i], 8);
            }
            // If the node we are looking at is not the child of the root, then it can be compressed. We go through the
            // suffix links and encode it that way
            else {
                // DLOG(INFO) << n << " can be compressed";
                size_t sym_ind = slMap[cst.parent(n)];
                // DLOG(INFO) << "Symbol " << symbols[sym_ind].c << " with length " << symbols[sym_ind].length << " will be used";
                output_text += std::to_string(sym_ind);

                // A 1 bit indicates that a compressed symbol is being encoded
                bito->write_bit(1);
                bito->write_compressed_int(sym_ind);

                i += symbols[sym_ind].length-1;
            }
        }

        // Flush out the last few bits
        bito->flush();
        DLOG(INFO) << "Encoded string: " << output_text;
    }

public:
    /// Default constructor (not supported)
    inline LCPSuffixLinkCompressor() = delete;

    /// Construct the class with an environment
    inline LCPSuffixLinkCompressor(Env&& env) : Compressor(std::move(env)) {
    }

    /// Compress the input into an output
    virtual void compress(Input& input, Output& output) override final {
        char c;
        auto in = input.as_stream();
        auto out_guard = output.as_stream();
        BitOStream* bito = new BitOStream(out_guard);
        std::stringstream input_str;
        size_t text_length;

        // Here we grab the input text and store it in input_text
        while(in.get(c) && c != '\0') {
            input_str << c;
        }
        std::string input_text = input_str.str();

        // Here we construct the cst and build the symbol table
        construct_im(cst, input_text, 1);
        cstGetSymbols();

        // Here we store the size of the compressed string
        text_length = (input_text.length());
        bito->write_compressed_int(text_length);

        // Here we encode the text
        encode(input_text, bito);

        // outputSymbolTable();
    }

    // Decompress the input into an output
    virtual void decompress(Input& input, Output& output) override {
        auto in_guard = input.as_stream();
        auto out = output.as_stream();
        BitIStream* biti = new BitIStream(in_guard);

        std::stringstream total_len_str;
        size_t curr_pos=0, total_len=0;
        ssize_t symbol;

        // Here we get the length of the original string from the compressed string
        // and use it to declare a vector of the correct size to hold the result
        total_len = biti->read_compressed_int<size_t>();
        sdsl::int_vector<8> text = sdsl::int_vector<8>(total_len, 0);

        while(total_len > 0) {
            // Here we have a compressed symbol
            if(biti->read_bit()) {
                symbol = biti->read_compressed_int();

                // Once the symbol points to -1, we are done decoding it
                while(symbol != -1) {
                    text[curr_pos] = symbols[symbol].c;
                    curr_pos++;
                    total_len--;
                    symbol = symbols[symbol].sl;
                }

            }
            // Here we have a simple char
            else {
                text[curr_pos] = biti->read_int<char>(8);
                curr_pos++;
                total_len--;
            }
        }

        // Here we write the output of the decompressed string
        out.write((const char*)text.data(), text.size());
    }
};

}