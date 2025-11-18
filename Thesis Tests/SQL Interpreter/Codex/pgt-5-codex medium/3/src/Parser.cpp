#include "Parser.h"

#include <stdexcept>
#include <utility>

#include "Utils.h"

Parser::Parser(const std::string &input) : lexer_(input) {}

StatementPtr Parser::parseStatement() {
    const Token &first = lexer_.peek();
    if (first.type == TokenType::End) {
        throw std::runtime_error("Empty statement");
    }

    StatementPtr statement;
    std::string keyword = Utils::toUpper(first.text);
    if (keyword == "CREATE") {
        lexer_.consume();
        statement = parseCreateTable();
    } else if (keyword == "INSERT") {
        lexer_.consume();
        statement = parseInsert();
    } else if (keyword == "SELECT") {
        lexer_.consume();
        statement = parseSelect();
    } else {
        throw std::runtime_error("Unsupported statement: " + first.text);
    }

    expectToken(TokenType::Semicolon, "Missing ';' at end of statement");
    const Token &end = lexer_.peek();
    if (end.type != TokenType::End) {
        throw std::runtime_error("Unexpected input after statement");
    }
    return statement;
}

StatementPtr Parser::parseCreateTable() {
    expectKeyword("TABLE", "Expected TABLE keyword after CREATE");
    Token nameToken = expectIdentifier("Expected table name");
    auto statement = std::make_unique<CreateTableStatement>();
    statement->tableName = nameToken.text;
    statement->columns = parseParenthesizedIdentifierList();
    if (statement->columns.empty()) {
        throw std::runtime_error("CREATE TABLE requires at least one column");
    }
    return statement;
}

StatementPtr Parser::parseInsert() {
    expectKeyword("INTO", "Expected INTO keyword after INSERT");
    Token nameToken = expectIdentifier("Expected table name for INSERT");
    expectKeyword("VALUES", "Expected VALUES keyword");
    auto statement = std::make_unique<InsertStatement>();
    statement->tableName = nameToken.text;
    statement->values = parseValueList();
    if (statement->values.empty()) {
        throw std::runtime_error("INSERT requires at least one value");
    }
    return statement;
}

StatementPtr Parser::parseSelect() {
    auto statement = std::make_unique<SelectStatement>();
    const Token &current = lexer_.peek();
    if (current.type == TokenType::Star) {
        statement->selectAll = true;
        lexer_.consume();
    } else {
        statement->columns = parsePlainIdentifierList();
        if (statement->columns.empty()) {
            throw std::runtime_error("SELECT requires columns or *");
        }
    }
    expectKeyword("FROM", "Expected FROM keyword in SELECT");
    Token tableToken = expectIdentifier("Expected table name in SELECT");
    statement->tableName = tableToken.text;

    if (peekKeyword("WHERE")) {
        lexer_.consume();
        Token columnToken = expectIdentifier("Expected column name after WHERE");
        expectToken(TokenType::Equals, "Expected '=' in WHERE clause");
        Token valueToken = lexer_.consume();
        if (valueToken.type != TokenType::Identifier &&
            valueToken.type != TokenType::Number &&
            valueToken.type != TokenType::StringLiteral) {
            throw std::runtime_error("Invalid value in WHERE clause");
        }
        statement->hasWhere = true;
        statement->whereColumn = columnToken.text;
        statement->whereValue = parseValueToken(valueToken, "WHERE clause value");
    }

    return statement;
}

std::vector<std::string> Parser::parseParenthesizedIdentifierList() {
    std::vector<std::string> identifiers;
    expectToken(TokenType::LParen, "Expected '(' for list");
    bool expectValue = true;
    while (true) {
        if (expectValue) {
            Token tok = lexer_.consume();
            if (tok.type != TokenType::Identifier) {
                throw std::runtime_error("Expected identifier in list");
            }
            identifiers.push_back(tok.text);
            expectValue = false;
        } else {
            Token tok = lexer_.consume();
            if (tok.type == TokenType::Comma) {
                expectValue = true;
                continue;
            }
            if (tok.type == TokenType::RParen) {
                break;
            }
            throw std::runtime_error("Expected ',' or ')' in list");
        }
    }
    return identifiers;
}

std::vector<std::string> Parser::parsePlainIdentifierList() {
    std::vector<std::string> identifiers;
    Token first = lexer_.consume();
    if (first.type != TokenType::Identifier) {
        throw std::runtime_error("Expected column name in SELECT");
    }
    identifiers.push_back(first.text);
    while (lexer_.peek().type == TokenType::Comma) {
        lexer_.consume();
        Token token = lexer_.consume();
        if (token.type != TokenType::Identifier) {
            throw std::runtime_error("Expected column name after ',' in SELECT");
        }
        identifiers.push_back(token.text);
    }
    return identifiers;
}

std::vector<std::string> Parser::parseValueList() {
    std::vector<std::string> values;
    expectToken(TokenType::LParen, "Expected '(' before VALUES list");
    bool expectValue = true;
    while (true) {
        if (expectValue) {
            Token tok = lexer_.consume();
            if (tok.type == TokenType::RParen) {
                if (values.empty()) {
                    throw std::runtime_error("VALUES list cannot be empty");
                }
                break;
            }
            values.push_back(parseValueToken(tok, "VALUES"));
            expectValue = false;
        } else {
            Token tok = lexer_.consume();
            if (tok.type == TokenType::Comma) {
                expectValue = true;
                continue;
            }
            if (tok.type == TokenType::RParen) {
                break;
            }
            throw std::runtime_error("Expected ',' or ')' in VALUES list");
        }
    }
    if (expectValue && !values.empty()) {
        throw std::runtime_error("Trailing comma in VALUES list");
    }
    return values;
}

Token Parser::expectToken(TokenType type, const std::string &message) {
    Token token = lexer_.consume();
    if (token.type != type) {
        throw std::runtime_error(message);
    }
    return token;
}

Token Parser::expectIdentifier(const std::string &message) {
    Token token = lexer_.consume();
    if (token.type != TokenType::Identifier) {
        throw std::runtime_error(message);
    }
    return token;
}

std::string Parser::parseValueToken(const Token &token, const std::string &contextMessage) {
    switch (token.type) {
        case TokenType::Identifier:
        case TokenType::Number:
        case TokenType::StringLiteral:
            return token.text;
        default:
            throw std::runtime_error("Invalid token in " + contextMessage);
    }
}

void Parser::expectKeyword(const std::string &keyword, const std::string &message) {
    Token token = lexer_.consume();
    if (token.type != TokenType::Identifier || Utils::toUpper(token.text) != keyword) {
        throw std::runtime_error(message);
    }
}

bool Parser::peekKeyword(const std::string &keyword) const {
    const Token &token = lexer_.peek();
    if (token.type != TokenType::Identifier) {
        return false;
    }
    return Utils::toUpper(token.text) == keyword;
}

std::string Parser::peekKeywordText() const {
    const Token &token = lexer_.peek();
    if (token.type != TokenType::Identifier) {
        return std::string();
    }
    return Utils::toUpper(token.text);
}
