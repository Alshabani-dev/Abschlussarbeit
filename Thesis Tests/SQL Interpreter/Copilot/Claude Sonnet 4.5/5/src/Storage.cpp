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
    if (tables_.find(name) != tables_.end()) {
        setError("Table '" + name + "' already exists");
        return false;
    }
    
    if (columns.empty()) {
        setError("Cannot create table with no columns");
        return false;
    }
    
    Table table;
    table.columns = columns;
    tables_[name] = table;
    
    // Immediately save to disk
    return saveTable(name);
}

bool Storage::insertRow(const std::string& tableName, const std::vector<std::string>& values) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        setError("Table '" + tableName + "' does not exist");
        return false;
    }
    
    Table& table = it->second;
    
    if (values.size() != table.columns.size()) {
        setError("Value count (" + std::to_string(values.size()) + 
                 ") does not match column count (" + std::to_string(table.columns.size()) + ")");
        return false;
    }
    
    Row row;
    row.values = values;
    table.rows.push_back(row);
    
    // Immediately save to disk
    return saveTable(tableName);
}

const Table* Storage::getTable(const std::string& name) const {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

void Storage::setError(const std::string& msg) {
    lastError_ = msg;
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
        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".csv") {
            loadTable(filename);
        }
    }
    
    closedir(dir);
}

bool Storage::loadTable(const std::string& filename) {
    std::string filepath = dataDir_ + "/" + filename;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        return false;
    }
    
    // Extract table name from filename (remove .csv)
    std::string tableName = filename.substr(0, filename.length() - 4);
    
    Table table;
    std::string line;
    bool firstLine = true;
    
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        
        // Parse CSV line
        std::vector<std::string> fields;
        std::string field;
        bool inQuotes = false;
        
        for (size_t i = 0; i < line.length(); ++i) {
            char ch = line[i];
            
            if (ch == '"') {
                if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                    // Double quote - add single quote to field
                    field += '"';
                    ++i;
                } else {
                    // Toggle quote state
                    inQuotes = !inQuotes;
                }
            } else if (ch == ',' && !inQuotes) {
                // End of field
                fields.push_back(field);
                field.clear();
            } else {
                field += ch;
            }
        }
        
        // Add last field
        fields.push_back(field);
        
        if (firstLine) {
            // First line is column names
            table.columns = fields;
            firstLine = false;
        } else {
            // Data row
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

bool Storage::saveTable(const std::string& tableName) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        setError("Table '" + tableName + "' not found");
        return false;
    }
    
    const Table& table = it->second;
    std::string filepath = dataDir_ + "/" + tableName + ".csv";
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        setError("Failed to open file for writing: " + filepath);
        return false;
    }
    
    // Write column headers
    for (size_t i = 0; i < table.columns.size(); ++i) {
        if (i > 0) {
            file << ",";
        }
        file << Utils::escapeCsv(table.columns[i]);
    }
    file << "\n";
    
    // Write data rows
    for (const Row& row : table.rows) {
        for (size_t i = 0; i < row.values.size(); ++i) {
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
