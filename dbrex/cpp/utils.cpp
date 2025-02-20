#include <csignal>
#include <atomic>
#include <string>
#include <random>
#include <sstream>
#include <tuple>
#include <fstream>
#include <iomanip>
#include <set>
#include <algorithm>
#include <regex>
#include <cctype>
#include "utils.h"
#include "topological_sort.h"
#include "cli.h"

using namespace std;

atomic<bool> running(true);

void signalHandler(int signal) {
    if(signal == SIGINT || signal == SIGTERM) {
        cout << "Stopping..." << endl;
        running = false; 
    }
}

void printName() {
    ifstream configFile("config.json");
    json config;
    configFile >> config;

    cout << "\n";
    cout << " _____    ____     _____     _____    __  __\n";
    cout << "|  _  \\  |  _ \\   |  _  \\   |  ___|   \\ \\/ /\n";
    cout << "| | | |  | |_) |  | |_) |   | |__      \\  / \n";
    cout << "| | | |  |  _ <   |  _ <    |  __|     /  \\ \n";
    cout << "| |/ /   | |_) |  | | \\ \\   | |___    / /\\ \\\n";
    cout << "|___/    |____/   |_|  \\_\\  |_____|  /_/  \\_\\\n";
    cout << "\n";
    cout << "Author: " << config["author"].get<string>() << "\n";
    cout << "v" << config["version"].get<string>() << " - " << config["github"].get<string>() << "\n";
    cout << "\n";
    cout << "Hint: You can see the magic happen at http://127.0.0.1:8080/ui/#\n";
    cout << "\n";
}

void printDfa(const json& dfaData) {
    cout << "Generated DFA: " << endl;
    cout << "Start state: " << dfaData["start_state"] << ", final states: " << dfaData["final_states"] << endl;
    cout << "digraph DFA {\n";
    cout << "    rankdir=LR;\n";

    for (const auto& [state, transitions] : dfaData["dfa_dict"].items()) {
        for (const auto& [symbol, nextState] : transitions.items()) {
            cout << "    \"" << state << "\" -> " << nextState << " [transition=\"" << symbol << "\"];\n";
        }
    }

    cout << "}" << endl;
}

string replaceWhitespace(const string& input) {
    string result = input;
    for(size_t i = 0; i < result.length(); i++) {
        if (result[i] == ' ') {
            result.replace(i, 1, "%20"); 
            i += 2;
        }
    }
    return result;
}

// simulate uuid with "_" instead of "-" due to SQL naming restrictions
string generateUuid() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);

    stringstream ss;
    ss << hex << setfill('0');
    ss << setw(8) << dis(gen) << "_";
    ss << setw(4) << ((dis(gen) & 0x0FFF) | 0x4000) << "_";
    ss << setw(4) << ((dis(gen) & 0x3FFF) | 0x8000) << "_";
    ss << setw(4) << (dis(gen) & 0xFFFF) << "_";
    ss << setw(12) << dis(gen) << dis(gen);

    return ss.str();
}

vector<string> splitString(const string& input, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(input);

    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

const string currentTimeWindowFilePath = "dbrex/data/currentTimeWindow.json";

vector<int> getCurrentTimeWindow() {
    vector<int> timeWindow;

    ifstream infile(currentTimeWindowFilePath);
    if (infile.is_open()) {
        json j;
        infile >> j;
        infile.close();
        timeWindow = j["time_window"].get<vector<int>>();
    } else {
        timeWindow = {-1, -1};
        saveCurrentTimeWindow(timeWindow);
    }

    return timeWindow;
}

void saveCurrentTimeWindow(const vector<int>& timeWindow) {
    json j;
    j["time_window"] = timeWindow;

    ofstream outfile(currentTimeWindowFilePath);

    outfile << j.dump(4);
    outfile.close();
}

// Constructor
SQLUtils::SQLUtils(string& originalTableName, string& outputTableName, string& catalogAndSchema, json& dfaData, TrinoRestClient& client)
    : originalTableName(originalTableName), outputTableName(outputTableName), catalogAndSchema(catalogAndSchema), dfaData(dfaData), client(client) {
        this->getColumns();
        this->setup();
    }

void SQLUtils::setup() {
    json metadata;
    json transitions;

    ifstream checkTransitionsFile("dbrex/data/transitions.json");
    ifstream checkMetadataFile("dbrex/data/metadata.json");
    // store both to preserve states when restarting
    // both depend on each other, storing both is important
    if(checkTransitionsFile && checkMetadataFile) {
        checkMetadataFile >> metadata;
        checkTransitionsFile >> transitions;
        cout << "Loaded existing metadata." << endl;
    } else {
        string startState = this->dfaData["start_state"];
        unordered_map<string, unordered_map<string, string>> dfa = parseGraph(this->dfaData["dfa_dict"]);
        vector<string> order = topologicalSort(dfa, startState);
        vector<tuple<string, string, string>> orderedTransitions = traverseGraph(dfa, order);
        vector<tuple<string, string, string, string>> extendedTransitions;  // orderedTransitions with table name -> {{startState, transitionSymbol, nextState, tableName}, ...}
    
        // states and their predecessors (predecessors are tuples of {startState, transitionSymbol, nextState})
        map<string, vector<vector<string>>> predecessors = this->getPredecessors(orderedTransitions);

        for(const auto& [state, symbol, nextState] : orderedTransitions) {
            vector<vector<string>> predecessorPaths = this->getPredecessorPaths(state, predecessors);
            
            if(state == startState) {
                string tableName = this->originalTableName + "_" + generateUuid();
                extendedTransitions.push_back({state, symbol, nextState, tableName});
                // start state path only has one symbol
                metadata[tableName]["symbols"] = {symbol};
                metadata[tableName]["predecessor_symbols"] = json::array();
                metadata[tableName]["num_symbols"] = 1;
            } else {
                for(vector<string> path : predecessorPaths) {
                    string tableName = this->originalTableName + "_" + generateUuid();
                    extendedTransitions.push_back({state, symbol, nextState, tableName});
                    vector<string> partialMatchPath = path;
                    partialMatchPath.push_back(symbol); // current symbol
                    metadata[tableName]["symbols"] = partialMatchPath;
                    metadata[tableName]["predecessor_symbols"] = path;
                    metadata[tableName]["num_symbols"] = partialMatchPath.size();
                }
            }
        }
        this->getPredecessorTables(metadata); // assign each table its predecessor
        ofstream metadataFile("dbrex/data/metadata.json");
        metadataFile << metadata.dump(4);

        transitions["dfa"] = dfa;
        transitions["state_order"] = order;
        transitions["extended_transitions"] = extendedTransitions;
        ofstream transitionsFile("dbrex/data/transitions.json");
        transitionsFile << transitions.dump(4);

        for(const auto& [_, __, ___, tableName] : extendedTransitions) {
            this->createTable(tableName, metadata[tableName]["num_symbols"]);
        }
    }

    this->metadata = metadata;
    this->transitions = transitions["extended_transitions"];
}

void SQLUtils::getColumns() {
    vector<string> splittedCatalogAndSchema = splitString(catalogAndSchema, '.');
    string catalog = splittedCatalogAndSchema[0];
    string schema = splittedCatalogAndSchema[1];
    string originalTableNameToLower = originalTableName;
    transform(originalTableNameToLower.begin(), originalTableNameToLower.end(), originalTableNameToLower.begin(), ::tolower);
    string getColumnsQuery = "SELECT column_name, data_type FROM " + catalog + ".information_schema.columns WHERE table_schema = '"+ schema + "' AND table_name = '" + originalTableNameToLower + "'";
    this->columns = this->client.executeQuery(getColumnsQuery);
}

/* 
Returns map of states and their predecessor transitions.
Example ("0" is start state and has no predecessors):
    {
        "1":  { {"0", "A", "1"}, {"2", "A", "1"} },
        "2":  { {"0", "B", "2"} },
        "3": { {"2", "C", "3"} },
        "4": { {"3", "D", "4"} }
    }
*/
map<string, vector<vector<string>>> SQLUtils::getPredecessors(const vector<tuple<string, string, string>>& transitions) {
    // transitions: {{startState, transitionSymbol, nextState}, ...}
    map<string, vector<vector<string>>> predecessors;

    for(const auto& [state, symbol, nextState] : transitions) {
        predecessors[nextState].push_back({state, symbol, nextState}); 
    }
    return predecessors;
}

/*
Recursively determine all paths to the state.
Example:
    {{"A", "B"}, {"A", "B", "C"}}
*/
vector<vector<string>> SQLUtils::getPredecessorPaths(const string& state, const map<string, vector<vector<string>>>& predecessors) {
    if(predecessors.find(state) == predecessors.end()) {
        return {{}};
    }

    vector<vector<string>> paths;
    
    for(const auto& transition : predecessors.at(state)) {
        string prevState = transition[0];  
        string symbol = transition[1];     

        vector<vector<string>> prevPaths = getPredecessorPaths(prevState, predecessors);

        // add current symbol to every previous path
        for(vector<string> path : prevPaths) {
            path.push_back(symbol); 
            paths.push_back(path);
        }
    }

    return paths;
}

void SQLUtils::getPredecessorTables(json& metadata) {
    for(const auto& [currentTableName, data] : metadata.items()) {
        metadata[currentTableName]["predecessor"] = "";   // initialise with empty string as start state has no predecessor
        vector<string> symbolsCurrentState = data["symbols"];
        // predecessor symbols are all current ones besides the last one
        vector<string> predecessorSymbols(symbolsCurrentState.begin(), symbolsCurrentState.end() - 1); 

        if(predecessorSymbols.size() > 0) {
            for(const auto& [possiblePredecessorTableName, data_] : metadata.items()) {
                vector<string> possiblePredecessorSymbols = data_["symbols"];
                if(predecessorSymbols == possiblePredecessorSymbols) {
                    metadata[currentTableName]["predecessor"] = possiblePredecessorTableName;
                    break;
                }
            }
        }
    }
}

void SQLUtils::createTable(const string& tableName, const int& numSymbols) {
    string createTableString = "CREATE TABLE IF NOT EXISTS " + this->catalogAndSchema + "." + tableName + " (";
    string columnsString = "";
    string columnName;
    string columnDataType;

    for(size_t i = 0; i < numSymbols; i++) {
        for(size_t j = 0; j < this->columns.size(); j++) {
            columnName = "s" + to_string(i) + "_" + get<string>(this->columns[j][0]);
            columnDataType = get<string>(this->columns[j][1]);
            columnsString += columnName + " " + columnDataType;

            if (j < this->columns.size() - 1) { // only add if not last element
                columnsString += ", ";
            }
        }
        if(i < numSymbols - 1) { // only add if not last element
                columnsString += ", ";
            }
    }

    createTableString += columnsString + ")";

    this->client.executeQuery(createTableString);
}

/*
Example of partialMatches: {"A", "B", "A"} means predecessor table has ABA matched.
index i represents si so column names get replaced, e.g. from B.id to s1_id
*/
string SQLUtils::replaceTableColumnNames(const string& query, const vector<string>& partialMatches) {
    
    string result = query;

    for (int i = 0; i < partialMatches.size(); i++) {
        const string& symbol = partialMatches[i];
        
        // finds symbol.column_name
        regex pattern(symbol + "\\.(\\w+)");
        
        // replace all occurences of symbol.column_name with si_column_name
        result = regex_replace(result, pattern, "s" + to_string(i) + "_$1");
    }

    return result;
}

string SQLUtils::getTimeWindowCondition(const string& columnName, vector<int>& currentTimeWindow) {
    string whereClause = columnName + " BETWEEN " + to_string(currentTimeWindow[0]) + " AND " + to_string(currentTimeWindow[1]); 

    return whereClause;
}

void SQLUtils::insertIntoTable(const string& tableName, const string& tableNameSymbol,  const string& predecessorTableName, 
const string& condition, const string& timeWindowCondition, const vector<string>& partialMatches, const bool& isStartState) {
    string insertStatement;
    string updatedCondition = this->replaceTableColumnNames(condition, partialMatches);

    if(isStartState) {
        insertStatement = 
        "INSERT INTO " + this->catalogAndSchema + "." + tableName + 
        " SELECT * FROM " + this->catalogAndSchema + "." + this->originalTableName + " " + tableNameSymbol + 
        " WHERE " + condition + " AND " + timeWindowCondition;
    } else {
        insertStatement = 
        "INSERT INTO " + this->catalogAndSchema + "." + tableName + 
        " SELECT * FROM " + this->catalogAndSchema + "." + predecessorTableName + 
        " JOIN " + this->catalogAndSchema + "." + this->originalTableName + " " + tableNameSymbol + 
        " ON " + updatedCondition + " WHERE " + timeWindowCondition;
    }

    this->client.executeQuery(insertStatement);
}

void SQLUtils::createOutputTable(const string& outputTableName) {
    int maxNumSymbols = 0;
    vector<string> resultTables;
    
    vector<string> finalStates = this->dfaData["final_states"];
    for(const string& finalState : finalStates) {
        for(const auto& [state, _, nextState, tableName] : this->transitions) {
            if(nextState == finalState || state == finalState) {    // all states that lead to a final state or are a final state itself
                resultTables.push_back(tableName);

                int numSymbolsCurrentTable = this->metadata[tableName]["num_symbols"];
                if(numSymbolsCurrentTable > maxNumSymbols) {
                    maxNumSymbols = numSymbolsCurrentTable;
                }
            }
        }
    }

    this->client.executeQuery("DROP TABLE IF EXISTS " + this->catalogAndSchema + "." + outputTableName);  // drop table to avoid duplicates
    this->createTable(outputTableName, maxNumSymbols);

    for(const string& resultTable : resultTables) {
        int differenceNumSymbols = maxNumSymbols - this->metadata[resultTable]["num_symbols"].get<int>();
        string nulls = "";
        for(size_t i = 0; i < differenceNumSymbols*this->columns.size(); i++) {
            nulls += ", NULL";
        }

        string insertStatement = "INSERT INTO " + this->catalogAndSchema + "." + outputTableName + " SELECT *" + nulls + " FROM " + this->catalogAndSchema + "." + resultTable;
        this->client.executeQuery(insertStatement);
    }

    cout << "Resulting matches at " + this->catalogAndSchema + "." + outputTableName + "." << endl;
}