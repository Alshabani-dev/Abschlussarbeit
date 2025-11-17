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
    loadAllTables();
}

Storage::~Storage() {
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

            std::string line;
            Table table;

            // Read header (column names)
            if (std::getline(file, line)) {
                table.columns = Utils::split(line, ',');
                for (auto& col : table.columns) {
                    col = Utils::trim(col);
                }
            }

            // Read rows
            while (std::getline(file, line)) {
                Row row;
                std::vector<std::string> values = Utils::split(line, ',');
                for (auto& val : values) {
                    val = Utils::trim(val);
                    row.values.push_back(val);
                }
                if (!values.empty()) {
                    table.rows.push_back(row);
                }
            }

            tables_[tableName] = table;
        }
    }
}

bool Storage::saveTable(const std::string &tableName) {
    std::ofstream file(dataDir_ + tableName + ".csv");
    if (!file.is_open()) {
        lastError_ = "Failed to open file for writing: " + dataDir_ + tableName + ".csv";
        return false;
    }

    const Table& table = tables_.at(tableName);

    // Write header
    for (size_t i = 0; i < table.columns.size(); i++) {
        if (i > 0) file << ",";
        file << table.columns[i];
    }
    file << "\n";

    // Write rows
    for (const auto& row : table.rows) {
        for (size_t i = 0; i < row.values.size(); i++) {
            if (i > 0) file << ",";
            file << Utils::escapeCsv(row.values[i]);
        }
        file << "\n";
    }

    return true;
}

bool Storage::createTable(const std::string &name, const std::vector<std::string> &columns) {
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

bool Storage::insertRow(const std::string &tableName, const std::vector<std::string> &values) {
    if (tables_.find(tableName) == tables_.end()) {
        lastError_ = "Table does not exist: " + tableName;
        return false;
    }

    Table& table = tables_[tableName];
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
