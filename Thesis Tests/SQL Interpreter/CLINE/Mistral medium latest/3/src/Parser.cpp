#include "Parser.h"
#include "Utils.h"
#include <stdexcept>
#include <memory>

Parser::Parser() {}

std::unique_ptr<Statement> Parser::parse(const std::string &sql) {
    tokens_ = lexer_.tokenize(sql);
    currentToken_ = 0;

    try {
        return parseStatement();
    } catch (const std::exception &e) {
        throw std::runtime_error("Parse error: " + std::string(e.what()));
    }
}

void Parser::advance() {
    if (currentToken_ < tokens_.size()) {
        currentToken_++;
    }
}

Token Parser::peek() const {
    if (currentToken_ >= tokens_.size()) {
        return {TokenType::END_OF_INPUT, "", 0};
    }
    return tokens_[currentToken_];
}

Token Parser::consume() {
    Token token = peek();
    advance();
    return token;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    Token token = peek();

    if (token.type == TokenType::KEYWORD && Utils::toLower(token.value) == "create") {
        return parseCreateTable();
    } else if (token.type == TokenType::KEYWORD && Utils::toLower(token.value) == "insert") {
        return parseInsert();
    } else if (token.type == TokenType::KEYWORD && Utils::toLower(token.value) == "select") {
        return parseSelect();
    } else {
        throw std::runtime_error("Unexpected token: " + token.value);
    }
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();

    // Consume CREATE
    consume();

    // Consume TABLE
    Token tableToken = consume();
    if (tableToken.type != TokenType::KEYWORD || Utils::toLower(tableToken.value) != "table") {
        throw std::runtime_error("Expected 'TABLE' after 'CREATE'");
    }

    // Parse table name
    Token nameToken = consume();
    if (nameToken.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = nameToken.value;

    // Consume '('
    Token openParen = consume();
    if (openParen.type != TokenType::SYMBOL || openParen.value != "(") {
        throw std::runtime_error("Expected '(' after table name");
    }

    // Parse columns
    while (true) {
        Token columnToken = peek();
        if (columnToken.type == TokenType::SYMBOL && columnToken.value == ")") {
            break;
        }

        if (columnToken.type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected column name");
        }

        stmt->columns.push_back(consume().value);

        Token nextToken = peek();
        if (nextToken.type == TokenType::SYMBOL && nextToken.value == ")") {
            break;
        } else if (nextToken.type == TokenType::SYMBOL && nextToken.value == ",") {
            consume(); // Consume comma
        } else {
            throw std::runtime_error("Expected ',' or ')' after column name");
        }
    }

    // Consume ')'
    consume();

    // Consume ';'
    Token semicolon = consume();
    if (semicolon.type != TokenType::SYMBOL || semicolon.value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }

    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();

    // Consume INSERT
    consume();

    // Consume INTO
    Token intoToken = consume();
    if (intoToken.type != TokenType::KEYWORD || Utils::toLower(intoToken.value) != "into") {
        throw std::runtime_error("Expected 'INTO' after 'INSERT'");
    }

    // Parse table name
    Token nameToken = consume();
    if (nameToken.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = nameToken.value;

    // Consume VALUES
    Token valuesToken = consume();
    if (valuesToken.type != TokenType::KEYWORD || Utils::toLower(valuesToken.value) != "values") {
        throw std::runtime_error("Expected 'VALUES' after table name");
    }

    // Consume '('
    Token openParen = consume();
    if (openParen.type != TokenType::SYMBOL || openParen.value != "(") {
        throw std::runtime_error("Expected '(' after VALUES");
    }

    // Parse values
    while (true) {
        Token valueToken = peek();
        if (valueToken.type == TokenType::SYMBOL && valueToken.value == ")") {
            break;
        }

        if (valueToken.type != TokenType::STRING && valueToken.type != TokenType::NUMBER && valueToken.type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected value");
        }

        stmt->values.push_back(consume().value);

        Token nextToken = peek();
        if (nextToken.type == TokenType::SYMBOL && nextToken.value == ")") {
            break;
        } else if (nextToken.type == TokenType::SYMBOL && nextToken.value == ",") {
            consume(); // Consume comma
        } else {
            throw std::runtime_error("Expected ',' or ')' after value");
        }
    }

    // Consume ')'
    consume();

    // Consume ';'
    Token semicolon = consume();
    if (semicolon.type != TokenType::SYMBOL || semicolon.value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }

    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();

    // Consume SELECT
    consume();

    // Consume '*'
    Token starToken = consume();
    if (starToken.type != TokenType::SYMBOL || starToken.value != "*") {
        throw std::runtime_error("Expected '*' after SELECT");
    }

    // Consume FROM
    Token fromToken = consume();
    if (fromToken.type != TokenType::KEYWORD || Utils::toLower(fromToken.value) != "from") {
        throw std::runtime_error("Expected 'FROM' after '*'");
    }

    // Parse table name
    Token nameToken = consume();
    if (nameToken.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = nameToken.value;

    // Check for WHERE
    Token whereToken = peek();
    if (whereToken.type == TokenType::KEYWORD && Utils::toLower(whereToken.value) == "where") {
        stmt->hasWhere = true;
        consume(); // Consume WHERE

        // Parse column name
        Token columnToken = consume();
        if (columnToken.type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected column name after WHERE");
        }
        stmt->whereColumn = columnToken.value;

        // Consume '='
        Token equalsToken = consume();
        if (equalsToken.type != TokenType::SYMBOL || equalsToken.value != "=") {
            throw std::runtime_error("Expected '=' after column name");
        }

        // Parse value
        Token valueToken = consume();
        if (valueToken.type != TokenType::STRING && valueToken.type != TokenType::NUMBER && valueToken.type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected value after '='");
        }
        stmt->whereValue = valueToken.value;
    }

    // Consume ';'
    Token semicolon = consume();
    if (semicolon.type != TokenType::SYMBOL || semicolon.value != ";") {
        throw std::runtime_error("Expected ';' at end of statement");
    }

    return stmt;
}
