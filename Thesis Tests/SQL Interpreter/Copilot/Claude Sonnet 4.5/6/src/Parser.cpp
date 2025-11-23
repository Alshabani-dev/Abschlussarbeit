#include "Parser.h"
#include "Utils.h"
#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (isAtEnd()) {
        throw std::runtime_error("Unexpected end of input");
    }
    
    Token current = peek();
    
    if (current.type == TokenType::KEYWORD) {
        if (current.value == "CREATE") {
            return parseCreateTable();
        } else if (current.value == "INSERT") {
            return parseInsert();
        } else if (current.value == "SELECT") {
            return parseSelect();
        }
    }
    
    throw std::runtime_error("Expected CREATE, INSERT, or SELECT statement");
}

Token Parser::peek() const {
    if (isAtEnd()) {
        return Token(TokenType::END_OF_FILE);
    }
    return tokens_[pos_];
}

Token Parser::advance() {
    if (!isAtEnd()) {
        return tokens_[pos_++];
    }
    return Token(TokenType::END_OF_FILE);
}

bool Parser::isAtEnd() const {
    return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::END_OF_FILE;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::matchKeyword(const std::string& keyword) {
    if (check(TokenType::KEYWORD) && peek().value == keyword) {
        advance();
        return true;
    }
    return false;
}

Token Parser::expect(TokenType type, const std::string& errorMessage) {
    if (check(type)) {
        return advance();
    }
    throw std::runtime_error(errorMessage);
}

Token Parser::expectKeyword(const std::string& keyword, const std::string& errorMessage) {
    if (matchKeyword(keyword)) {
        return tokens_[pos_ - 1];
    }
    throw std::runtime_error(errorMessage);
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();
    
    expectKeyword("CREATE", "Expected CREATE keyword");
    expectKeyword("TABLE", "Expected TABLE keyword");
    
    Token tableName = expect(TokenType::IDENTIFIER, "Expected table name");
    stmt->tableName = Utils::toLower(tableName.value);
    
    expect(TokenType::LPAREN, "Expected '(' after table name");
    
    // Parse column names
    do {
        Token col = expect(TokenType::IDENTIFIER, "Expected column name");
        stmt->columns.push_back(Utils::toLower(col.value));
    } while (match(TokenType::COMMA));
    
    expect(TokenType::RPAREN, "Expected ')' after column list");
    match(TokenType::SEMICOLON); // Optional semicolon
    
    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();
    
    expectKeyword("INSERT", "Expected INSERT keyword");
    expectKeyword("INTO", "Expected INTO keyword");
    
    Token tableName = expect(TokenType::IDENTIFIER, "Expected table name");
    stmt->tableName = Utils::toLower(tableName.value);
    
    expectKeyword("VALUES", "Expected VALUES keyword");
    expect(TokenType::LPAREN, "Expected '(' after VALUES");
    
    // Parse values
    do {
        Token value = peek();
        if (value.type == TokenType::IDENTIFIER || 
            value.type == TokenType::NUMBER || 
            value.type == TokenType::STRING_LITERAL) {
            stmt->values.push_back(advance().value);
        } else {
            throw std::runtime_error("Expected value in INSERT statement");
        }
    } while (match(TokenType::COMMA));
    
    expect(TokenType::RPAREN, "Expected ')' after values");
    match(TokenType::SEMICOLON); // Optional semicolon
    
    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    
    expectKeyword("SELECT", "Expected SELECT keyword");
    expect(TokenType::ASTERISK, "Expected '*' after SELECT");
    expectKeyword("FROM", "Expected FROM keyword");
    
    Token tableName = expect(TokenType::IDENTIFIER, "Expected table name");
    stmt->tableName = Utils::toLower(tableName.value);
    
    // Optional WHERE clause
    if (matchKeyword("WHERE")) {
        stmt->hasWhere = true;
        
        Token col = expect(TokenType::IDENTIFIER, "Expected column name in WHERE clause");
        stmt->whereColumn = Utils::toLower(col.value);
        
        expect(TokenType::EQUALS, "Expected '=' in WHERE clause");
        
        Token value = peek();
        if (value.type == TokenType::IDENTIFIER || 
            value.type == TokenType::NUMBER || 
            value.type == TokenType::STRING_LITERAL) {
            stmt->whereValue = advance().value;
        } else {
            throw std::runtime_error("Expected value in WHERE clause");
        }
    }
    
    match(TokenType::SEMICOLON); // Optional semicolon
    
    return stmt;
}
