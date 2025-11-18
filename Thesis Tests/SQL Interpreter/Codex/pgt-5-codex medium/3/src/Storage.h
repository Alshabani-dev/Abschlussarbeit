#ifndef STORAGE_H
#define STORAGE_H

#include <optional>
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

    std::string createTable(const std::string &tableName, const std::vector<std::string> &columns);
    std::string insertRow(const std::string &tableName, const std::vector<std::string> &values);
    QueryResult selectRows(const std::string &tableName,
                           const std::vector<std::string> &columns,
                           bool selectAll,
                           const std::optional<std::pair<std::string, std::string>> &whereClause) const;

private:
    std::string normalize(const std::string &value) const;
    std::string resolveTableFile(const std::string &normalizedName) const;
    void ensureDataDirectory();
    void loadExistingTables();
    void saveTable(const std::string &normalizedName) const;

    std::string dataDirectory_;
    std::unordered_map<std::string, Table> tables_;
};

#endif // STORAGE_H
