#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

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
    
    StatementType type() const override {
        return StatementType::CREATE_TABLE;
    }
};

struct InsertStatement : Statement {
    std::string tableName;
    std::vector<std::string> values;
    
    StatementType type() const override {
        return StatementType::INSERT;
    }
};

struct SelectStatement : Statement {
    std::string tableName;
    bool hasWhere = false;
    std::string whereColumn;
    std::string whereValue;
    
    StatementType type() const override {
        return StatementType::SELECT;
    }
};

#endif // AST_H
