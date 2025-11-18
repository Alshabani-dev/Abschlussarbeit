#include "Parser.h"

#include <stdexcept>

#include "Utils.h"

Parser::Parser(const std::string &input) : lexer_(input) {}

Statement Parser::parseStatement() {
    Token token = lexer_.peek();
    Statement stmt;
    switch (token.type) {
    case TokenType::KeywordCreate:
        stmt.value = parseCreateTable();
        break;
    case TokenType::KeywordInsert:
        stmt.value = parseInsert();
        break;
    case TokenType::KeywordSelect:
        stmt.value = parseSelect();
        break;
    default:
        throw std::runtime_error("Unsupported statement");
    }

    if (match(TokenType::Semicolon)) {
        return stmt;
    }
    Token next = lexer_.peek();
    if (next.type != TokenType::End) {
        throw std::runtime_error("Expected ';' or end of input");
    }
    return stmt;
}

Token Parser::expect(TokenType type, const std::string &message) {
    Token token = lexer_.next();
    if (token.type != type) {
        throw std::runtime_error(message);
    }
    return token;
}

bool Parser::match(TokenType type) {
    Token token = lexer_.peek();
    if (token.type == type) {
        lexer_.next();
        return true;
    }
    return false;
}

CreateTableStatement Parser::parseCreateTable() {
    expect(TokenType::KeywordCreate, "Expected CREATE");
    expect(TokenType::KeywordTable, "Expected TABLE");
    Token name = expect(TokenType::Identifier, "Expected table name");
    expect(TokenType::LParen, "Expected '(' after table name");

    std::vector<std::string> columns;
    do {
        Token column = expect(TokenType::Identifier, "Expected column name");
        columns.push_back(toLower(column.text));
    } while (match(TokenType::Comma));

    expect(TokenType::RParen, "Expected ')' after column list");

    CreateTableStatement stmt;
    stmt.tableName = toLower(name.text);
    stmt.columns = std::move(columns);
    return stmt;
}

InsertStatement Parser::parseInsert() {
    expect(TokenType::KeywordInsert, "Expected INSERT");
    expect(TokenType::KeywordInto, "Expected INTO");
    Token name = expect(TokenType::Identifier, "Expected table name");
    expect(TokenType::KeywordValues, "Expected VALUES");
    expect(TokenType::LParen, "Expected '(' before values");

    std::vector<std::string> values;
    do {
        Token token = lexer_.next();
        switch (token.type) {
        case TokenType::String:
        case TokenType::Number:
        case TokenType::Identifier:
            values.push_back(token.text);
            break;
        default:
            throw std::runtime_error("Expected literal value in INSERT");
        }
    } while (match(TokenType::Comma));

    expect(TokenType::RParen, "Expected ')' after values");

    InsertStatement stmt;
    stmt.tableName = toLower(name.text);
    stmt.values = std::move(values);
    return stmt;
}

SelectStatement Parser::parseSelect() {
    expect(TokenType::KeywordSelect, "Expected SELECT");

    std::vector<std::string> columns;
    bool selectAll = false;
    if (match(TokenType::Star)) {
        selectAll = true;
    } else {
        do {
            Token column = expect(TokenType::Identifier, "Expected column");
            columns.push_back(toLower(column.text));
        } while (match(TokenType::Comma));
    }

    expect(TokenType::KeywordFrom, "Expected FROM");
    Token table = expect(TokenType::Identifier, "Expected table name");

    std::optional<WhereClause> where;
    if (match(TokenType::KeywordWhere)) {
        Token column = expect(TokenType::Identifier, "Expected column in WHERE");
        expect(TokenType::Equal, "Expected '=' in WHERE");
        Token value = lexer_.next();
        if (value.type != TokenType::String && value.type != TokenType::Number && value.type != TokenType::Identifier) {
            throw std::runtime_error("Expected literal value in WHERE");
        }
        where = WhereClause{toLower(column.text), value.text};
    }

    SelectStatement stmt;
    if (selectAll) {
        stmt.columns = {};
    } else {
        stmt.columns = std::move(columns);
    }
    stmt.tableName = toLower(table.text);
    stmt.where = where;
    return stmt;
}
