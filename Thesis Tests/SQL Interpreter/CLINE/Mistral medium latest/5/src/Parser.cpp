#include "Parser.h"
#include "Utils.h"
#include <stdexcept>
#include <iostream>

Parser::Parser(const std::vector<Token> &tokens) : tokens_(tokens) {}

std::unique_ptr<Statement> Parser::parse() {
    return parseStatement();
}

Token Parser::peek() const {
    if (position_ >= tokens_.size()) {
        return {TokenType::END_OF_INPUT, "", 0};
    }
    return tokens_[position_];
}

Token Parser::consume() {
    return tokens_[position_++];
}

std::unique_ptr<Statement> Parser::parseStatement() {
    Token token = peek();

    if (token.type == TokenType::KEYWORD && Utils::toLower(token.value) == "create") {
        return parseCreateTable();
    }
    else if (token.type == TokenType::KEYWORD && Utils::toLower(token.value) == "insert") {
        return parseInsert();
    }
    else if (token.type == TokenType::KEYWORD && Utils::toLower(token.value) == "select") {
        return parseSelect();
    }
    else {
        throw std::runtime_error("Unexpected token: " + token.value);
    }
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();

    // Consume CREATE
    consume();

    // Expect TABLE
    Token token = consume();
    if (token.type != TokenType::KEYWORD || Utils::toLower(token.value) != "table") {
        throw std::runtime_error("Expected 'TABLE' keyword");
    }

    // Expect table name
    token = consume();
    if (token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = token.value;

    // Expect (
    token = consume();
    if (token.type != TokenType::SYMBOL || token.value != "(") {
        throw std::runtime_error("Expected '(' after table name");
    }

    // Parse column list
    stmt->columns = parseColumnList();

    // Expect )
    token = consume();
    if (token.type != TokenType::SYMBOL || token.value != ")") {
        throw std::runtime_error("Expected ')' after column list");
    }

    // Expect ;
    token = consume();
    if (token.type != TokenType::SYMBOL || token.value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }

    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();

    // Consume INSERT
    consume();

    // Expect INTO
    Token token = consume();
    if (token.type != TokenType::KEYWORD || Utils::toLower(token.value) != "into") {
        throw std::runtime_error("Expected 'INTO' keyword");
    }

    // Expect table name
    token = consume();
    if (token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = token.value;

    // Expect VALUES
    token = consume();
    if (token.type != TokenType::KEYWORD || Utils::toLower(token.value) != "values") {
        throw std::runtime_error("Expected 'VALUES' keyword");
    }

    // Expect (
    token = consume();
    if (token.type != TokenType::SYMBOL || token.value != "(") {
        throw std::runtime_error("Expected '(' after VALUES");
    }

    // Parse value list
    stmt->values = parseValueList();

    // Expect )
    token = consume();
    if (token.type != TokenType::SYMBOL || token.value != ")") {
        throw std::runtime_error("Expected ')' after value list");
    }

    // Expect ;
    token = consume();
    if (token.type != TokenType::SYMBOL || token.value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }

    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();

    // Consume SELECT
    consume();

    // Expect *
    Token token = consume();
    if (token.type != TokenType::SYMBOL || token.value != "*") {
        throw std::runtime_error("Expected '*' after SELECT");
    }

    // Expect FROM
    token = consume();
    if (token.type != TokenType::KEYWORD || Utils::toLower(token.value) != "from") {
        throw std::runtime_error("Expected 'FROM' keyword");
    }

    // Expect table name
    token = consume();
    if (token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = token.value;

    // Check for WHERE clause
    token = peek();
    if (token.type == TokenType::KEYWORD && Utils::toLower(token.value) == "where") {
        consume(); // Consume WHERE
        auto whereClause = parseWhereClause();
        stmt->hasWhere = true;
        stmt->whereColumn = whereClause.first;
        stmt->whereValue = whereClause.second;
    }

    // Expect ;
    token = consume();
    if (token.type != TokenType::SYMBOL || token.value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }

    return stmt;
}

std::vector<std::string> Parser::parseColumnList() {
    std::vector<std::string> columns;

    while (true) {
        Token token = peek();
        if (token.type == TokenType::SYMBOL && token.value == ")") {
            break;
        }

        token = consume();
        if (token.type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected column name");
        }
        columns.push_back(token.value);

        token = peek();
        if (token.type == TokenType::SYMBOL && token.value == ",") {
            consume(); // Consume comma
        }
        else if (token.type == TokenType::SYMBOL && token.value == ")") {
            break;
        }
        else {
            throw std::runtime_error("Expected ',' or ')' after column name");
        }
    }

    return columns;
}

std::vector<std::string> Parser::parseValueList() {
    std::vector<std::string> values;

    while (true) {
        Token token = peek();
        if (token.type == TokenType::SYMBOL && token.value == ")") {
            break;
        }

        token = consume();
        if (token.type == TokenType::STRING_LITERAL || token.type == TokenType::NUMBER || token.type == TokenType::IDENTIFIER) {
            values.push_back(token.value);
        }
        else {
            throw std::runtime_error("Expected value");
        }

        token = peek();
        if (token.type == TokenType::SYMBOL && token.value == ",") {
            consume(); // Consume comma
        }
        else if (token.type == TokenType::SYMBOL && token.value == ")") {
            break;
        }
        else {
            throw std::runtime_error("Expected ',' or ')' after value");
        }
    }

    return values;
}

std::pair<std::string, std::string> Parser::parseWhereClause() {
    // Expect column name
    Token token = consume();
    if (token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected column name in WHERE clause");
    }
    std::string column = token.value;

    // Expect =
    token = consume();
    if (token.type != TokenType::SYMBOL || token.value != "=") {
        throw std::runtime_error("Expected '=' in WHERE clause");
    }

    // Expect value
    token = consume();
    if (token.type != TokenType::STRING_LITERAL && token.type != TokenType::NUMBER && token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected value in WHERE clause");
    }
    std::string value = token.value;

    return {column, value};
}
