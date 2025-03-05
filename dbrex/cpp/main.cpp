#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <set>
#include <cstdlib>
#include <format>
#include <cstdlib>
#include <variant>
#include <tuple>
#include <chrono>
#include <thread>
#include "json.hpp"
#include "httplib.h"
#include "trino_connect.h"
#include "utils.h"
#include "topological_sort.h"
#include "cli.h"
#include "dbrex.h"
#include "benchmark_utils.h"

using namespace std;
using namespace chrono;
using json = nlohmann::json;   

string pythonBaseUrl = "http://127.0.0.1:8000";
string trinoBaseUrl = "http://host.docker.internal:8080"; 
httplib::Client pythonClient(pythonBaseUrl);
TrinoRestClient client(trinoBaseUrl);

void benchmark() {
    cout << "Starting benchmarking..." << endl;
    // --- Setup ---
    deleteSQLUtilsJSONs(); // initial cleaning
    
    const string benchmarkResultsDirectory = "benchmarks";

    string catalogAndSchema = "postgres.public";
    string tableName = "crimedata";
    string benchmarkTableName = tableName + "Benchmark";
    string outputTableName = benchmarkTableName + "Result";
    string column = "id";
    vector<int> differentTws = {50, 100, 200, 500, 10000};

    vector<string> patterns = {
        "M D H",
        "T C M",
        "H T V",
        "T C",
        "H (T | V) M C"
    };
    vector<map<string, string>> defines = {
        {   // High-risk neighbourhood cluster, unconditional defines
            {"M", "M.neighbourhood = 'Montréal-Nord'"},
            {"D", "D.neighbourhood = 'Downtown Montreal'"},
            {"H", "H.neighbourhood = 'Centre-Sud'"}
        },
        {   // Close sequence of offences, conditional defines
            {"T", "T.category = 'Theft in / from a motor vehicle'"},
            {"C", "C.category = 'Confirmed Theft'"},
            {"M", "M.category = 'Mischief' AND T.city = C.city AND C.city = M.city AND DATE_DIFF('day', T.date, M.date) <= 7"}
        },
        {   // Geographically related property offences, conditional defines
            {"H", "H.category = 'Home Invasion'"},
            {"T", "T.category = 'Theft in / from a motor vehicle'"},
            {"V", "V.category = 'Motor vehicle theft' AND ABS(H.longitude - V.longitude) < 0.02"}
        },
        {   // Multi-city cross-Neighbourhood offences, conditional defines
            {"T", "T.category = 'Theft in / from a motor vehicle' AND T.city IN ('MONTREAL', 'MONTREAL-NORD')"},
            {"C", "C.category = 'Confirmed Theft' AND C.city IN ('SAINT-LEONARD', 'SAINT-LAURENT') AND T.neighbourhood != C.neighbourhood"}
        },
        {   //  Organised criminal activities or specific crime series, conditional defines
            {"H", "H.category = 'Home Invasion'"},
            {"T", "T.category = 'Theft in / from a motor vehicle' AND T.city IN ('MONTREAL', 'MONTREAL-NORD')"},
            {"V", "V.category = 'Motor vehicle theft' AND (V.neighbourhood IN ('Downtown Montreal', 'Mercier') OR T.neighbourhood IN ('Plateau Mont-Royal', 'Centre-Sud'))"},
            {"M", "M.category = 'Mischief' AND ABS(H.longitude - M.longitude) < 0.02 AND M.year = H.year"},
            {"C", "C.category = 'Confirmed Theft' AND C.city IN ('SAINT-LEONARD', 'SAINT-LAURENT') AND M.neighbourhood != C.neighbourhood AND DATE_DIFF('day', C.date, H.date) <= 90"}
        }
    };
    // for MR to determine first and last elements of a pattern for time window
    // a pattern can have more than one first and last element, example (A|B)(C|D)
    vector<pair<vector<string>, vector<string>>> firstAndLastSymbols = {  
        {{"M"}, {"H"}}, 
        {{"T"}, {"M"}},
        {{"H"}, {"V"}},
        {{"T"}, {"C"}},
        {{"H"}, {"C"}}
    };
    size_t numTests = patterns.size();
    int numExecutionsPerTest = 3;
    int insertBatchSize = 2;
    int maxIndex = get<int>(client.executeQuery("SELECT max("+column+") FROM " + catalogAndSchema + "." + tableName)[0][0]);
    BenchmarkUtils butils(tableName, benchmarkTableName, catalogAndSchema, client);
    butils.deleteBenchmarkTableEntries(benchmarkTableName);

    // --- generate all DFAs previously due to API latency ---
    vector<json> allDfaData;
    vector<string> whitespaceReplacedPatterns;
    for (const auto& pattern : patterns) {
        string wrPattern = replaceWhitespace(pattern);
        whitespaceReplacedPatterns.push_back(wrPattern);
        string pythonUrl = "/create_dfa?regex=" + wrPattern;
        auto response = pythonClient.Get(pythonUrl.c_str());
        if (response && response->status == 200) {
            allDfaData.push_back(json::parse(response->body));
        } else {
            cerr << "Error generation DFA for pattern: " << pattern << endl;
            exit(1);
        }
    }

    // --- Create csv initially ---
    vector<string> headers = {"algorithm", "timestamp", "total_elements", "delta_elements", "time_window_size", "latency", "delta_latency", "throughput", "memory", "#test"};
    for (size_t i = 0; i < numTests; ++i) {
        const string filePath = benchmarkResultsDirectory + "/" + whitespaceReplacedPatterns[i] + ".csv";
        ifstream fileCheck(filePath);
        if (!fileCheck.good()) {
            appendToCSV(filePath, {}, headers);
        }
    }

    // --- main execution ---
    for (int j = 0; j < numExecutionsPerTest; ++j) {    // execute each test n times
        for (int tws : differentTws) {                  // for each test, try different time window sizes
            duration<double> dbrexTotalLatency(0.0);
            duration<double> mrTotalLatency(0.0);

            for (size_t i = 0; i < numTests; ++i) {
                const string& benchmarkFilePath = benchmarkResultsDirectory + "/" + whitespaceReplacedPatterns[i] + ".csv";
                SQLUtils utils(benchmarkTableName, outputTableName, catalogAndSchema, allDfaData[i], client);

                for (int processedIndex = 0; processedIndex < maxIndex; processedIndex += insertBatchSize) {
                    // simulate insertions
                    butils.insertBatch(column, insertBatchSize, processedIndex);
                    string testId = to_string(processedIndex) + "_" + to_string(tws);   // as the same test gets executed n times -> group them together

                    // --- DBrex benchmarking ---
                    auto dbrexStart = high_resolution_clock::now();
                    string dbrexTimestamp = getTimestamp();
                    benchmarkDBrex(catalogAndSchema, benchmarkTableName, outputTableName, processedIndex, column, tws, defines[i], allDfaData[i], utils, client);
                    auto dbrexEnd = high_resolution_clock::now();
                    
                    duration<double> dbrexLatency = dbrexEnd - dbrexStart;
                    dbrexTotalLatency += dbrexLatency;
                    size_t memory = butils.getUsedMemory(utils);
                    double throughput = butils.getThroughput(utils, insertBatchSize, dbrexLatency);
                    
                    // store to csv
                    vector<string> dbrexRow = {"dbrex", dbrexTimestamp, to_string(processedIndex), to_string(insertBatchSize), to_string(tws), to_string(dbrexTotalLatency.count()),
                    to_string(dbrexLatency.count()), to_string(throughput), to_string(memory), testId};
                    appendToCSV(benchmarkFilePath, dbrexRow, {});
                    
                    // --- MATCH_RECOGNIZE benchmarking ---
                    auto mrStart = high_resolution_clock::now();
                    string mrTimestamp = getTimestamp();
                    butils.benchmarkMR(utils, benchmarkTableName, patterns[i], column, defines[i], firstAndLastSymbols[i], tws, insertBatchSize);
                    auto mrEnd = high_resolution_clock::now();
                    
                    duration<double> mrLatency = mrEnd - mrStart;
                    mrTotalLatency += mrLatency;
                    
                    // store to csv
                    vector<string> mrRow = {"MATCH_RECOGNIZE", mrTimestamp, to_string(processedIndex), to_string(insertBatchSize), to_string(tws), to_string(mrTotalLatency.count()),
                    to_string(mrLatency.count()), to_string(-1), to_string(-1), testId};
                    appendToCSV(benchmarkFilePath, mrRow, {});
                    // ------------------------------------
                }

                // Clean up after all iterations for this test
                deleteSQLUtilsJSONs();
                butils.deleteBenchmarkTableEntries(benchmarkTableName);
                butils.deleteBenchmarkTableEntries(outputTableName);
            }
        }
    }

    system("python3 ./benchmarks/benchmarks.py"); // execute evaluation
    cout << "Benchmarking finished successfully." << endl;
}

void run(CLIParams& params) {
    // properly handle termination 
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    string catalog = params.catalog;
    string schema = params.schema;
    string catalogAndSchema = catalog + "." + schema;
    string tableName = params.tableName;
    string rawRegEx = params.pattern;                       // ! explicit use of parentheses, example: "A B | C" evaluates to "(A B) | C" --> instead make A (B | C)
    map<string, string> defines;                            // alphabet and queries combined
    for(size_t i = 0; i < params.alphabet.size(); i++) {
        defines[params.alphabet[i]] = params.queries[i];
    }
    string outputTable = params.outputTableName;
    string twt = params.timeWindowType;
    string twc = params.twColumn;
    int tws = params.twSize;
    int sleepFor = params.sleepFor;

    httplib::Client pythonClient(pythonBaseUrl);
    string pythonUrl = "/create_dfa?regex=" + replaceWhitespace(rawRegEx);
    auto response = pythonClient.Get(pythonUrl.c_str());
    json dfaData = json::parse(response->body);
    // plot DFA in terminal
    printDfa(dfaData);
    SQLUtils utils(tableName, params.outputTableName, catalogAndSchema, dfaData, client);   // initialise utils

    // caution: loops until termination
    runDBrex(catalogAndSchema, tableName, outputTable, twt, twc, tws, sleepFor, defines, dfaData, utils, client);
}

int main(int argc, char* argv[]) {  
    // plot docker image name in terminal
    printName();  

    ifstream configFile("dbrex/config/args.json");

    if (!configFile && argc == 1) {
        std::cout << "No args.json or arguments provided.\n\n";
        printHelp();
        return 1;
    }

    CLIParams params = parseCommandLine(argc, argv);

    if(params.benchmark) {
        benchmark();
    } else {
        run(params);
    }

    return 0;
}