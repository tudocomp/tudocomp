
#include <tudocomp/grammar/sequitur/classes.hpp>


/**
 * @brief A file containing global variables used by sequitur
 * 
 */
namespace tdc {
namespace grammar {
namespace sequitur {

int num_rules;
int num_symbols;
int delimiter;
int K;
int memory_to_use;
int table_size;
int quiet;
int max_rule_len;
bool compression_initialized;

int occupied;
int numbers;
int current_rule;
int do_uncompress;
int print_rule_usage;

void reset_vars() {
    num_rules = 0;
    num_symbols = 0;
    delimiter = -1;
    K = 1;
    memory_to_use = 1000000000;
    table_size = memory_to_use / (K * sizeof(symbols *));
    quiet = 0;
    max_rule_len = 2;
    compression_initialized = false;
    occupied = 0;
    numbers = 0;
    current_rule = 0;
    print_rule_usage = 0;
}

}}}