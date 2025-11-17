#include "Parser.h"
#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

Token Parser::peek() const {
    if (position_ >= tokens_.size()) {
        return {TokenType::END_OF_INPUT, "", 0};
    }
    return tokens_[position_];
}

Token Parser::consume() {
    return tokens_[position_++];
}

void Parser::expect(TokenType type, const std::string& value) {
    Token token = consume();
    if (token.type != type || (!value.empty() && token.value != value)) {
        throw std::runtime_error("Unexpected token: " + token.value);
    }
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();

    // Skip "CREATE TABLE"
    expect(TokenType::KEYWORD, "CREATE");
    expect(TokenType::KEYWORD, "TABLE");

    // Table name
    stmt->tableName = consume().value;

    // Columns
    expect(TokenType::SYMBOL, "(");
    while (peek().type != TokenType::SYMBOL || peek().value != ")") {
        stmt->columns.push_back(consume().value);
        if (peek().type == TokenType::SYMBOL && peek().value == ",") {
            consume(); // Skip comma
        }
    }
    expect(TokenType::SYMBOL, ")");
    expect(TokenType::SYMBOL, ";");

    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();

    // Skip "INSERT INTO"
    expect(TokenType::KEYWORD, "INSERT");
    expect(TokenType::KEYWORD, "INTO");

    // Table name
    stmt->tableName = consume().value;

    // Skip "VALUES"
    expect(TokenType::KEYWORD, "VALUES");
    expect(TokenType::SYMBOL, "(");

    // Values
    while (peek().type != TokenType::SYMBOL || peek().value != ")") {
        Token token = consume();
        if (token.type == TokenType::STRING_LITERAL || token.type == TokenType::NUMBER_LITERAL) {
            stmt->values.push_back(token.value);
        } else {
            throw std::runtime_error("Expected value, got: " + token.value);
        }

        if (peek().type == TokenType::SYMBOL && peek().value == ",") {
            consume(); // Skip comma
        }
    }

    expect(TokenType::SYMBOL, ")");
    expect(TokenType::SYMBOL, ";");

    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();

    // Skip "SELECT * FROM"
    expect(TokenType::KEYWORD, "SELECT");
    expect(TokenType::SYMBOL, "*");
    expect(TokenType::KEYWORD, "FROM");

    // Table name
    stmt->tableName = consume().value;

    // WHERE clause (optional)
    if (peek().type == TokenType::KEYWORD && peek().value == "WHERE") {
        consume(); // Skip "WHERE"
        stmt->hasWhere = true;
        stmt->whereColumn = consume().value;
        expect(TokenType::SYMBOL, "=");
        stmt->whereValue = consume().value;
    }

    expect(TokenType::SYMBOL, ";");

    return stmt;
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

    throw std::runtime_error("Unexpected statement: " + token.value);
}
