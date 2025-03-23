#ifndef BENCHMARK_UTILS_H
#define BENCHMARK_UTILS_H

#include <csignal>
#include <atomic>
#include <string>
#include <random>
#include <tuple>
#include <sstream>
#include <iomanip>
#include <ctime> 
#include <chrono>
#include "trino_connect.h"

using namespace std;
using namespace chrono;

void deleteSQLUtilsJSONs();
void appendToCSV(const string& filePath, const vector<string>& row = {}, const vector<string>& headers = {});
string getTimestamp();

class BenchmarkUtils {
public:
    // avoid const class variables to avoid const method declaration
    string& originalTableName;
    string& benchmarkTableName;
    string& catalogAndSchema;
    TrinoRestClient& client;
    DataTable columns;

    // Constructor
    BenchmarkUtils(string& originalTableName, string& benchmarkTableName, string& catalogAndSchema, TrinoRestClient& client);

    void getColumns();
    void createBenchmarkTable();
    void deleteBenchmarkTableEntries(const string& tableName);
    void insertBatch(const string& column, const int& batchSize, const int& startIndex);
    size_t getUsedMemory(const SQLUtils utils);
    double getThroughput(const SQLUtils utils, const int& batchSize, duration<double> latency);
    duration<double> benchmarkSelfJoins(const size_t& testNum, const size_t& twNum);
    void benchmarkMR(const SQLUtils utils, const string& benchmarkTableName, const string& pattern, const string& column, 
    const map<string, string>& defines, const pair<vector<string>, vector<string>>& firstAndLastSymbols, const int& tws, const int& batchSize);
};

#endif 