#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

Storage::Storage() : dataDir_("data") {
    // Create data directory if it doesn't exist
    mkdir(dataDir_.c_str(), 0755);
    
    // Load existing tables from CSV files
    loadAllTables();
}

Storage::~Storage() {
    // Save all tables on destruction (backup)
    for (const auto& pair : tables_) {
        saveTable(pair.first);
    }
}

bool Storage::createTable(const std::string& name, const std::vector<std::string>& columns) {
    std::string normalizedName = normalizeTableName(name);
    
    if (tables_.find(normalizedName) != tables_.end()) {
        lastError_ = "Table '" + name + "' already exists";
        return false;
    }
    
    if (columns.empty()) {
        lastError_ = "Table must have at least one column";
        return false;
    }
    
    Table table;
    table.columns = columns;
    tables_[normalizedName] = table;
    
    // Immediately save to CSV
    if (!saveTable(normalizedName)) {
        tables_.erase(normalizedName);
        return false;
    }
    
    return true;
}

bool Storage::insertRow(const std::string& tableName, const std::vector<std::string>& values) {
    std::string normalizedName = normalizeTableName(tableName);
    
    auto it = tables_.find(normalizedName);
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
    
    // Immediately save to CSV
    return saveTable(normalizedName);
}

const Table* Storage::getTable(const std::string& name) const {
    std::string normalizedName = normalizeTableName(name);
    
    auto it = tables_.find(normalizedName);
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
        return;  // Directory doesn't exist yet
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        
        // Check if it's a CSV file
        if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".csv") {
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
    
    std::string filename = dataDir_ + "/" + tableName + ".csv";
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        lastError_ = "Failed to open file: " + filename;
        return false;
    }
    
    const Table& table = it->second;
    
    // Write header
    for (size_t i = 0; i < table.columns.size(); ++i) {
        file << Utils::escapeCsv(table.columns[i]);
        if (i < table.columns.size() - 1) {
            file << ",";
        }
    }
    file << "\n";
    
    // Write rows
    for (const auto& row : table.rows) {
        for (size_t i = 0; i < row.values.size(); ++i) {
            file << Utils::escapeCsv(row.values[i]);
            if (i < row.values.size() - 1) {
                file << ",";
            }
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
    std::string tableName = filename.substr(0, filename.size() - 4);
    
    Table table;
    std::string line;
    
    // Read header
    if (std::getline(file, line)) {
        table.columns = Utils::parseCsvLine(line);
        
        // Unescape column names
        for (auto& col : table.columns) {
            col = Utils::unescapeCsv(col);
        }
    }
    
    // Read rows
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::vector<std::string> values = Utils::parseCsvLine(line);
        
        // Unescape values
        for (auto& val : values) {
            val = Utils::unescapeCsv(val);
        }
        
        Row row;
        row.values = values;
        table.rows.push_back(row);
    }
    
    file.close();
    tables_[tableName] = table;
    
    return true;
}

std::string Storage::normalizeTableName(const std::string& name) const {
    return Utils::toLower(name);
}
