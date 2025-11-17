#include "Storage.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

Storage::Storage() : dataDir_("data/") {
    loadAllTables();
}

Storage::~Storage() {
    // Save all tables to disk
    for (const auto& pair : tables_) {
        saveTable(pair.first);
    }
}

void Storage::loadAllTables() {
    // Create data directory if it doesn't exist
    if (!fs::exists(dataDir_)) {
        fs::create_directory(dataDir_);
    }

    // Load all CSV files from data directory
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
                    std::stringstream ss(line);
                    std::string value;
                    Row row;

                    while (std::getline(ss, value, ',')) {
                        row.values.push_back(value);
                    }

                    if (!row.values.empty()) {
                        table.rows.push_back(row);
                    }
                }

                tables_[tableName] = table;
            }
        }
    }
}

bool Storage::saveTable(const std::string &tableName) {
    std::ofstream file(dataDir_ + tableName + ".csv");
    if (!file.is_open()) {
        lastError_ = "Failed to open file for writing: " + dataDir_ + tableName + ".csv";
        return false;
    }

    const Table& table = tables_[tableName];

    // Write header (column names)
    for (size_t i = 0; i < table.columns.size(); ++i) {
        if (i > 0) file << ",";
        file << table.columns[i];
    }
    file << "\n";

    // Write rows
    for (const auto& row : table.rows) {
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
        lastError_ = "Table '" + name + "' already exists";
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
    // Check if table exists
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table '" + tableName + "' not found";
        return false;
    }

    // Check if number of values matches number of columns
    if (values.size() != it->second.columns.size()) {
        lastError_ = "Number of values does not match number of columns";
        return false;
    }

    // Add row to table
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
