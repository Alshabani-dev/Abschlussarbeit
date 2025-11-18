#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>

Storage::Storage() : dataDir_("data") {
    ensureDataDirectory();
    loadAllTables();
}

Storage::~Storage() {
    // Save all tables on destruction
    for (const auto& pair : tables_) {
        saveTable(pair.first);
    }
}

void Storage::ensureDataDirectory() {
    struct stat info;
    if (stat(dataDir_.c_str(), &info) != 0) {
        // Directory doesn't exist, create it
        mkdir(dataDir_.c_str(), 0755);
    }
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

bool Storage::loadTable(const std::string& filename) {
    std::string filepath = dataDir_ + "/" + filename;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        return false;
    }
    
    // Extract table name from filename (remove .csv)
    std::string tableName = filename.substr(0, filename.length() - 4);
    tableName = Utils::toLower(tableName);
    
    Table table;
    std::string line;
    
    // Read header (column names)
    if (std::getline(file, line)) {
        std::vector<std::string> columns = Utils::split(line, ',');
        for (const auto& col : columns) {
            table.columns.push_back(Utils::unescapeCsv(Utils::trim(col)));
        }
    }
    
    // Read data rows
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        Row row;
        std::vector<std::string> values = Utils::split(line, ',');
        for (const auto& val : values) {
            row.values.push_back(Utils::unescapeCsv(Utils::trim(val)));
        }
        
        if (row.values.size() == table.columns.size()) {
            table.rows.push_back(row);
        }
    }
    
    file.close();
    tables_[tableName] = table;
    return true;
}

bool Storage::saveTable(const std::string& tableName) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table not found: " + tableName;
        return false;
    }
    
    std::string filepath = dataDir_ + "/" + tableName + ".csv";
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        lastError_ = "Could not open file for writing: " + filepath;
        return false;
    }
    
    const Table& table = it->second;
    
    // Write header (column names)
    for (size_t i = 0; i < table.columns.size(); ++i) {
        if (i > 0) file << ",";
        file << Utils::escapeCsv(table.columns[i]);
    }
    file << "\n";
    
    // Write data rows
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

bool Storage::createTable(const std::string& name, const std::vector<std::string>& columns) {
    std::string lowerName = Utils::toLower(name);
    
    if (tables_.find(lowerName) != tables_.end()) {
        lastError_ = "Table already exists: " + name;
        return false;
    }
    
    if (columns.empty()) {
        lastError_ = "Cannot create table with no columns";
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
        lastError_ = "Table not found: " + tableName;
        return false;
    }
    
    Table& table = it->second;
    
    if (values.size() != table.columns.size()) {
        lastError_ = "Column count mismatch. Expected " + 
                     std::to_string(table.columns.size()) + 
                     " but got " + std::to_string(values.size());
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
    
    if (it == tables_.end()) {
        return nullptr;
    }
    
    return &(it->second);
}

std::string Storage::getLastError() const {
    return lastError_;
}
