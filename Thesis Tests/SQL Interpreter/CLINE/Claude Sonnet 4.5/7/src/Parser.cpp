#include "Parser.h"
#include "Utils.h"
#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

std::unique_ptr<Statement> Parser::parseStatement() {
    Token token = currentToken();
    
    if (token.type != TokenType::KEYWORD) {
        throw std::runtime_error("Expected keyword at start of statement");
    }
    
    if (token.value == "CREATE") {
        return parseCreateTable();
    } else if (token.value == "INSERT") {
        return parseInsert();
    } else if (token.value == "SELECT") {
        return parseSelect();
    } else {
        throw std::runtime_error("Unknown statement type: " + token.value);
    }
}

Token Parser::currentToken() {
    if (pos_ >= tokens_.size()) {
        return Token(TokenType::END_OF_INPUT, "");
    }
    return tokens_[pos_];
}

Token Parser::peek(size_t offset) {
    if (pos_ + offset >= tokens_.size()) {
        return Token(TokenType::END_OF_INPUT, "");
    }
    return tokens_[pos_ + offset];
}

void Parser::advance() {
    if (pos_ < tokens_.size()) {
        pos_++;
    }
}

bool Parser::expect(TokenType type, const std::string& value) {
    Token token = currentToken();
    if (token.type != type) {
        return false;
    }
    if (!value.empty() && token.value != value) {
        return false;
    }
    return true;
}

void Parser::consume(TokenType type, const std::string& value) {
    if (!expect(type, value)) {
        std::string expected = "type=" + std::to_string(static_cast<int>(type));
        if (!value.empty()) {
            expected += ", value=" + value;
        }
        Token token = currentToken();
        std::string got = "type=" + std::to_string(static_cast<int>(token.type)) + 
                         ", value=" + token.value;
        throw std::runtime_error("Expected " + expected + ", got " + got);
    }
    advance();
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();
    
    consume(TokenType::KEYWORD, "CREATE");
    consume(TokenType::KEYWORD, "TABLE");
    
    stmt->tableName = parseIdentifier();
    
    consume(TokenType::SYMBOL, "(");
    stmt->columns = parseColumnList();
    consume(TokenType::SYMBOL, ")");
    consume(TokenType::SYMBOL, ";");
    
    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();
    
    consume(TokenType::KEYWORD, "INSERT");
    consume(TokenType::KEYWORD, "INTO");
    
    stmt->tableName = parseIdentifier();
    
    consume(TokenType::KEYWORD, "VALUES");
    consume(TokenType::SYMBOL, "(");
    stmt->values = parseValueList();
    consume(TokenType::SYMBOL, ")");
    consume(TokenType::SYMBOL, ";");
    
    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    
    consume(TokenType::KEYWORD, "SELECT");
    consume(TokenType::SYMBOL, "*");
    consume(TokenType::KEYWORD, "FROM");
    
    stmt->tableName = parseIdentifier();
    
    // Check for WHERE clause
    if (expect(TokenType::KEYWORD, "WHERE")) {
        advance();
        stmt->hasWhere = true;
        stmt->whereColumn = parseIdentifier();
        consume(TokenType::SYMBOL, "=");
        
        Token valueToken = currentToken();
        if (valueToken.type == TokenType::STRING_LITERAL || 
            valueToken.type == TokenType::NUMBER ||
            valueToken.type == TokenType::IDENTIFIER) {
            stmt->whereValue = valueToken.value;
            advance();
        } else {
            throw std::runtime_error("Expected value after = in WHERE clause");
        }
    }
    
    consume(TokenType::SYMBOL, ";");
    
    return stmt;
}

std::string Parser::parseIdentifier() {
    Token token = currentToken();
    if (token.type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected identifier, got: " + token.value);
    }
    advance();
    return token.value;
}

std::vector<std::string> Parser::parseColumnList() {
    std::vector<std::string> columns;
    
    columns.push_back(parseIdentifier());
    
    while (expect(TokenType::SYMBOL, ",")) {
        advance();
        columns.push_back(parseIdentifier());
    }
    
    return columns;
}

std::vector<std::string> Parser::parseValueList() {
    std::vector<std::string> values;
    
    Token token = currentToken();
    if (token.type == TokenType::STRING_LITERAL || 
        token.type == TokenType::NUMBER ||
        token.type == TokenType::IDENTIFIER) {
        values.push_back(token.value);
        advance();
    } else {
        throw std::runtime_error("Expected value in VALUES list");
    }
    
    while (expect(TokenType::SYMBOL, ",")) {
        advance();
        token = currentToken();
        if (token.type == TokenType::STRING_LITERAL || 
            token.type == TokenType::NUMBER ||
            token.type == TokenType::IDENTIFIER) {
            values.push_back(token.value);
            advance();
        } else {
            throw std::runtime_error("Expected value after comma in VALUES list");
        }
    }
    
    return values;
}
