#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

Storage::Storage() : dataDir_("data/") {
    loadAllTables();
}

Storage::~Storage() {
    for (const auto &pair : tables_) {
        saveTable(pair.first);
    }
}

void Storage::loadAllTables() {
    if (!std::filesystem::exists(dataDir_)) {
        std::filesystem::create_directory(dataDir_);
        return;
    }

    for (const auto &entry : std::filesystem::directory_iterator(dataDir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            std::string tableName = entry.path().stem().string();
            std::ifstream file(entry.path());

            if (!file.is_open()) {
                continue;
            }

            std::string line;
            if (!std::getline(file, line)) {
                continue;
            }

            Table table;
            table.columns = Utils::split(line, ',');

            while (std::getline(file, line)) {
                Row row;
                row.values = Utils::split(line, ',');
                table.rows.push_back(row);
            }

            tables_[tableName] = table;
        }
    }
}

bool Storage::saveTable(const std::string &tableName) {
    const Table *table = getTable(tableName);
    if (!table) {
        lastError_ = "Table '" + tableName + "' not found";
        return false;
    }

    std::ofstream file(dataDir_ + tableName + ".csv");
    if (!file.is_open()) {
        lastError_ = "Failed to open file for writing: " + dataDir_ + tableName + ".csv";
        return false;
    }

    // Write header
    file << Utils::join(table->columns, ",") << "\n";

    // Write rows
    for (const auto &row : table->rows) {
        std::vector<std::string> escapedValues;
        for (const auto &value : row.values) {
            escapedValues.push_back(Utils::escapeCsv(value));
        }
        file << Utils::join(escapedValues, ",") << "\n";
    }

    return true;
}

bool Storage::createTable(const std::string &name, const std::vector<std::string> &columns) {
    if (tables_.find(name) != tables_.end()) {
        lastError_ = "Table '" + name + "' already exists";
        return false;
    }

    Table table;
    table.columns = columns;
    tables_[name] = table;
    return saveTable(name);
}

bool Storage::insertRow(const std::string &tableName, const std::vector<std::string> &values) {
    Table *table = const_cast<Table*>(getTable(tableName));
    if (!table) {
        lastError_ = "Table '" + tableName + "' not found";
        return false;
    }

    if (values.size() != table->columns.size()) {
        lastError_ = "Column count mismatch. Expected " + std::to_string(table->columns.size()) +
                     " values, got " + std::to_string(values.size());
        return false;
    }

    Row row;
    row.values = values;
    table->rows.push_back(row);
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
