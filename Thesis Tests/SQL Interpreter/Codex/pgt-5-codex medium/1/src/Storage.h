#ifndef STORAGE_H
#define STORAGE_H

#include "Ast.h"

#include <string>
#include <unordered_map>
#include <vector>

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

    void createTable(const CreateTableStatement &stmt);
    void insertRow(const InsertStatement &stmt);
    SelectResult select(const SelectStatement &stmt) const;

private:
    void ensureDataDirectory();
    void loadExistingTables();
    void loadTableFromFile(const std::string &path, const std::string &tableName);
    void saveTable(const std::string &tableName);
    std::string normalizeName(const std::string &name) const;
    std::vector<std::string> parseCsvLine(const std::string &line) const;
    std::string toCsvLine(const std::vector<std::string> &values) const;

    std::unordered_map<std::string, Table> tables_;
};

#endif // STORAGE_H
