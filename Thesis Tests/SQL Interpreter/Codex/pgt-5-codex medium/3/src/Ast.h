#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <vector>

enum class StatementType {
    CreateTable,
    Insert,
    Select
};

struct Statement {
    explicit Statement(StatementType type) : type(type) {}
    virtual ~Statement() = default;
    StatementType type;
};

struct CreateTableStatement : public Statement {
    CreateTableStatement() : Statement(StatementType::CreateTable) {}
    std::string tableName;
    std::vector<std::string> columns;
};

struct InsertStatement : public Statement {
    InsertStatement() : Statement(StatementType::Insert) {}
    std::string tableName;
    std::vector<std::string> values;
};

struct SelectStatement : public Statement {
    SelectStatement() : Statement(StatementType::Select) {}
    bool selectAll = false;
    std::vector<std::string> columns;
    std::string tableName;
    bool hasWhere = false;
    std::string whereColumn;
    std::string whereValue;
};

using StatementPtr = std::unique_ptr<Statement>;

#endif // AST_H
