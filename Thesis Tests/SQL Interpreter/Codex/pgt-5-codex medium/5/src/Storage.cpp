#include "Storage.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include "Utils.h"

Storage::Storage() {
    loadFromDisk();
}

Table *Storage::findTable(const std::string &name) {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

const Table *Storage::findTable(const std::string &name) const {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

bool Storage::createTable(const std::string &name, const std::vector<std::string> &columns, std::string &error) {
    std::string normalized = Utils::toLower(name);
    if (tables_.count(normalized)) {
        error = "Table already exists";
        return false;
    }
    if (columns.empty()) {
        error = "Table must have at least one column";
        return false;
    }
    Table table;
    table.name = normalized;
    for (const std::string &column : columns) {
        table.columns.push_back(Utils::toLower(column));
    }
    tables_[normalized] = table;
    saveTable(tables_[normalized]);
    return true;
}

bool Storage::insertRow(const std::string &name, const std::vector<std::string> &values, std::string &error) {
    Table *table = findTable(Utils::toLower(name));
    if (!table) {
        error = "Table does not exist";
        return false;
    }
    if (table->columns.size() != values.size()) {
        error = "Value count does not match column count";
        return false;
    }
    table->rows.push_back({values});
    saveTable(*table);
    return true;
}

bool Storage::selectRows(const SelectStmt &stmt, std::vector<std::string> &header, std::vector<Row> &rows, std::string &error) const {
    const Table *table = findTable(Utils::toLower(stmt.tableName));
    if (!table) {
        error = "Table does not exist";
        return false;
    }
    std::vector<int> columnIndexes;
    if (stmt.selectAll) {
        header = table->columns;
        columnIndexes.resize(header.size());
        for (size_t i = 0; i < header.size(); ++i) {
            columnIndexes[i] = static_cast<int>(i);
        }
    } else {
        header = stmt.columns;
        for (const std::string &column : stmt.columns) {
            auto it = std::find(table->columns.begin(), table->columns.end(), column);
            if (it == table->columns.end()) {
                error = "Unknown column '" + column + "'";
                return false;
            }
            columnIndexes.push_back(static_cast<int>(std::distance(table->columns.begin(), it)));
        }
    }

    int whereIndex = -1;
    if (stmt.hasWhere) {
        auto it = std::find(table->columns.begin(), table->columns.end(), stmt.whereColumn);
        if (it == table->columns.end()) {
            error = "Unknown column in WHERE clause";
            return false;
        }
        whereIndex = static_cast<int>(std::distance(table->columns.begin(), it));
    }

    for (const Row &row : table->rows) {
        if (whereIndex >= 0 && row.values[whereIndex] != stmt.whereValue) {
            continue;
        }
        Row projected;
        for (int idx : columnIndexes) {
            projected.values.push_back(row.values[idx]);
        }
        rows.push_back(projected);
    }

    return true;
}

void Storage::ensureDataDirectory() const {
    std::error_code ec;
    std::filesystem::create_directories(dataDirectory_, ec);
}

void Storage::saveTable(const Table &table) const {
    ensureDataDirectory();
    std::filesystem::path path = std::filesystem::path(dataDirectory_) / (table.name + ".csv");
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + path.string());
    }
    for (size_t i = 0; i < table.columns.size(); ++i) {
        out << csvEscape(table.columns[i]);
        if (i + 1 < table.columns.size()) {
            out << ',';
        }
    }
    out << '\n';
    for (const Row &row : table.rows) {
        for (size_t i = 0; i < row.values.size(); ++i) {
            out << csvEscape(row.values[i]);
            if (i + 1 < row.values.size()) {
                out << ',';
            }
        }
        out << '\n';
    }
}

std::string Storage::csvEscape(const std::string &value) {
    bool needsQuotes = value.find_first_of(",\"\n") != std::string::npos;
    if (!needsQuotes) {
        return value;
    }
    std::string escaped = "\"";
    for (char c : value) {
        if (c == '"') {
            escaped += "\"";
        }
        escaped.push_back(c);
    }
    escaped.push_back('"');
    return escaped;
}

std::vector<std::string> Storage::parseCsvLine(const std::string &line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current.push_back(c);
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.push_back(current);
                current.clear();
            } else {
                current.push_back(c);
            }
        }
    }
    fields.push_back(current);
    return fields;
}

void Storage::loadFromDisk() {
    tables_.clear();
    ensureDataDirectory();
    std::filesystem::path dir(dataDirectory_);
    if (!std::filesystem::exists(dir)) {
        return;
    }
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        auto path = entry.path();
        if (path.extension() != ".csv") {
            continue;
        }
        std::ifstream in(path);
        if (!in.is_open()) {
            continue;
        }
        std::string headerLine;
        if (!std::getline(in, headerLine)) {
            continue;
        }
        std::vector<std::string> columns = parseCsvLine(headerLine);
        if (columns.empty()) {
            continue;
        }
        Table table;
        table.name = Utils::toLower(path.stem().string());
        table.columns = columns;
        for (std::string &column : table.columns) {
            column = Utils::toLower(Utils::trim(column));
        }
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) {
                continue;
            }
            Row row;
            row.values = parseCsvLine(line);
            table.rows.push_back(row);
        }
        tables_[table.name] = table;
    }
}

void Storage::saveAll() const {
    for (const auto &pair : tables_) {
        saveTable(pair.second);
    }
}
