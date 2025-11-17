#include "Storage.h"
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
    loadAllTables();
}

Storage::~Storage() {
    // Save all tables on destruction
    for (const auto& pair : tables_) {
        saveTable(pair.first);
    }
}

void Storage::loadAllTables() {
    for (const auto& entry : fs::directory_iterator(dataDir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            std::string tableName = entry.path().stem().string();
            std::ifstream file(entry.path());

            if (!file.is_open()) {
                lastError_ = "Failed to open file: " + entry.path().string();
                continue;
            }

            Table table;
            std::string line;

            // Read header (columns)
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
                if (!row.values.empty()) {
                    table.rows.push_back(row);
                }
            }

            tables_[tableName] = table;
        }
    }
}

bool Storage::saveTable(const std::string& tableName) {
    const Table* table = getTable(tableName);
    if (!table) {
        lastError_ = "Table not found: " + tableName;
        return false;
    }

    std::ofstream file(dataDir_ + tableName + ".csv");
    if (!file.is_open()) {
        lastError_ = "Failed to open file for writing: " + dataDir_ + tableName + ".csv";
        return false;
    }

    // Write header (columns)
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

bool Storage::createTable(const std::string& name, const std::vector<std::string>& columns) {
    if (tables_.find(name) != tables_.end()) {
        lastError_ = "Table already exists: " + name;
        return false;
    }

    Table table;
    table.columns = columns;
    tables_[name] = table;

    // Save immediately
    return saveTable(name);
}

bool Storage::insertRow(const std::string& tableName, const std::vector<std::string>& values) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table not found: " + tableName;
        return false;
    }

    Table& table = it->second;
    if (values.size() != table.columns.size()) {
        lastError_ = "Column count mismatch for table: " + tableName;
        return false;
    }

    Row row;
    row.values = values;
    table.rows.push_back(row);

    // Save immediately
    return saveTable(tableName);
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
