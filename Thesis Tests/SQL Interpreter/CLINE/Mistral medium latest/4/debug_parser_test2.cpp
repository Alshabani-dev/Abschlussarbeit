#include <iostream>
#include <string>
#include "Lexer.h"
#include "Parser.h"

int main() {
    std::string sql = "SELECT * FROM users;";

    Lexer lexer;
    std::vector<Token> tokens = lexer.tokenize(sql);

    std::cout << "Tokens:" << std::endl;
    for (const auto& token : tokens) {
        std::string typeStr;
        switch (token.type) {
            case TokenType::KEYWORD: typeStr = "KEYWORD"; break;
            case TokenType::IDENTIFIER: typeStr = "IDENTIFIER"; break;
            case TokenType::STRING: typeStr = "STRING"; break;
            case TokenType::NUMBER: typeStr = "NUMBER"; break;
            case TokenType::SYMBOL: typeStr = "SYMBOL"; break;
            case TokenType::END_OF_INPUT: typeStr = "END_OF_INPUT"; break;
            default: typeStr = "UNKNOWN"; break;
        }

        std::cout << "Type: " << typeStr
                  << ", Value: '" << token.value << "'"
                  << ", Position: " << token.position << std::endl;
    }

    Parser parser;
    try {
        std::unique_ptr<Statement> stmt = parser.parse(sql);
        if (stmt) {
            std::cout << "Parsed successfully!" << std::endl;
        } else {
            std::cout << "Failed to parse statement" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
