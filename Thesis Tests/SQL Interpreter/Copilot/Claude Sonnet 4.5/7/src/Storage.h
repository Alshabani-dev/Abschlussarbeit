#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <vector>
#include <unordered_map>

struct Row {
    std::vector<std::string> values;
};

struct Table {
    std::vector<std::string> columns;
    std::vector<Row> rows;
};

class Storage {
public:
    Storage();
    ~Storage();
    
    bool createTable(const std::string& name, const std::vector<std::string>& columns);
    bool insertRow(const std::string& tableName, const std::vector<std::string>& values);
    const Table* getTable(const std::string& name) const;
    
    std::string getLastError() const;

private:
    std::unordered_map<std::string, Table> tables_;
    std::string lastError_;
    std::string dataDir_;
    
    void loadAllTables();
    bool saveTable(const std::string& tableName);
    bool loadTable(const std::string& filename);
    std::string normalizeTableName(const std::string& name) const;
};

#endif // STORAGE_H
