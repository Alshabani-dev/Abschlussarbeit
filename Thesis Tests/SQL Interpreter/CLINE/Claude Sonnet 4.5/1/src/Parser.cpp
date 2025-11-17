#include "Parser.h"
#include "Utils.h"

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens), current_(0) {}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (isAtEnd()) {
        error_ = "Unexpected end of input";
        return nullptr;
    }
    
    const Token& token = currentToken();
    
    if (token.type == TokenType::CREATE) {
        return parseCreateTable();
    } else if (token.type == TokenType::INSERT) {
        return parseInsert();
    } else if (token.type == TokenType::SELECT) {
        return parseSelect();
    } else {
        error_ = "Expected CREATE, INSERT, or SELECT statement";
        return nullptr;
    }
}

const Token& Parser::currentToken() const {
    if (current_ < tokens_.size()) {
        return tokens_[current_];
    }
    return tokens_.back(); // Return EOF token
}

const Token& Parser::peek(int offset) const {
    size_t pos = current_ + offset;
    if (pos < tokens_.size()) {
        return tokens_[pos];
    }
    return tokens_.back();
}

bool Parser::isAtEnd() const {
    return current_ >= tokens_.size() || currentToken().type == TokenType::END_OF_FILE;
}

void Parser::advance() {
    if (!isAtEnd()) {
        current_++;
    }
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return currentToken().type == type;
}

bool Parser::expect(TokenType type, const std::string& message) {
    if (check(type)) {
        advance();
        return true;
    }
    error_ = message + " (got: " + currentToken().value + ")";
    return false;
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();
    
    // CREATE
    if (!expect(TokenType::CREATE, "Expected CREATE")) {
        return nullptr;
    }
    
    // TABLE
    if (!expect(TokenType::TABLE, "Expected TABLE")) {
        return nullptr;
    }
    
    // table_name
    if (!check(TokenType::IDENTIFIER)) {
        error_ = "Expected table name";
        return nullptr;
    }
    stmt->tableName = Utils::toLower(currentToken().value);
    advance();
    
    // (
    if (!expect(TokenType::LEFT_PAREN, "Expected '('")) {
        return nullptr;
    }
    
    // column list
    stmt->columns = parseColumnList();
    if (hasError()) {
        return nullptr;
    }
    
    // )
    if (!expect(TokenType::RIGHT_PAREN, "Expected ')'")) {
        return nullptr;
    }
    
    // ;
    if (!expect(TokenType::SEMICOLON, "Expected ';'")) {
        return nullptr;
    }
    
    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();
    
    // INSERT
    if (!expect(TokenType::INSERT, "Expected INSERT")) {
        return nullptr;
    }
    
    // INTO
    if (!expect(TokenType::INTO, "Expected INTO")) {
        return nullptr;
    }
    
    // table_name
    if (!check(TokenType::IDENTIFIER)) {
        error_ = "Expected table name";
        return nullptr;
    }
    stmt->tableName = Utils::toLower(currentToken().value);
    advance();
    
    // VALUES
    if (!expect(TokenType::VALUES, "Expected VALUES")) {
        return nullptr;
    }
    
    // (
    if (!expect(TokenType::LEFT_PAREN, "Expected '('")) {
        return nullptr;
    }
    
    // value list
    stmt->values = parseValueList();
    if (hasError()) {
        return nullptr;
    }
    
    // )
    if (!expect(TokenType::RIGHT_PAREN, "Expected ')'")) {
        return nullptr;
    }
    
    // ;
    if (!expect(TokenType::SEMICOLON, "Expected ';'")) {
        return nullptr;
    }
    
    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    
    // SELECT
    if (!expect(TokenType::SELECT, "Expected SELECT")) {
        return nullptr;
    }
    
    // *
    if (!expect(TokenType::ASTERISK, "Expected '*'")) {
        return nullptr;
    }
    
    // FROM
    if (!expect(TokenType::FROM, "Expected FROM")) {
        return nullptr;
    }
    
    // table_name
    if (!check(TokenType::IDENTIFIER)) {
        error_ = "Expected table name";
        return nullptr;
    }
    stmt->tableName = Utils::toLower(currentToken().value);
    advance();
    
    // Optional WHERE clause
    if (match(TokenType::WHERE)) {
        stmt->hasWhere = true;
        
        // column name
        if (!check(TokenType::IDENTIFIER)) {
            error_ = "Expected column name in WHERE clause";
            return nullptr;
        }
        stmt->whereColumn = Utils::toLower(currentToken().value);
        advance();
        
        // =
        if (!expect(TokenType::EQUALS, "Expected '=' in WHERE clause")) {
            return nullptr;
        }
        
        // value
        if (!check(TokenType::IDENTIFIER) && !check(TokenType::STRING_LITERAL) && !check(TokenType::NUMBER)) {
            error_ = "Expected value in WHERE clause";
            return nullptr;
        }
        stmt->whereValue = currentToken().value;
        advance();
    }
    
    // ;
    if (!expect(TokenType::SEMICOLON, "Expected ';'")) {
        return nullptr;
    }
    
    return stmt;
}

std::vector<std::string> Parser::parseColumnList() {
    std::vector<std::string> columns;
    
    // First column
    if (!check(TokenType::IDENTIFIER)) {
        error_ = "Expected column name";
        return columns;
    }
    columns.push_back(Utils::toLower(currentToken().value));
    advance();
    
    // Additional columns
    while (match(TokenType::COMMA)) {
        if (!check(TokenType::IDENTIFIER)) {
            error_ = "Expected column name after ','";
            return columns;
        }
        columns.push_back(Utils::toLower(currentToken().value));
        advance();
    }
    
    return columns;
}

std::vector<std::string> Parser::parseValueList() {
    std::vector<std::string> values;
    
    // First value
    if (!check(TokenType::IDENTIFIER) && !check(TokenType::STRING_LITERAL) && !check(TokenType::NUMBER)) {
        error_ = "Expected value";
        return values;
    }
    values.push_back(currentToken().value);
    advance();
    
    // Additional values
    while (match(TokenType::COMMA)) {
        if (!check(TokenType::IDENTIFIER) && !check(TokenType::STRING_LITERAL) && !check(TokenType::NUMBER)) {
            error_ = "Expected value after ','";
            return values;
        }
        values.push_back(currentToken().value);
        advance();
    }
    
    return values;
}
