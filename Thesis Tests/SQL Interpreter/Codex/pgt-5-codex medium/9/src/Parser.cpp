#include "Parser.h"

#include <sstream>

Parser::Parser(const std::vector<Token> &tokens)
    : tokens_(tokens), position_(0) {}

std::optional<Statement> Parser::parseStatement() {
    if (tokens_.empty()) {
        error_ = "No tokens to parse";
        return std::nullopt;
    }

    const Token &token = peek();
    std::optional<Statement> statement;
    switch (token.type) {
    case TokenType::KEYWORD_CREATE:
        statement = parseCreateTable();
        break;
    case TokenType::KEYWORD_INSERT:
        statement = parseInsert();
        break;
    case TokenType::KEYWORD_SELECT:
        statement = parseSelect();
        break;
    default:
        error_ = "Unexpected token at start of statement: " + token.text;
        return std::nullopt;
    }

    if (!statement || !error_.empty()) {
        return std::nullopt;
    }

    if (peek().type != TokenType::END) {
        error_ = "Unexpected token after statement: " + peek().text;
        return std::nullopt;
    }

    return statement;
}

std::string Parser::getError() const {
    return error_;
}

const Token &Parser::peek() const {
    return tokens_[position_];
}

const Token &Parser::advance() {
    return tokens_[position_++];
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

bool Parser::expect(TokenType type, const std::string &message) {
    if (match(type)) {
        return true;
    }
    if (error_.empty()) {
        error_ = message;
    }
    return false;
}

std::string Parser::parseIdentifier(const std::string &context) {
    if (!error_.empty()) {
        return "";
    }
    if (peek().type != TokenType::IDENTIFIER) {
        if (error_.empty()) {
            error_ = "Expected " + context;
        }
        return "";
    }
    std::string value = peek().text;
    advance();
    return value;
}

std::string Parser::parseValue(const std::string &context) {
    if (!error_.empty()) {
        return "";
    }
    if (peek().type == TokenType::STRING_LITERAL || peek().type == TokenType::IDENTIFIER) {
        std::string value = peek().text;
        advance();
        return value;
    }
    if (error_.empty()) {
        error_ = "Expected " + context;
    }
    return "";
}

std::optional<Statement> Parser::parseCreateTable() {
    advance(); // consume CREATE
    if (!expect(TokenType::KEYWORD_TABLE, "Expected TABLE after CREATE")) {
        return std::nullopt;
    }
    std::string tableName = parseIdentifier("table name");
    if (!expect(TokenType::LPAREN, "Expected '(' after table name")) {
        return std::nullopt;
    }
    std::vector<std::string> columns;
    columns.push_back(parseIdentifier("column name"));
    while (match(TokenType::COMMA)) {
        columns.push_back(parseIdentifier("column name"));
    }
    if (!expect(TokenType::RPAREN, "Expected ')' after column list")) {
        return std::nullopt;
    }
    if (!error_.empty()) {
        return std::nullopt;
    }
    if (columns.empty()) {
        error_ = "CREATE TABLE requires at least one column";
        return std::nullopt;
    }
    CreateTableStatement create{tableName, columns};
    return Statement{create};
}

std::optional<Statement> Parser::parseInsert() {
    advance(); // consume INSERT
    if (!expect(TokenType::KEYWORD_INTO, "Expected INTO after INSERT")) {
        return std::nullopt;
    }
    std::string tableName = parseIdentifier("table name");
    if (!expect(TokenType::KEYWORD_VALUES, "Expected VALUES keyword")) {
        return std::nullopt;
    }
    if (!expect(TokenType::LPAREN, "Expected '(' before values")) {
        return std::nullopt;
    }
    std::vector<std::string> values;
    values.push_back(parseValue("value"));
    while (match(TokenType::COMMA)) {
        values.push_back(parseValue("value"));
    }
    if (!expect(TokenType::RPAREN, "Expected ')' after values")) {
        return std::nullopt;
    }
    if (!error_.empty()) {
        return std::nullopt;
    }
    InsertStatement insert{tableName, values};
    return Statement{insert};
}

std::optional<Statement> Parser::parseSelect() {
    advance(); // consume SELECT
    SelectStatement select;
    if (match(TokenType::STAR)) {
        select.selectAll = true;
    } else {
        select.columns.push_back(parseIdentifier("column name"));
        while (match(TokenType::COMMA)) {
            select.columns.push_back(parseIdentifier("column name"));
        }
    }
    if (!expect(TokenType::KEYWORD_FROM, "Expected FROM in SELECT")) {
        return std::nullopt;
    }
    select.tableName = parseIdentifier("table name");
    if (match(TokenType::KEYWORD_WHERE)) {
        select.hasWhere = true;
        select.where.column = parseIdentifier("column name in WHERE");
        if (!expect(TokenType::EQUAL, "Expected '=' in WHERE clause")) {
            return std::nullopt;
        }
        select.where.value = parseValue("WHERE value");
    }

    if (!error_.empty()) {
        return std::nullopt;
    }

    return Statement{select};
}
