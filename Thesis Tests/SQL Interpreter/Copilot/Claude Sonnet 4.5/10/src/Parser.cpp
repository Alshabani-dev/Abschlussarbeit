#include "Parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

std::unique_ptr<Statement> Parser::parse() {
    if (isAtEnd()) {
        setError("Empty statement");
        return nullptr;
    }
    
    const Token& first = peek();
    
    if (first.type != TokenType::KEYWORD) {
        setError("Expected keyword at start of statement");
        return nullptr;
    }
    
    if (first.value == "CREATE") {
        return parseCreateTable();
    } else if (first.value == "INSERT") {
        return parseInsert();
    } else if (first.value == "SELECT") {
        return parseSelect();
    } else {
        setError("Unknown keyword: " + first.value);
        return nullptr;
    }
}

bool Parser::isAtEnd() const {
    return pos_ >= tokens_.size() || peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::peek() const {
    if (pos_ >= tokens_.size()) {
        static Token eofToken(TokenType::END_OF_FILE, "");
        return eofToken;
    }
    return tokens_[pos_];
}

const Token& Parser::advance() {
    if (!isAtEnd()) {
        pos_++;
    }
    return tokens_[pos_ - 1];
}

bool Parser::match(TokenType type) {
    if (isAtEnd() || peek().type != type) {
        return false;
    }
    return true;
}

bool Parser::match(TokenType type, const std::string& value) {
    if (!match(type)) {
        return false;
    }
    return peek().value == value;
}

void Parser::setError(const std::string& message) {
    error_ = message;
}

bool Parser::expect(TokenType type, const std::string& message) {
    if (!match(type)) {
        setError(message + " (got: " + peek().value + ")");
        return false;
    }
    advance();
    return true;
}

bool Parser::expectSymbol(const std::string& symbol, const std::string& message) {
    if (!match(TokenType::SYMBOL, symbol)) {
        setError(message + " (got: " + peek().value + ")");
        return false;
    }
    advance();
    return true;
}

bool Parser::expectKeyword(const std::string& keyword, const std::string& message) {
    if (!match(TokenType::KEYWORD, keyword)) {
        setError(message + " (got: " + peek().value + ")");
        return false;
    }
    advance();
    return true;
}

std::unique_ptr<Statement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();
    
    // CREATE
    advance();
    
    // TABLE
    if (!expectKeyword("TABLE", "Expected TABLE keyword")) {
        return nullptr;
    }
    
    // table_name
    if (!match(TokenType::IDENTIFIER)) {
        setError("Expected table name");
        return nullptr;
    }
    stmt->tableName = advance().value;
    
    // (
    if (!expectSymbol("(", "Expected '(' after table name")) {
        return nullptr;
    }
    
    // column1, column2, ...
    while (!isAtEnd() && !match(TokenType::SYMBOL, ")")) {
        if (!match(TokenType::IDENTIFIER)) {
            setError("Expected column name");
            return nullptr;
        }
        stmt->columns.push_back(advance().value);
        
        // Optional comma
        if (match(TokenType::SYMBOL, ",")) {
            advance();
        }
    }
    
    // )
    if (!expectSymbol(")", "Expected ')' after column list")) {
        return nullptr;
    }
    
    // Optional semicolon
    if (match(TokenType::SEMICOLON)) {
        advance();
    }
    
    if (stmt->columns.empty()) {
        setError("Table must have at least one column");
        return nullptr;
    }
    
    return stmt;
}

std::unique_ptr<Statement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();
    
    // INSERT
    advance();
    
    // INTO
    if (!expectKeyword("INTO", "Expected INTO keyword")) {
        return nullptr;
    }
    
    // table_name
    if (!match(TokenType::IDENTIFIER)) {
        setError("Expected table name");
        return nullptr;
    }
    stmt->tableName = advance().value;
    
    // VALUES
    if (!expectKeyword("VALUES", "Expected VALUES keyword")) {
        return nullptr;
    }
    
    // (
    if (!expectSymbol("(", "Expected '(' after VALUES")) {
        return nullptr;
    }
    
    // value1, value2, ...
    while (!isAtEnd() && !match(TokenType::SYMBOL, ")")) {
        const Token& tok = peek();
        
        if (tok.type == TokenType::IDENTIFIER || 
            tok.type == TokenType::STRING || 
            tok.type == TokenType::NUMBER) {
            stmt->values.push_back(advance().value);
        } else {
            setError("Expected value in VALUES list");
            return nullptr;
        }
        
        // Optional comma
        if (match(TokenType::SYMBOL, ",")) {
            advance();
        }
    }
    
    // )
    if (!expectSymbol(")", "Expected ')' after values list")) {
        return nullptr;
    }
    
    // Optional semicolon
    if (match(TokenType::SEMICOLON)) {
        advance();
    }
    
    if (stmt->values.empty()) {
        setError("INSERT must have at least one value");
        return nullptr;
    }
    
    return stmt;
}

std::unique_ptr<Statement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    
    // SELECT
    advance();
    
    // *
    if (!expectSymbol("*", "Expected '*' after SELECT")) {
        return nullptr;
    }
    
    // FROM
    if (!expectKeyword("FROM", "Expected FROM keyword")) {
        return nullptr;
    }
    
    // table_name
    if (!match(TokenType::IDENTIFIER)) {
        setError("Expected table name");
        return nullptr;
    }
    stmt->tableName = advance().value;
    
    // Optional WHERE clause
    if (match(TokenType::KEYWORD, "WHERE")) {
        advance();
        stmt->hasWhere = true;
        
        // column
        if (!match(TokenType::IDENTIFIER)) {
            setError("Expected column name after WHERE");
            return nullptr;
        }
        stmt->whereColumn = advance().value;
        
        // =
        if (!expectSymbol("=", "Expected '=' in WHERE clause")) {
            return nullptr;
        }
        
        // value
        const Token& tok = peek();
        if (tok.type == TokenType::IDENTIFIER || 
            tok.type == TokenType::STRING || 
            tok.type == TokenType::NUMBER) {
            stmt->whereValue = advance().value;
        } else {
            setError("Expected value after '=' in WHERE clause");
            return nullptr;
        }
    }
    
    // Optional semicolon
    if (match(TokenType::SEMICOLON)) {
        advance();
    }
    
    return stmt;
}
