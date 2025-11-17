#include "Parser.h"

Parser::Parser(const std::vector<Token>& tokens) 
    : tokens_(tokens), pos_(0) {}

const Token& Parser::peek() const {
    if (pos_ >= tokens_.size()) {
        static Token eof(TokenType::END_OF_FILE, "");
        return eof;
    }
    return tokens_[pos_];
}

const Token& Parser::advance() {
    if (pos_ < tokens_.size()) {
        return tokens_[pos_++];
    }
    static Token eof(TokenType::END_OF_FILE, "");
    return eof;
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::checkValue(const std::string& value) const {
    return peek().value == value;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::matchValue(const std::string& value) {
    if (checkValue(value)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::expect(TokenType type, const std::string& errorMsg) {
    if (!check(type)) {
        lastError_ = errorMsg + " (got: " + peek().value + ")";
        return false;
    }
    advance();
    return true;
}

bool Parser::expectValue(const std::string& value, const std::string& errorMsg) {
    if (!checkValue(value)) {
        lastError_ = errorMsg + " (got: " + peek().value + ")";
        return false;
    }
    advance();
    return true;
}

std::string Parser::getLastError() const {
    return lastError_;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (check(TokenType::KEYWORD)) {
        const std::string& keyword = peek().value;
        
        if (keyword == "CREATE") {
            return parseCreateTable();
        } else if (keyword == "INSERT") {
            return parseInsert();
        } else if (keyword == "SELECT") {
            return parseSelect();
        }
    }
    
    lastError_ = "Expected CREATE, INSERT, or SELECT";
    return nullptr;
}

std::unique_ptr<CreateTableStatement> Parser::parseCreateTable() {
    auto stmt = std::make_unique<CreateTableStatement>();
    
    // CREATE
    if (!expectValue("CREATE", "Expected CREATE")) {
        return nullptr;
    }
    
    // TABLE
    if (!expectValue("TABLE", "Expected TABLE")) {
        return nullptr;
    }
    
    // table_name
    if (!expect(TokenType::IDENTIFIER, "Expected table name")) {
        return nullptr;
    }
    stmt->tableName = tokens_[pos_ - 1].value;
    
    // (
    if (!expectValue("(", "Expected '('")) {
        return nullptr;
    }
    
    // column list
    do {
        if (!expect(TokenType::IDENTIFIER, "Expected column name")) {
            return nullptr;
        }
        stmt->columns.push_back(tokens_[pos_ - 1].value);
    } while (matchValue(","));
    
    // )
    if (!expectValue(")", "Expected ')'")) {
        return nullptr;
    }
    
    // ;
    if (!expectValue(";", "Expected ';'")) {
        return nullptr;
    }
    
    return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
    auto stmt = std::make_unique<InsertStatement>();
    
    // INSERT
    if (!expectValue("INSERT", "Expected INSERT")) {
        return nullptr;
    }
    
    // INTO
    if (!expectValue("INTO", "Expected INTO")) {
        return nullptr;
    }
    
    // table_name
    if (!expect(TokenType::IDENTIFIER, "Expected table name")) {
        return nullptr;
    }
    stmt->tableName = tokens_[pos_ - 1].value;
    
    // VALUES
    if (!expectValue("VALUES", "Expected VALUES")) {
        return nullptr;
    }
    
    // (
    if (!expectValue("(", "Expected '('")) {
        return nullptr;
    }
    
    // value list
    do {
        const Token& token = peek();
        if (token.type == TokenType::IDENTIFIER || 
            token.type == TokenType::NUMBER ||
            token.type == TokenType::STRING_LITERAL) {
            stmt->values.push_back(advance().value);
        } else {
            lastError_ = "Expected value";
            return nullptr;
        }
    } while (matchValue(","));
    
    // )
    if (!expectValue(")", "Expected ')'")) {
        return nullptr;
    }
    
    // ;
    if (!expectValue(";", "Expected ';'")) {
        return nullptr;
    }
    
    return stmt;
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
    auto stmt = std::make_unique<SelectStatement>();
    
    // SELECT
    if (!expectValue("SELECT", "Expected SELECT")) {
        return nullptr;
    }
    
    // *
    if (!expectValue("*", "Expected '*'")) {
        return nullptr;
    }
    
    // FROM
    if (!expectValue("FROM", "Expected FROM")) {
        return nullptr;
    }
    
    // table_name
    if (!expect(TokenType::IDENTIFIER, "Expected table name")) {
        return nullptr;
    }
    stmt->tableName = tokens_[pos_ - 1].value;
    
    // Optional WHERE clause
    if (checkValue("WHERE")) {
        advance(); // consume WHERE
        stmt->hasWhere = true;
        
        // column name
        if (!expect(TokenType::IDENTIFIER, "Expected column name")) {
            return nullptr;
        }
        stmt->whereColumn = tokens_[pos_ - 1].value;
        
        // =
        if (!expectValue("=", "Expected '='")) {
            return nullptr;
        }
        
        // value
        const Token& token = peek();
        if (token.type == TokenType::IDENTIFIER || 
            token.type == TokenType::NUMBER ||
            token.type == TokenType::STRING_LITERAL) {
            stmt->whereValue = advance().value;
        } else {
            lastError_ = "Expected value in WHERE clause";
            return nullptr;
        }
    }
    
    // ;
    if (!expectValue(";", "Expected ';'")) {
        return nullptr;
    }
    
    return stmt;
}
