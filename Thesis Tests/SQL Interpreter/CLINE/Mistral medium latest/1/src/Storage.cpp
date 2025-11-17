#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

Storage::Storage() : dataDir_("data/") {
    // Create data directory if it doesn't exist
    if (!std::filesystem::exists(dataDir_)) {
        std::filesystem::create_directory(dataDir_);
    }
    loadAllTables();
}

Storage::~Storage() {
    for (const auto& pair : tables_) {
        saveTable(pair.first);
    }
}

void Storage::loadAllTables() {
    for (const auto& entry : std::filesystem::directory_iterator(dataDir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            std::string tableName = entry.path().stem().string();
            std::string normalizedName = Utils::toLower(tableName);
            std::ifstream file(entry.path());

            if (!file.is_open()) {
                lastError_ = "Failed to open file: " + entry.path().string();
                continue;
            }

            Table table;
            std::string line;

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
                row.values = Utils::split(line, ',');
                for (auto& val : row.values) {
                    val = Utils::unescapeCsv(Utils::trim(val));
                }
                table.rows.push_back(row);
            }

            tables_[normalizedName] = table;
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
        file << Utils::escapeCsv(table.columns[i]);
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
    std::string normalizedName = Utils::toLower(name);
    if (tables_.find(normalizedName) != tables_.end()) {
        lastError_ = "Table already exists: " + name;
        return false;
    }

    Table table;
    table.columns = columns;
    tables_[normalizedName] = table;

    return saveTable(normalizedName);
}

bool Storage::insertRow(const std::string &tableName, const std::vector<std::string> &values) {
    std::string normalizedName = Utils::toLower(tableName);
    if (tables_.find(normalizedName) == tables_.end()) {
        lastError_ = "Table not found: " + tableName;
        return false;
    }

    Table& table = tables_[normalizedName];
    if (values.size() != table.columns.size()) {
        lastError_ = "Column count mismatch for table: " + tableName;
        return false;
    }

    Row row;
    row.values = values;
    table.rows.push_back(row);

    return saveTable(normalizedName);
}

const Table* Storage::getTable(const std::string &name) const {
    std::string normalizedName = Utils::toLower(name);
    auto it = tables_.find(normalizedName);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &(it->second);
}

std::string Storage::getLastError() const {
    return lastError_;
}
