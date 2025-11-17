#include "Lexer.h"
#include <cctype>
#include <stdexcept>
#include <unordered_set>

Lexer::Lexer(const std::string& input) : input_(input) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (position_ < input_.size()) {
        char current = peek();

        if (isspace(current)) {
            skipWhitespace();
            continue;
        }

        if (isalpha(current)) {
            tokens.push_back(readIdentifierOrKeyword());
        } else if (current == '"' || current == '\'') {
            tokens.push_back(readStringLiteral());
        } else if (isdigit(current)) {
            tokens.push_back(readNumber());
        } else {
            tokens.push_back(readSymbol());
        }
    }

    tokens.push_back({TokenType::END_OF_INPUT, "", line_, column_});
    return tokens;
}

char Lexer::peek() const {
    if (position_ >= input_.size()) {
        return '\0';
    }
    return input_[position_];
}

char Lexer::consume() {
    char c = peek();
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    position_++;
    return c;
}

void Lexer::skipWhitespace() {
    while (position_ < input_.size()) {
        char c = peek();
        if (isspace(c)) {
            consume();
    } else if (c == '#' || (c == '-' && peek() == '-' && position_ + 1 < input_.size())) {
        // Skip SQL comments
        while (position_ < input_.size() && peek() != '\n') {
            consume();
        }
        } else {
            break;
        }
    }
}

Token Lexer::readIdentifierOrKeyword() {
    std::string value;
    while (position_ < input_.size()) {
        char c = peek();
        if (isalnum(c) || c == '_') {
            value += consume();
        } else {
            break;
        }
    }

    // Check if it's a keyword
    static const std::unordered_set<std::string> keywords = {
        "CREATE", "TABLE", "INSERT", "INTO", "VALUES", "SELECT", "FROM", "WHERE", "AND", "OR"
    };

    TokenType type = TokenType::IDENTIFIER;
    if (keywords.find(value) != keywords.end()) {
        type = TokenType::KEYWORD;
    }

    return {type, value, line_, column_};
}

Token Lexer::readStringLiteral() {
    char quote = consume(); // Consume opening quote
    std::string value;

    while (position_ < input_.size()) {
        char c = consume();
        if (c == quote) {
            break; // Closing quote
        }
        if (c == '\\') {
            // Handle escape sequences
            c = consume();
            if (c == 'n') value += '\n';
            else if (c == 't') value += '\t';
            else if (c == 'r') value += '\r';
            else value += c;
        } else {
            value += c;
        }
    }

    return {TokenType::STRING_LITERAL, value, line_, column_};
}

Token Lexer::readNumber() {
    std::string value;
    while (position_ < input_.size()) {
        char c = peek();
        if (isdigit(c) || c == '.') {
            value += consume();
        } else {
            break;
        }
    }
    return {TokenType::NUMBER, value, line_, column_};
}

Token Lexer::readSymbol() {
    char c = consume();
    std::string value(1, c);

    // Handle multi-character symbols
    if (c == '=' && peek() == '=') {
        value += consume();
    } else if (c == '!' && peek() == '=') {
        value += consume();
    } else if (c == '<' && peek() == '=') {
        value += consume();
    } else if (c == '>' && peek() == '=') {
        value += consume();
    }

    return {TokenType::SYMBOL, value, line_, column_};
}
