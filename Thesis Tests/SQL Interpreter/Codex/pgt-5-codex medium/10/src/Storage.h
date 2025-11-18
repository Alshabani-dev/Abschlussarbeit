#ifndef STORAGE_H
#define STORAGE_H

#include <optional>
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

struct SelectResult {
    std::vector<std::string> columns;
    std::vector<Row> rows;
};

class Storage {
public:
    Storage();

    bool createTable(const std::string &name, const std::vector<std::string> &columns, std::string &error);
    bool insertInto(const std::string &name, const std::vector<std::string> &values, std::string &error);
    std::optional<SelectResult> selectFrom(const SelectStatement &stmt, std::string &error) const;

private:
    std::unordered_map<std::string, Table> tables_;
    std::string dataPath_;

    void loadTables();
    void saveTable(const std::string &name, const Table &table) const;
    static std::string tableFilePath(const std::string &dataPath, const std::string &tableName);
    static std::vector<std::string> parseCsvLine(const std::string &line);
    static std::string toCsvLine(const std::vector<std::string> &values);
};

#endif // STORAGE_H
