#include "Parser.h"

#include <stdexcept>

namespace {

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END: return "end of input";
        case TokenType::IDENTIFIER: return "identifier";
        case TokenType::NUMBER: return "number";
        case TokenType::STRING: return "string";
        case TokenType::STAR: return "*";
        case TokenType::COMMA: return ",";
        case TokenType::SEMICOLON: return ";";
        case TokenType::LPAREN: return "(";
        case TokenType::RPAREN: return ")";
        case TokenType::EQUAL: return "=";
        case TokenType::KEYWORD_CREATE: return "CREATE";
        case TokenType::KEYWORD_TABLE: return "TABLE";
        case TokenType::KEYWORD_INSERT: return "INSERT";
        case TokenType::KEYWORD_INTO: return "INTO";
        case TokenType::KEYWORD_VALUES: return "VALUES";
        case TokenType::KEYWORD_SELECT: return "SELECT";
        case TokenType::KEYWORD_FROM: return "FROM";
        case TokenType::KEYWORD_WHERE: return "WHERE";
        default: return "token";
    }
}

} // namespace

Parser::Parser(const std::string &source) {
    Lexer lexer(source);
    tokens_ = lexer.tokenize();
}

const Token &Parser::peek(size_t offset) const {
    size_t index = current_ + offset;
    if (index >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[index];
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        ++current_;
        return true;
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string &message) {
    if (peek().type != type) {
        throw std::runtime_error(message + " (found " + tokenTypeToString(peek().type) + ")");
    }
    return tokens_[current_++];
}

Token Parser::consumeIdentifier(const std::string &message) {
    if (peek().type != TokenType::IDENTIFIER) {
        throw std::runtime_error(message);
    }
    return tokens_[current_++];
}

std::vector<StatementPtr> Parser::parseStatements() {
    std::vector<StatementPtr> statements;
    while (peek().type != TokenType::END) {
        if (match(TokenType::SEMICOLON)) {
            continue;
        }
        StatementPtr stmt = parseStatement();
        statements.push_back(std::move(stmt));
        consume(TokenType::SEMICOLON, "Expected ';' at end of statement");
    }
    return statements;
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
            throw std::runtime_error("Unexpected token at start of statement");
    }
}

StatementPtr Parser::parseCreateTable() {
    consume(TokenType::KEYWORD_CREATE, "Expected CREATE");
    consume(TokenType::KEYWORD_TABLE, "Expected TABLE after CREATE");
    Token name = consumeIdentifier("Expected table name");
    consume(TokenType::LPAREN, "Expected '(' after table name");
    std::vector<std::string> columns = parseIdentifierList();
    consume(TokenType::RPAREN, "Expected ')' after column list");

    auto stmt = std::make_unique<CreateTableStatement>();
    stmt->tableName = name.text;
    stmt->columns = columns;
    return stmt;
}

StatementPtr Parser::parseInsert() {
    consume(TokenType::KEYWORD_INSERT, "Expected INSERT");
    consume(TokenType::KEYWORD_INTO, "Expected INTO after INSERT");
    Token name = consumeIdentifier("Expected table name");
    consume(TokenType::KEYWORD_VALUES, "Expected VALUES keyword");
    consume(TokenType::LPAREN, "Expected '(' before values");
    std::vector<std::string> values = parseValueList();
    consume(TokenType::RPAREN, "Expected ')' after values");

    auto stmt = std::make_unique<InsertStatement>();
    stmt->tableName = name.text;
    stmt->values = values;
    return stmt;
}

StatementPtr Parser::parseSelect() {
    consume(TokenType::KEYWORD_SELECT, "Expected SELECT");
    auto stmt = std::make_unique<SelectStatement>();
    if (match(TokenType::STAR)) {
        stmt->selectAll = true;
    } else {
        stmt->columns = parseIdentifierList();
    }
    consume(TokenType::KEYWORD_FROM, "Expected FROM");
    Token table = consumeIdentifier("Expected table name after FROM");
    stmt->tableName = table.text;
    if (match(TokenType::KEYWORD_WHERE)) {
        stmt->hasWhere = true;
        Token column = consumeIdentifier("Expected column name in WHERE clause");
        stmt->whereColumn = column.text;
        consume(TokenType::EQUAL, "Expected '=' in WHERE clause");
        Token value = peek();
        if (value.type != TokenType::STRING &&
            value.type != TokenType::NUMBER &&
            value.type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected literal value in WHERE clause");
        }
        ++current_;
        stmt->whereValue = value.text;
    }
    return stmt;
}

std::vector<std::string> Parser::parseIdentifierList() {
    std::vector<std::string> items;
    Token first = consumeIdentifier("Expected identifier");
    items.push_back(first.text);
    while (match(TokenType::COMMA)) {
        Token next = consumeIdentifier("Expected identifier");
        items.push_back(next.text);
    }
    return items;
}

std::vector<std::string> Parser::parseValueList() {
    std::vector<std::string> values;
    Token token = peek();
    if (token.type != TokenType::STRING &&
        token.type != TokenType::NUMBER &&
        token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected literal value");
    }
    values.push_back(token.text);
    ++current_;
    while (match(TokenType::COMMA)) {
        Token t = peek();
        if (t.type != TokenType::STRING &&
            t.type != TokenType::NUMBER &&
            t.type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected literal value");
        }
        values.push_back(t.text);
        ++current_;
    }
    return values;
}
