/// \cond
#ifndef COMMANDARS_H
#define COMMANDARS_H

#include <vector>
#include <cstdint>
#include <string>

struct CommandArgs {
    // TODO: Find Option<> pattern solution, pick better types for path, etc

    bool text;
    std::string text_value;

    bool file;
    std::string file_value;

    bool theta;
    int32_t theta_value;

    bool compressor;
    std::string compressor_value; // TODO proper enum

    bool maxlcp;
    std::string maxlcp_value;

    bool code1;
    std::string code1_value;

    bool code2;
    std::string code2_value;

    bool kgram;
    int32_t kgram_value;

    bool cacheESA;

    bool cacheRules;

    bool printESA;

    bool printSufList;

    bool printTime;

    bool printProgress;

    bool printRules;

    bool printStats;

    bool printCode;
    std::string printCode_value;
};

extern CommandArgs ARGS;

#endif
/// \endcond
