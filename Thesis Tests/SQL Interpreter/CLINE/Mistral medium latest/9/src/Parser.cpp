#include "Parser.h"
#include "Lexer.h"
#include "Utils.h"
#include <stdexcept>
#include <memory>
#include <iostream>
#include <ostream>

std::unique_ptr<Statement> Parser::parse(const std::string &sql) {
    Lexer lexer;
    std::vector<Token> tokens = lexer.tokenize(sql);
    size_t pos = 0;

    try {
        return parseStatement(tokens, pos);
    } catch (const std::runtime_error &e) {
        // In a real implementation, we'd include more error details
        throw std::runtime_error("Parse error: " + std::string(e.what()));
    }
}

std::unique_ptr<Statement> Parser::parseStatement(const std::vector<Token> &tokens, size_t &pos) {
    if (pos >= tokens.size() || tokens[pos].type == TokenType::END_OF_INPUT) {
        throw std::runtime_error("Unexpected end of input");
    }

    // Print debug info for all tokens
    std::cerr << "DEBUG: Token: type=" << static_cast<int>(tokens[pos].type)
              << ", value='" << tokens[pos].value << "'" << std::endl;

    // Check for CREATE TABLE statement
    if (Utils::toLower(tokens[pos].value) == "create") {
        return parseCreateTable(tokens, pos);
    }
    // Check for INSERT statement
    else if (Utils::toLower(tokens[pos].value) == "insert") {
        return parseInsert(tokens, pos);
    }
    // Check for SELECT statement
    else if (Utils::toLower(tokens[pos].value) == "select") {
        return parseSelect(tokens, pos);
    }
    else {
        // Print debug info
        std::cerr << "DEBUG: Unexpected token: type=" << static_cast<int>(tokens[pos].type)
                  << ", value='" << tokens[pos].value << "'" << std::endl;
        throw std::runtime_error("Expected statement, got: " + tokens[pos].value);
    }
}

std::unique_ptr<Statement> Parser::parseCreateTable(const std::vector<Token> &tokens, size_t &pos) {
    auto stmt = std::make_unique<CreateTableStatement>();

    // Skip "CREATE"
    pos++;

    // Expect "TABLE"
    if (pos >= tokens.size() || Utils::toLower(tokens[pos].value) != "table") {
        throw std::runtime_error("Expected 'TABLE' after 'CREATE'");
    }
    pos++;

    // Expect table name (identifier)
    if (pos >= tokens.size()) {
        throw std::runtime_error("Expected table name after 'CREATE TABLE'");
    }
    stmt->tableName = tokens[pos].value;
    pos++;

    // Expect '('
    if (pos >= tokens.size() || tokens[pos].value != "(") {
        throw std::runtime_error("Expected '(' after table name");
    }
    pos++;

    // Parse column list
    while (pos < tokens.size() && tokens[pos].value != ")") {
        // Expect column name (identifier)
        if (pos >= tokens.size()) {
            throw std::runtime_error("Expected column name");
        }
        stmt->columns.push_back(tokens[pos].value);
        pos++;

        // Expect ',' or ')'
        if (pos < tokens.size()) {
            if (tokens[pos].value == ",") {
                pos++;
            } else if (tokens[pos].value == ")") {
                break;
            }
        }
    }

    // Expect ')'
    if (pos >= tokens.size() || tokens[pos].value != ")") {
        throw std::runtime_error("Expected ')' after column list");
    }
    pos++;

    // Expect ';'
    if (pos >= tokens.size() || tokens[pos].value != ";") {
        throw std::runtime_error("Expected ';' after CREATE TABLE statement");
    }
    pos++;

    return stmt;
}

std::unique_ptr<Statement> Parser::parseInsert(const std::vector<Token> &tokens, size_t &pos) {
    auto stmt = std::make_unique<InsertStatement>();

    // Skip "INSERT"
    pos++;

    // Expect "INTO"
    if (pos >= tokens.size() || Utils::toLower(tokens[pos].value) != "into") {
        throw std::runtime_error("Expected 'INTO' after 'INSERT'");
    }
    pos++;

    // Expect table name (identifier)
    if (pos >= tokens.size()) {
        throw std::runtime_error("Expected table name after 'INSERT INTO'");
    }
    stmt->tableName = tokens[pos].value;
    pos++;

    // Expect "VALUES"
    if (pos >= tokens.size() || Utils::toLower(tokens[pos].value) != "values") {
        throw std::runtime_error("Expected 'VALUES' after table name");
    }
    pos++;

    // Expect '('
    if (pos >= tokens.size() || tokens[pos].value != "(") {
        throw std::runtime_error("Expected '(' after 'VALUES'");
    }
    pos++;

    // Parse value list
    while (pos < tokens.size() && tokens[pos].value != ")") {
        // Expect value (string, number, or identifier)
        if (pos >= tokens.size()) {
            throw std::runtime_error("Expected value");
        }
        stmt->values.push_back(tokens[pos].value);
        pos++;

        // Expect ',' or ')'
        if (pos < tokens.size()) {
            if (tokens[pos].value == ",") {
                pos++;
            } else if (tokens[pos].value == ")") {
                break;
            }
        }
    }

    // Expect ')'
    if (pos >= tokens.size() || tokens[pos].value != ")") {
        throw std::runtime_error("Expected ')' after value list");
    }
    pos++;

    // Expect ';'
    if (pos >= tokens.size() || tokens[pos].value != ";") {
        throw std::runtime_error("Expected ';' after INSERT statement");
    }
    pos++;

    return stmt;
}

std::unique_ptr<Statement> Parser::parseSelect(const std::vector<Token> &tokens, size_t &pos) {
    auto stmt = std::make_unique<SelectStatement>();

    // Skip "SELECT"
    pos++;

    // Expect '*'
    if (pos >= tokens.size() || tokens[pos].value != "*") {
        throw std::runtime_error("Expected '*' after 'SELECT'");
    }
    pos++;

    // Expect "FROM"
    if (pos >= tokens.size() || Utils::toLower(tokens[pos].value) != "from") {
        throw std::runtime_error("Expected 'FROM' after 'SELECT *'");
    }
    pos++;

    // Expect table name (identifier)
    if (pos >= tokens.size()) {
        throw std::runtime_error("Expected table name after 'FROM'");
    }
    stmt->tableName = tokens[pos].value;
    pos++;

    // Check for WHERE clause
    if (pos < tokens.size() && Utils::toLower(tokens[pos].value) == "where") {
        stmt->hasWhere = true;
        pos++;

        // Expect column name (identifier)
        if (pos >= tokens.size()) {
            throw std::runtime_error("Expected column name after 'WHERE'");
        }
        stmt->whereColumn = tokens[pos].value;
        pos++;

        // Expect '='
        if (pos >= tokens.size() || tokens[pos].value != "=") {
            throw std::runtime_error("Expected '=' after column name in WHERE clause");
        }
        pos++;

        // Expect value (string, number, or identifier)
        if (pos >= tokens.size()) {
            throw std::runtime_error("Expected value after '=' in WHERE clause");
        }
        stmt->whereValue = tokens[pos].value;
        pos++;
    }

    // Expect ';'
    if (pos >= tokens.size() || tokens[pos].value != ";") {
        throw std::runtime_error("Expected ';' after SELECT statement");
    }
    pos++;

    return stmt;
}
