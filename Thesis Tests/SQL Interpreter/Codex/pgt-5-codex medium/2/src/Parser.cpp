#include "Parser.h"

#include <stdexcept>
#include <string>

Parser::Parser(const std::vector<Token> &tokens) : tokens_(tokens), position_(0) {}

std::vector<StatementPtr> Parser::parseStatements() {
    std::vector<StatementPtr> statements;
    while (!atEnd()) {
        if (match(TokenType::SEMICOLON)) {
            continue;
        }
        statements.push_back(parseStatement());
        expect(TokenType::SEMICOLON, "Expected ';' at end of statement");
    }
    return statements;
}

StatementPtr Parser::parseStatement() {
    const Token &token = peek();
    switch (token.type) {
        case TokenType::KEYWORD_CREATE:
            return parseCreateTable();
        case TokenType::KEYWORD_INSERT:
            return parseInsert();
        case TokenType::KEYWORD_SELECT:
            return parseSelect();
        default:
            throw std::runtime_error("Unexpected token at start of statement: " + token.text);
    }
}

StatementPtr Parser::parseCreateTable() {
    expect(TokenType::KEYWORD_CREATE, "Expected CREATE keyword");
    expect(TokenType::KEYWORD_TABLE, "Expected TABLE keyword");
    auto stmt = std::make_unique<CreateTableStatement>();
    stmt->tableName = parseIdentifier("table name");
    expect(TokenType::LPAREN, "Expected '(' after table name");
    stmt->columns = parseIdentifierList();
    if (stmt->columns.empty()) {
        throw std::runtime_error("CREATE TABLE requires at least one column");
    }
    expect(TokenType::RPAREN, "Expected ')' after column list");
    return stmt;
}

StatementPtr Parser::parseInsert() {
    expect(TokenType::KEYWORD_INSERT, "Expected INSERT keyword");
    expect(TokenType::KEYWORD_INTO, "Expected INTO keyword");
    auto stmt = std::make_unique<InsertStatement>();
    stmt->tableName = parseIdentifier("table name");
    expect(TokenType::KEYWORD_VALUES, "Expected VALUES keyword");
    expect(TokenType::LPAREN, "Expected '(' before VALUES list");
    stmt->values = parseValueList();
    expect(TokenType::RPAREN, "Expected ')' after VALUES list");
    if (stmt->values.empty()) {
        throw std::runtime_error("INSERT requires at least one value");
    }
    return stmt;
}

StatementPtr Parser::parseSelect() {
    expect(TokenType::KEYWORD_SELECT, "Expected SELECT keyword");
    expect(TokenType::STAR, "Only SELECT * is supported");
    expect(TokenType::KEYWORD_FROM, "Expected FROM keyword");
    auto stmt = std::make_unique<SelectStatement>();
    stmt->tableName = parseIdentifier("table name");
    if (match(TokenType::KEYWORD_WHERE)) {
        stmt->hasWhere = true;
        stmt->whereColumn = parseIdentifier("column name in WHERE");
        expect(TokenType::EQUALS, "Expected '=' in WHERE clause");
        stmt->whereValue = parseValue("WHERE value");
    }
    return stmt;
}

std::string Parser::parseIdentifier(const std::string &context) {
    const Token &token = peek();
    if (token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected identifier for " + context + ", got: " + token.text);
    }
    advance();
    return token.text;
}

std::string Parser::parseValue(const std::string &context) {
    const Token &token = peek();
    if (token.type == TokenType::STRING_LITERAL || token.type == TokenType::IDENTIFIER) {
        advance();
        return token.text;
    }
    throw std::runtime_error("Expected value for " + context + ", got: " + token.text);
}

std::vector<std::string> Parser::parseIdentifierList() {
    std::vector<std::string> items;
    bool expectAnother = true;
    while (expectAnother) {
        items.push_back(parseIdentifier("identifier"));
        if (!match(TokenType::COMMA)) {
            expectAnother = false;
        }
    }
    return items;
}

std::vector<std::string> Parser::parseValueList() {
    std::vector<std::string> items;
    bool expectAnother = true;
    while (expectAnother) {
        items.push_back(parseValue("value"));
        if (!match(TokenType::COMMA)) {
            expectAnother = false;
        }
    }
    return items;
}

const Token &Parser::peek() const {
    return tokens_[position_];
}

const Token &Parser::previous() const {
    return tokens_[position_ - 1];
}

bool Parser::atEnd() const {
    return peek().type == TokenType::END;
}

const Token &Parser::advance() {
    if (!atEnd()) {
        ++position_;
    }
    return previous();
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TokenType type, const std::string &message) {
    if (!match(type)) {
        throw std::runtime_error(message + ", got: " + peek().text);
    }
}
