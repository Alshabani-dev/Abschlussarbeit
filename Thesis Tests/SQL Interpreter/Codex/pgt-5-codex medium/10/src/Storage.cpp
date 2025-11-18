#include "Storage.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "Utils.h"

namespace fs = std::filesystem;

Storage::Storage() {
    dataPath_ = "data";
    if (!fs::exists(dataPath_)) {
        fs::create_directories(dataPath_);
    }
    loadTables();
}

bool Storage::createTable(const std::string &name, const std::vector<std::string> &columns, std::string &error) {
    std::string normalized = utils::sanitizeIdentifier(name);
    if (tables_.count(normalized) != 0) {
        error = "Table already exists";
        return false;
    }
    if (columns.empty()) {
        error = "Table must have at least one column";
        return false;
    }
    Table table;
    table.columns.reserve(columns.size());
    for (const auto &col : columns) {
        table.columns.push_back(utils::sanitizeIdentifier(col));
    }
    tables_[normalized] = table;
    saveTable(normalized, tables_[normalized]);
    return true;
}

bool Storage::insertInto(const std::string &name, const std::vector<std::string> &values, std::string &error) {
    std::string normalized = utils::sanitizeIdentifier(name);
    auto it = tables_.find(normalized);
    if (it == tables_.end()) {
        error = "Table does not exist";
        return false;
    }
    Table &table = it->second;
    if (table.columns.size() != values.size()) {
        error = "Column count does not match value count";
        return false;
    }
    Row row;
    row.values.reserve(values.size());
    for (const auto &value : values) {
        row.values.push_back(value);
    }
    table.rows.push_back(row);
    saveTable(normalized, table);
    return true;
}

std::optional<SelectResult> Storage::selectFrom(const SelectStatement &stmt, std::string &error) const {
    std::string normalized = utils::sanitizeIdentifier(stmt.tableName);
    auto it = tables_.find(normalized);
    if (it == tables_.end()) {
        error = "Table does not exist";
        return std::nullopt;
    }
    const Table &table = it->second;
    SelectResult result;

    if (stmt.columns.size() == 1 && stmt.columns[0] == "*") {
        result.columns = table.columns;
    } else {
        result.columns.reserve(stmt.columns.size());
        for (const auto &name : stmt.columns) {
            std::string normalizedColumn = utils::sanitizeIdentifier(name);
            bool found = false;
            for (const auto &col : table.columns) {
                if (col == normalizedColumn) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                error = "Unknown column in SELECT list: " + normalizedColumn;
                return std::nullopt;
            }
            result.columns.push_back(normalizedColumn);
        }
    }

    // Determine column indexes for result
    std::vector<size_t> columnIndexes;
    if (result.columns == table.columns) {
        for (size_t i = 0; i < table.columns.size(); ++i) {
            columnIndexes.push_back(i);
        }
    } else {
        for (const auto &col : result.columns) {
            bool found = false;
            for (size_t i = 0; i < table.columns.size(); ++i) {
                if (table.columns[i] == col) {
                    columnIndexes.push_back(i);
                    found = true;
                    break;
                }
            }
            if (!found) {
                error = "Unknown column: " + col;
                return std::nullopt;
            }
        }
    }

    std::optional<size_t> whereColumnIndex;
    std::string whereValue;
    if (stmt.whereClause) {
        std::string normalizedColumn = utils::sanitizeIdentifier(stmt.whereClause->column);
        for (size_t i = 0; i < table.columns.size(); ++i) {
            if (table.columns[i] == normalizedColumn) {
                whereColumnIndex = i;
                break;
            }
        }
        if (!whereColumnIndex) {
            error = "Unknown column in WHERE clause";
            return std::nullopt;
        }
        whereValue = stmt.whereClause->value;
    }

    for (const auto &row : table.rows) {
        if (whereColumnIndex) {
            if (row.values[*whereColumnIndex] != whereValue) {
                continue;
            }
        }
        Row projection;
        for (size_t idx : columnIndexes) {
            projection.values.push_back(row.values[idx]);
        }
        result.rows.push_back(projection);
    }

    return result;
}

void Storage::loadTables() {
    for (const auto &entry : fs::directory_iterator(dataPath_)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".csv") {
            continue;
        }
        std::ifstream input(entry.path());
        if (!input.is_open()) {
            continue;
        }
        std::string line;
        Table table;
        if (!std::getline(input, line)) {
            continue;
        }
        table.columns = parseCsvLine(line);
        for (auto &col : table.columns) {
            col = utils::sanitizeIdentifier(col);
        }
        while (std::getline(input, line)) {
            if (line.empty()) {
                continue;
            }
            Row row;
            row.values = parseCsvLine(line);
            table.rows.push_back(row);
        }
        std::string tableName = entry.path().stem().string();
        tables_[utils::sanitizeIdentifier(tableName)] = table;
    }
}

void Storage::saveTable(const std::string &name, const Table &table) const {
    std::string path = tableFilePath(dataPath_, name);
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        throw std::runtime_error("Failed to open file for table " + name);
    }
    output << toCsvLine(table.columns) << "\n";
    for (const auto &row : table.rows) {
        output << toCsvLine(row.values) << "\n";
    }
}

std::string Storage::tableFilePath(const std::string &dataPath, const std::string &tableName) {
    return (fs::path(dataPath) / (tableName + ".csv")).string();
}

std::vector<std::string> Storage::parseCsvLine(const std::string &line) {
    std::vector<std::string> result;
    std::string value;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (inQuotes) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    value.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                value.push_back(ch);
            }
        } else {
            if (ch == '"') {
                inQuotes = true;
            } else if (ch == ',') {
                result.push_back(value);
                value.clear();
            } else {
                value.push_back(ch);
            }
        }
    }
    result.push_back(value);
    return result;
}

std::string Storage::toCsvLine(const std::vector<std::string> &values) {
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            oss << ',';
        }
        bool needsQuotes = values[i].find_first_of(",\"\n") != std::string::npos;
        if (needsQuotes) {
            oss << '"';
            for (char ch : values[i]) {
                if (ch == '"') {
                    oss << '"';
                }
                oss << ch;
            }
            oss << '"';
        } else {
            oss << values[i];
        }
    }
    return oss.str();
}
