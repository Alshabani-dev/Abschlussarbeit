#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <memory>
#include "Lexer.h"

enum class StatementType {
    CREATE_TABLE,
    INSERT,
    SELECT
};

struct Statement {
    virtual ~Statement() = default;
    virtual StatementType type() const = 0;
};

struct CreateTableStatement : Statement {
    std::string tableName;
    std::vector<std::string> columns;
    StatementType type() const override { return StatementType::CREATE_TABLE; }
};

struct InsertStatement : Statement {
    std::string tableName;
    std::vector<std::string> values;
    StatementType type() const override { return StatementType::INSERT; }
};

struct SelectStatement : Statement {
    std::string tableName;
    bool hasWhere = false;
    std::string whereColumn;
    std::string whereValue;
    StatementType type() const override { return StatementType::SELECT; }
};

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<Statement> parse();

private:
    std::vector<Token> tokens_;
    size_t position_ = 0;

    Token peek() const;
    Token consume();
    std::unique_ptr<Statement> parseCreateTable();
    std::unique_ptr<Statement> parseInsert();
    std::unique_ptr<Statement> parseSelect();
    std::vector<std::string> parseColumnList();
    std::vector<std::string> parseValueList();
    std::pair<std::string, std::string> parseWhereClause();
};

#endif // PARSER_H
