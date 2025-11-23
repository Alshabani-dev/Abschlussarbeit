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

// Base class for all SQL statements
struct Statement {
    virtual ~Statement() = default;
    virtual StatementType type() const = 0;
};

// CREATE TABLE table_name (col1, col2, col3);
struct CreateTableStatement : Statement {
    std::string tableName;
    std::vector<std::string> columns;
    
    StatementType type() const override {
        return StatementType::CREATE_TABLE;
    }
};

// INSERT INTO table_name VALUES (val1, val2, val3);
struct InsertStatement : Statement {
    std::string tableName;
    std::vector<std::string> values;
    
    StatementType type() const override {
        return StatementType::INSERT;
    }
};

// SELECT * FROM table_name [WHERE column = value];
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
