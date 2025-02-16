#ifndef TOPOLOGICAL_SORT_H
#define TOPOLOGICAL_SORT_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <utility> 

using namespace std;

void topologicalSortUtil(
    const unordered_map<string, unordered_map<string, string>>& graph,
    const string& state,
    unordered_set<string>& visited,
    vector<string>& order
);

vector<string> topologicalSort(
    const unordered_map<string, unordered_map<string, string>>& graph,
    const string& startState
);

vector<tuple<string, string, string>> traverseGraph(
    const unordered_map<string, unordered_map<string, string>>& graph,
    const vector<string>& order
);

unordered_map<string, unordered_map<string, string>> parseGraph(json& j);

#endif 
