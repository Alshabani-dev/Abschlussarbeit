#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <variant>

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
    bool selectAll = false;
    std::vector<std::string> columns;
    std::string tableName;
    bool hasWhere = false;
    WhereClause where;
};

using Statement = std::variant<CreateTableStatement, InsertStatement, SelectStatement>;

#endif // AST_H
