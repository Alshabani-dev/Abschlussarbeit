#include "Parser.h"

#include <vector>

#include "Utils.h"

Parser::Parser(const std::string &input) : lexer_(input) {
    current_ = lexer_.next();
}

Token Parser::advance() {
    Token previous = current_;
    current_ = lexer_.next();
    return previous;
}

bool Parser::check(TokenType type) const {
    return current_.type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TokenType type, const std::string &message) {
    if (!match(type)) {
        throw std::runtime_error(message);
    }
}

bool Parser::matchKeyword(const std::string &keyword) {
    if (current_.type == TokenType::Keyword && current_.text == keyword) {
        advance();
        return true;
    }
    return false;
}

void Parser::expectKeyword(const std::string &keyword, const std::string &message) {
    if (!matchKeyword(keyword)) {
        throw std::runtime_error(message);
    }
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (current_.type == TokenType::End) {
        throw std::runtime_error("Empty statement");
    }

    if (matchKeyword("CREATE")) {
        return parseCreate();
    }
    if (matchKeyword("INSERT")) {
        return parseInsert();
    }
    if (matchKeyword("SELECT")) {
        return parseSelect();
    }

    throw std::runtime_error("Unsupported statement");
}

std::unique_ptr<Statement> Parser::parseCreate() {
    expectKeyword("TABLE", "Expected TABLE keyword after CREATE");
    auto stmt = std::make_unique<CreateTableStatement>();
    stmt->tableName = utils::toLower(parseIdentifier());
    expect(TokenType::LParen, "Expected '(' after table name");
    while (true) {
        stmt->columns.push_back(utils::toLower(parseIdentifier()));
        if (match(TokenType::Comma)) {
            continue;
        }
        break;
    }
    expect(TokenType::RParen, "Expected ')' after column list");
    match(TokenType::Semicolon);
    return stmt;
}

std::unique_ptr<Statement> Parser::parseInsert() {
    expectKeyword("INTO", "Expected INTO keyword after INSERT");
    auto stmt = std::make_unique<InsertStatement>();
    stmt->tableName = utils::toLower(parseIdentifier());
    expectKeyword("VALUES", "Expected VALUES keyword");
    expect(TokenType::LParen, "Expected '(' before values");
    while (true) {
        stmt->values.push_back(parseLiteral());
        if (match(TokenType::Comma)) {
            continue;
        }
        break;
    }
    expect(TokenType::RParen, "Expected ')' after values");
    match(TokenType::Semicolon);
    return stmt;
}

std::unique_ptr<Statement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    if (match(TokenType::Star)) {
        stmt->columns.push_back("*");
    } else {
        while (true) {
            stmt->columns.push_back(utils::toLower(parseIdentifier()));
            if (match(TokenType::Comma)) {
                continue;
            }
            break;
        }
    }
    expectKeyword("FROM", "Expected FROM keyword");
    stmt->tableName = utils::toLower(parseIdentifier());
    if (matchKeyword("WHERE")) {
        stmt->whereColumn = utils::toLower(parseIdentifier());
        expect(TokenType::Equal, "Expected '=' in WHERE clause");
        stmt->whereValue = parseLiteral();
    }
    match(TokenType::Semicolon);
    return stmt;
}

std::string Parser::parseIdentifier() {
    if (current_.type != TokenType::Identifier) {
        throw std::runtime_error("Expected identifier");
    }
    return advance().text;
}

std::string Parser::parseLiteral() {
    if (current_.type == TokenType::String || current_.type == TokenType::Number ||
        current_.type == TokenType::Identifier) {
        return advance().text;
    }
    throw std::runtime_error("Expected literal value");
}
