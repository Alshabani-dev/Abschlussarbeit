#ifndef AST_H
#define AST_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

enum class StatementKind {
    CreateTable,
    Insert,
    Select
};

struct Statement {
    explicit Statement(StatementKind kind) : kind(kind) {}
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
    std::vector<std::string> columns; // "*" represented by {"*"}
    std::string tableName;
    std::optional<std::string> whereColumn;
    std::optional<std::string> whereValue;
};

#endif // AST_H
