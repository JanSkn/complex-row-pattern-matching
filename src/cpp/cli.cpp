#include <iostream>
#include <string>
#include <vector>
#include <getopt.h>

using namespace std;

struct CLIParams {
    string pattern;
    vector<string> alphabet;
    vector<string> query;
    string timeWindowType;
    string twColumn;
    int twSize = 0;
    int twStepSize = 0;
};

void printHelp() {
    cout << "Usage: program [OPTIONS]\n"
              << "Options:\n"
              << "  -r, --regex                 RegEx pattern. Explicit use of parenthesises, example: 'A B | C' evaluates to '(A B) | C', instead use A (B | C)\n"
              << "  -a, --alphabet              RegEx alphabet\n"
              << "  -q, --query                 SQL queries separated by RegEx symbol. Must be same order as the alphabet\n"
              << "  -twt, --time_window_type    Time window type (countbased or timebased)\n"
              << "  -twc, --tw_column           Name of the column for the time window selection\n"
              << "  -tws, --tw_size             Time window size\n"
              << "  -twss, --tw_step_size       Time window step size\n"
              << "  -h, --help                  Show this help message\n\n"
              << "Example:\n"
              << "  dbrex -r 'A1 | (A2 A3)' -a A1 A2 A3 ...\n";
}

CLIParams parseCommandLine(int argc, char* argv[]) {
    CLIParams params;
    
    for(int i = 1; i < argc; ++i) {
        string arg = argv[i];
        
        if(arg == "-h" || arg == "--help") {
            printHelp();
            exit(0);
        } else if (arg == "-r" || arg == "--regex") {
            if (i + 1 < argc) {
                params.pattern = argv[++i];
            }
        } else if (arg == "-a" || arg == "--alphabet") {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                params.alphabet.push_back(argv[++i]);
            }
        } else if(arg == "-q" || arg == "--query") {
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                params.query.push_back(argv[++i]);
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
        } else if(arg == "-twss" || arg == "--tw_step_size") {
            if (i + 1 < argc) {
                params.twStepSize = stoi(argv[++i]);
            }
        } else if(arg[0] == '-') {
            cerr << "Unknown option: " << arg << "\n";
            printHelp();
            exit(1);
        }
    }
    
    return params;
}