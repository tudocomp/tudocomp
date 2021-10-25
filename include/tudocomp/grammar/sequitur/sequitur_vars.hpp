#pragma once

namespace tdc {
namespace grammar {
namespace sequitur {
    
extern int num_rules;
extern int num_symbols;
extern int delimiter;
extern int memory_to_use;
extern int table_size;
extern int quiet;
extern int max_rule_len;
extern int K;
extern bool compression_initialized;

extern int occupied;
extern int numbers;
extern int current_rule;
extern int do_uncompress;
extern int print_rule_usage;

void reset_vars(); 

} // namespace sequitur
} // namespace grammar
} // namespace tdc
