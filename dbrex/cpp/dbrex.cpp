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

// temp variablen, die später wo anders oder per cli
string twt = "countbased";
// string schema = "memory.pattern";
string tableName = "CrimeTable";
string timeWindowColumnName = "id";
int timeWindowSize = 3;
// ---------

void run(string& schema, json& dfaData, string& outputTable, vector<string>& outputTableColumns, map<string, string>& defineConditions, SQLUtils& utils, TrinoRestClient& client) {
    // ---------- initial setup ----------
    // string twt = getenv("twt");
    string twc = getenv("twc");
    twc = "id"; // temp
    // int tws = stoi(getenv("tws"));
    vector<int> currentTimeWindow = {0, timeWindowSize - 1};   // for countbased time windows
    string startTimestamp; string endTimestamp;                // for time based time windows
    string timeWindowCondition;    
    // -----------------------------------

    while(running) {
        this_thread::sleep_for(chrono::milliseconds(100));
        int numRows = get<int>(client.executeQuery("SELECT COUNT(*) FROM " + schema + "." + tableName)[0][0]);
        // todo evtl 1 sek timer oder so um ressourcen zu sparen, aber anwendungsfall ja evtl wo oft updates sind

        while(currentTimeWindow[0] < numRows) {
            for(const auto& [state, symbol, _, partialMatchTableName] : utils.transitions) {
                string condition = defineConditions[symbol];
                // condition = "C.crimetype = 'Credit Card Fraud' AND C.lat BETWEEN T.lat - 0.005";
                vector<string> partialMatch = utils.metadata[partialMatchTableName]["predecessor_symbols"];
                // partialMatch = {"C"};

                const string combinedColumnName = schema + "." + tableName + "." + timeWindowColumnName;
                
                timeWindowCondition = utils.getTimeWindowCondition(twt, twc, currentTimeWindow);

                bool isStartState = state == dfaData["start_state"].get<string>();
                string predecessorTableName = utils.metadata[partialMatchTableName]["predecessor"];
        
                utils.insertIntoTable(partialMatchTableName, symbol, predecessorTableName, condition, timeWindowCondition, partialMatch, isStartState);
            }

            currentTimeWindow[0] += 1;
            currentTimeWindow[1] += 1;
        }

    }
}

int main(int argc, char* argv[]) {  
    /*
    TODOS
    Check data types --> if something like primary key: remove, because otherwise multiple columns --> error
    What if several equal symbols, e.g. AAB, which column replacement for As?
    No variable dependencies for variables after current variable, e.g. AB: A must not depend on B
    change host to docker.host.internal
    */

    // if (argc == 1) {
    //     std::cout << "No arguments provided." << endl;
    //     printHelp();
    //     return 1;
    // }

    CLIParams params = parseCommandLine(argc, argv);
    // .c_str() to convert from string to const char*
    setenv("twt", params.timeWindowType.c_str(), 1);    
    setenv("twc", params.twColumn.c_str(), 1);
    setenv("wts", to_string(params.twSize).c_str(), 1);

    // properly handle docker container termination 
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // plot docker image name in terminal
    printName();  

    // const string rawRegEx = params.pattern; // ! explicit use of parenthesises, example: "A B | C" evaluates to "(A B) | C" --> instead make A (B | C)
    const string rawRegEx = "T C M";
    // const string rawRegEx = "A B";

    // ----- initialise Python API and Trino (SQL) API -----
    string host = "http://127.0.0.1";
    string pythonBaseUrl = host + ":" + "8000";
    string trinoBaseUrl = host + ":" + "8080";

    httplib::Client pythonClient(pythonBaseUrl);
    string pythonUrl = "/create_dfa?regex=" + replaceWhitespace(rawRegEx);
    auto response = pythonClient.Get(pythonUrl.c_str());
    json dfaData = json::parse(response->body);
    // plot DFA in terminal
    printDfa(dfaData);

    TrinoRestClient client(trinoBaseUrl);
    // -----------------------------------------------------

    string schema = "memory.pattern";
    SQLUtils utils(tableName, params.outputTableName, schema, dfaData, client);   // initialise utils
    vector<string> outputTableColums = params.outputTableColumns;    
    map<string, string> defines;    

    for(int i = 0; i < defines.size(); i++) {
        defines[params.alphabet[i]] = params.queries[i];
    }

    //temp
    defines = {
        {"T", "T.crimetype = 'Theft'"},
        {"C", "C.crimetype = 'Credit Card Fraud'"},
        {"M", "M.crimetype = 'Money Laundering'"}
    };
    //temp
    string outputTable = tableName;

    // main execution; caution: loops until termination
    run(schema, dfaData, outputTable, outputTableColums, defines, utils, client);

    return 0;
}
