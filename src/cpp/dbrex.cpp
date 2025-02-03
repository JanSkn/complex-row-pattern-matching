#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <set>
#include <format>
#include <cstdlib>
#include <variant>
#include "json.hpp"
#include "httplib.h"
#include "trino_connect.h"
#include "utils.h"
#include "regex_parser.h"
#include "topological_sort.h"
#include "cli.h"

using namespace std;
using json = nlohmann::json;   

// temp variablen, die später wo anders oder per cli
string twt = "countbased";
// string schema = "memory.pattern";
string tableName = "CrimeTable";
string timeWindowColumnName = "id";
int stepSize = 1;
int timeWindowSize = 3;
// ---------

void run(string& schema, json& dfaData, vector<string>& outputTableColumns, unordered_map<string, string>& defineClauses, int& maxRegExSymbols, SQLUtils& utils, TrinoRestClient& client) {
    // ---------- initial setup ----------
    unordered_map<string, unordered_map<string, string>> dfa = parseGraph(dfaData["dfa_dict"]);
    string startState = dfaData["start_state"];
    vector<string> order = topologicalSort(dfa, startState);
    vector<pair<string, string>> orderedTransitions = traverseGraph(dfa, order);
    int processedIndex = 0;   // everything until this index is already processed
    vector<int> currentTimeWindow = {0, timeWindowSize - 1};   // for countbased time windows
    string timeWindowWhereClause;    

    for(const auto& [state, symbol] : orderedTransitions) {     // create tables in the beginning to avoid API call every loop execution
        string tableName = utils.metadata[state][symbol]["table_name"];
        utils.createTable(tableName);
    }
    // -----------------------------------

    // while(running) {
        // todo nochmal schauen bei processIndex ab welcher id wirklich bei mehreren nach den joins und so
        // string processCondition = "WHERE s0_id >= " + processedIndex; // only process after this id because everything else is already processed
        // bool timeWindowCondition = getTimeWindowCondition(twt, tableNameWithSchema, currentTimeWindow);
        
        int temploop = 0;
        while(temploop < 3) {
            temploop++;

            for(const auto& [state, symbol] : orderedTransitions) {
                string partialMatchTableName = utils.metadata[state][symbol]["table_name"];
                string tempcond = "T.crimetype = 'Theft' AND C.crimetype = 'Credit Card Fraud' AND C.lat BETWEEN T.lat - 0.005";
                
                const string combinedColumnName = schema + "." + tableName + "." + timeWindowColumnName;

                if(twt == "countbased") {
                    timeWindowWhereClause = "WHERE " + combinedColumnName + " BETWEEN " + to_string(currentTimeWindow[0]) + " AND " + to_string(currentTimeWindow[1]);
                } else {
                    // todo
                    string startTimestamp = "";
                    string endTimestamp = "";
                    timeWindowWhereClause = "WHERE " + combinedColumnName + " >= " + startTimestamp + " AND " + combinedColumnName + " <= " + endTimestamp;
                }
                bool isStartState = state == dfaData["start_state"];

                tempcond += " " + timeWindowWhereClause;
                // string conditionWithTWClause = defineClauses[symbol] + " " + timeWindowWhereClause;

                if(isStartState) {
                    utils.insertIntoTable(partialMatchTableName, symbol, "", "", tempcond, isStartState);
                } else {
                    for(const auto& [predecessorTableName, predecessorSymbol] : utils.findPredecessors(state)) {
                        utils.insertIntoTable(partialMatchTableName, symbol, predecessorTableName, predecessorSymbol, tempcond, isStartState);
                    }
                }
            }

            currentTimeWindow[0] += stepSize;
            currentTimeWindow[1] += stepSize;
        }
        processedIndex += timeWindowSize; // todo stimmt noch nicht so
}

int main(int argc, char* argv[]) {  
    if (argc == 1) {
        std::cout << "No arguments provided.\n";
        printHelp();
        return 1;
    }

    CLIParams params = parseCommandLine(argc, argv);
    // .c_str() to convert from string to const char*
    setenv("twt", params.timeWindowType.c_str(), 1);    
    setenv("twc", params.twColumn.c_str(), 1);
    setenv("wts", to_string(params.twSize).c_str(), 1);
    setenv("twss", to_string(params.twStepSize).c_str(), 1);

    // properly handle docker container termination 
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // plot docker image name in terminal
    printName();  

    const string rawRegEx = params.pattern; // ! explicit use of parenthesises, example: "A B | C" evaluates to "(A B) | C" --> instead make A (B | C)

    RegexParser parser;
    int numRegExSymbols = parser.calculateMaxSymbolLength(rawRegEx);    // determine length to derive needed num of columns per SQL table

    // ----- initialise Python API and Trino (SQL) API -----
    string host = "http://127.0.0.1";
    string pythonBaseUrl = host + ":" + "8000";
    string trinoBaseUrl = host + ":" + "8080";

    httplib::Client pythonClient(pythonBaseUrl);
    string pythonUrl = "/create_dfa?regex=" + replaceWhitespace(rawRegEx);
    auto response = pythonClient.Get(pythonUrl.c_str());
    json dfaData = json::parse(response->body);

    TrinoRestClient client(trinoBaseUrl);
    // -----------------------------------------------------

    string schema = "memory.pattern";
    SQLUtils utils(tableName, schema, numRegExSymbols, dfaData, client);   // initialise utils
    vector<string> outputTableColums = params.outputTableColums;    
    unordered_map<string, string> defines;

    for(int i = 0; i < defines.size(); i++) {
        defines[params.alphabet[i]] = params.queries[i];
    }

    // main execution; caution: loops until termination
    run(schema, dfaData, outputTableColums, defines, numRegExSymbols, utils, client);

    // TODO metadata res tabelle von params.outputTable

    return 0;
}
