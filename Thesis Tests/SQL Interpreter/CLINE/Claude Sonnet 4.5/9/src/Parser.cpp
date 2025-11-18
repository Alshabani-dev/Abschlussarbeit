#include "Parser.h"
#include <sstream>

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

std::unique_ptr<Statement> Parser::parse() {
    if (current().type == TokenType::END_OF_INPUT) {
        throw ParseError("Empty statement");
    }
    
    if (current().type != TokenType::KEYWORD) {
        throw ParseError("Expected SQL keyword, got: " + current().value);
    }
    
    std::string keyword = current().value;
    
    if (keyword == "CREATE") {
        return parseCreateTable();
    } else if (keyword == "INSERT") {
        return parseInsert();
    } else if (keyword == "SELECT") {
        return parseSelect();
    } else {
        throw ParseError("Unknown keyword: " + keyword);
    }
}

const Token& Parser::current() const {
    if (pos_ < tokens_.size()) {
        return tokens_[pos_];
    }
    static Token endToken(TokenType::END_OF_INPUT, "");
    return endToken;
}

const Token& Parser::peek(size_t offset) const {
    if (pos_ + offset < tokens_.size()) {
        return tokens_[pos_ + offset];
    }
    static Token endToken(TokenType::END_OF_INPUT, "");
    return endToken;
}

void Parser::advance() {
    if (pos_ < tokens_.size()) {
        pos_++;
    }
}

bool Parser::match(TokenType type, const std::string& value) {
    if (current().type != type) {
        return false;
    }
    if (!value.empty() && current().value != value) {
        return false;
    }
    return true;
}

void Parser::expect(TokenType type, const std::string& value) {
    if (!match(type, value)) {
        std::stringstream ss;
        ss << "Expected ";
        if (!value.empty()) {
            ss << "'" << value << "'";
        } else {
            ss << "token type " << static_cast<int>(type);
        }
        ss << ", got: '" << current().value << "'";
        throw ParseError(ss.str());
    }
    advance();
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();
    
    expect(TokenType::KEYWORD, "CREATE");
    expect(TokenType::KEYWORD, "TABLE");
    
    if (current().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected table name, got: " + current().value);
    }
    stmt->tableName = current().value;
    advance();
    
    expect(TokenType::SYMBOL, "(");
    
    // Parse column list
    while (true) {
        if (current().type != TokenType::IDENTIFIER) {
            throw ParseError("Expected column name, got: " + current().value);
        }
        stmt->columns.push_back(current().value);
        advance();
        
        if (match(TokenType::SYMBOL, ")")) {
            advance();
            break;
        }
        
        expect(TokenType::SYMBOL, ",");
    }
    
    if (stmt->columns.empty()) {
        throw ParseError("Table must have at least one column");
    }
    
    expect(TokenType::SYMBOL, ";");
    
    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();
    
    expect(TokenType::KEYWORD, "INSERT");
    expect(TokenType::KEYWORD, "INTO");
    
    if (current().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected table name, got: " + current().value);
    }
    stmt->tableName = current().value;
    advance();
    
    expect(TokenType::KEYWORD, "VALUES");
    expect(TokenType::SYMBOL, "(");
    
    // Parse values list
    while (true) {
        if (current().type == TokenType::IDENTIFIER ||
            current().type == TokenType::STRING_LITERAL ||
            current().type == TokenType::NUMBER) {
            stmt->values.push_back(current().value);
            advance();
        } else {
            throw ParseError("Expected value, got: " + current().value);
        }
        
        if (match(TokenType::SYMBOL, ")")) {
            advance();
            break;
        }
        
        expect(TokenType::SYMBOL, ",");
    }
    
    if (stmt->values.empty()) {
        throw ParseError("INSERT must have at least one value");
    }
    
    expect(TokenType::SYMBOL, ";");
    
    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    
    expect(TokenType::KEYWORD, "SELECT");
    expect(TokenType::SYMBOL, "*");
    expect(TokenType::KEYWORD, "FROM");
    
    if (current().type != TokenType::IDENTIFIER) {
        throw ParseError("Expected table name, got: " + current().value);
    }
    stmt->tableName = current().value;
    advance();
    
    // Check for optional WHERE clause
    if (match(TokenType::KEYWORD, "WHERE")) {
        advance();
        stmt->hasWhere = true;
        
        if (current().type != TokenType::IDENTIFIER) {
            throw ParseError("Expected column name in WHERE clause, got: " + current().value);
        }
        stmt->whereColumn = current().value;
        advance();
        
        expect(TokenType::SYMBOL, "=");
        
        if (current().type == TokenType::IDENTIFIER ||
            current().type == TokenType::STRING_LITERAL ||
            current().type == TokenType::NUMBER) {
            stmt->whereValue = current().value;
            advance();
        } else {
            throw ParseError("Expected value in WHERE clause, got: " + current().value);
        }
    }
    
    expect(TokenType::SYMBOL, ";");
    
    return stmt;
}
