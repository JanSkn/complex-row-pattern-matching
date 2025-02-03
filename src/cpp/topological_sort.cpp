#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <utility>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

void topologicalSortUtil(
    const unordered_map<string, unordered_map<string, string>>& graph,
    const string& state,
    unordered_set<string>& visited,
    vector<string>& order
) {
    visited.insert(state);

    // all neighbours of the current state
    if (graph.find(state) != graph.end()) {
        for (const auto& edge : graph.at(state)) {
            string nextState = edge.second;
            if (visited.find(nextState) == visited.end()) {
                topologicalSortUtil(graph, nextState, visited, order);
            }
        }
    }

    order.push_back(state);
}

vector<string> topologicalSort(const unordered_map<string, unordered_map<string, string>>& graph, const string& startState) {
    unordered_set<string> visited;
    vector<string> order;

    topologicalSortUtil(graph, startState, visited, order);

    // reverse to retain topological sorting
    reverse(order.begin(), order.end());
    return order;
}

vector<pair<string, string>> traverseGraph(const unordered_map<string, unordered_map<string, string>>& graph, const vector<string>& order) {
    vector<pair<string, string>> edgeOrder;

    // Kanten in der Reihenfolge der topologischen Sortierung hinzufügen
    for (const string& state : order) {
        if (graph.find(state) != graph.end()) {
            for (const auto& edge : graph.at(state)) {
                // string nextState = edge.second;
                // edgeOrder.push_back({state, nextState});
                string transitionSymbol = edge.first;
                edgeOrder.push_back({state, transitionSymbol});
            }
        }
    }

    return edgeOrder;
}

// function to transform JSON from Python API to map
unordered_map<string, unordered_map<string, string>> parseGraph(json& j) {
    unordered_map<string, unordered_map<string, string>> graph;

    for (auto& [key, value] : j.items()) {
        cout << key << value << endl;
        string node = key;  // Der Knoten (z. B. "0;2;4")
        unordered_map<string, string> edges;
        for (auto& [label, target] : value.items()) {
            // Die Kanten (z. B. "A": "6")
            edges[label] = target.get<string>();
        }
        graph[node] = edges;
    }

    return graph;
}