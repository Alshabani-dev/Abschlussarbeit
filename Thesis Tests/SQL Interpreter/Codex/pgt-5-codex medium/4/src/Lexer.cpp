#include "Lexer.h"

#include <cctype>
#include <stdexcept>
#include <unordered_map>

Lexer::Lexer(const std::string &input) : input_(input), pos_(0) {}

void Lexer::skipWhitespace() {
    while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
        ++pos_;
    }
}

char Lexer::peek() const {
    if (pos_ >= input_.size()) {
        return '\0';
    }
    return input_[pos_];
}

char Lexer::get() {
    if (pos_ >= input_.size()) {
        return '\0';
    }
    return input_[pos_++];
}

Token Lexer::identifier() {
    size_t start = pos_;
    while (pos_ < input_.size() && (std::isalnum(static_cast<unsigned char>(input_[pos_])) || input_[pos_] == '_')) {
        ++pos_;
    }
    std::string text = input_.substr(start, pos_ - start);
    std::string upper;
    upper.reserve(text.size());
    for (char c : text) {
        upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    static const std::unordered_map<std::string, TokenType> keywords = {
        {"CREATE", TokenType::KEYWORD_CREATE},
        {"TABLE", TokenType::KEYWORD_TABLE},
        {"INSERT", TokenType::KEYWORD_INSERT},
        {"INTO", TokenType::KEYWORD_INTO},
        {"VALUES", TokenType::KEYWORD_VALUES},
        {"SELECT", TokenType::KEYWORD_SELECT},
        {"FROM", TokenType::KEYWORD_FROM},
        {"WHERE", TokenType::KEYWORD_WHERE},
    };

    auto it = keywords.find(upper);
    if (it != keywords.end()) {
        return Token{it->second, upper, start};
    }
    return Token{TokenType::IDENTIFIER, text, start};
}

Token Lexer::number() {
    size_t start = pos_;
    while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
        ++pos_;
    }
    return Token{TokenType::NUMBER, input_.substr(start, pos_ - start), start};
}

Token Lexer::stringLiteral() {
    size_t start = pos_;
    get(); // consume opening quote
    std::string value;
    bool closed = false;
    while (pos_ < input_.size()) {
        char c = get();
        if (c == '"') {
            if (peek() == '"') {
                value += '"';
                get();
            } else {
                closed = true;
                break;
            }
        } else {
            value += c;
        }
    }
    if (!closed) {
        throw std::runtime_error("Unterminated string literal");
    }
    return Token{TokenType::STRING, value, start};
}

Token Lexer::nextToken() {
    skipWhitespace();
    size_t start = pos_;
    char c = peek();
    if (c == '\0') {
        return Token{TokenType::END, "", pos_};
    }
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return identifier();
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
        return number();
    }
    if (c == '"') {
        return stringLiteral();
    }
    get();
    switch (c) {
        case ',':
            return Token{TokenType::COMMA, ",", start};
        case '(':
            return Token{TokenType::LPAREN, "(", start};
        case ')':
            return Token{TokenType::RPAREN, ")", start};
        case '*':
            return Token{TokenType::STAR, "*", start};
        case ';':
            return Token{TokenType::SEMICOLON, ";", start};
        case '=':
            return Token{TokenType::EQUALS, "=", start};
        default:
            throw std::runtime_error(std::string("Unexpected character: ") + c);
    }
}

const char *tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END:
            return "END";
        case TokenType::IDENTIFIER:
            return "IDENTIFIER";
        case TokenType::NUMBER:
            return "NUMBER";
        case TokenType::STRING:
            return "STRING";
        case TokenType::COMMA:
            return "COMMA";
        case TokenType::LPAREN:
            return "LPAREN";
        case TokenType::RPAREN:
            return "RPAREN";
        case TokenType::STAR:
            return "STAR";
        case TokenType::SEMICOLON:
            return "SEMICOLON";
        case TokenType::EQUALS:
            return "EQUALS";
        case TokenType::KEYWORD_CREATE:
            return "CREATE";
        case TokenType::KEYWORD_TABLE:
            return "TABLE";
        case TokenType::KEYWORD_INSERT:
            return "INSERT";
        case TokenType::KEYWORD_INTO:
            return "INTO";
        case TokenType::KEYWORD_VALUES:
            return "VALUES";
        case TokenType::KEYWORD_SELECT:
            return "SELECT";
        case TokenType::KEYWORD_FROM:
            return "FROM";
        case TokenType::KEYWORD_WHERE:
            return "WHERE";
    }
    return "UNKNOWN";
}
