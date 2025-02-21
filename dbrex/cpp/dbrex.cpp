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

void run(string& catalogAndSchema, const string& tableName, const string& outputTable, const string& twt, 
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
    string catalog = params.catalog;
    string schema = params.schema;
    string catalogAndSchema = catalog + "." + schema;
    string tableName = params.tableName;
    string rawRegEx = params.pattern;                       // ! explicit use of parenthesises, example: "A B | C" evaluates to "(A B) | C" --> instead make A (B | C)
    map<string, string> defines;                            // alphabet and queries combined
    for(size_t i = 0; i < params.alphabet.size(); i++) {
        defines[params.alphabet[i]] = params.queries[i];
    }
    string outputTable = params.outputTableName;
    string twt = params.timeWindowType;
    string twc = params.twColumn;
    int tws = params.twSize;
    int sleepFor = params.sleepFor;

    // properly handle termination 
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // ----- initialise Python API and Trino (SQL) API -----
    string pythonBaseUrl = "http://127.0.0.1:8000";
    string trinoBaseUrl = "http://127.0.0.1:8080";

    httplib::Client pythonClient(pythonBaseUrl);
    string pythonUrl = "/create_dfa?regex=" + replaceWhitespace(rawRegEx);
    auto response = pythonClient.Get(pythonUrl.c_str());
    json dfaData = json::parse(response->body);
    // plot DFA in terminal
    printDfa(dfaData);

    TrinoRestClient client(trinoBaseUrl);
    // -----------------------------------------------------

    SQLUtils utils(tableName, params.outputTableName, catalogAndSchema, dfaData, client);   // initialise utils

    // main execution; caution: loops until termination
    run(catalogAndSchema, tableName, outputTable, twt, twc, tws, sleepFor, defines, dfaData, utils, client);

    return 0;
}