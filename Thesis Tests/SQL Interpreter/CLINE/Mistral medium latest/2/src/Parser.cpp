#include "Parser.h"
#include <stdexcept>
#include <algorithm>

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

std::unique_ptr<Statement> Parser::parse() {
    Token token = peek();
    if (token.type == TokenType::END_OF_INPUT) {
        return nullptr;
    }

    if (token.value == "CREATE") {
        return parseCreateTable();
    } else if (token.value == "INSERT") {
        return parseInsert();
    } else if (token.value == "SELECT") {
        return parseSelect();
    } else {
        throw std::runtime_error("Unexpected token: " + token.value);
    }
}

Token Parser::peek() const {
    if (position_ >= tokens_.size()) {
        return {TokenType::END_OF_INPUT, "", 0, 0};
    }
    return tokens_[position_];
}

Token Parser::consume() {
    return tokens_[position_++];
}

std::unique_ptr<Statement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();

    // Skip CREATE
    consume();

    // Expect TABLE
    Token token = consume();
    if (token.value != "TABLE") {
        throw std::runtime_error("Expected TABLE keyword");
    }

    // Get table name
    token = consume();
    if (token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = token.value;

    // Get column list
    token = consume();
    if (token.value != "(") {
        throw std::runtime_error("Expected '(' after table name");
    }

    stmt->columns = parseColumnList();

    // Expect closing parenthesis
    token = consume();
    if (token.value != ")") {
        throw std::runtime_error("Expected ')' after column list");
    }

    // Expect semicolon
    token = consume();
    if (token.value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }

    return stmt;
}

std::unique_ptr<Statement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();

    // Skip INSERT
    consume();

    // Expect INTO
    Token token = consume();
    if (token.value != "INTO") {
        throw std::runtime_error("Expected INTO keyword");
    }

    // Get table name
    token = consume();
    if (token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = token.value;

    // Expect VALUES
    token = consume();
    if (token.value != "VALUES") {
        throw std::runtime_error("Expected VALUES keyword");
    }

    // Get value list
    token = consume();
    if (token.value != "(") {
        throw std::runtime_error("Expected '(' after VALUES");
    }

    stmt->values = parseValueList();

    // Expect closing parenthesis
    token = consume();
    if (token.value != ")") {
        throw std::runtime_error("Expected ')' after value list");
    }

    // Expect semicolon
    token = consume();
    if (token.value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }

    return stmt;
}

std::unique_ptr<Statement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();

    // Skip SELECT
    consume();

    // Expect *
    Token token = consume();
    if (token.value != "*") {
        throw std::runtime_error("Expected '*' after SELECT");
    }

    // Expect FROM
    token = consume();
    if (token.value != "FROM") {
        throw std::runtime_error("Expected FROM keyword");
    }

    // Get table name
    token = consume();
    if (token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = token.value;

    // Check for WHERE clause
    token = peek();
    if (token.value == "WHERE") {
        consume(); // Skip WHERE
        auto whereClause = parseWhereClause();
        stmt->hasWhere = true;
        stmt->whereColumn = whereClause.first;
        stmt->whereValue = whereClause.second;
    }

    // Expect semicolon
    token = consume();
    if (token.value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }

    return stmt;
}

std::vector<std::string> Parser::parseColumnList() {
    std::vector<std::string> columns;

    while (true) {
        Token token = peek();
        if (token.value == ")") {
            break;
        }

        token = consume();
        if (token.type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected column name");
        }
        columns.push_back(token.value);

        token = peek();
        if (token.value == ")") {
            break;
        } else if (token.value == ",") {
            consume(); // Skip comma
        } else {
            throw std::runtime_error("Expected ',' or ')' after column name");
        }
    }

    return columns;
}

std::vector<std::string> Parser::parseValueList() {
    std::vector<std::string> values;

    while (true) {
        Token token = peek();
        if (token.value == ")") {
            break;
        }

        token = consume();
        if (token.type == TokenType::STRING_LITERAL || token.type == TokenType::NUMBER || token.type == TokenType::IDENTIFIER) {
            values.push_back(token.value);
        } else {
            throw std::runtime_error("Expected value");
        }

        token = peek();
        if (token.value == ")") {
            break;
        } else if (token.value == ",") {
            consume(); // Skip comma
        } else {
            throw std::runtime_error("Expected ',' or ')' after value");
        }
    }

    return values;
}

std::pair<std::string, std::string> Parser::parseWhereClause() {
    // Get column name
    Token token = consume();
    if (token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected column name in WHERE clause");
    }
    std::string column = token.value;

    // Expect =
    token = consume();
    if (token.value != "=") {
        throw std::runtime_error("Expected '=' in WHERE clause");
    }

    // Get value
    token = consume();
    if (token.type != TokenType::STRING_LITERAL && token.type != TokenType::NUMBER && token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected value in WHERE clause");
    }
    std::string value = token.value;

    return {column, value};
}
