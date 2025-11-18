#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <vector>

enum class StatementType {
    CREATE_TABLE,
    INSERT,
    SELECT
};

struct Statement {
    explicit Statement(StatementType t) : type(t) {}
    virtual ~Statement() = default;
    StatementType type;
};

struct CreateTableStatement : public Statement {
    CreateTableStatement() : Statement(StatementType::CREATE_TABLE) {}
    std::string tableName;
    std::vector<std::string> columns;
};

struct InsertStatement : public Statement {
    InsertStatement() : Statement(StatementType::INSERT) {}
    std::string tableName;
    std::vector<std::string> values;
};

struct SelectStatement : public Statement {
    SelectStatement() : Statement(StatementType::SELECT) {}
    std::string tableName;
    std::vector<std::string> columns; // empty when *
    bool selectAll = false;
    bool hasWhere = false;
    std::string whereColumn;
    std::string whereValue;
};

using StatementPtr = std::unique_ptr<Statement>;

#endif // AST_H
