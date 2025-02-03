#ifndef CLI_H
#define CLI_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct CLIParams {
    string tableName;
    string pattern;
    vector<string> alphabet;
    vector<string> queries;
    string outputTableName;
    vector<string> outputTableColums;
    string timeWindowType;
    string twColumn;
    int twSize = 0;
    int twStepSize = 0;
};

void printHelp();
CLIParams parseCommandLine(int argc, char* argv[]);

#endif
