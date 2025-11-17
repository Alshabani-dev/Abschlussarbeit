#include "Storage.h"
#include "Utils.h"
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
    // Clear existing tables
    tables_.clear();

    // Iterate through CSV files in data directory
    for (const auto& entry : fs::directory_iterator(dataDir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            std::string tableName = entry.path().stem().string();
            std::ifstream file(entry.path());

            if (file.is_open()) {
                Table table;
                std::string line;

                // Read header (column names)
                if (std::getline(file, line)) {
                    table.columns = Utils::split(line, ',');
                }

                // Read rows
                while (std::getline(file, line)) {
                    Row row;
                    row.values = Utils::split(line, ',');
                    table.rows.push_back(row);
                }

                tables_[tableName] = table;
            }
        }
    }
}

bool Storage::saveTable(const std::string &tableName) {
    const Table* table = getTable(tableName);
    if (!table) {
        lastError_ = "Table '" + tableName + "' not found";
        return false;
    }

    std::ofstream file(dataDir_ + tableName + ".csv");
    if (!file.is_open()) {
        lastError_ = "Failed to open file for writing: " + dataDir_ + tableName + ".csv";
        return false;
    }

    // Write header (column names)
    for (size_t i = 0; i < table->columns.size(); ++i) {
        if (i > 0) file << ",";
        file << table->columns[i];
    }
    file << "\n";

    // Write rows
    for (const auto& row : table->rows) {
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

    // Check column count
    if (values.size() != it->second.columns.size()) {
        lastError_ = "Column count mismatch. Expected " +
                     std::to_string(it->second.columns.size()) +
                     " values, got " + std::to_string(values.size());
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
