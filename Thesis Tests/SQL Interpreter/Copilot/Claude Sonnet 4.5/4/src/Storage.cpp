#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef _WIN32
#include <dirent.h>
#endif

Storage::Storage() : dataDir_("data") {
    // Create data directory if it doesn't exist
#ifdef _WIN32
    _mkdir(dataDir_.c_str());
#else
    mkdir(dataDir_.c_str(), 0755);
#endif
    
    loadAllTables();
}

Storage::~Storage() {
    // Save all tables on shutdown
    for (const auto& pair : tables_) {
        saveTable(pair.first);
    }
}

bool Storage::createTable(const std::string& name, const std::vector<std::string>& columns) {
    std::string tableName = Utils::toLower(name);
    
    if (tables_.find(tableName) != tables_.end()) {
        lastError_ = "Table '" + tableName + "' already exists";
        return false;
    }
    
    if (columns.empty()) {
        lastError_ = "Table must have at least one column";
        return false;
    }
    
    Table table;
    table.columns = columns;
    tables_[tableName] = table;
    
    // Immediately save to disk
    return saveTable(tableName);
}

bool Storage::insertRow(const std::string& tableName, const std::vector<std::string>& values) {
    std::string lowerTableName = Utils::toLower(tableName);
    
    auto it = tables_.find(lowerTableName);
    if (it == tables_.end()) {
        lastError_ = "Table '" + lowerTableName + "' does not exist";
        return false;
    }
    
    if (values.size() != it->second.columns.size()) {
        lastError_ = "Value count (" + std::to_string(values.size()) + 
                     ") does not match column count (" + 
                     std::to_string(it->second.columns.size()) + ")";
        return false;
    }
    
    Row row;
    row.values = values;
    it->second.rows.push_back(row);
    
    // Immediately save to disk
    return saveTable(lowerTableName);
}

const Table* Storage::getTable(const std::string& name) const {
    std::string lowerName = Utils::toLower(name);
    auto it = tables_.find(lowerName);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

void Storage::loadAllTables() {
    // Try to load all CSV files from data directory
#ifdef _WIN32
    std::string pattern = dataDir_ + "\\*.csv";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                loadTable(findData.cFileName);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    // POSIX implementation
    DIR* dir = opendir(dataDir_.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".csv") {
                loadTable(filename);
            }
        }
        closedir(dir);
    }
#endif
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
    
    // Read header (column names)
    if (std::getline(file, line)) {
        table.columns = parseCSVLine(line);
    } else {
        file.close();
        return false;
    }
    
    // Read data rows
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        Row row;
        row.values = parseCSVLine(line);
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
        lastError_ = "Table '" + tableName + "' not found";
        return false;
    }
    
    std::string filepath = getTableFilePath(tableName);
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        lastError_ = "Failed to open file: " + filepath;
        return false;
    }
    
    const Table& table = it->second;
    
    // Write header
    for (size_t i = 0; i < table.columns.size(); ++i) {
        if (i > 0) file << ",";
        file << Utils::escapeCSV(table.columns[i]);
    }
    file << "\n";
    
    // Write rows
    for (const Row& row : table.rows) {
        for (size_t i = 0; i < row.values.size(); ++i) {
            if (i > 0) file << ",";
            file << Utils::escapeCSV(row.values[i]);
        }
        file << "\n";
    }
    
    file.close();
    return true;
}

std::string Storage::getTableFilePath(const std::string& tableName) const {
    return dataDir_ + "/" + tableName + ".csv";
}

std::vector<std::string> Storage::parseCSVLine(const std::string& line) {
    std::vector<std::string> values;
    std::string current;
    bool inQuotes = false;
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        
        if (c == '"') {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '"') {
                // Double quote - add single quote to value
                current += '"';
                ++i;
            } else {
                // Toggle quote state
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            // End of field
            values.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    
    // Add last field
    values.push_back(current);
    
    return values;
}
