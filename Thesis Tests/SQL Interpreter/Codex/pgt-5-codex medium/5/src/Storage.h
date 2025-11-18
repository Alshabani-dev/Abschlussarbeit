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
    std::string name;
    std::vector<std::string> columns;
    std::vector<Row> rows;
};

class Storage {
public:
    Storage();

    bool createTable(const std::string &name, const std::vector<std::string> &columns, std::string &error);
    bool insertRow(const std::string &name, const std::vector<std::string> &values, std::string &error);
    bool selectRows(const SelectStmt &stmt, std::vector<std::string> &header, std::vector<Row> &rows, std::string &error) const;

    void loadFromDisk();
    void saveAll() const;

private:
    std::unordered_map<std::string, Table> tables_;
    std::string dataDirectory_ = "data";

    Table *findTable(const std::string &name);
    const Table *findTable(const std::string &name) const;
    void ensureDataDirectory() const;
    void saveTable(const Table &table) const;
    static std::string csvEscape(const std::string &value);
    static std::vector<std::string> parseCsvLine(const std::string &line);
};

#endif // STORAGE_H
