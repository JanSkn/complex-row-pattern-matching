#include "regex_parser.h"
#include <algorithm>

using namespace std;

// Constructor
RegexParser::RegexParser() : pos(0) {}

void RegexParser::skipWhitespace() {
    while (pos < regex.length() && regex[pos] == ' ') {
        pos++;
    }
}

bool RegexParser::readSymbol() {
    skipWhitespace();
    if (pos >= regex.length()) return false;

    char c = regex[pos];
    if (c == '|' || c == '(' || c == ')') return false;

    while (pos < regex.length() && regex[pos] != ' ' &&
           regex[pos] != '|' && regex[pos] != '(' && regex[pos] != ')') {
        pos++;
    }
    return true;
}

int RegexParser::parseExpression() {
    int maxLength = 0;
    int currentLength = 0;

    while (pos < regex.length()) {
        skipWhitespace();
        if (pos >= regex.length()) break;

        char c = regex[pos];

        if (c == ')') {
            break;
        } else if (c == '(') {
            pos++;
            int innerLength = parseExpression();
            pos++;
            currentLength += innerLength;
        } else if (c == '|') {
            maxLength = max(maxLength, currentLength);
            currentLength = 0;
            pos++;
        } else if (readSymbol()) {
            currentLength++;
        }
    }

    return max(maxLength, currentLength);
}

int RegexParser::calculateMaxSymbolLength(const string& input) {
    regex = input;
    pos = 0;
    return parseExpression();
}
