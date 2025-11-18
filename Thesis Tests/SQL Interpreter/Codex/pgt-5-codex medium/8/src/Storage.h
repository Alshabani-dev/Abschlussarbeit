#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <unordered_map>
#include <vector>

struct Row {
    std::vector<std::string> values;
};

struct Table {
    std::string name;
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

    bool createTable(const std::string &name, const std::vector<std::string> &columns, std::string &error);
    bool insertRow(const std::string &name, const std::vector<std::string> &values, std::string &error);
    bool selectRows(const std::string &name, const std::vector<std::string> &columns,
                    const std::string &whereColumn, const std::string &whereValue,
                    bool hasWhere, QueryResult &result, std::string &error) const;

private:
    std::unordered_map<std::string, Table> tables_;
    std::string dataDir_{"data"};

    void loadExistingTables();
    bool loadTableFromFile(const std::string &path, const std::string &tableName);
    bool persistTable(const Table &table, std::string &error) const;
    std::string tablePath(const std::string &name) const;
};

#endif // STORAGE_H
