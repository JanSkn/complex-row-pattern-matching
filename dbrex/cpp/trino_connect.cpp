/*
Trino REST API call responses get chunked.
First few JSONs do not containt data, and the data itself can get chunked if too long.
No nextUri parameter indicates end of response stream.

Reference: https://trino.io/docs/current/develop/client-protocol.html
*/

#include <iostream>
#include <string>
#include <curl/curl.h>
#include <any>
#include "json.hpp"
#include "trino_connect.h"

using namespace std;

// callback function for CURL to collect response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

TrinoRestClient::TrinoRestClient(string server_url, string username, string password): 
    m_serverUrl(server_url), 
    m_username(username), 
    m_password(password) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

TrinoRestClient::~TrinoRestClient() {
    curl_global_cleanup();
}

    DataTable TrinoRestClient::executeQuery(string query) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        cerr << "Failed to initialize CURL" << endl;
        return {};
    }

    string responseString;
    struct curl_slist* headers = nullptr;

    string fullUrl = m_serverUrl + "/v1/statement";

    headers = curl_slist_append(headers, "Content-Type: text/plain");
    headers = curl_slist_append(headers, "X-Trino-Source: cpp-client");

    if (!m_username.empty()) {
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        curl_easy_setopt(curl, CURLOPT_USERPWD, (m_username + ":" + m_password).c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

    // first query
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        cerr << "CURL error: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        return {};
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    json jsonResponse = parseResponse(responseString);
    DataTable queryResult;

    // incomplete response as long as nextUri
    while (jsonResponse.contains("nextUri")) {
        string nextUri = jsonResponse["nextUri"];
        responseString.clear();

        curl = curl_easy_init();
        if (!curl) {
            cerr << "Failed to initialize CURL for nextUri" << endl;
            return {}; 
        }

        curl_easy_setopt(curl, CURLOPT_URL, nextUri.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "CURL error on nextUri: " << curl_easy_strerror(res) << endl;
            curl_easy_cleanup(curl);
            return {}; 
        }

        curl_easy_cleanup(curl);

        if(!(jsonResponse["data"].empty())) {       // chunk containts data
            for (const auto& row : jsonResponse["data"]) {
                Row currentRow;
                for (const auto& item : row) {
                    if (item.is_number_integer()) {
                        currentRow.push_back(item.get<int>());
                    } else if (item.is_string()) {
                        currentRow.push_back(item.get<string>());
                    } else if (item.is_number_float()) {
                        currentRow.push_back(item.get<float>());
                    } else if (item.is_boolean()) {
                        currentRow.push_back(item.get<bool>());
                    } else {
                        cerr << "Unknown data type." << endl;
                    }
                }
                queryResult.push_back(currentRow);
            }

        }
                jsonResponse = parseResponse(responseString);
    }   
    return queryResult;
}

json TrinoRestClient::parseResponse(string response) {
    try {
        return json::parse(response);
    } catch (json::parse_error& e) {
        cerr << "JSON parse error: " << e.what() << endl;
        return {};
    }
}