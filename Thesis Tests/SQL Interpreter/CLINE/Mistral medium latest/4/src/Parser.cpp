#include "Parser.h"
#include "Utils.h"
#include <stdexcept>

Parser::Parser() : currentToken_(0) {}

std::unique_ptr<Statement> Parser::parse(const std::string &sql) {
    tokens_ = lexer_.tokenize(sql);
    currentToken_ = 0;
    return parseStatement();
}

void Parser::advance() {
    if (currentToken_ < tokens_.size()) {
        currentToken_++;
    }
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (currentToken_ >= tokens_.size()) {
        return nullptr;
    }

    const Token& token = tokens_[currentToken_];

    if (token.type == TokenType::KEYWORD) {
        std::string keyword = Utils::toLower(token.value);

        if (keyword == "create") {
            return parseCreateTable();
        } else if (keyword == "insert") {
            return parseInsert();
        } else if (keyword == "select") {
            return parseSelect();
        }
    }

    throw std::runtime_error("Unexpected token: " + token.value);
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();

    // Skip "CREATE"
    advance();

    // Expect "TABLE"
    if (currentToken_ >= tokens_.size() ||
        Utils::toLower(tokens_[currentToken_].value) != "table") {
        throw std::runtime_error("Expected 'TABLE' after 'CREATE'");
    }
    advance();

    // Expect table name
    if (currentToken_ >= tokens_.size() ||
        tokens_[currentToken_].type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name after 'CREATE TABLE'");
    }
    stmt->tableName = tokens_[currentToken_].value;
    advance();

    // Expect '('
    if (currentToken_ >= tokens_.size() ||
        tokens_[currentToken_].value != "(") {
        throw std::runtime_error("Expected '(' after table name");
    }
    advance();

    // Parse column list
    while (currentToken_ < tokens_.size() &&
           tokens_[currentToken_].value != ")") {
        if (tokens_[currentToken_].type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected column name");
        }
        stmt->columns.push_back(tokens_[currentToken_].value);
        advance();

        // Check for comma or closing parenthesis
        if (currentToken_ < tokens_.size() &&
            tokens_[currentToken_].value == ",") {
            advance();
        }
    }

    // Expect ')'
    if (currentToken_ >= tokens_.size() ||
        tokens_[currentToken_].value != ")") {
        throw std::runtime_error("Expected ')' after column list");
    }
    advance();

    // Expect semicolon
    if (currentToken_ >= tokens_.size() ||
        tokens_[currentToken_].value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }
    advance();

    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();

    // Skip "INSERT"
    advance();

    // Expect "INTO"
    if (currentToken_ >= tokens_.size() ||
        Utils::toLower(tokens_[currentToken_].value) != "into") {
        throw std::runtime_error("Expected 'INTO' after 'INSERT'");
    }
    advance();

    // Expect table name
    if (currentToken_ >= tokens_.size() ||
        tokens_[currentToken_].type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name after 'INSERT INTO'");
    }
    stmt->tableName = tokens_[currentToken_].value;
    advance();

    // Expect "VALUES"
    if (currentToken_ >= tokens_.size() ||
        Utils::toLower(tokens_[currentToken_].value) != "values") {
        throw std::runtime_error("Expected 'VALUES' after table name");
    }
    advance();

    // Expect '('
    if (currentToken_ >= tokens_.size() ||
        tokens_[currentToken_].value != "(") {
        throw std::runtime_error("Expected '(' after 'VALUES'");
    }
    advance();

    // Parse value list
    while (currentToken_ < tokens_.size() &&
           tokens_[currentToken_].value != ")") {
        if (tokens_[currentToken_].type != TokenType::STRING &&
            tokens_[currentToken_].type != TokenType::NUMBER &&
            tokens_[currentToken_].type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected value");
        }
        stmt->values.push_back(tokens_[currentToken_].value);
        advance();

        // Check for comma or closing parenthesis
        if (currentToken_ < tokens_.size() &&
            tokens_[currentToken_].value == ",") {
            advance();
        }
    }

    // Expect ')'
    if (currentToken_ >= tokens_.size() ||
        tokens_[currentToken_].value != ")") {
        throw std::runtime_error("Expected ')' after value list");
    }
    advance();

    // Expect semicolon
    if (currentToken_ >= tokens_.size() ||
        tokens_[currentToken_].value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }
    advance();

    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();

    // Skip "SELECT"
    advance();

    // Expect '*'
    if (currentToken_ >= tokens_.size() ||
        tokens_[currentToken_].value != "*") {
        throw std::runtime_error("Expected '*' after 'SELECT'");
    }
    advance();

    // Expect "FROM"
    if (currentToken_ >= tokens_.size() ||
        Utils::toLower(tokens_[currentToken_].value) != "from") {
        throw std::runtime_error("Expected 'FROM' after '*'");
    }
    advance();

    // Expect table name
    if (currentToken_ >= tokens_.size() ||
        tokens_[currentToken_].type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name after 'FROM'");
    }
    stmt->tableName = tokens_[currentToken_].value;
    advance();

    // Check for WHERE clause
    if (currentToken_ < tokens_.size() &&
        Utils::toLower(tokens_[currentToken_].value) == "where") {
        stmt->hasWhere = true;
        advance();

        // Expect column name
        if (currentToken_ >= tokens_.size() ||
            tokens_[currentToken_].type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected column name after 'WHERE'");
        }
        stmt->whereColumn = tokens_[currentToken_].value;
        advance();

        // Expect '='
        if (currentToken_ >= tokens_.size() ||
            tokens_[currentToken_].value != "=") {
            throw std::runtime_error("Expected '=' after column name in WHERE clause");
        }
        advance();

        // Expect value
        if (currentToken_ >= tokens_.size() ||
            (tokens_[currentToken_].type != TokenType::STRING &&
             tokens_[currentToken_].type != TokenType::NUMBER &&
             tokens_[currentToken_].type != TokenType::IDENTIFIER)) {
            throw std::runtime_error("Expected value after '=' in WHERE clause");
        }
        stmt->whereValue = tokens_[currentToken_].value;
        advance();
    }

    // Expect semicolon
    if (currentToken_ >= tokens_.size() ||
        tokens_[currentToken_].value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }
    advance();

    return stmt;
}
