#include "Parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), position_(0) {}

Token Parser::peek() const {
    if (position_ >= tokens_.size()) {
        return {TokenType::END_OF_INPUT, "", 0};
    }
    return tokens_[position_];
}

Token Parser::consume() {
    return tokens_[position_++];
}

std::unique_ptr<Statement> Parser::parse() {
    Token token = peek();

    if (token.type == TokenType::KEYWORD) {
        if (token.value == "CREATE") {
            return parseCreateTable();
        } else if (token.value == "INSERT") {
            return parseInsert();
        } else if (token.value == "SELECT") {
            return parseSelect();
        }
    }

    return nullptr;
}

std::unique_ptr<Statement> Parser::parseCreateTable() {
    // Skip CREATE
    consume();

    // Skip TABLE
    if (peek().value != "TABLE") {
        return nullptr;
    }
    consume();

    // Get table name
    Token tableNameToken = consume();
    if (tableNameToken.type != TokenType::IDENTIFIER) {
        return nullptr;
    }

    // Skip '('
    if (peek().value != "(") {
        return nullptr;
    }
    consume();

    // Parse columns
    std::vector<std::string> columns;
    while (true) {
        Token token = peek();
        if (token.value == ")") {
            break;
        }

        if (token.type == TokenType::IDENTIFIER) {
            columns.push_back(consume().value);
        } else {
            return nullptr;
        }

        // Check for comma or closing parenthesis
        token = peek();
        if (token.value == ")") {
            break;
        } else if (token.value == ",") {
            consume();
        } else {
            return nullptr;
        }
    }

    // Skip ')'
    consume();

    auto stmt = std::make_unique<CreateTableStatement>();
    stmt->tableName = tableNameToken.value;
    stmt->columns = columns;
    return stmt;
}

std::unique_ptr<Statement> Parser::parseInsert() {
    // Skip INSERT
    consume();

    // Skip INTO
    if (peek().value != "INTO") {
        return nullptr;
    }
    consume();

    // Get table name
    Token tableNameToken = consume();
    if (tableNameToken.type != TokenType::IDENTIFIER) {
        return nullptr;
    }

    // Skip VALUES
    if (peek().value != "VALUES") {
        return nullptr;
    }
    consume();

    // Skip '('
    if (peek().value != "(") {
        return nullptr;
    }
    consume();

    // Parse values
    std::vector<std::string> values;
    while (true) {
        Token token = peek();
        if (token.value == ")") {
            break;
        }

        if (token.type == TokenType::STRING_LITERAL || token.type == TokenType::NUMBER || token.type == TokenType::IDENTIFIER) {
            values.push_back(consume().value);
        } else {
            return nullptr;
        }

        // Check for comma or closing parenthesis
        token = peek();
        if (token.value == ")") {
            break;
        } else if (token.value == ",") {
            consume();
        } else {
            return nullptr;
        }
    }

    // Skip ')'
    consume();

    auto stmt = std::make_unique<InsertStatement>();
    stmt->tableName = tableNameToken.value;
    stmt->values = values;
    return stmt;
}

std::unique_ptr<Statement> Parser::parseSelect() {
    // Skip SELECT
    consume();

    // Skip *
    if (peek().value != "*") {
        return nullptr;
    }
    consume();

    // Skip FROM
    if (peek().value != "FROM") {
        return nullptr;
    }
    consume();

    // Get table name
    Token tableNameToken = consume();
    if (tableNameToken.type != TokenType::IDENTIFIER) {
        return nullptr;
    }

    // Check for WHERE clause
    bool hasWhere = false;
    std::string whereColumn;
    std::string whereValue;

    if (peek().value == "WHERE") {
        hasWhere = true;
        consume(); // Skip WHERE

        // Get column name
        Token columnToken = consume();
        if (columnToken.type != TokenType::IDENTIFIER) {
            return nullptr;
        }
        whereColumn = columnToken.value;

        // Skip =
        if (peek().value != "=") {
            return nullptr;
        }
        consume();

        // Get value
        Token valueToken = consume();
        if (valueToken.type != TokenType::STRING_LITERAL && valueToken.type != TokenType::NUMBER && valueToken.type != TokenType::IDENTIFIER) {
            return nullptr;
        }
        whereValue = valueToken.value;
    }

    auto stmt = std::make_unique<SelectStatement>();
    stmt->tableName = tableNameToken.value;
    stmt->hasWhere = hasWhere;
    stmt->whereColumn = whereColumn;
    stmt->whereValue = whereValue;
    return stmt;
}
