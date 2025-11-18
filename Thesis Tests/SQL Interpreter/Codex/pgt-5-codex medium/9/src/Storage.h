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

class Storage {
public:
    Storage();
    ~Storage();

    bool createTable(const std::string &name, const std::vector<std::string> &columns);
    bool insertRow(const std::string &tableName, const std::vector<std::string> &values);
    const Table *getTable(const std::string &name) const;

    std::string getLastError() const;

private:
    std::unordered_map<std::string, Table> tables_;
    std::string lastError_;
    std::string dataDir_;

    void loadAllTables();
    bool loadTableFromFile(const std::string &tableName, const std::string &filePath);
    bool saveTable(const std::string &tableName);
    std::string normalize(const std::string &name) const;
    bool ensureDataDir();
    static std::vector<std::string> parseCsvLine(const std::string &line);
    static std::string escapeCsv(const std::string &value);
};

#endif // STORAGE_H
