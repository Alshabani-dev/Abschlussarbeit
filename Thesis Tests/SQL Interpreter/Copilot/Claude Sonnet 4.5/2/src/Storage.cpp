#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

Storage::Storage() : dataDir_("data") {
    // Create data directory if it doesn't exist
    struct stat st;
    if (stat(dataDir_.c_str(), &st) != 0) {
        mkdir(dataDir_.c_str(), 0755);
    }
    
    loadAllTables();
}

Storage::~Storage() {
    // Save all tables on shutdown
    for (const auto& pair : tables_) {
        saveTable(pair.first);
    }
}

std::string Storage::normalizeTableName(const std::string &name) const {
    return Utils::toLower(name);
}

bool Storage::createTable(const std::string &name, const std::vector<std::string> &columns) {
    std::string normName = normalizeTableName(name);
    
    if (tables_.find(normName) != tables_.end()) {
        lastError_ = "Table '" + name + "' already exists";
        return false;
    }
    
    if (columns.empty()) {
        lastError_ = "Table must have at least one column";
        return false;
    }
    
    Table table;
    table.columns = columns;
    tables_[normName] = table;
    
    // Save immediately
    if (!saveTable(normName)) {
        tables_.erase(normName);
        return false;
    }
    
    return true;
}

bool Storage::insertRow(const std::string &tableName, const std::vector<std::string> &values) {
    std::string normName = normalizeTableName(tableName);
    
    auto it = tables_.find(normName);
    if (it == tables_.end()) {
        lastError_ = "Table '" + tableName + "' does not exist";
        return false;
    }
    
    if (values.size() != it->second.columns.size()) {
        lastError_ = "Column count mismatch: expected " + 
                    std::to_string(it->second.columns.size()) + 
                    ", got " + std::to_string(values.size());
        return false;
    }
    
    Row row;
    row.values = values;
    it->second.rows.push_back(row);
    
    // Save immediately
    return saveTable(normName);
}

const Table* Storage::getTable(const std::string &name) const {
    std::string normName = normalizeTableName(name);
    auto it = tables_.find(normName);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::string Storage::getLastError() const {
    return lastError_;
}

void Storage::loadAllTables() {
    DIR* dir = opendir(dataDir_.c_str());
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        
        // Check if file ends with .csv
        if (filename.length() > 4 && 
            filename.substr(filename.length() - 4) == ".csv") {
            
            std::string tableName = filename.substr(0, filename.length() - 4);
            std::string filepath = dataDir_ + "/" + filename;
            
            std::ifstream file(filepath);
            if (!file.is_open()) continue;
            
            Table table;
            std::string line;
            
            // Read header (column names)
            if (std::getline(file, line)) {
                std::vector<std::string> cols = Utils::split(line, ',');
                for (auto& col : cols) {
                    table.columns.push_back(Utils::unescapeCsv(Utils::trim(col)));
                }
                
                // Read data rows
                while (std::getline(file, line)) {
                    if (line.empty()) continue;
                    
                    std::vector<std::string> values = Utils::split(line, ',');
                    if (values.size() == table.columns.size()) {
                        Row row;
                        for (auto& val : values) {
                            row.values.push_back(Utils::unescapeCsv(Utils::trim(val)));
                        }
                        table.rows.push_back(row);
                    }
                }
                
                tables_[tableName] = table;
            }
            
            file.close();
        }
    }
    
    closedir(dir);
}

bool Storage::saveTable(const std::string &tableName) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table '" + tableName + "' not found";
        return false;
    }
    
    std::string filepath = dataDir_ + "/" + tableName + ".csv";
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        lastError_ = "Failed to open file: " + filepath;
        return false;
    }
    
    const Table& table = it->second;
    
    // Write header
    for (size_t i = 0; i < table.columns.size(); ++i) {
        if (i > 0) file << ",";
        file << Utils::escapeCsv(table.columns[i]);
    }
    file << "\n";
    
    // Write rows
    for (const auto& row : table.rows) {
        for (size_t i = 0; i < row.values.size(); ++i) {
            if (i > 0) file << ",";
            file << Utils::escapeCsv(row.values[i]);
        }
        file << "\n";
    }
    
    file.close();
    return true;
}
