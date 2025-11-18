#ifndef AST_H
#define AST_H

#include <optional>
#include <string>
#include <variant>
#include <vector>

struct CreateTableStatement {
    std::string tableName;
    std::vector<std::string> columns;
};

struct InsertStatement {
    std::string tableName;
    std::vector<std::string> values;
};

struct WhereClause {
    std::string column;
    std::string value;
};

struct SelectStatement {
    std::vector<std::string> columns;
    std::string tableName;
    std::optional<WhereClause> where;
};

using StatementVariant = std::variant<CreateTableStatement, InsertStatement, SelectStatement>;

struct Statement {
    StatementVariant value;
};

#endif // AST_H
