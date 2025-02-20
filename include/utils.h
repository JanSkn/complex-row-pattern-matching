#ifndef UTILS_H
#define UTILS_H

#include <csignal>
#include <atomic>
#include <string>
#include <random>
#include <tuple>
#include <sstream>
#include <iomanip>
#include "trino_connect.h"
#include "cli.h"

using namespace std;

extern atomic<bool> running;

void signalHandler(int signal);
void printName();
void printDfa(const json& dfaData);
string replaceWhitespace(const string& input);
string generateUuid();
vector<int> getCurrentTimeWindow();
void saveCurrentTimeWindow(const vector<int>& timeWindow);

class SQLUtils {
public:
    // avoid const class variables to avoid const method declaration
    string& originalTableName;
    string& outputTableName;
    json metadata;
    vector<tuple<string, string, string, string>> transitions;
    string& catalogAndSchema;
    DataTable columns;
    json& dfaData;
    TrinoRestClient& client;

    // Constructor
    SQLUtils(string& originalTableName, string& outputTableName, string& schema, json& dfaData, TrinoRestClient& client);
    
    void setup();
    void getColumns();
    map<string, vector<vector<string>>> getPredecessors(const vector<tuple<string, string, string>>& transitions);
    vector<vector<string>> getPredecessorPaths(const string& state, const map<string, vector<vector<string>>>& predecessors);
    void getPredecessorTables(json& metadata);

    void createTable(const string& tableName, const int& numSymbols);
    string replaceTableColumnNames(const std::string& query, const std::vector<std::string>& symbolNames);
    string getTimeWindowCondition(const string& columnName, vector<int>& currentTimeWindow);
    void insertIntoTable(const string& tableName, const string& symbol, const string& predecessorTableName, const string& condition, const string& timeWindowCondition, const vector<string>& partialMatches, const bool& isStartState);
    void createOutputTable(const string& outputTableName);
};

#endif 