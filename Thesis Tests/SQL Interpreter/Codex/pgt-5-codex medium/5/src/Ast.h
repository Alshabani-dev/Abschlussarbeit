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
    virtual ~Statement() = default;
    StatementType type;
};

struct CreateTableStmt : public Statement {
    std::string tableName;
    std::vector<std::string> columns;

    CreateTableStmt() { type = StatementType::CreateTable; }
};

struct InsertStmt : public Statement {
    std::string tableName;
    std::vector<std::string> values;

    InsertStmt() { type = StatementType::Insert; }
};

struct SelectStmt : public Statement {
    bool selectAll = false;
    std::vector<std::string> columns;
    std::string tableName;
    bool hasWhere = false;
    std::string whereColumn;
    std::string whereValue;

    SelectStmt() { type = StatementType::Select; }
};

using StatementPtr = std::unique_ptr<Statement>;

#endif // AST_H
