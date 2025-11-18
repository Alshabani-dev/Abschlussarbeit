#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <dirent.h>

Storage::Storage() : dataDir_("data") {
    // Create data directory if it doesn't exist
    mkdir(dataDir_.c_str(), 0755);
    
    // Load existing tables from CSV files
    loadAllTables();
}

Storage::~Storage() {
    // Save all tables to CSV files
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
    
    // Save immediately
    if (!saveTable(name)) {
        tables_.erase(name);
        return false;
    }
    
    return true;
}

bool Storage::insertRow(const std::string& tableName, const std::vector<std::string>& values) {
    // Check if table exists
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table '" + tableName + "' does not exist";
        return false;
    }
    
    // Check if number of values matches number of columns
    if (values.size() != it->second.columns.size()) {
        std::ostringstream oss;
        oss << "Column count mismatch: table has " << it->second.columns.size()
            << " columns, but " << values.size() << " values provided";
        lastError_ = oss.str();
        return false;
    }
    
    // Insert row
    Row row;
    row.values = values;
    it->second.rows.push_back(row);
    
    // Save immediately
    if (!saveTable(tableName)) {
        // Rollback
        it->second.rows.pop_back();
        return false;
    }
    
    return true;
}

const Table* Storage::getTable(const std::string& name) const {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &(it->second);
}

std::string Storage::getLastError() const {
    return lastError_;
}

void Storage::loadAllTables() {
    DIR* dir = opendir(dataDir_.c_str());
    if (!dir) {
        return; // Directory doesn't exist yet
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        
        // Check if it's a CSV file
        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".csv") {
            std::string tableName = filename.substr(0, filename.length() - 4);
            std::string filepath = getTableFilePath(tableName);
            
            std::ifstream file(filepath);
            if (!file.is_open()) {
                continue;
            }
            
            Table table;
            std::string line;
            
            // Read header (column names)
            if (std::getline(file, line)) {
                table.columns = Utils::parseCsvLine(line);
                
                // Unescape column names
                for (auto& col : table.columns) {
                    col = Utils::unescapeCsv(col);
                }
            }
            
            // Read data rows
            while (std::getline(file, line)) {
                if (line.empty()) {
                    continue;
                }
                
                std::vector<std::string> fields = Utils::parseCsvLine(line);
                
                // Unescape fields
                for (auto& field : fields) {
                    field = Utils::unescapeCsv(field);
                }
                
                Row row;
                row.values = fields;
                table.rows.push_back(row);
            }
            
            file.close();
            tables_[tableName] = table;
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
    std::string filepath = getTableFilePath(tableName);
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        lastError_ = "Failed to open file: " + filepath;
        return false;
    }
    
    // Write header (column names)
    for (size_t i = 0; i < table.columns.size(); i++) {
        if (i > 0) {
            file << ",";
        }
        file << Utils::escapeCsv(table.columns[i]);
    }
    file << "\n";
    
    // Write data rows
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

std::string Storage::getTableFilePath(const std::string& tableName) const {
    return dataDir_ + "/" + tableName + ".csv";
}
