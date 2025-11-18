#include "Parser.h"
#include "Utils.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

std::unique_ptr<Statement> Parser::parseStatement() {
    error_.clear();
    
    if (currentToken().type == TokenType::END_OF_FILE) {
        return nullptr;
    }
    
    try {
        if (currentToken().type == TokenType::CREATE) {
            return parseCreateTable();
        } else if (currentToken().type == TokenType::INSERT) {
            return parseInsert();
        } else if (currentToken().type == TokenType::SELECT) {
            return parseSelect();
        } else {
            error_ = "Unexpected token: " + currentToken().value;
            return nullptr;
        }
    } catch (const std::exception& e) {
        error_ = e.what();
        return nullptr;
    }
}

const Token& Parser::currentToken() const {
    if (pos_ >= tokens_.size()) {
        static Token eofToken(TokenType::END_OF_FILE, "");
        return eofToken;
    }
    return tokens_[pos_];
}

const Token& Parser::peek(size_t offset) const {
    size_t peekPos = pos_ + offset;
    if (peekPos >= tokens_.size()) {
        static Token eofToken(TokenType::END_OF_FILE, "");
        return eofToken;
    }
    return tokens_[peekPos];
}

void Parser::advance() {
    if (pos_ < tokens_.size()) {
        pos_++;
    }
}

bool Parser::match(TokenType type) {
    if (currentToken().type == type) {
        advance();
        return true;
    }
    return false;
}

bool Parser::expect(TokenType type, const std::string& errorMsg) {
    if (currentToken().type != type) {
        throw std::runtime_error(errorMsg + " (got: " + currentToken().value + ")");
    }
    advance();
    return true;
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();
    
    expect(TokenType::CREATE, "Expected CREATE");
    expect(TokenType::TABLE, "Expected TABLE");
    
    if (currentToken().type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = Utils::toLower(currentToken().value);
    advance();
    
    expect(TokenType::LPAREN, "Expected '('");
    
    // Parse column names
    while (currentToken().type != TokenType::RPAREN && 
           currentToken().type != TokenType::END_OF_FILE) {
        if (currentToken().type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected column name");
        }
        stmt->columns.push_back(Utils::toLower(currentToken().value));
        advance();
        
        if (currentToken().type == TokenType::COMMA) {
            advance();
        } else if (currentToken().type != TokenType::RPAREN) {
            throw std::runtime_error("Expected ',' or ')'");
        }
    }
    
    expect(TokenType::RPAREN, "Expected ')'");
    
    if (currentToken().type == TokenType::SEMICOLON) {
        advance();
    }
    
    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();
    
    expect(TokenType::INSERT, "Expected INSERT");
    expect(TokenType::INTO, "Expected INTO");
    
    if (currentToken().type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = Utils::toLower(currentToken().value);
    advance();
    
    expect(TokenType::VALUES, "Expected VALUES");
    expect(TokenType::LPAREN, "Expected '('");
    
    // Parse values
    while (currentToken().type != TokenType::RPAREN && 
           currentToken().type != TokenType::END_OF_FILE) {
        if (currentToken().type == TokenType::IDENTIFIER ||
            currentToken().type == TokenType::STRING_LITERAL ||
            currentToken().type == TokenType::NUMBER) {
            stmt->values.push_back(currentToken().value);
            advance();
        } else {
            throw std::runtime_error("Expected value");
        }
        
        if (currentToken().type == TokenType::COMMA) {
            advance();
        } else if (currentToken().type != TokenType::RPAREN) {
            throw std::runtime_error("Expected ',' or ')'");
        }
    }
    
    expect(TokenType::RPAREN, "Expected ')'");
    
    if (currentToken().type == TokenType::SEMICOLON) {
        advance();
    }
    
    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    
    expect(TokenType::SELECT, "Expected SELECT");
    
    // We only support SELECT *
    expect(TokenType::ASTERISK, "Expected '*'");
    
    expect(TokenType::FROM, "Expected FROM");
    
    if (currentToken().type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected table name");
    }
    stmt->tableName = Utils::toLower(currentToken().value);
    advance();
    
    // Optional WHERE clause
    if (currentToken().type == TokenType::WHERE) {
        advance();
        stmt->hasWhere = true;
        
        if (currentToken().type != TokenType::IDENTIFIER) {
            throw std::runtime_error("Expected column name in WHERE clause");
        }
        stmt->whereColumn = Utils::toLower(currentToken().value);
        advance();
        
        expect(TokenType::EQUALS, "Expected '=' in WHERE clause");
        
        if (currentToken().type == TokenType::IDENTIFIER ||
            currentToken().type == TokenType::STRING_LITERAL ||
            currentToken().type == TokenType::NUMBER) {
            stmt->whereValue = currentToken().value;
            advance();
        } else {
            throw std::runtime_error("Expected value in WHERE clause");
        }
    }
    
    if (currentToken().type == TokenType::SEMICOLON) {
        advance();
    }
    
    return stmt;
}
