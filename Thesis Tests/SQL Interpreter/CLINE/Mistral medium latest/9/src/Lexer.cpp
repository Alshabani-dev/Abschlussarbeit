#include "Lexer.h"
#include "Utils.h"
#include <unordered_set>
#include <cctype>

std::vector<Token> Lexer::tokenize(const std::string &input) {
    std::vector<Token> tokens;
    size_t pos = 0;
    size_t len = input.length();

    // SQL keywords
    static const std::unordered_set<std::string> keywords = {
        "CREATE", "TABLE", "INSERT", "INTO", "VALUES", "SELECT", "FROM", "WHERE", "AND", "OR"
    };

    while (pos < len) {
        // Skip whitespace
        if (std::isspace(input[pos])) {
            pos++;
            continue;
        }

        // Handle symbols
        if (input[pos] == '(' || input[pos] == ')' || input[pos] == ',' ||
            input[pos] == ';' || input[pos] == '=' || input[pos] == '*') {
            tokens.push_back({TokenType::SYMBOL, std::string(1, input[pos]), pos});
            pos++;
            continue;
        }

        // Handle string literals
        if (input[pos] == '"') {
            pos++; // Skip opening quote
            size_t start = pos;
            bool escaped = false;

            while (pos < len) {
                if (input[pos] == '"' && !escaped) {
                    break;
                }
                escaped = (input[pos] == '\\' && !escaped);
                pos++;
            }

            std::string value = input.substr(start, pos - start);
            tokens.push_back({TokenType::STRING, value, start});
            pos++; // Skip closing quote
            continue;
        }

        // Handle identifiers and keywords
        if (std::isalpha(input[pos]) || input[pos] == '_') {
            size_t start = pos;
            while (pos < len && (std::isalnum(input[pos]) || input[pos] == '_')) {
                pos++;
            }

            std::string value = input.substr(start, pos - start);
            TokenType type = keywords.find(Utils::toLower(value)) != keywords.end()
                ? TokenType::KEYWORD
                : TokenType::IDENTIFIER;

            tokens.push_back({type, value, start});
            continue;
        }

        // Handle numbers
        if (std::isdigit(input[pos])) {
            size_t start = pos;
            while (pos < len && std::isdigit(input[pos])) {
                pos++;
            }

            std::string value = input.substr(start, pos - start);
            tokens.push_back({TokenType::NUMBER, value, start});
            continue;
        }

        // If we get here, it's an unrecognized character
        tokens.push_back({TokenType::SYMBOL, std::string(1, input[pos]), pos});
        pos++;
    }

    // Add end of input token
    tokens.push_back({TokenType::END_OF_INPUT, "", pos});

    return tokens;
}
