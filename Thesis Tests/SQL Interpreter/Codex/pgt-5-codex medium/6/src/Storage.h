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
    explicit Storage(const std::string &dataDir = "data");

    bool createTable(const std::string &name, const std::vector<std::string> &columns, std::string &error);
    bool insertRow(const std::string &name, const std::vector<std::string> &values, std::string &error);
    bool selectRows(const SelectStatement &statement,
                    std::vector<std::string> &resultColumns,
                    std::vector<Row> &resultRows,
                    std::string &error) const;

private:
    std::string normalize(const std::string &value) const;
    Table *findTable(const std::string &name);
    const Table *findTableConst(const std::string &name) const;
    void loadTables();
    void saveTable(const Table &table) const;
    std::vector<std::string> parseCsvLine(const std::string &line) const;
    std::string buildCsvLine(const std::vector<std::string> &values) const;

    std::string dataDir_;
    std::unordered_map<std::string, Table> tables_;
};

#endif // STORAGE_H
