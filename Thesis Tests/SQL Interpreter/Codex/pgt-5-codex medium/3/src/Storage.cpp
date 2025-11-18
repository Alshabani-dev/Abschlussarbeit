#include "Storage.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <optional>

#include "Utils.h"

namespace {
std::string toLower(const std::string &value) {
    std::string result(value.size(), '\0');
    std::transform(value.begin(), value.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return result;
}
}

Storage::Storage() : dataDirectory_("data") {
    ensureDataDirectory();
    loadExistingTables();
}

std::string Storage::createTable(const std::string &tableName, const std::vector<std::string> &columns) {
    if (columns.empty()) {
        throw std::runtime_error("CREATE TABLE requires columns");
    }
    std::string key = normalize(tableName);
    if (tables_.find(key) != tables_.end()) {
        throw std::runtime_error("Table already exists: " + tableName);
    }
    Table table;
    table.name = tableName;
    table.columns = columns;
    tables_[key] = table;
    saveTable(key);
    return "OK";
}

std::string Storage::insertRow(const std::string &tableName, const std::vector<std::string> &values) {
    std::string key = normalize(tableName);
    auto it = tables_.find(key);
    if (it == tables_.end()) {
        throw std::runtime_error("Unknown table: " + tableName);
    }
    if (it->second.columns.size() != values.size()) {
        throw std::runtime_error("INSERT has mismatched column count");
    }
    Row row;
    row.values = values;
    it->second.rows.push_back(row);
    saveTable(key);
    return "OK";
}

QueryResult Storage::selectRows(const std::string &tableName,
                                const std::vector<std::string> &columns,
                                bool selectAll,
                                const std::optional<std::pair<std::string, std::string>> &whereClause) const {
    std::string key = normalize(tableName);
    auto it = tables_.find(key);
    if (it == tables_.end()) {
        throw std::runtime_error("Unknown table: " + tableName);
    }

    const Table &table = it->second;
    std::vector<size_t> indices;
    QueryResult result;

    auto findColumnIndex = [&](const std::string &name) -> size_t {
        std::string normalizedName = toLower(name);
        for (size_t i = 0; i < table.columns.size(); ++i) {
            if (toLower(table.columns[i]) == normalizedName) {
                return i;
            }
        }
        throw std::runtime_error("Unknown column: " + name);
    };

    if (selectAll) {
        result.columns = table.columns;
        indices.resize(table.columns.size());
        for (size_t i = 0; i < table.columns.size(); ++i) {
            indices[i] = i;
        }
    } else {
        for (const std::string &column : columns) {
            size_t idx = findColumnIndex(column);
            indices.push_back(idx);
            result.columns.push_back(table.columns[idx]);
        }
    }

    std::optional<size_t> whereIndex;
    std::string whereValue;
    if (whereClause) {
        whereIndex = findColumnIndex(whereClause->first);
        whereValue = whereClause->second;
    }

    for (const Row &row : table.rows) {
        if (whereIndex) {
            if (row.values[*whereIndex] != whereValue) {
                continue;
            }
        }
        Row outputRow;
        for (size_t idx : indices) {
            outputRow.values.push_back(row.values[idx]);
        }
        result.rows.push_back(std::move(outputRow));
    }

    return result;
}

std::string Storage::normalize(const std::string &value) const {
    return toLower(value);
}

std::string Storage::resolveTableFile(const std::string &normalizedName) const {
    return dataDirectory_ + "/" + normalizedName + ".csv";
}

void Storage::ensureDataDirectory() {
    namespace fs = std::filesystem;
    fs::path path(dataDirectory_);
    if (!fs::exists(path)) {
        fs::create_directories(path);
    }
}

void Storage::loadExistingTables() {
    namespace fs = std::filesystem;
    fs::path dataPath(dataDirectory_);
    if (!fs::exists(dataPath)) {
        return;
    }

    for (const auto &entry : fs::directory_iterator(dataPath)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".csv") {
            continue;
        }
        std::ifstream stream(entry.path());
        if (!stream.is_open()) {
            continue;
        }
        std::string header;
        if (!std::getline(stream, header)) {
            continue;
        }
        Table table;
        table.name = entry.path().stem().string();
        table.columns = Utils::parseCsvLine(header);
        std::string line;
        while (std::getline(stream, line)) {
            Row row;
            row.values = Utils::parseCsvLine(line);
            if (row.values.size() == table.columns.size()) {
                table.rows.push_back(std::move(row));
            }
        }
        tables_[normalize(table.name)] = std::move(table);
    }
}

void Storage::saveTable(const std::string &normalizedName) const {
    auto it = tables_.find(normalizedName);
    if (it == tables_.end()) {
        return;
    }
    const Table &table = it->second;
    std::ofstream stream(resolveTableFile(normalizedName));
    if (!stream.is_open()) {
        throw std::runtime_error("Unable to persist table: " + table.name);
    }
    stream << Utils::buildCsvLine(table.columns) << '\n';
    for (const Row &row : table.rows) {
        stream << Utils::buildCsvLine(row.values) << '\n';
    }
}
