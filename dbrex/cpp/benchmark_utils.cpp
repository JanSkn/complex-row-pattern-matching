#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cstdio>
#include <format>
#include <chrono>
#include <iomanip> 
#include <ctime> 
#include "utils.h"
#include "trino_connect.h"
#include "benchmark_utils.h"

using namespace std;
using namespace chrono;

void deleteSQLUtilsJSONs() {
    remove("dbrex/data/metadata.json");
    remove("dbrex/data/transitions.json");
    // don't delete processed_index.json as it only gets generated in non-benchmarking mode
}

void appendToCSV(const string& filePath, const vector<string>& row, const vector<string>& headers) {
    bool fileExists = ifstream(filePath).good(); 
    ofstream file(filePath, ios::app); // open file in append mode

    if(!fileExists && !headers.empty()) {   // create file with column names
        ostringstream headerStream;
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i > 0) headerStream << ",";
            headerStream << headers[i];
        }
        file << headerStream.str() << "\n";
    }

    ostringstream lineStream; 
    for (size_t i = 0; i < row.size(); ++i) {
        if (i > 0) lineStream << ","; 
        lineStream << row[i];
    }

    file << lineStream.str() << "\n"; 

    file.close();
}

string getTimestamp() {
    auto now = chrono::system_clock::now();                
    time_t now_c = chrono::system_clock::to_time_t(now);  

    ostringstream timestampStream;
    timestampStream << put_time(localtime(&now_c), "%Y-%m-%d %H:%M:%S");

    return timestampStream.str(); 
}

string adjustPattern(const string& input) {
    string result = input;
    for(size_t i = 0; i < result.length(); i++) {
        if (result[i] == ' ') {
            result.replace(i, 1, " Z* "); 
            i += 3;
        }
    }
    return result;
}

// Constructor
BenchmarkUtils::BenchmarkUtils(string& originalTableName, string& benchmarkTableName, string& catalogAndSchema, TrinoRestClient& client)
    : originalTableName(originalTableName), benchmarkTableName(benchmarkTableName), catalogAndSchema(catalogAndSchema), client(client) {
        this->getColumns();
        this->createBenchmarkTable();
    }

void BenchmarkUtils::getColumns() {
    vector<string> splittedCatalogAndSchema = splitString(catalogAndSchema, '.');
    string catalog = splittedCatalogAndSchema[0];
    string schema = splittedCatalogAndSchema[1];
    string originalTableNameToLower = originalTableName;
    transform(originalTableNameToLower.begin(), originalTableNameToLower.end(), originalTableNameToLower.begin(), ::tolower);
    string getColumnsQuery = "SELECT column_name, data_type FROM " + catalog + ".information_schema.columns WHERE table_schema = '"+ schema + "' AND table_name = '" + originalTableNameToLower + "'";
    this->columns = this->client.executeQuery(getColumnsQuery);
}

void BenchmarkUtils::createBenchmarkTable() {
    string createTableString = "CREATE TABLE IF NOT EXISTS " + this->catalogAndSchema + "." + this->benchmarkTableName + "(";
    string columnsString = "";
    string columnName;
    string columnDataType;

    for(size_t j = 0; j < this->columns.size(); j++) {
        columnName = get<string>(this->columns[j][0]);
        columnDataType = get<string>(this->columns[j][1]);
        columnsString += columnName + " " + columnDataType;

        if (j < this->columns.size() - 1) { // only add if not last element
            columnsString += ", ";
        }
    }

    createTableString += columnsString + ")";

    this->client.executeQuery(createTableString);
}

void BenchmarkUtils::deleteBenchmarkTableEntries(const string& tableName) {
    this->client.executeQuery("DELETE FROM " + this->catalogAndSchema + "." + tableName);
}

void BenchmarkUtils::insertBatch(const string& column, const int& batchSize, const int& startIndex) {
    string insertStatement = "INSERT INTO " + this->catalogAndSchema + "." + this->benchmarkTableName + 
    " SELECT * FROM " + this->catalogAndSchema + "." + this->originalTableName + " WHERE " 
    + column + " > " + to_string(startIndex) + " AND " + column + " <= " + to_string(startIndex+batchSize);
    this->client.executeQuery(insertStatement);        
}

// estimation without overhead, only #cells x #datatype size
size_t BenchmarkUtils::getUsedMemory(const SQLUtils utils) {
    size_t memoryPerRow = 0; // in bytes

    for (size_t i = 0; i < this->columns.size(); i++) {
        string columnDataType = get<string>(this->columns[i][1]);

        // Convert columnDataType to lowercase
        transform(columnDataType.begin(), columnDataType.end(), columnDataType.begin(), ::tolower);

        // Add memory usage based on the data type
        if(columnDataType == "integer") {
            memoryPerRow += 4; 
        } else if(columnDataType == "bigint") {
            memoryPerRow += 8; 
        } else if(columnDataType == "double") {
            memoryPerRow += 8; 
        } else if(columnDataType == "boolean") {
            memoryPerRow += 1; 
        } else if(columnDataType == "date") {
            memoryPerRow += 4; 
        } else if(columnDataType == "timestamp") {
            memoryPerRow += 8; 
        } else if(columnDataType.length() > 7 && columnDataType.substr(0, 7) == "varchar") {
            // Extract the length from VARCHAR(n)
            size_t varcharLength = 0;
            try {
                varcharLength = stoi(columnDataType.substr(8, columnDataType.length() - 9));
            } catch (const invalid_argument& e) {
                cerr << "Error parsing VARCHAR length: " << e.what() << endl;
            }
            memoryPerRow += varcharLength; // VARCHAR uses 1 byte per character
        } else if (columnDataType.length() > 7 && columnDataType.substr(0, 7) == "decimal") {
            // DECIMAL(p, s) uses variable space, but we can assume 8 bytes as a rough estimate
            memoryPerRow += 8;
        }
    }

    size_t totalMemoryUsage = 0;

    for(const auto& [tableName, values] : utils.metadata.items()) {
        int numRows = get<int>(client.executeQuery("SELECT COUNT(*) FROM " + catalogAndSchema + "." + tableName)[0][0]);
        totalMemoryUsage += memoryPerRow * numRows * values["num_symbols"].get<size_t>();     // num_symbols because the columns get duplicated
    }

    return totalMemoryUsage;
}

double BenchmarkUtils::getThroughput(const SQLUtils utils, const int& batchSize, duration<double> latency) {
    double throughput = 0;

    for(const auto& [tableName, values] : utils.metadata.items()) {
        int numRows = get<int>(client.executeQuery("SELECT COUNT(*) FROM " + catalogAndSchema + "." + tableName)[0][0]);
        throughput += numRows * batchSize;
    }

    return throughput/latency.count();
}
void BenchmarkUtils::benchmarkMR(const SQLUtils utils, const string& benchmarkTableName, const string& pattern, const string& column, 
const map<string, string>& defines, const pair<vector<string>, vector<string>>& firstAndLastSymbols, const int& tws, const int& batchSize) {    
    string adjustedPattern = adjustPattern(pattern);
    
    string measuresString = "";
    string definesString = "";

    vector<string> symbols;
    vector<string> defines_;

    for(const auto& [symbol, defineStatement] : defines) {
        symbols.push_back(symbol);
        defines_.push_back(defineStatement);
    }
    for(size_t i = 0; i < defines_.size(); i++) {
        measuresString += symbols[i] + "." + column + " AS " + "s" + to_string(i) + "_" + column;
        definesString += symbols[i] + " AS " + defines_[i];
        if(i != defines_.size() - 1) {  // if not last element
            definesString += ", ";
            measuresString += ", ";
        } else {
            vector<string> firsts = firstAndLastSymbols.first;
            vector<string> lasts = firstAndLastSymbols.second;
            definesString += " AND ";
            for(size_t j = 0; j < firsts.size(); j++) {
                for(string last : lasts) {
                    definesString += last + "." + column + " <= " + firsts[j] + "." + column + " + " + to_string(tws);
                }
                if(j != firsts.size() - 1) {
                    definesString += " OR ";
                }
            }
            
        }
    }

    string mrString = format(
        "SELECT * "
        "FROM {} "
        "MATCH_RECOGNIZE ("
        "    ORDER BY {}"
        "    MEASURES"
        "       {}"
        "    ONE ROW PER MATCH"
        "    AFTER MATCH SKIP TO NEXT ROW"
        "    PATTERN({})"
        "    DEFINE"
        "       {})",
        (this->catalogAndSchema+"."+benchmarkTableName), column, measuresString, adjustedPattern, definesString);

    this->client.executeQuery(mrString);
}