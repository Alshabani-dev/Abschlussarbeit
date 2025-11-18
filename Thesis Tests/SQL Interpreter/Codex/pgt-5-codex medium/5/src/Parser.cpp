#include "Parser.h"

#include <sstream>
#include <stdexcept>

#include "Utils.h"

StatementPtr Parser::parse(const std::string &sql) {
    Lexer lexer(sql);
    tokens_ = lexer.tokenize();
    pos_ = 0;
    auto statement = parseStatement();
    expect(TokenType::End, "Unexpected tokens after statement");
    return statement;
}

const Token &Parser::current() const {
    if (pos_ >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[pos_];
}

const Token &Parser::consume() {
    if (pos_ >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[pos_++];
}

bool Parser::match(TokenType type) {
    if (current().type == type) {
        ++pos_;
        return true;
    }
    return false;
}

void Parser::expect(TokenType type, const std::string &message) {
    if (!match(type)) {
        std::ostringstream oss;
        oss << message << ", found '" << current().text << "'";
        throw std::runtime_error(oss.str());
    }
}

StatementPtr Parser::parseStatement() {
    switch (current().type) {
        case TokenType::Create:
            return parseCreate();
        case TokenType::Insert:
            return parseInsert();
        case TokenType::Select:
            return parseSelect();
        default:
            throw std::runtime_error("Unsupported statement");
    }
}

StatementPtr Parser::parseCreate() {
    expect(TokenType::Create, "Expected CREATE keyword");
    expect(TokenType::TableKW, "Expected TABLE keyword");
    auto stmt = std::make_unique<CreateTableStmt>();
    stmt->tableName = Utils::toLower(parseIdentifier());
    expect(TokenType::LParen, "Expected '('");
    do {
        stmt->columns.push_back(Utils::toLower(parseIdentifier()));
    } while (match(TokenType::Comma));
    expect(TokenType::RParen, "Expected ')'");
    return stmt;
}

StatementPtr Parser::parseInsert() {
    expect(TokenType::Insert, "Expected INSERT keyword");
    expect(TokenType::Into, "Expected INTO keyword");
    auto stmt = std::make_unique<InsertStmt>();
    stmt->tableName = Utils::toLower(parseIdentifier());
    expect(TokenType::Values, "Expected VALUES keyword");
    expect(TokenType::LParen, "Expected '('");
    do {
        stmt->values.push_back(parseValue());
    } while (match(TokenType::Comma));
    expect(TokenType::RParen, "Expected ')'");
    return stmt;
}

StatementPtr Parser::parseSelect() {
    expect(TokenType::Select, "Expected SELECT keyword");
    auto stmt = std::make_unique<SelectStmt>();
    if (match(TokenType::Star)) {
        stmt->selectAll = true;
    } else {
        stmt->columns.push_back(Utils::toLower(parseIdentifier()));
        while (match(TokenType::Comma)) {
            stmt->columns.push_back(Utils::toLower(parseIdentifier()));
        }
    }
    expect(TokenType::From, "Expected FROM keyword");
    stmt->tableName = Utils::toLower(parseIdentifier());
    if (match(TokenType::Where)) {
        stmt->hasWhere = true;
        stmt->whereColumn = Utils::toLower(parseIdentifier());
        expect(TokenType::Equal, "Expected '=' in WHERE clause");
        stmt->whereValue = parseValue();
    }
    return stmt;
}

std::string Parser::parseIdentifier() {
    const Token &token = current();
    if (token.type != TokenType::Identifier) {
        throw std::runtime_error("Expected identifier, found '" + token.text + "'");
    }
    ++pos_;
    return token.text;
}

std::string Parser::parseValue() {
    const Token &token = current();
    switch (token.type) {
        case TokenType::StringLiteral:
        case TokenType::Number:
        case TokenType::Identifier: {
            ++pos_;
            return token.text;
        }
        default:
            break;
    }
    throw std::runtime_error("Expected literal value, found '" + token.text + "'");
}
