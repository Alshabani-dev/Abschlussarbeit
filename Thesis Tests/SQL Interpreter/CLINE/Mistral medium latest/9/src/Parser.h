#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
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
    std::unique_ptr<Statement> parse(const std::string &sql);

private:
    std::unique_ptr<Statement> parseStatement(const std::vector<Token> &tokens, size_t &pos);
    std::unique_ptr<Statement> parseCreateTable(const std::vector<Token> &tokens, size_t &pos);
    std::unique_ptr<Statement> parseInsert(const std::vector<Token> &tokens, size_t &pos);
    std::unique_ptr<Statement> parseSelect(const std::vector<Token> &tokens, size_t &pos);
};

#endif // PARSER_H
