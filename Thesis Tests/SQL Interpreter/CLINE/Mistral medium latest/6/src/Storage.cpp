#include "Storage.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

Storage::Storage() : dataDir_("data/") {
    // Create data directory if it doesn't exist
    if (!fs::exists(dataDir_)) {
        fs::create_directory(dataDir_);
    }

    // Load existing tables
    loadAllTables();
}

Storage::~Storage() {
    // Save all tables on destruction
    for (const auto& pair : tables_) {
        saveTable(pair.first);
    }
}

void Storage::loadAllTables() {
    // Iterate through all CSV files in data directory
    for (const auto& entry : fs::directory_iterator(dataDir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            std::string tableName = entry.path().stem().string();
            std::ifstream file(entry.path());

            if (file.is_open()) {
                std::string line;
                Table table;

                // Read header (column names)
                if (std::getline(file, line)) {
                    std::stringstream ss(line);
                    std::string column;
                    while (std::getline(ss, column, ',')) {
                        table.columns.push_back(column);
                    }
                }

                // Read rows
                while (std::getline(file, line)) {
                    Row row;
                    std::stringstream ss(line);
                    std::string value;
                    while (std::getline(ss, value, ',')) {
                        row.values.push_back(value);
                    }
                    table.rows.push_back(row);
                }

                tables_[tableName] = table;
            }
        }
    }
}

bool Storage::saveTable(const std::string &tableName) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table not found: " + tableName;
        return false;
    }

    std::ofstream file(dataDir_ + tableName + ".csv");
    if (!file.is_open()) {
        lastError_ = "Failed to open file for writing: " + dataDir_ + tableName + ".csv";
        return false;
    }

    // Write header (column names)
    for (size_t i = 0; i < it->second.columns.size(); ++i) {
        if (i > 0) file << ",";
        file << it->second.columns[i];
    }
    file << "\n";

    // Write rows
    for (const auto& row : it->second.rows) {
        for (size_t i = 0; i < row.values.size(); ++i) {
            if (i > 0) file << ",";
            file << row.values[i];
        }
        file << "\n";
    }

    return true;
}

bool Storage::createTable(const std::string &name, const std::vector<std::string> &columns) {
    // Check if table already exists
    if (tables_.find(name) != tables_.end()) {
        lastError_ = "Table already exists: " + name;
        return false;
    }

    // Create new table
    Table table;
    table.columns = columns;
    tables_[name] = table;

    // Save to disk
    return saveTable(name);
}

bool Storage::insertRow(const std::string &tableName, const std::vector<std::string> &values) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table not found: " + tableName;
        return false;
    }

    // Check column count matches
    if (values.size() != it->second.columns.size()) {
        lastError_ = "Column count mismatch for table: " + tableName;
        return false;
    }

    // Add row
    Row row;
    row.values = values;
    it->second.rows.push_back(row);

    // Save to disk
    return saveTable(tableName);
}

const Table* Storage::getTable(const std::string &name) const {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &(it->second);
}

std::string Storage::getLastError() const {
    return lastError_;
}
