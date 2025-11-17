#include "Storage.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

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

            if (!file.is_open()) {
                lastError_ = "Failed to open file: " + entry.path().string();
                continue;
            }

            std::string line;
            if (!std::getline(file, line)) {
                continue; // Empty file
            }

            // Parse header (columns)
            std::vector<std::string> columns = Utils::split(line, ',');
            for (auto& col : columns) {
                col = Utils::unescapeCsvField(col);
            }

            // Create table
            tables_[tableName] = Table{columns, {}};

            // Parse rows
            while (std::getline(file, line)) {
                if (line.empty()) continue;

                std::vector<std::string> values = Utils::split(line, ',');
                for (auto& val : values) {
                    val = Utils::unescapeCsvField(val);
                }

                if (values.size() == columns.size()) {
                    tables_[tableName].rows.push_back({values});
                }
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

    // Write header
    file << Utils::join(it->second.columns, ",") << "\n";

    // Write rows
    for (const auto& row : it->second.rows) {
        std::vector<std::string> escapedValues;
        for (const auto& val : row.values) {
            escapedValues.push_back(Utils::escapeCsvField(val));
        }
        file << Utils::join(escapedValues, ",") << "\n";
    }

    return true;
}

bool Storage::createTable(const std::string &name, const std::vector<std::string> &columns) {
    if (tables_.find(name) != tables_.end()) {
        lastError_ = "Table already exists: " + name;
        return false;
    }

    tables_[name] = Table{columns, {}};
    return saveTable(name);
}

bool Storage::insertRow(const std::string &tableName, const std::vector<std::string> &values) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table not found: " + tableName;
        return false;
    }

    if (values.size() != it->second.columns.size()) {
        lastError_ = "Column count mismatch for table: " + tableName;
        return false;
    }

    it->second.rows.push_back({values});
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
