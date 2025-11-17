#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <dirent.h>

Storage::Storage() : dataDir_("data") {
    // Create data directory if it doesn't exist
    struct stat st;
    if (stat(dataDir_.c_str(), &st) != 0) {
        #ifdef _WIN32
        _mkdir(dataDir_.c_str());
        #else
        mkdir(dataDir_.c_str(), 0755);
        #endif
    }
    
    loadAllTables();
}

Storage::~Storage() {
    // Save all tables on exit
    for (const auto& pair : tables_) {
        saveTable(pair.first);
    }
}

bool Storage::createTable(const std::string& name, const std::vector<std::string>& columns) {
    std::string lowerName = Utils::toLower(name);
    
    if (tables_.find(lowerName) != tables_.end()) {
        lastError_ = "Table '" + name + "' already exists";
        return false;
    }
    
    if (columns.empty()) {
        lastError_ = "Table must have at least one column";
        return false;
    }
    
    Table table;
    table.columns = columns;
    tables_[lowerName] = table;
    
    // Immediately save to CSV
    return saveTable(lowerName);
}

bool Storage::insertRow(const std::string& tableName, const std::vector<std::string>& values) {
    std::string lowerName = Utils::toLower(tableName);
    
    auto it = tables_.find(lowerName);
    if (it == tables_.end()) {
        lastError_ = "Table '" + tableName + "' does not exist";
        return false;
    }
    
    Table& table = it->second;
    
    if (values.size() != table.columns.size()) {
        lastError_ = "Column count mismatch: expected " + 
                     std::to_string(table.columns.size()) + 
                     ", got " + std::to_string(values.size());
        return false;
    }
    
    Row row;
    row.values = values;
    table.rows.push_back(row);
    
    // Immediately save to CSV
    return saveTable(lowerName);
}

const Table* Storage::getTable(const std::string& name) const {
    std::string lowerName = Utils::toLower(name);
    auto it = tables_.find(lowerName);
    if (it != tables_.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::string Storage::getLastError() const {
    return lastError_;
}

void Storage::loadAllTables() {
    DIR* dir = opendir(dataDir_.c_str());
    if (!dir) {
        return; // Directory doesn't exist or can't be opened
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        
        // Check if it's a CSV file
        if (filename.length() > 4 && 
            filename.substr(filename.length() - 4) == ".csv") {
            loadTable(filename);
        }
    }
    
    closedir(dir);
}

bool Storage::saveTable(const std::string& tableName) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table '" + tableName + "' not found";
        return false;
    }
    
    const Table& table = it->second;
    std::string filename = dataDir_ + "/" + tableName + ".csv";
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        lastError_ = "Failed to open file: " + filename;
        return false;
    }
    
    // Write header
    for (size_t i = 0; i < table.columns.size(); ++i) {
        if (i > 0) file << ",";
        file << Utils::escapeCsv(table.columns[i]);
    }
    file << "\n";
    
    // Write rows
    for (const Row& row : table.rows) {
        for (size_t i = 0; i < row.values.size(); ++i) {
            if (i > 0) file << ",";
            file << Utils::escapeCsv(row.values[i]);
        }
        file << "\n";
    }
    
    file.close();
    return true;
}

bool Storage::loadTable(const std::string& filename) {
    std::string filepath = dataDir_ + "/" + filename;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        return false;
    }
    
    // Extract table name from filename (remove .csv extension)
    std::string tableName = filename.substr(0, filename.length() - 4);
    tableName = Utils::toLower(tableName);
    
    Table table;
    std::string line;
    bool isHeader = true;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::vector<std::string> fields = Utils::parseCsvLine(line);
        
        if (isHeader) {
            table.columns = fields;
            isHeader = false;
        } else {
            Row row;
            row.values = fields;
            table.rows.push_back(row);
        }
    }
    
    file.close();
    
    if (!table.columns.empty()) {
        tables_[tableName] = table;
        return true;
    }
    
    return false;
}
