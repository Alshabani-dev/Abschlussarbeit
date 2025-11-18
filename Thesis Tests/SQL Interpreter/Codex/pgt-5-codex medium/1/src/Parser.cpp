#include "Parser.h"

#include <stdexcept>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

StatementList Parser::parseStatements() {
    StatementList statements;
    while (!isAtEnd()) {
        if (match(Token::Type::Semicolon)) {
            continue;
        }
        statements.emplace_back(parseStatement());
        match(Token::Type::Semicolon);
    }
    return statements;
}

StatementPtr Parser::parseStatement() {
    if (match(Token::Type::Create)) {
        return parseCreateTable();
    }
    if (match(Token::Type::Insert)) {
        return parseInsert();
    }
    if (match(Token::Type::Select)) {
        return parseSelect();
    }
    throw std::runtime_error("Unexpected token: " + peek().text);
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    consume(Token::Type::Table, "Expected TABLE after CREATE");
    auto stmt = std::make_unique<CreateTableStatement>();
    stmt->tableName = parseIdentifier("Expected table name");
    consume(Token::Type::LParen, "Expected '(' after table name");
    stmt->columns.push_back(parseIdentifier("Expected column name"));
    while (match(Token::Type::Comma)) {
        stmt->columns.push_back(parseIdentifier("Expected column name"));
    }
    consume(Token::Type::RParen, "Expected ')' after column list");
    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    consume(Token::Type::Into, "Expected INTO after INSERT");
    auto stmt = std::make_unique<InsertStatement>();
    stmt->tableName = parseIdentifier("Expected table name");
    consume(Token::Type::Values, "Expected VALUES after table name");
    consume(Token::Type::LParen, "Expected '(' before values");
    stmt->values.push_back(parseValue());
    while (match(Token::Type::Comma)) {
        stmt->values.push_back(parseValue());
    }
    consume(Token::Type::RParen, "Expected ')' after values");
    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    if (match(Token::Type::Asterisk)) {
        stmt->selectAll = true;
    } else {
        stmt->columns.push_back(parseIdentifier("Expected column name"));
        while (match(Token::Type::Comma)) {
            stmt->columns.push_back(parseIdentifier("Expected column name"));
        }
    }
    consume(Token::Type::From, "Expected FROM after columns");
    stmt->tableName = parseIdentifier("Expected table name");
    if (match(Token::Type::Where)) {
        WhereClause clause;
        clause.column = parseIdentifier("Expected column name in WHERE clause");
        consume(Token::Type::Equal, "Expected '=' in WHERE clause");
        clause.value = parseValue();
        stmt->where = clause;
    }
    return stmt;
}

std::string Parser::parseIdentifier(const std::string &message) {
    if (match(Token::Type::Identifier)) {
        return previous().text;
    }
    throw std::runtime_error(message);
}

std::string Parser::parseValue() {
    if (match(Token::Type::String)) {
        return previous().text;
    }
    if (match(Token::Type::Number)) {
        return previous().text;
    }
    if (match(Token::Type::Identifier)) {
        return previous().text;
    }
    throw std::runtime_error("Expected literal value");
}

const Token &Parser::peek() const { return tokens_[current_]; }

const Token &Parser::previous() const { return tokens_[current_ - 1]; }

const Token &Parser::advance() {
    if (!isAtEnd()) {
        ++current_;
    }
    return previous();
}

bool Parser::match(Token::Type type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

const Token &Parser::consume(Token::Type type, const std::string &message) {
    if (peek().type == type) {
        return advance();
    }
    throw std::runtime_error(message);
}

bool Parser::isAtEnd() const { return peek().type == Token::Type::EndOfFile; }
