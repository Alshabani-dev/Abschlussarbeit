#include "Parser.h"
#include <memory>
#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

std::unique_ptr<Statement> Parser::parse() {
    if (tokens_.empty() || tokens_[0].type != Token::Type::KEYWORD) {
        throw std::runtime_error("Expected a statement");
    }

    const std::string& keyword = tokens_[0].value;

    if (keyword == "CREATE") {
        return parseCreateTable();
    } else if (keyword == "INSERT") {
        return parseInsert();
    } else if (keyword == "SELECT") {
        return parseSelect();
    } else {
        throw std::runtime_error("Unknown statement type: " + keyword);
    }
}

std::unique_ptr<Statement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();

    // Skip CREATE
    position_++;
    // Skip TABLE
    if (position_ >= tokens_.size() || tokens_[position_].value != "TABLE") {
        throw std::runtime_error("Expected TABLE keyword");
    }
    position_++;

    // Get table name
    if (position_ >= tokens_.size() || tokens_[position_].type != Token::Type::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = tokens_[position_++].value;

    // Get column list
    if (position_ >= tokens_.size() || tokens_[position_].value != "(") {
        throw std::runtime_error("Expected '(' after table name");
    }
    position_++;

    while (position_ < tokens_.size() && tokens_[position_].value != ")") {
        if (tokens_[position_].type != Token::Type::IDENTIFIER) {
            throw std::runtime_error("Expected column name");
        }
        stmt->columns.push_back(tokens_[position_++].value);

        if (position_ < tokens_.size() && tokens_[position_].value == ",") {
            position_++;
        }
    }

    if (position_ >= tokens_.size() || tokens_[position_].value != ")") {
        throw std::runtime_error("Expected ')' to close column list");
    }
    position_++;

    if (position_ >= tokens_.size() || tokens_[position_].value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }
    position_++;

    return stmt;
}

std::unique_ptr<Statement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();

    // Skip INSERT
    position_++;
    // Skip INTO
    if (position_ >= tokens_.size() || tokens_[position_].value != "INTO") {
        throw std::runtime_error("Expected INTO keyword");
    }
    position_++;

    // Get table name
    if (position_ >= tokens_.size() || tokens_[position_].type != Token::Type::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = tokens_[position_++].value;

    // Skip VALUES
    if (position_ >= tokens_.size() || tokens_[position_].value != "VALUES") {
        throw std::runtime_error("Expected VALUES keyword");
    }
    position_++;

    // Get value list
    if (position_ >= tokens_.size() || tokens_[position_].value != "(") {
        throw std::runtime_error("Expected '(' after VALUES");
    }
    position_++;

    while (position_ < tokens_.size() && tokens_[position_].value != ")") {
        if (tokens_[position_].type != Token::Type::STRING &&
            tokens_[position_].type != Token::Type::NUMBER &&
            tokens_[position_].type != Token::Type::IDENTIFIER) {
            throw std::runtime_error("Expected value");
        }
        stmt->values.push_back(tokens_[position_++].value);

        if (position_ < tokens_.size() && tokens_[position_].value == ",") {
            position_++;
        }
    }

    if (position_ >= tokens_.size() || tokens_[position_].value != ")") {
        throw std::runtime_error("Expected ')' to close value list");
    }
    position_++;

    if (position_ >= tokens_.size() || tokens_[position_].value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }
    position_++;

    return stmt;
}

std::unique_ptr<Statement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();

    // Skip SELECT
    position_++;
    // Skip *
    if (position_ >= tokens_.size() || tokens_[position_].value != "*") {
        throw std::runtime_error("Expected '*' after SELECT");
    }
    position_++;

    // Skip FROM
    if (position_ >= tokens_.size() || tokens_[position_].value != "FROM") {
        throw std::runtime_error("Expected FROM keyword");
    }
    position_++;

    // Get table name
    if (position_ >= tokens_.size() || tokens_[position_].type != Token::Type::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = tokens_[position_++].value;

    // Check for WHERE clause
    if (position_ < tokens_.size() && tokens_[position_].value == "WHERE") {
        stmt->hasWhere = true;
        position_++;

        // Get column name
        if (position_ >= tokens_.size() || tokens_[position_].type != Token::Type::IDENTIFIER) {
            throw std::runtime_error("Expected column name in WHERE clause");
        }
        stmt->whereColumn = tokens_[position_++].value;

        // Skip =
        if (position_ >= tokens_.size() || tokens_[position_].value != "=") {
            throw std::runtime_error("Expected '=' in WHERE clause");
        }
        position_++;

        // Get value
        if (position_ >= tokens_.size() ||
            (tokens_[position_].type != Token::Type::STRING &&
             tokens_[position_].type != Token::Type::NUMBER &&
             tokens_[position_].type != Token::Type::IDENTIFIER)) {
            throw std::runtime_error("Expected value in WHERE clause");
        }
        stmt->whereValue = tokens_[position_++].value;
    }

    if (position_ >= tokens_.size() || tokens_[position_].value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }
    position_++;

    return stmt;
}
