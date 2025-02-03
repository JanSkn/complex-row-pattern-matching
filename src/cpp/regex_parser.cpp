#include "regex_parser.h"
#include <algorithm>

using namespace std;

// Constructor
RegexParser::RegexParser() : pos(0) {}

void RegexParser::skipWhitespace() {
    while (this->pos < regex.length() && regex[this->pos] == ' ') {
        this->pos++;
    }
}

bool RegexParser::readSymbol() {
    skipWhitespace();
    if (this->pos >= regex.length()) return false;

    char c = regex[this->pos];
    if (c == '|' || c == '(' || c == ')') return false;

    while (this->pos < regex.length() && regex[this->pos] != ' ' &&
           regex[this->pos] != '|' && regex[this->pos] != '(' && regex[this->pos] != ')') {
        this->pos++;
    }
    return true;
}

int RegexParser::parseExpression() {
    int maxLength = 0;
    int currentLength = 0;

    while (this->pos < regex.length()) {
        skipWhitespace();
        if (this->pos >= regex.length()) break;

        char c = regex[this->pos];

        if (c == ')') {
            break;
        } else if (c == '(') {
            this->pos++;
            int innerLength = parseExpression();
            this->pos++;
            currentLength += innerLength;
        } else if (c == '|') {
            maxLength = max(maxLength, currentLength);
            currentLength = 0;
            this->pos++;
        } else if (readSymbol()) {
            currentLength++;
        }
    }

    return max(maxLength, currentLength);
}

int RegexParser::calculateMaxSymbolLength(const string& input) {
    regex = input;
    this->pos = 0;
    return parseExpression();
}
