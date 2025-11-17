#include "Parser.h"
#include "Utils.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (pos_ >= tokens_.size()) {
        setError("Unexpected end of input");
        return nullptr;
    }
    
    const Token& current = peek();
    
    if (current.type == TokenType::CREATE) {
        return parseCreateTable();
    } else if (current.type == TokenType::INSERT) {
        return parseInsert();
    } else if (current.type == TokenType::SELECT) {
        return parseSelect();
    } else {
        setError("Expected CREATE, INSERT, or SELECT statement");
        return nullptr;
    }
}

const Token& Parser::peek() const {
    if (pos_ < tokens_.size()) {
        return tokens_[pos_];
    }
    static Token eof(TokenType::END_OF_FILE, "");
    return eof;
}

const Token& Parser::advance() {
    if (pos_ < tokens_.size()) {
        return tokens_[pos_++];
    }
    static Token eof(TokenType::END_OF_FILE, "");
    return eof;
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

bool Parser::expect(TokenType type, const std::string& errorMsg) {
    if (match(type)) {
        return true;
    }
    setError(errorMsg);
    return false;
}

void Parser::setError(const std::string& msg) {
    errorMessage_ = msg;
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();
    
    // CREATE
    if (!expect(TokenType::CREATE, "Expected CREATE")) {
        return nullptr;
    }
    
    // TABLE
    if (!expect(TokenType::TABLE, "Expected TABLE after CREATE")) {
        return nullptr;
    }
    
    // table_name
    if (peek().type != TokenType::IDENTIFIER) {
        setError("Expected table name");
        return nullptr;
    }
    stmt->tableName = Utils::toLower(advance().value);
    
    // (
    if (!expect(TokenType::LPAREN, "Expected '(' after table name")) {
        return nullptr;
    }
    
    // column list
    do {
        if (peek().type != TokenType::IDENTIFIER) {
            setError("Expected column name");
            return nullptr;
        }
        stmt->columns.push_back(Utils::toLower(advance().value));
    } while (match(TokenType::COMMA));
    
    // )
    if (!expect(TokenType::RPAREN, "Expected ')' after column list")) {
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
    if (!expect(TokenType::INTO, "Expected INTO after INSERT")) {
        return nullptr;
    }
    
    // table_name
    if (peek().type != TokenType::IDENTIFIER) {
        setError("Expected table name");
        return nullptr;
    }
    stmt->tableName = Utils::toLower(advance().value);
    
    // VALUES
    if (!expect(TokenType::VALUES, "Expected VALUES")) {
        return nullptr;
    }
    
    // (
    if (!expect(TokenType::LPAREN, "Expected '(' after VALUES")) {
        return nullptr;
    }
    
    // value list
    do {
        const Token& valueToken = peek();
        
        if (valueToken.type == TokenType::IDENTIFIER ||
            valueToken.type == TokenType::STRING ||
            valueToken.type == TokenType::NUMBER) {
            stmt->values.push_back(advance().value);
        } else {
            setError("Expected value in INSERT");
            return nullptr;
        }
    } while (match(TokenType::COMMA));
    
    // )
    if (!expect(TokenType::RPAREN, "Expected ')' after value list")) {
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
    if (!expect(TokenType::ASTERISK, "Expected '*' after SELECT")) {
        return nullptr;
    }
    
    // FROM
    if (!expect(TokenType::FROM, "Expected FROM")) {
        return nullptr;
    }
    
    // table_name
    if (peek().type != TokenType::IDENTIFIER) {
        setError("Expected table name");
        return nullptr;
    }
    stmt->tableName = Utils::toLower(advance().value);
    
    // Optional WHERE clause
    if (match(TokenType::WHERE)) {
        stmt->hasWhere = true;
        
        // column_name
        if (peek().type != TokenType::IDENTIFIER) {
            setError("Expected column name in WHERE clause");
            return nullptr;
        }
        stmt->whereColumn = Utils::toLower(advance().value);
        
        // =
        if (!expect(TokenType::EQUALS, "Expected '=' in WHERE clause")) {
            return nullptr;
        }
        
        // value
        const Token& valueToken = peek();
        if (valueToken.type == TokenType::IDENTIFIER ||
            valueToken.type == TokenType::STRING ||
            valueToken.type == TokenType::NUMBER) {
            stmt->whereValue = advance().value;
        } else {
            setError("Expected value in WHERE clause");
            return nullptr;
        }
    }
    
    // ;
    if (!expect(TokenType::SEMICOLON, "Expected ';' at end of statement")) {
        return nullptr;
    }
    
    return stmt;
}
