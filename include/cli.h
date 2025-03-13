#ifndef CLI_H
#define CLI_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct CLIParams {
    string catalog;
    string schema;
    string tableName;
    string pattern;
    vector<string> alphabet;
    vector<string> queries;
    string outputTableName;
    string timeWindowType;
    string twColumn;
    int twSize;
    int twStepSize;
    int sleepFor;
    bool benchmark;
    int benchmarkTestNum;
};

void printHelp();
CLIParams parseCommandLine(int argc, char* argv[]);

#endif
