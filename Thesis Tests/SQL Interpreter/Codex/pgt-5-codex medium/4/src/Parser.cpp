#include "Parser.h"

#include <stdexcept>

#include "Utils.h"

Parser::Parser(const std::string &input) {
    Lexer lexer(input);
    while (true) {
        Token token = lexer.nextToken();
        tokens_.push_back(token);
        if (token.type == TokenType::END) {
            break;
        }
    }
}

const Token &Parser::peek() const {
    if (index_ >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[index_];
}

const Token &Parser::consume() {
    const Token &token = peek();
    if (index_ < tokens_.size()) {
        ++index_;
    }
    return token;
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        consume();
        return true;
    }
    return false;
}

const Token &Parser::expect(TokenType type, const std::string &message) {
    if (peek().type != type) {
        throw std::runtime_error(message + ", found " + tokenTypeToString(peek().type));
    }
    return consume();
}

void Parser::expectEnd() {
    if (peek().type != TokenType::END) {
        throw std::runtime_error("Unexpected trailing input after statement");
    }
}

StatementPtr Parser::parseStatement() {
    switch (peek().type) {
        case TokenType::KEYWORD_CREATE:
            return parseCreateTable();
        case TokenType::KEYWORD_INSERT:
            return parseInsert();
        case TokenType::KEYWORD_SELECT:
            return parseSelect();
        default:
            throw std::runtime_error("Unexpected token at start of statement: " + std::string(tokenTypeToString(peek().type)));
    }
}

StatementPtr Parser::parseCreateTable() {
    consume(); // CREATE
    expect(TokenType::KEYWORD_TABLE, "Expected TABLE keyword");
    auto stmt = std::make_unique<CreateTableStatement>();
    stmt->tableName = parseIdentifier();
    expect(TokenType::LPAREN, "Expected '('");
    stmt->columns = parseIdentifierList();
    expect(TokenType::RPAREN, "Expected ')'");
    expect(TokenType::SEMICOLON, "Missing ';' at end of CREATE TABLE");
    expectEnd();
    return stmt;
}

StatementPtr Parser::parseInsert() {
    consume(); // INSERT
    expect(TokenType::KEYWORD_INTO, "Expected INTO keyword");
    auto stmt = std::make_unique<InsertStatement>();
    stmt->tableName = parseIdentifier();
    expect(TokenType::KEYWORD_VALUES, "Expected VALUES keyword");
    expect(TokenType::LPAREN, "Expected '('");
    stmt->values = parseValueList();
    expect(TokenType::RPAREN, "Expected ')'");
    expect(TokenType::SEMICOLON, "Missing ';' at end of INSERT");
    expectEnd();
    return stmt;
}

StatementPtr Parser::parseSelect() {
    consume(); // SELECT
    expect(TokenType::STAR, "Only SELECT * is supported");
    expect(TokenType::KEYWORD_FROM, "Expected FROM keyword");
    auto stmt = std::make_unique<SelectStatement>();
    stmt->tableName = parseIdentifier();
    if (match(TokenType::KEYWORD_WHERE)) {
        stmt->hasWhere = true;
        stmt->whereColumn = parseIdentifier();
        expect(TokenType::EQUALS, "Expected '=' in WHERE clause");
        stmt->whereValue = parseValueToken();
    }
    expect(TokenType::SEMICOLON, "Missing ';' at end of SELECT");
    expectEnd();
    return stmt;
}

std::string Parser::parseIdentifier() {
    const Token &token = peek();
    if (token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected identifier but found " + std::string(tokenTypeToString(token.type)));
    }
    consume();
    return Utils::toLower(token.text);
}

std::vector<std::string> Parser::parseIdentifierList() {
    std::vector<std::string> ids;
    ids.push_back(parseIdentifier());
    while (match(TokenType::COMMA)) {
        ids.push_back(parseIdentifier());
    }
    return ids;
}

std::vector<std::string> Parser::parseValueList() {
    std::vector<std::string> values;
    values.push_back(parseValueToken());
    while (match(TokenType::COMMA)) {
        values.push_back(parseValueToken());
    }
    return values;
}

std::string Parser::parseValueToken() {
    const Token &token = peek();
    if (token.type == TokenType::STRING || token.type == TokenType::IDENTIFIER || token.type == TokenType::NUMBER) {
        consume();
        return token.text;
    }
    throw std::runtime_error("Expected literal value but found " + std::string(tokenTypeToString(token.type)));
}
