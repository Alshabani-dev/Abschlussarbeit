#include "Parser.h"
#include "Utils.h"

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens), current_(0) {}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (isAtEnd()) {
        error_ = "Empty statement";
        return nullptr;
    }
    
    const Token& first = peek();
    
    if (first.type == TokenType::CREATE) {
        return parseCreateTable();
    } else if (first.type == TokenType::INSERT) {
        return parseInsert();
    } else if (first.type == TokenType::SELECT) {
        return parseSelect();
    } else {
        error_ = "Expected CREATE, INSERT, or SELECT, got: " + first.value;
        return nullptr;
    }
}

const Token& Parser::peek() const {
    if (current_ >= tokens_.size()) {
        static Token eof(TokenType::END_OF_FILE, "", 0);
        return eof;
    }
    return tokens_[current_];
}

const Token& Parser::advance() {
    if (current_ < tokens_.size()) {
        return tokens_[current_++];
    }
    static Token eof(TokenType::END_OF_FILE, "", 0);
    return eof;
}

bool Parser::check(TokenType type) const {
    return !isAtEnd() && peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::expect(TokenType type, const std::string& errorMsg) {
    if (check(type)) {
        advance();
        return true;
    }
    error_ = errorMsg + " Expected type but got: " + peek().value;
    return false;
}

bool Parser::isAtEnd() const {
    return current_ >= tokens_.size() || peek().type == TokenType::END_OF_FILE;
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
    stmt->tableName = Utils::toLower(advance().value);
    
    // (
    if (!expect(TokenType::LEFT_PAREN, "Expected '(' after table name")) {
        return nullptr;
    }
    
    // column list
    do {
        if (!check(TokenType::IDENTIFIER)) {
            error_ = "Expected column name";
            return nullptr;
        }
        stmt->columns.push_back(Utils::toLower(advance().value));
    } while (match(TokenType::COMMA));
    
    // )
    if (!expect(TokenType::RIGHT_PAREN, "Expected ')' after column list")) {
        return nullptr;
    }
    
    // ;
    if (!expect(TokenType::SEMICOLON, "Expected ';' at end of statement")) {
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
    stmt->tableName = Utils::toLower(advance().value);
    
    // VALUES
    if (!expect(TokenType::VALUES, "Expected VALUES")) {
        return nullptr;
    }
    
    // (
    if (!expect(TokenType::LEFT_PAREN, "Expected '(' after VALUES")) {
        return nullptr;
    }
    
    // value list
    do {
        const Token& token = peek();
        if (token.type == TokenType::IDENTIFIER || 
            token.type == TokenType::NUMBER) {
            stmt->values.push_back(advance().value);
        } else if (token.type == TokenType::STRING_LITERAL) {
            stmt->values.push_back(advance().value);
        } else {
            error_ = "Expected value (identifier, number, or string)";
            return nullptr;
        }
    } while (match(TokenType::COMMA));
    
    // )
    if (!expect(TokenType::RIGHT_PAREN, "Expected ')' after value list")) {
        return nullptr;
    }
    
    // ;
    if (!expect(TokenType::SEMICOLON, "Expected ';' at end of statement")) {
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
    if (!expect(TokenType::ASTERISK, "Expected '*' (only SELECT * is supported)")) {
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
    stmt->tableName = Utils::toLower(advance().value);
    
    // Optional WHERE clause
    if (match(TokenType::WHERE)) {
        stmt->hasWhere = true;
        
        // column_name
        if (!check(TokenType::IDENTIFIER)) {
            error_ = "Expected column name in WHERE clause";
            return nullptr;
        }
        stmt->whereColumn = Utils::toLower(advance().value);
        
        // =
        if (!expect(TokenType::EQUALS, "Expected '=' in WHERE clause")) {
            return nullptr;
        }
        
        // value
        const Token& token = peek();
        if (token.type == TokenType::IDENTIFIER || 
            token.type == TokenType::NUMBER ||
            token.type == TokenType::STRING_LITERAL) {
            stmt->whereValue = advance().value;
        } else {
            error_ = "Expected value in WHERE clause";
            return nullptr;
        }
    }
    
    // ;
    if (!expect(TokenType::SEMICOLON, "Expected ';' at end of statement")) {
        return nullptr;
    }
    
    return stmt;
}
