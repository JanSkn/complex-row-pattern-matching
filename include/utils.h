#ifndef UTILS_H
#define UTILS_H

#include <csignal>
#include <atomic>
#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include "trino_connect.h"

using namespace std;

extern atomic<bool> running;

class Utils {
public:
    // avoid const class variables to avoid const method declaration
    string& originalTableName;
    json metadata;
    string& schema;
    DataTable columns;
    int& numRegExSymbols;
    json& dfaData;
    TrinoRestClient& client;

    // Constructor
    Utils(string& originalTableName, string& schema, int& numRegExSymbols, json& dfaData, TrinoRestClient& client);
    
    json loadOrCreateMetadata();
    vector<pair<string, string>> findPredecessors(const string& state);
    void createTable(const string& tableName);
    void insertIntoTable(const string& tableName, const string& symbol, const string& predecessorTableName, const string& predecessorSymbol, const string& condition, const bool& isStartState);
    
};

void signalHandler(int signal);
string replaceWhitespace(const string& input);
void printName();
string generateUuid();

#endif 