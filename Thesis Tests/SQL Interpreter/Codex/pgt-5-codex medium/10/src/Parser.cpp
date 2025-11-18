#include "Parser.h"

#include <stdexcept>

#include "Utils.h"

Parser::Parser(const std::string &input) {
    Lexer lexer(input);
    tokens_ = lexer.tokenize();
}

StatementPtr Parser::parseStatement() {
    if (isEnd()) {
        throw std::runtime_error("Empty statement");
    }

    const Token &token = peek();
    if (token.type == TokenType::Keyword) {
        if (token.text == "CREATE") {
            return parseCreateTable();
        }
        if (token.text == "INSERT") {
            return parseInsert();
        }
        if (token.text == "SELECT") {
            return parseSelect();
        }
    }
    throw std::runtime_error("Unknown statement type");
}

const Token &Parser::peek() const {
    if (position_ >= tokens_.size()) {
        static Token eofToken{TokenType::EndOfFile, ""};
        return eofToken;
    }
    return tokens_[position_];
}

const Token &Parser::get() {
    if (position_ < tokens_.size()) {
        return tokens_[position_++];
    }
    static Token eofToken{TokenType::EndOfFile, ""};
    return eofToken;
}

bool Parser::matchKeyword(const std::string &keyword) {
    if (peek().type == TokenType::Keyword && peek().text == keyword) {
        get();
        return true;
    }
    return false;
}

void Parser::expect(TokenType type, const std::string &message) {
    if (peek().type != type) {
        throw std::runtime_error(message);
    }
    get();
}

void Parser::expectKeyword(const std::string &keyword) {
    if (!matchKeyword(keyword)) {
        throw std::runtime_error("Expected keyword " + keyword);
    }
}

bool Parser::isEnd() const {
    return peek().type == TokenType::EndOfFile;
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();
    expectKeyword("CREATE");
    expectKeyword("TABLE");

    if (peek().type != TokenType::Identifier) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = get().text;

    expect(TokenType::LParen, "Expected '('");
    stmt->columns = parseIdentifierList();
    expect(TokenType::RParen, "Expected ')'");
    expect(TokenType::Semicolon, "Expected ';'");
    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();
    expectKeyword("INSERT");
    expectKeyword("INTO");
    if (peek().type != TokenType::Identifier) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = get().text;

    expectKeyword("VALUES");
    expect(TokenType::LParen, "Expected '(' after VALUES");
    std::vector<std::string> values;
    while (true) {
        const Token &token = peek();
        if (token.type == TokenType::String || token.type == TokenType::Number || token.type == TokenType::Identifier) {
            values.push_back(get().text);
        } else {
            throw std::runtime_error("Expected literal value in VALUES clause");
        }
        if (peek().type == TokenType::Comma) {
            get();
            continue;
        }
        break;
    }
    expect(TokenType::RParen, "Expected ')' after VALUES list");
    expect(TokenType::Semicolon, "Expected ';'");
    stmt->values = values;
    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    expectKeyword("SELECT");
    if (peek().type == TokenType::Star) {
        get();
        stmt->columns = {"*"};
    } else {
        stmt->columns = parseIdentifierList();
    }
    expectKeyword("FROM");
    if (peek().type != TokenType::Identifier) {
        throw std::runtime_error("Expected table name after FROM");
    }
    stmt->tableName = get().text;

    if (matchKeyword("WHERE")) {
        Condition condition;
        if (peek().type != TokenType::Identifier) {
            throw std::runtime_error("Expected column name in WHERE clause");
        }
        condition.column = get().text;
        expect(TokenType::Equal, "Expected '=' in WHERE clause");
        const Token &valueToken = peek();
        if (valueToken.type != TokenType::String && valueToken.type != TokenType::Identifier && valueToken.type != TokenType::Number) {
            throw std::runtime_error("Expected literal value in WHERE clause");
        }
        condition.value = get().text;
        stmt->whereClause = condition;
    }

    expect(TokenType::Semicolon, "Expected ';'");
    return stmt;
}

std::vector<std::string> Parser::parseIdentifierList() {
    std::vector<std::string> items;
    while (true) {
        if (peek().type != TokenType::Identifier) {
            throw std::runtime_error("Expected identifier");
        }
        items.push_back(get().text);
        if (peek().type == TokenType::Comma) {
            get();
            continue;
        }
        break;
    }
    return items;
}
