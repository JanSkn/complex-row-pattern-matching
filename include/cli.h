#ifndef CLI_H
#define CLI_H

#include <iostream>
#include <string>
#include <vector>

struct CLIParams {
    std::string pattern;
    std::vector<std::string> alphabet;
    std::vector<std::string> query;
    std::string timeWindowType;
    std::string twColumn;
    int twSize = 0;
    int twStepSize = 0;
};

void printHelp();
CLIParams parseCommandLine(int argc, char* argv[]);

#endif
