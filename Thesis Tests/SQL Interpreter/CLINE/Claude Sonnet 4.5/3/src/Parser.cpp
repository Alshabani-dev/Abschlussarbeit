#include "Parser.h"
#include <sstream>

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (isAtEnd()) {
        throw std::runtime_error("No statement to parse");
    }
    
    Token current = peek();
    
    if (current.type == TokenType::CREATE) {
        return parseCreateTable();
    } else if (current.type == TokenType::INSERT) {
        return parseInsert();
    } else if (current.type == TokenType::SELECT) {
        return parseSelect();
    } else {
        throw std::runtime_error("Unknown statement type: " + current.value);
    }
}

Token Parser::peek() const {
    if (isAtEnd()) {
        return Token(TokenType::END_OF_FILE, "");
    }
    return tokens_[pos_];
}

Token Parser::advance() {
    if (!isAtEnd()) {
        return tokens_[pos_++];
    }
    return Token(TokenType::END_OF_FILE, "");
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

void Parser::expect(TokenType type, const std::string& message) {
    if (!check(type)) {
        std::stringstream ss;
        ss << message << " (got: " << peek().value << ")";
        throw std::runtime_error(ss.str());
    }
    advance();
}

// CREATE TABLE table_name (col1, col2, col3);
std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();
    
    expect(TokenType::CREATE, "Expected CREATE");
    expect(TokenType::TABLE, "Expected TABLE");
    
    if (!check(TokenType::IDENTIFIER)) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = advance().value;
    
    expect(TokenType::LPAREN, "Expected '(' after table name");
    
    // Parse column names
    do {
        if (check(TokenType::RPAREN)) break;
        
        if (!check(TokenType::IDENTIFIER)) {
            throw std::runtime_error("Expected column name");
        }
        stmt->columns.push_back(advance().value);
        
    } while (match(TokenType::COMMA));
    
    expect(TokenType::RPAREN, "Expected ')' after column list");
    expect(TokenType::SEMICOLON, "Expected ';' at end of statement");
    
    return stmt;
}

// INSERT INTO table_name VALUES (val1, val2, val3);
std::unique_ptr<InsertStatement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();
    
    expect(TokenType::INSERT, "Expected INSERT");
    expect(TokenType::INTO, "Expected INTO");
    
    if (!check(TokenType::IDENTIFIER)) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = advance().value;
    
    expect(TokenType::VALUES, "Expected VALUES");
    expect(TokenType::LPAREN, "Expected '(' after VALUES");
    
    // Parse values
    do {
        if (check(TokenType::RPAREN)) break;
        
        Token val = peek();
        if (val.type == TokenType::IDENTIFIER || 
            val.type == TokenType::STRING_LITERAL || 
            val.type == TokenType::NUMBER) {
            stmt->values.push_back(advance().value);
        } else {
            throw std::runtime_error("Expected value in INSERT");
        }
        
    } while (match(TokenType::COMMA));
    
    expect(TokenType::RPAREN, "Expected ')' after values");
    expect(TokenType::SEMICOLON, "Expected ';' at end of statement");
    
    return stmt;
}

// SELECT * FROM table_name [WHERE column = value];
std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    
    expect(TokenType::SELECT, "Expected SELECT");
    expect(TokenType::ASTERISK, "Expected '*' (only * is supported)");
    expect(TokenType::FROM, "Expected FROM");
    
    if (!check(TokenType::IDENTIFIER)) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = advance().value;
    
    // Optional WHERE clause
    if (match(TokenType::WHERE)) {
        stmt->hasWhere = true;
        
        if (!check(TokenType::IDENTIFIER)) {
            throw std::runtime_error("Expected column name in WHERE clause");
        }
        stmt->whereColumn = advance().value;
        
        expect(TokenType::EQUALS, "Expected '=' in WHERE clause");
        
        Token val = peek();
        if (val.type == TokenType::IDENTIFIER || 
            val.type == TokenType::STRING_LITERAL || 
            val.type == TokenType::NUMBER) {
            stmt->whereValue = advance().value;
        } else {
            throw std::runtime_error("Expected value in WHERE clause");
        }
    }
    
    expect(TokenType::SEMICOLON, "Expected ';' at end of statement");
    
    return stmt;
}
