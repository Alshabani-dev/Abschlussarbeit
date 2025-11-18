#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "Ast.h"

struct Row {
    std::vector<std::string> values;
};

struct Table {
    std::vector<std::string> columns;
    std::vector<Row> rows;
};

struct QueryResult {
    std::vector<std::string> columns;
    std::vector<Row> rows;
};

class Storage {
public:
    Storage();

    bool createTable(const CreateTableStatement &stmt, std::string &error);
    bool insertRow(const InsertStatement &stmt, std::string &error);
    bool select(const SelectStatement &stmt, QueryResult &result, std::string &error);

private:
    std::unordered_map<std::string, Table> tables_;

    void loadFromDisk();
    void loadTable(const std::string &path, const std::string &name);
    void persistTable(const std::string &name, const Table &table);
};

#endif // STORAGE_H
