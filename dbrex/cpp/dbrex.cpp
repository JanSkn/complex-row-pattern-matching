#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <set>
#include <format>
#include <cstdlib>
#include <variant>
#include <tuple>
#include "json.hpp"
#include "httplib.h"
#include "trino_connect.h"
#include "utils.h"
#include "topological_sort.h"
#include "cli.h"

using namespace std;
using json = nlohmann::json;   

void runDBrex(string& catalogAndSchema, const string& tableName, const string& outputTable, const string& twt, 
const string& twc, const int& tws, const int& sleepFor, map<string, string>& defineConditions, const json& dfaData, SQLUtils& utils, TrinoRestClient& client) {
    bool firstIdleLoop = true;

    while(running) {
        this_thread::sleep_for(chrono::milliseconds(sleepFor));             // save ressources
        // after one execution, all elements until are processed until rows with value > processedIndex
        int processedIndex = utils.getProcessedIndex(twc);
        int newIndex = get<int>(client.executeQuery("SELECT max("+twc+") FROM " + catalogAndSchema + "." + tableName)[0][0]);

        if(processedIndex < newIndex) {             
            cout << "Executing..." << endl;
         
            for(const auto& [state, symbol, _, partialMatchTableName] : utils.transitions) {
                string condition = defineConditions[symbol];
                vector<string> partialMatch = utils.metadata[partialMatchTableName]["predecessor_symbols"];
                
                bool isStartState = state == dfaData["start_state"].get<string>();
                string predecessorTableName = utils.metadata[partialMatchTableName]["predecessor"];
        
                utils.insertIntoTable(partialMatchTableName, symbol, predecessorTableName, condition, processedIndex, twc, tws, partialMatch, isStartState);
            }

            utils.saveProcessedIndex(newIndex);
            
            utils.insertIntoOutputTable(outputTable);
            
            firstIdleLoop = true;
        }

        if(firstIdleLoop) {
            cout << "Idling..." << endl;
            firstIdleLoop = false;
        }
    }
}

void benchmarkDBrex(string& catalogAndSchema, const string& tableName, const string& outputTable, const int& processedIndex, 
const string& column, const int& tws, map<string, string>& defineConditions, const json& dfaData, SQLUtils& utils, TrinoRestClient& client) {    
    for(const auto& [state, symbol, _, partialMatchTableName] : utils.transitions) {
        string condition = defineConditions[symbol];
        vector<string> partialMatch = utils.metadata[partialMatchTableName]["predecessor_symbols"];
        
        bool isStartState = state == dfaData["start_state"].get<string>();
        string predecessorTableName = utils.metadata[partialMatchTableName]["predecessor"];

        utils.insertIntoTable(partialMatchTableName, symbol, predecessorTableName, condition, processedIndex, column, tws, partialMatch, isStartState);
    }    
    utils.insertIntoOutputTable(outputTable);
}