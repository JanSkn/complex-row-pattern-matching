#ifndef DBREX_H
#define DBREX_H

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

void runDBrex(string& catalogAndSchema, const string& tableName, const string& outputTable, const string& twt, 
const string& twc, const int& tws, const int& sleepFor, map<string, string>& defineConditions, const json& dfaData, SQLUtils& utils, TrinoRestClient& client);
void benchmarkDBrex(string& catalogAndSchema, const string& tableName, const string& outputTable, const int& processedIndex, 
const string& column, const int& tws, map<string, string>& defineConditions, const json& dfaData, SQLUtils& utils, TrinoRestClient& client);

#endif
