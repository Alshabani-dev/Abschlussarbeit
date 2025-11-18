#include "Storage.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "Utils.h"

namespace fs = std::filesystem;

Storage::Storage() : dataDir_("data") {
    ensureDataDir();
    loadAllTables();
}

Storage::~Storage() {
    for (const auto &entry : tables_) {
        saveTable(entry.first);
    }
}

bool Storage::ensureDataDir() {
    std::error_code ec;
    if (!fs::exists(dataDir_, ec)) {
        if (!fs::create_directories(dataDir_, ec)) {
            lastError_ = "Failed to create data directory: " + ec.message();
            return false;
        }
    }
    return true;
}

void Storage::loadAllTables() {
    std::error_code ec;
    if (!fs::exists(dataDir_, ec)) {
        return;
    }
    for (const auto &entry : fs::directory_iterator(dataDir_, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            loadTableFromFile(entry.path().string());
        }
    }
}

std::string Storage::tableNameFromPath(const std::string &path) const {
    fs::path p(path);
    return Utils::toLower(p.stem().string());
}

void Storage::loadTableFromFile(const std::string &path) {
    std::ifstream in(path);
    if (!in) {
        return;
    }
    std::string header;
    if (!std::getline(in, header)) {
        return;
    }
    auto columnsRaw = Utils::parseCsvLine(header);
    if (columnsRaw.empty()) {
        return;
    }
    Table table;
    for (auto &col : columnsRaw) {
        table.columns.push_back(Utils::toLower(Utils::trim(col)));
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        Row row;
        row.values = Utils::parseCsvLine(line);
        table.rows.push_back(std::move(row));
    }
    std::string name = tableNameFromPath(path);
    tables_[name] = std::move(table);
}

bool Storage::createTable(const std::string &name, const std::vector<std::string> &columns) {
    if (columns.empty()) {
        lastError_ = "Table must have at least one column";
        return false;
    }
    std::string normalizedName = Utils::toLower(name);
    if (tables_.count(normalizedName) > 0) {
        lastError_ = "Table already exists";
        return false;
    }
    Table table;
    for (const auto &column : columns) {
        table.columns.push_back(Utils::toLower(column));
    }
    tables_[normalizedName] = table;
    if (!saveTable(normalizedName)) {
        tables_.erase(normalizedName);
        return false;
    }
    return true;
}

bool Storage::insertRow(const std::string &tableName, const std::vector<std::string> &values) {
    std::string normalizedName = Utils::toLower(tableName);
    auto it = tables_.find(normalizedName);
    if (it == tables_.end()) {
        lastError_ = "Table does not exist";
        return false;
    }
    Table &table = it->second;
    if (values.size() != table.columns.size()) {
        lastError_ = "Column/value count mismatch";
        return false;
    }
    Row row;
    row.values = values;
    table.rows.push_back(row);
    if (!saveTable(normalizedName)) {
        table.rows.pop_back();
        return false;
    }
    return true;
}

const Table *Storage::getTable(const std::string &name) const {
    std::string normalized = Utils::toLower(name);
    auto it = tables_.find(normalized);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

bool Storage::saveTable(const std::string &tableName) {
    if (!ensureDataDir()) {
        return false;
    }
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        lastError_ = "Table not found";
        return false;
    }
    fs::path filePath = fs::path(dataDir_) / (tableName + ".csv");
    std::ofstream out(filePath);
    if (!out) {
        lastError_ = "Failed to open " + filePath.string() + " for writing";
        return false;
    }
    const Table &table = it->second;
    for (size_t i = 0; i < table.columns.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << Utils::csvEscape(table.columns[i]);
    }
    out << '\n';
    for (const auto &row : table.rows) {
        for (size_t i = 0; i < row.values.size(); ++i) {
            if (i > 0) {
                out << ',';
            }
            out << Utils::csvEscape(row.values[i]);
        }
        out << '\n';
    }
    return true;
}
