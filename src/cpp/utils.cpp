#include <csignal>
#include <atomic>
#include <string>
#include <random>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <set>
#include <algorithm>
#include "utils.h"

using namespace std;

atomic<bool> running(true);

void signalHandler(int signal) {
    if(signal == SIGINT || signal == SIGTERM) {
        running = false; 
    }
}

string replaceWhitespace(const string& input) {
    string result = input;
    for(int i = 0; i < result.length(); i++) {
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

void printName() {
    cout << "\n";
    cout << " _____     ____       _____     _____     __  __\n";
    cout << "|  _  \\   |  _ \\     |  _  \\   |  ___|    \\ \\/ /\n";
    cout << "| | | |   | |_) |    | |_) |   | |__       \\  / \n";
    cout << "| | | |   |  _ <     |  _ <    |  __|      /  \\ \n";
    cout << "| |/ /    | |_) |    | | \\ \\   | |___     / /\\ \\\n";
    cout << "|___/     |____/     |_|  \\_\\  |_____|   /_/  \\_\\\n";
    cout << "\n";
}

// Constructor
SQLUtils::SQLUtils(string& originalTableName, string& schema, int& numRegExSymbols, json& dfaData, TrinoRestClient& client)
    : originalTableName(originalTableName), schema(schema), numRegExSymbols(numRegExSymbols), dfaData(dfaData), client(client) {
        this->client.execute_query("CREATE SCHEMA IF NOT EXISTS " + schema); 
        this->metadata = loadOrCreateMetadata();
        this->columns = this->client.execute_query("SHOW COLUMNS FROM " + schema + "." + originalTableName);
    }

json SQLUtils::loadOrCreateMetadata() {
    json metadata;
    
    string partialMatchTableName;
    ifstream checkFile("src/data/metadata.json");

    if(checkFile) {
        checkFile >> metadata;
        cout << "Loaded existing metadata." << endl;
    } else {
        for(const auto& [state, transitions] : this->dfaData["dfa_dict"].items()) {
            for(const auto& [symbol, endState] : transitions.items()) {
                partialMatchTableName = symbol + "_" + this->originalTableName + "_" + generateUuid();   
                metadata[state][symbol]["table_name"] = partialMatchTableName;
            }
        }
        metadata["res"] = this->originalTableName + "_" + generateUuid();
        ofstream file("src/data/metadata.json");
        file << metadata.dump(4);
        cout << "First execution. Created metadata." << endl;
    }
    
    return metadata;
}

// returns predecessor pair of table name and symbol
vector<pair<string, string>> SQLUtils::findPredecessors(const string& state) {
    vector<pair<string, string>> predecessors;

    for(const auto& [startState, transitions] : this->dfaData["dfa_dict"].items()) {
        for (const auto& [symbol, endState] : transitions.items()) {
            if(endState == state) {
                predecessors.push_back({this->metadata[startState][symbol]["table_name"].get<string>(), symbol});
            }
        }
    }

    return predecessors;
}

void SQLUtils::createTable(const string& tableName) {
    string createTableString = "CREATE TABLE IF NOT EXISTS " + this->schema + "." + tableName + " (";
    string columnsString = "";
    string columnName;
    string columnDataType;

    for(int i = 0; i < this->numRegExSymbols; i++) {
        for(int j = 0; j < this->columns.size(); j++) {
            columnName = "s" + to_string(i) + "_" + get<string>(this->columns[j][0]);
            columnDataType = get<string>(this->columns[j][1]);

            columnsString += columnName + " " + columnDataType;

            if (j < this->columns.size() - 1) { // only add if not last element
                columnsString += ", ";
            }
        }
        if(i < this->numRegExSymbols - 1) { // only add if not last element
                columnsString += ", ";
            }
    }

    createTableString += columnsString + ")";

    this->client.execute_query(createTableString);
}

void SQLUtils::insertIntoTable(const string& tableName, const string& tableNameSymbol,  const string& predecessorTableName, const string& predecessorSymbol, const string& condition, const bool& isStartState) {
    string insertStatement;

    if(isStartState) {
        insertStatement = "INSERT INTO " + this->schema + "." + tableName + " " + tableNameSymbol + " WHERE " + condition;
    } else {
        insertStatement = "INSERT INTO " + this->schema + "." + tableName + " SELECT * FROM " + predecessorTableName + " " + predecessorSymbol + " JOIN " + this->schema + "." + this->originalTableName + " " + tableNameSymbol + " ON " + condition;
    }
    cout << insertStatement << endl;
    // client.execute_query(insertStatement);
}