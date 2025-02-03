#ifndef TRINO_CONNECT_H
#define TRINO_CONNECT_H

#include <string>
#include <iostream>
#include <curl/curl.h>
#include <variant>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;
using Row = vector<variant<int, string, float, bool>>; 
using DataTable = vector<Row>;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output);

class TrinoRestClient {
private:
    string m_serverUrl;
    string m_username;
    string m_password;

public:
    
    // Constructor
    TrinoRestClient(string server_url, 
                    string username = "username",  // beliebig
                    string password = "");

    // Destructor
    ~TrinoRestClient();

    DataTable execute_query(string query); 

    // prevents copy construction and assignment
    TrinoRestClient(const TrinoRestClient&) = delete;
    TrinoRestClient& operator=(const TrinoRestClient&) = delete;
  
    json parseResponse(string response);
};

#endif