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
    Storage();  // Loads existing tables from data/ directory
    ~Storage(); // Saves all tables to data/ directory
    
    bool createTable(const std::string &name, const std::vector<std::string> &columns);
    bool insertRow(const std::string &tableName, const std::vector<std::string> &values);
    const Table* getTable(const std::string &name) const;
    
    std::string getLastError() const;

private:
    std::unordered_map<std::string, Table> tables_;
    std::string lastError_;
    std::string dataDir_;
    
    void loadAllTables();  // Load CSV files on startup
    bool loadTable(const std::string &filename);  // Load specific CSV file
    bool saveTable(const std::string &tableName);  // Save table to CSV
    void ensureDataDirectory();  // Create data/ directory if needed
};

#endif // STORAGE_H
