#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

Storage::Storage() : dataDir_("data") {
    // Create data directory if it doesn't exist
    if (!fs::exists(dataDir_)) {
        fs::create_directory(dataDir_);
    }
    
    loadAllTables();
}

Storage::~Storage() {
    // Save all tables on destruction
    for (const auto& pair : tables_) {
        saveTable(pair.first);
    }
}

bool Storage::createTable(const std::string& name, const std::vector<std::string>& columns) {
    // Check if table already exists
    if (tables_.find(name) != tables_.end()) {
        lastError_ = "Table '" + name + "' already exists";
        return false;
    }
    
    // Create new table
    Table table;
    table.columns = columns;
    tables_[name] = table;
    
    // Immediately persist to disk
    if (!saveTable(name)) {
        tables_.erase(name);
        return false;
    }
    
    return true;
}

bool Storage::insertRow(const std::string& tableName, const std::vector<std::string>& values) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table '" + tableName + "' does not exist";
        return false;
    }
    
    Table& table = it->second;
    
    // Validate number of values matches number of columns
    if (values.size() != table.columns.size()) {
        lastError_ = "Column count mismatch: expected " + 
                     std::to_string(table.columns.size()) + 
                     " values, got " + std::to_string(values.size());
        return false;
    }
    
    // Add row
    Row row;
    row.values = values;
    table.rows.push_back(row);
    
    // Immediately persist to disk
    if (!saveTable(tableName)) {
        table.rows.pop_back();
        return false;
    }
    
    return true;
}

const Table* Storage::getTable(const std::string& name) const {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::string Storage::getLastError() const {
    return lastError_;
}

void Storage::loadAllTables() {
    if (!fs::exists(dataDir_)) {
        return;
    }
    
    // Iterate through all CSV files in data directory
    for (const auto& entry : fs::directory_iterator(dataDir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            std::string tableName = entry.path().stem().string();
            
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                continue;
            }
            
            Table table;
            std::string line;
            
            // Read header (column names)
            if (std::getline(file, line)) {
                std::vector<std::string> parts = Utils::split(line, ',');
                for (const auto& col : parts) {
                    table.columns.push_back(Utils::trim(Utils::unescapeCsv(col)));
                }
            }
            
            // Read data rows
            while (std::getline(file, line)) {
                if (line.empty()) {
                    continue;
                }
                
                Row row;
                std::vector<std::string> parts = Utils::split(line, ',');
                for (const auto& val : parts) {
                    row.values.push_back(Utils::trim(Utils::unescapeCsv(val)));
                }
                
                // Only add row if it has the right number of columns
                if (row.values.size() == table.columns.size()) {
                    table.rows.push_back(row);
                }
            }
            
            file.close();
            tables_[tableName] = table;
        }
    }
}

bool Storage::saveTable(const std::string& tableName) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table '" + tableName + "' not found";
        return false;
    }
    
    const Table& table = it->second;
    std::string path = getTablePath(tableName);
    
    std::ofstream file(path);
    if (!file.is_open()) {
        lastError_ = "Failed to open file: " + path;
        return false;
    }
    
    // Write header
    for (size_t i = 0; i < table.columns.size(); i++) {
        if (i > 0) {
            file << ",";
        }
        file << Utils::escapeCsv(table.columns[i]);
    }
    file << "\n";
    
    // Write rows
    for (const auto& row : table.rows) {
        for (size_t i = 0; i < row.values.size(); i++) {
            if (i > 0) {
                file << ",";
            }
            file << Utils::escapeCsv(row.values[i]);
        }
        file << "\n";
    }
    
    file.close();
    return true;
}

std::string Storage::getTablePath(const std::string& tableName) const {
    return dataDir_ + "/" + tableName + ".csv";
}
