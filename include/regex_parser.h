#ifndef REGEX_PARSER_H
#define REGEX_PARSER_H

#include <string>

class RegexParser {
private:
    std::string regex;
    size_t pos;
    
    void skipWhitespace();
    bool readSymbol();
    int parseExpression();

public:
    // Constructor
    RegexParser();
    
    int calculateMaxSymbolLength(const std::string& input);
};

#endif 