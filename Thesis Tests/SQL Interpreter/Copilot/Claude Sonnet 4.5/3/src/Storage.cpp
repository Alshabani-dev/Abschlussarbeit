#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <algorithm>

Storage::Storage() : dataDir_("data") {
    // Create data directory if it doesn't exist
    struct stat st;
    if (stat(dataDir_.c_str(), &st) != 0) {
        mkdir(dataDir_.c_str(), 0755);
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
    std::string normalized = normalizeTableName(name);
    
    if (tables_.find(normalized) != tables_.end()) {
        lastError_ = "Table '" + name + "' already exists";
        return false;
    }
    
    if (columns.empty()) {
        lastError_ = "Cannot create table with no columns";
        return false;
    }
    
    Table table;
    table.columns = columns;
    tables_[normalized] = table;
    
    // Immediately save to disk
    if (!saveTable(normalized)) {
        tables_.erase(normalized);
        return false;
    }
    
    lastError_.clear();
    return true;
}

bool Storage::insertRow(const std::string& tableName, const std::vector<std::string>& values) {
    std::string normalized = normalizeTableName(tableName);
    
    auto it = tables_.find(normalized);
    if (it == tables_.end()) {
        lastError_ = "Table '" + tableName + "' does not exist";
        return false;
    }
    
    Table& table = it->second;
    
    if (values.size() != table.columns.size()) {
        std::stringstream ss;
        ss << "Column count mismatch: expected " << table.columns.size() 
           << ", got " << values.size();
        lastError_ = ss.str();
        return false;
    }
    
    Row row;
    row.values = values;
    table.rows.push_back(row);
    
    // Immediately save to disk
    if (!saveTable(normalized)) {
        table.rows.pop_back();
        return false;
    }
    
    lastError_.clear();
    return true;
}

const Table* Storage::getTable(const std::string& name) const {
    std::string normalized = normalizeTableName(name);
    auto it = tables_.find(normalized);
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
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        
        // Skip . and ..
        if (filename == "." || filename == "..") {
            continue;
        }
        
        // Check if it's a .csv file
        if (filename.length() > 4 && 
            filename.substr(filename.length() - 4) == ".csv") {
            
            std::string tableName = filename.substr(0, filename.length() - 4);
            std::string filepath = dataDir_ + "/" + filename;
            
            std::ifstream file(filepath);
            if (!file.is_open()) {
                continue;
            }
            
            Table table;
            std::string line;
            
            // Read header (column names)
            if (std::getline(file, line)) {
                table.columns = Utils::parseCSVLine(line);
            }
            
            // Read data rows
            while (std::getline(file, line)) {
                if (line.empty()) continue;
                
                Row row;
                row.values = Utils::parseCSVLine(line);
                
                if (row.values.size() == table.columns.size()) {
                    table.rows.push_back(row);
                }
            }
            
            file.close();
            tables_[normalizeTableName(tableName)] = table;
        }
    }
    
    closedir(dir);
}

bool Storage::saveTable(const std::string& tableName) {
    std::string normalized = normalizeTableName(tableName);
    auto it = tables_.find(normalized);
    if (it == tables_.end()) {
        lastError_ = "Table '" + tableName + "' not found for saving";
        return false;
    }
    
    const Table& table = it->second;
    std::string filepath = dataDir_ + "/" + normalized + ".csv";
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        lastError_ = "Failed to open file for writing: " + filepath;
        return false;
    }
    
    // Write header
    for (size_t i = 0; i < table.columns.size(); ++i) {
        if (i > 0) file << ",";
        file << Utils::escapeCSV(table.columns[i]);
    }
    file << "\n";
    
    // Write rows
    for (const auto& row : table.rows) {
        for (size_t i = 0; i < row.values.size(); ++i) {
            if (i > 0) file << ",";
            file << Utils::escapeCSV(row.values[i]);
        }
        file << "\n";
    }
    
    file.close();
    return true;
}

std::string Storage::normalizeTableName(const std::string& name) const {
    return Utils::toLower(name);
}
