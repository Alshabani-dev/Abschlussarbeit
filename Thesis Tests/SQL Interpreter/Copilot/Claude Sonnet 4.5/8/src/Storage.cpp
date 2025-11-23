#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>

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
    lastError_.clear();
    
    if (tables_.find(name) != tables_.end()) {
        lastError_ = "Table '" + name + "' already exists";
        return false;
    }
    
    if (columns.empty()) {
        lastError_ = "Table must have at least one column";
        return false;
    }
    
    Table table;
    table.columns = columns;
    tables_[name] = table;
    
    // Save immediately
    return saveTable(name);
}

bool Storage::insertRow(const std::string& tableName, const std::vector<std::string>& values) {
    lastError_.clear();
    
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table '" + tableName + "' does not exist";
        return false;
    }
    
    if (values.size() != it->second.columns.size()) {
        lastError_ = "Value count mismatch: expected " + 
                     std::to_string(it->second.columns.size()) + 
                     ", got " + std::to_string(values.size());
        return false;
    }
    
    Row row;
    row.values = values;
    it->second.rows.push_back(row);
    
    // Save immediately
    return saveTable(tableName);
}

const Table* Storage::getTable(const std::string& name) const {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

void Storage::loadAllTables() {
    DIR* dir = opendir(dataDir_.c_str());
    if (!dir) {
        return;
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
    bool isHeader = true;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::vector<std::string> fields;
        std::string field;
        bool inQuotes = false;
        
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            
            if (c == '"') {
                if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i; // Skip next quote
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (c == ',' && !inQuotes) {
                fields.push_back(Utils::unescapeCsv(field));
                field.clear();
            } else {
                field += c;
            }
        }
        fields.push_back(Utils::unescapeCsv(field));
        
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
