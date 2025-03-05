#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <getopt.h>
#include "cli.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json; 

void printHelp() {
    cout << "Usage: program [OPTIONS]\n"
              << "Options:\n"
              << "  -c, --catalog               Name of the data source\n"
              << "  -s, --schema                Name of the schema of the catalog\n"
              << "  -t, --table_name            Name of the source table\n"
              << "  -r, --regex                 RegEx pattern. Explicit use of parentheses, example: 'A1 A2 | A3' evaluates to '(A1 A2) | A3', instead use 'A1 (A2 | A3)'\n"
              << "  -a, --alphabet              RegEx alphabet\n"
              << "  -q, --queries               SQL queries separated by RegEx symbol. Must be same order as the alphabet\n"
              << "  -o, --output_table          Name of the output table\n"
              << "  -twt, --time_window_type    Time window type (countbased or sliding_window). Only countbased supported at this version\n"
              << "  -twc, --tw_column           Must be integers in asceding order with step size 1\n"
              << "  -tws, --tw_size             Time window size\n"
              << "  -sf, --sleep_for            Sleep for n milliseconds after each execution to conserve resources\n"
              << "  --benchmark                 Optional. Runs benchmark on predefined table. Use only this for benchmarking\n"
              << "  -h, --help                  Show this help message\n\n";
}

void saveConfigFile(const std::string& configPath, const CLIParams& params) {
    try {
        json config;

        if(!params.catalog.empty()) config["catalog"] = params.catalog;
        if(!params.schema.empty()) config["schema"] = params.schema;
        if(!params.tableName.empty()) config["table_name"] = params.tableName;
        if(!params.pattern.empty()) config["regex"] = params.pattern;
        if(!params.alphabet.empty()) config["alphabet"] = params.alphabet;
        if(!params.queries.empty()) config["queries"] = params.queries;
        if(!params.outputTableName.empty()) config["output_table"] = params.outputTableName;
        if(!params.timeWindowType.empty()) config["time_window_type"] = params.timeWindowType;
        if(!params.twColumn.empty()) config["tw_column"] = params.twColumn;
        if(params.twSize > 0) config["tw_size"] = params.twSize;
        if(params.sleepFor > 0) config["sleep_for"] = params.sleepFor;

        ofstream configFile(configPath);
        if(!configFile.is_open()) {
            throw std::runtime_error("Unable to open config file for writing: " + configPath);
        }

        configFile << config.dump(4);
        configFile.close();

    } catch (const exception& e) {
        std::cerr << "Error saving config file: " << e.what() << std::endl;
        exit(1);
    }
}

CLIParams loadConfigFile(const string& configPath) {
    CLIParams params;

    try {
        ifstream configFile(configPath);
        if(!configFile.is_open()) {
            throw runtime_error("Unable to open config file: " + configPath);
        }

        json config;
        configFile >> config;

        if(config.contains("catalog")) params.catalog = config["catalog"];
        if(config.contains("schema")) params.schema = config["schema"];
        if(config.contains("table_name")) params.tableName = config["table_name"];
        if(config.contains("regex")) params.pattern = config["regex"];
        if(config.contains("alphabet")) {
            for(const auto& item : config["alphabet"]) {
                params.alphabet.push_back(item);
            }
        }
        if(config.contains("queries")) {
            for(const auto& item : config["queries"]) {
                params.queries.push_back(item);
            }
        }
        if(config.contains("output_table")) params.outputTableName = config["output_table"];
        if(config.contains("time_window_type")) params.timeWindowType = config["time_window_type"];
        if(config.contains("tw_column")) params.twColumn = config["tw_column"];
        if(config.contains("tw_size")) params.twSize = config["tw_size"];
        if(config.contains("sleep_for")) params.sleepFor = config["sleep_for"];
        params.benchmark = config.contains("benchmark");

    } catch(const exception& e) {
        cerr << "Error loading config file: " << e.what() << endl;
        exit(1);
    }

    return params;
}

CLIParams parseCommandLine(int argc, char* argv[]) {
    CLIParams params;
    params.benchmark = false;

    const string configPath = "dbrex/config/args.json";
    ifstream configFile(configPath);

    if(configFile) {
        cout << "Loading args from file." << endl;
        return loadConfigFile(configPath);
    }

    for(int i = 1; i < argc; ++i) {
        string arg = argv[i];

        if(arg == "--benchmark") {  
            params.benchmark = true;
            return params;
        } 

        if(arg == "-h" || arg == "--help") {
            printHelp();
            exit(0);
        } else if(arg == "-c" || arg == "--catalog") {
            if (i + 1 < argc) {
                params.catalog = argv[++i];
            }
        } else if(arg == "-s" || arg == "--schema") {
            if (i + 1 < argc) {
                params.schema = argv[++i];
            }
        } else if(arg == "-t" || arg == "--table_name") {
            if (i + 1 < argc) {
                params.tableName = argv[++i];
            }
        } else if(arg == "-r" || arg == "--regex") {
            if (i + 1 < argc) {
                params.pattern = argv[++i];
            }
        } else if(arg == "-a" || arg == "--alphabet") {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                params.alphabet.push_back(argv[++i]);
            }
        } else if(arg == "-q" || arg == "--queries") {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                params.queries.push_back(argv[++i]);
            }
        } else if(arg == "-o" || arg == "--output_table") {
             if (i + 1 < argc) {
                params.outputTableName = argv[++i];
            }
        } else if(arg == "-twt" || arg == "--time_window_type") {
            if (i + 1 < argc) {
                params.timeWindowType = argv[++i];
            }
        } else if(arg == "-twc" || arg == "--tw_column") {
            if (i + 1 < argc) {
                params.twColumn = argv[++i];
            }
        } else if(arg == "-tws" || arg == "--tw_size") {
            if (i + 1 < argc) {
                params.twSize = stoi(argv[++i]);
            }
        } else if(arg == "-sf" || arg == "--sleep_for") { 
            if (i + 1 < argc) {
                params.sleepFor = stoi(argv[++i]);
            }
        } else if(arg[0] == '-') {
            cerr << "Unknown option: " << arg << "\n";
            printHelp();
            exit(1);
        }
    }

    saveConfigFile(configPath, params);
    
    return params;
}