#include "Parser.h"
#include <stdexcept>
#include <memory>

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

std::unique_ptr<Statement> Parser::parse() {
    if (tokens_.empty() || tokens_[0].type == TokenType::END) {
        return nullptr;
    }

    TokenType firstToken = tokens_[0].type;

    if (firstToken == TokenType::CREATE) {
        return parseCreateTable();
    } else if (firstToken == TokenType::INSERT) {
        return parseInsert();
    } else if (firstToken == TokenType::SELECT) {
        return parseSelect();
    } else {
        throw std::runtime_error("Unexpected token: " + tokens_[0].value);
    }
}

std::unique_ptr<Statement> Parser::parseCreateTable() {
    // Skip CREATE
    position_++;

    // Expect TABLE
    if (tokens_[position_].type != TokenType::TABLE) {
        throw std::runtime_error("Expected TABLE after CREATE");
    }
    position_++;

    // Get table name
    if (tokens_[position_].type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name after CREATE TABLE");
    }
    std::string tableName = tokens_[position_].value;
    position_++;

    // Expect (
    if (tokens_[position_].type != TokenType::LPAREN) {
        throw std::runtime_error("Expected '(' after table name");
    }
    position_++;

    // Parse columns
    std::vector<std::string> columns;
    while (true) {
        if (tokens_[position_].type == TokenType::RPAREN) {
            position_++;
            break;
        }

        if (tokens_[position_].type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected column name");
        }
        columns.push_back(tokens_[position_].value);
        position_++;

        if (tokens_[position_].type == TokenType::COMMA) {
            position_++;
        } else if (tokens_[position_].type == TokenType::RPAREN) {
            position_++;
            break;
        } else {
            throw std::runtime_error("Expected ',' or ')' after column name");
        }
    }

    // Expect semicolon
    if (tokens_[position_].type != TokenType::SEMICOLON) {
        throw std::runtime_error("Expected ';' at end of CREATE TABLE statement");
    }
    position_++;

    auto stmt = std::make_unique<CreateTableStatement>();
    stmt->tableName = tableName;
    stmt->columns = columns;
    return stmt;
}

std::unique_ptr<Statement> Parser::parseInsert() {
    // Skip INSERT
    position_++;

    // Expect INTO
    if (tokens_[position_].type != TokenType::INTO) {
        throw std::runtime_error("Expected INTO after INSERT");
    }
    position_++;

    // Get table name
    if (tokens_[position_].type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name after INSERT INTO");
    }
    std::string tableName = tokens_[position_].value;
    position_++;

    // Expect VALUES
    if (tokens_[position_].type != TokenType::VALUES) {
        throw std::runtime_error("Expected VALUES after table name");
    }
    position_++;

    // Expect (
    if (tokens_[position_].type != TokenType::LPAREN) {
        throw std::runtime_error("Expected '(' after VALUES");
    }
    position_++;

    // Parse values
    std::vector<std::string> values;
    while (true) {
        if (tokens_[position_].type == TokenType::RPAREN) {
            position_++;
            break;
        }

        if (tokens_[position_].type != TokenType::IDENTIFIER &&
            tokens_[position_].type != TokenType::STRING_LITERAL &&
            tokens_[position_].type != TokenType::NUMBER) {
            throw std::runtime_error("Expected value");
        }
        values.push_back(tokens_[position_].value);
        position_++;

        if (tokens_[position_].type == TokenType::COMMA) {
            position_++;
        } else if (tokens_[position_].type == TokenType::RPAREN) {
            position_++;
            break;
        } else {
            throw std::runtime_error("Expected ',' or ')' after value");
        }
    }

    // Expect semicolon
    if (tokens_[position_].type != TokenType::SEMICOLON) {
        throw std::runtime_error("Expected ';' at end of INSERT statement");
    }
    position_++;

    auto stmt = std::make_unique<InsertStatement>();
    stmt->tableName = tableName;
    stmt->values = values;
    return stmt;
}

std::unique_ptr<Statement> Parser::parseSelect() {
    // Skip SELECT
    position_++;

    auto stmt = std::make_unique<SelectStatement>();

    // Check for * or column list (we only support * in this implementation)
    if (tokens_[position_].type == TokenType::ASTERISK) {
        position_++;
    } else {
        // For simplicity, we only support SELECT * in this implementation
        throw std::runtime_error("Only SELECT * is supported");
    }

    // Expect FROM
    if (tokens_[position_].type != TokenType::FROM) {
        throw std::runtime_error("Expected FROM after SELECT *");
    }
    position_++;

    // Get table name
    if (tokens_[position_].type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name after FROM");
    }
    stmt->tableName = tokens_[position_].value;
    position_++;

    // Check for WHERE clause
    if (position_ < tokens_.size() && tokens_[position_].type == TokenType::WHERE) {
        position_++;
        stmt->hasWhere = true;

        // Get column name
        if (tokens_[position_].type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected column name after WHERE");
        }
        stmt->whereColumn = tokens_[position_].value;
        position_++;

        // Expect =
        if (tokens_[position_].type != TokenType::EQUALS) {
            throw std::runtime_error("Expected '=' after column name in WHERE");
        }
        position_++;

        // Get value
        if (tokens_[position_].type != TokenType::IDENTIFIER &&
            tokens_[position_].type != TokenType::STRING_LITERAL &&
            tokens_[position_].type != TokenType::NUMBER) {
            throw std::runtime_error("Expected value after '=' in WHERE");
        }
        stmt->whereValue = tokens_[position_].value;
        position_++;
    }

    // Expect semicolon
    if (tokens_[position_].type != TokenType::SEMICOLON) {
        throw std::runtime_error("Expected ';' at end of SELECT statement");
    }
    position_++;

    return stmt;
}
