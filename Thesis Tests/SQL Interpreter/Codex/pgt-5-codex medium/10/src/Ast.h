#ifndef AST_H
#define AST_H

#include <memory>
#include <optional>
#include <string>
#include <vector>

struct Condition {
    std::string column;
    std::string value;
};

struct Statement {
    virtual ~Statement() = default;
};

struct CreateTableStatement : public Statement {
    std::string tableName;
    std::vector<std::string> columns;
};

struct InsertStatement : public Statement {
    std::string tableName;
    std::vector<std::string> values;
};

struct SelectStatement : public Statement {
    std::vector<std::string> columns;
    std::string tableName;
    std::optional<Condition> whereClause;
};

using StatementPtr = std::unique_ptr<Statement>;

#endif // AST_H
