#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

Storage::Storage() : dataDir_("data/") {
    loadAllTables();
}

Storage::~Storage() {
    for (const auto& pair : tables_) {
        saveTable(pair.first);
    }
}

void Storage::loadAllTables() {
    if (!fs::exists(dataDir_)) {
        fs::create_directory(dataDir_);
        return;
    }

    for (const auto& entry : fs::directory_iterator(dataDir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            std::string tableName = entry.path().stem().string();
            std::ifstream file(entry.path());

            if (file.is_open()) {
                std::string line;
                if (std::getline(file, line)) {
                    Table table;
                    table.columns = Utils::split(line, ',');

                    while (std::getline(file, line)) {
                        Row row;
                        row.values = Utils::split(line, ',');
                        table.rows.push_back(row);
                    }

                    tables_[tableName] = table;
                }
                file.close();
            }
        }
    }
}

bool Storage::saveTable(const std::string& tableName) {
    std::ofstream file(dataDir_ + tableName + ".csv");
    if (!file.is_open()) {
        lastError_ = "Failed to open file for writing: " + dataDir_ + tableName + ".csv";
        return false;
    }

    const Table& table = tables_.at(tableName);

    // Write header
    file << Utils::join(table.columns, ",") << "\n";

    // Write rows
    for (const auto& row : table.rows) {
        file << Utils::join(row.values, ",") << "\n";
    }

    file.close();
    return true;
}

bool Storage::createTable(const std::string& name, const std::vector<std::string>& columns) {
    if (tables_.find(name) != tables_.end()) {
        lastError_ = "Table '" + name + "' already exists";
        return false;
    }

    Table table;
    table.columns = columns;
    tables_[name] = table;

    return saveTable(name);
}

bool Storage::insertRow(const std::string& tableName, const std::vector<std::string>& values) {
    if (tables_.find(tableName) == tables_.end()) {
        lastError_ = "Table '" + tableName + "' does not exist";
        return false;
    }

    Table& table = tables_[tableName];
    if (values.size() != table.columns.size()) {
        lastError_ = "Column count mismatch. Expected " + std::to_string(table.columns.size()) +
                     " values, got " + std::to_string(values.size());
        return false;
    }

    Row row;
    row.values = values;
    table.rows.push_back(row);

    return saveTable(tableName);
}

const Table* Storage::getTable(const std::string& name) const {
    auto it = tables_.find(name);
    if (it != tables_.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::string Storage::getLastError() const {
    return lastError_;
}
