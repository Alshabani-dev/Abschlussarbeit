#ifndef AST_H
#define AST_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

struct WhereClause {
    std::string column;
    std::string value;
};

enum class StatementKind {
    CreateTable,
    Insert,
    Select
};

struct Statement {
    explicit Statement(StatementKind k) : kind(k) {}
    virtual ~Statement() = default;
    StatementKind kind;
};

struct CreateTableStatement : public Statement {
    CreateTableStatement() : Statement(StatementKind::CreateTable) {}
    std::string tableName;
    std::vector<std::string> columns;
};

struct InsertStatement : public Statement {
    InsertStatement() : Statement(StatementKind::Insert) {}
    std::string tableName;
    std::vector<std::string> values;
};

struct SelectStatement : public Statement {
    SelectStatement() : Statement(StatementKind::Select) {}
    std::string tableName;
    bool selectAll = false;
    std::vector<std::string> columns;
    std::optional<WhereClause> where;
};

using StatementPtr = std::unique_ptr<Statement>;
using StatementList = std::vector<StatementPtr>;

#endif // AST_H
