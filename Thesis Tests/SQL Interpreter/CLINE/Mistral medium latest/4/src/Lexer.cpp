#include "Lexer.h"
#include "Utils.h"
#include <cctype>
#include <unordered_set>

Lexer::Lexer() : currentPos_(0) {}

std::vector<Token> Lexer::tokenize(const std::string &input) {
    input_ = input;
    tokens_.clear();
    currentPos_ = 0;

    static const std::unordered_set<std::string> keywords = {
        "CREATE", "TABLE", "INSERT", "INTO", "VALUES", "SELECT", "FROM", "WHERE"
    };

    while (currentPos_ < input_.size()) {
        skipWhitespace();

        if (currentPos_ >= input_.size()) {
            break;
        }

        char current = input_[currentPos_];

        if (isalpha(current) || current == '_') {
            Token token = readIdentifierOrKeyword();
            std::string upperValue = Utils::toLower(token.value);
            std::transform(upperValue.begin(), upperValue.end(), upperValue.begin(), ::toupper);

            if (keywords.find(upperValue) != keywords.end()) {
                token.type = TokenType::KEYWORD;
            }
            tokens_.push_back(token);
        } else if (current == '"' || current == '\'') {
            tokens_.push_back(readString());
        } else if (isdigit(current)) {
            tokens_.push_back(readNumber());
        } else {
            tokens_.push_back(readSymbol());
        }
    }

    tokens_.push_back({TokenType::END_OF_INPUT, "", currentPos_});
    return tokens_;
}

void Lexer::skipWhitespace() {
    while (currentPos_ < input_.size()) {
        if (isspace(input_[currentPos_])) {
            currentPos_++;
        } else if (currentPos_ + 1 < input_.size() &&
                  input_[currentPos_] == '-' &&
                  input_[currentPos_ + 1] == '-') {
            // Skip comment until end of line
            while (currentPos_ < input_.size() && input_[currentPos_] != '\n') {
                currentPos_++;
            }
        } else {
            break;
        }
    }
}

Token Lexer::nextToken() {
    if (currentPos_ >= input_.size()) {
        return {TokenType::END_OF_INPUT, "", currentPos_};
    }
    return tokens_[currentPos_++];
}

Token Lexer::readIdentifierOrKeyword() {
    size_t start = currentPos_;
    while (currentPos_ < input_.size() &&
           (isalnum(input_[currentPos_]) || input_[currentPos_] == '_')) {
        currentPos_++;
    }
    return {TokenType::IDENTIFIER, input_.substr(start, currentPos_ - start), start};
}

Token Lexer::readString() {
    char quote = input_[currentPos_++];
    size_t start = currentPos_;

    while (currentPos_ < input_.size() && input_[currentPos_] != quote) {
        if (input_[currentPos_] == '\\') {
            currentPos_++; // Skip escape character
        }
        currentPos_++;
    }

    if (currentPos_ < input_.size()) {
        currentPos_++; // Skip closing quote
    }

    return {TokenType::STRING, input_.substr(start, currentPos_ - start - 1), start};
}

Token Lexer::readNumber() {
    size_t start = currentPos_;
    while (currentPos_ < input_.size() && isdigit(input_[currentPos_])) {
        currentPos_++;
    }
    return {TokenType::NUMBER, input_.substr(start, currentPos_ - start), start};
}

Token Lexer::readSymbol() {
    size_t start = currentPos_;
    char current = input_[currentPos_++];

    // Handle multi-character symbols
    if (current == '=' && currentPos_ < input_.size() && input_[currentPos_] == '=') {
        currentPos_++;
        return {TokenType::SYMBOL, "==", start};
    }

    return {TokenType::SYMBOL, std::string(1, current), start};
}
