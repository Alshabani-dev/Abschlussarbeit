#include "Storage.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "Utils.h"

namespace fs = std::filesystem;

Storage::Storage() : dataDir_("data") {
    loadAllTables();
}

Storage::~Storage() {
    for (const auto &pair : tables_) {
        saveTable(pair.first);
    }
}

bool Storage::createTable(const std::string &name, const std::vector<std::string> &columns) {
    if (columns.empty()) {
        setError("Cannot create table with zero columns");
        return false;
    }
    std::string normalized = normalizeName(name);
    if (tables_.count(normalized)) {
        setError("Table already exists: " + name);
        return false;
    }
    Table table;
    table.columns = columns;
    tables_[normalized] = table;
    return saveTable(normalized);
}

bool Storage::insertRow(const std::string &tableName, const std::vector<std::string> &values) {
    std::string normalized = normalizeName(tableName);
    auto it = tables_.find(normalized);
    if (it == tables_.end()) {
        setError("Table does not exist: " + tableName);
        return false;
    }
    if (values.size() != it->second.columns.size()) {
        setError("Expected " + std::to_string(it->second.columns.size()) +
                 " values but got " + std::to_string(values.size()));
        return false;
    }
    Row row;
    row.values = values;
    it->second.rows.push_back(row);
    return saveTable(normalized);
}

const Table *Storage::getTable(const std::string &name) const {
    std::string normalized = normalizeName(name);
    auto it = tables_.find(normalized);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::string Storage::getLastError() const {
    return lastError_;
}

std::string Storage::normalizeName(const std::string &name) const {
    return Utils::toLower(name);
}

void Storage::setError(const std::string &error) {
    lastError_ = error;
}

void Storage::loadAllTables() {
    try {
        if (!fs::exists(dataDir_)) {
            fs::create_directories(dataDir_);
        }
        for (const auto &entry : fs::directory_iterator(dataDir_)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            if (entry.path().extension() != ".csv") {
                continue;
            }
            std::ifstream input(entry.path());
            if (!input.is_open()) {
                setError("Failed to open " + entry.path().string());
                continue;
            }
            std::string header;
            if (!std::getline(input, header)) {
                continue;
            }
            auto columns = Utils::parseCsvLine(header);
            if (columns.empty()) {
                continue;
            }
            Table table;
            table.columns = columns;
            std::string line;
            while (std::getline(input, line)) {
                if (line.empty()) {
                    continue;
                }
                auto values = Utils::parseCsvLine(line);
                if (values.size() != columns.size()) {
                    continue;
                }
                Row row;
                row.values = values;
                table.rows.push_back(row);
            }
            tables_[normalizeName(entry.path().stem().string())] = table;
        }
    } catch (const std::exception &ex) {
        setError("Failed to load tables: " + std::string(ex.what()));
    }
}

bool Storage::saveTable(const std::string &normalizedName) {
    auto it = tables_.find(normalizedName);
    if (it == tables_.end()) {
        setError("Table not found: " + normalizedName);
        return false;
    }
    try {
        if (!fs::exists(dataDir_)) {
            fs::create_directories(dataDir_);
        }
        fs::path filePath = fs::path(dataDir_) / (normalizedName + ".csv");
        std::ofstream output(filePath, std::ios::trunc);
        if (!output.is_open()) {
            setError("Failed to open file for writing: " + filePath.string());
            return false;
        }
        const Table &table = it->second;
        std::vector<std::string> escapedColumns;
        escapedColumns.reserve(table.columns.size());
        for (const auto &col : table.columns) {
            escapedColumns.push_back(Utils::csvEscape(col));
        }
        output << Utils::join(escapedColumns, ",") << '\n';
        for (const auto &row : table.rows) {
            std::vector<std::string> escapedValues;
            escapedValues.reserve(row.values.size());
            for (const auto &value : row.values) {
                escapedValues.push_back(Utils::csvEscape(value));
            }
            output << Utils::join(escapedValues, ",") << '\n';
        }
    } catch (const std::exception &ex) {
        setError("Failed to save table: " + std::string(ex.what()));
        return false;
    }
    return true;
}
