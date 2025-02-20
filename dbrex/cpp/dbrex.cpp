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

void run(string& catalogAndSchema, const string& tableName, const string& outputTable, const vector<string>& outputTableColumns, const string& twt, 
const string& twc, const int& tws, const int& sleepFor, map<string, string>& defineConditions, const json& dfaData, SQLUtils& utils, TrinoRestClient& client) {
    bool firstLoop = true;
    // --- get initial values for the time window determination ---
    DataTable twcValues_dt = client.executeQuery("SELECT " + twc + " FROM " + catalogAndSchema + "." + tableName);
    vector<int> twcValues;
    for(Row& row : twcValues_dt) {
        twcValues.push_back(get<int>(row[0])); 
    }
    vector<int> currentTimeWindow = getCurrentTimeWindow();                 // stores start and end index of the current time window, initialised with {-1, -1}
    int i = (currentTimeWindow[0] == -1) ? 0 : currentTimeWindow[0];        // i is 0 when first execution (= initialisation of currentTimeWindow with -1), else the last processed value 
    string timeWindowCondition;    
    // ------------------------------------------------------------
    while(running) {
        this_thread::sleep_for(chrono::milliseconds(sleepFor));             // save ressources
        // --- update values for time window in case of new insertions ---
        DataTable newTwcValues = client.executeQuery("SELECT " + twc + " FROM " + catalogAndSchema + "." + tableName + " WHERE " + twc + " > " + to_string(twcValues[twcValues.size()-1]));
        for(Row& row : newTwcValues) {
            twcValues.push_back(get<int>(row[0])); 
        }
        // ---------------------------------------------------------------

        while(currentTimeWindow[1] < twcValues[twcValues.size()-1]) {   // example: 15 rows, tw size: 5: ..., 8-13, 9-14, 10-15, stop then because everything is covered
            cout << "Executing (" + to_string(i+1) + "/" + to_string(twcValues.size()-min(tws, static_cast<int>(twcValues.size())-1)) + ")..." << endl;
            
            currentTimeWindow[0] = twcValues[i];
            currentTimeWindow[1] = twcValues[min(i+tws, static_cast<int>(twcValues.size()-1))];     // min if user's tws is greater than num of rows
            i++;

            for(const auto& [state, symbol, _, partialMatchTableName] : utils.transitions) {
                string condition = defineConditions[symbol];
                vector<string> partialMatch = utils.metadata[partialMatchTableName]["predecessor_symbols"];
                
                timeWindowCondition = utils.getTimeWindowCondition(twc, currentTimeWindow);     // gets appended to the insert statement to simulate time windows

                bool isStartState = state == dfaData["start_state"].get<string>();
                string predecessorTableName = utils.metadata[partialMatchTableName]["predecessor"];
        
                utils.insertIntoTable(partialMatchTableName, symbol, predecessorTableName, condition, timeWindowCondition, partialMatch, isStartState);
            }
            saveCurrentTimeWindow(currentTimeWindow);

            bool lastExec = i+tws-1 == twcValues.size()-1;
            if(lastExec) {
                utils.createOutputTable(outputTable);
                firstLoop = true;
            }
        }
        if(firstLoop) {
            cout << "Idling..." << endl;
            firstLoop = false;
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
    vector<string> outputColumns = params.outputTableColumns;
    string twt = params.timeWindowType;
    string twc = params.twColumn;
    int tws = params.twSize;
    int sleepFor = params.sleepFor;

    // properly handle docker container termination 
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // ----- initialise Python API and Trino (SQL) API -----
    string pythonBaseUrl = "http://127.0.0.1:8000";
    string trinoBaseUrl = "http://host.docker.internal:8080";

    httplib::Client pythonClient(pythonBaseUrl);
    string pythonUrl = "/create_dfa?regex=" + replaceWhitespace(rawRegEx);
    auto response = pythonClient.Get(pythonUrl.c_str());
    json dfaData = json::parse(response->body);
    // plot DFA in terminal
    printDfa(dfaData);

    TrinoRestClient client(trinoBaseUrl);
    // -----------------------------------------------------

    SQLUtils utils(tableName, params.outputTableName, catalogAndSchema, dfaData, client);   // initialise utils
    vector<string> outputTableColums = params.outputTableColumns;    

    // main execution; caution: loops until termination
    run(catalogAndSchema, tableName, outputTable, outputTableColums, twt, twc, tws, sleepFor, defines, dfaData, utils, client);

    return 0;
}