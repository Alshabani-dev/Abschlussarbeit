#include "Storage.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "Utils.h"

namespace fs = std::filesystem;

Storage::Storage() {
    fs::create_directories(dataDir_);
    loadExistingTables();
}

void Storage::loadExistingTables() {
    for (const auto &entry : fs::directory_iterator(dataDir_)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        auto path = entry.path();
        if (path.extension() != ".csv") {
            continue;
        }
        std::string tableName = utils::toLower(path.stem().string());
        loadTableFromFile(path.string(), tableName);
    }
}

bool Storage::loadTableFromFile(const std::string &path, const std::string &tableName) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }
    std::string header;
    if (!std::getline(in, header)) {
        return false;
    }
    Table table;
    table.name = tableName;
    table.columns = utils::parseCsvLine(header);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        Row row;
        row.values = utils::parseCsvLine(line);
        table.rows.push_back(std::move(row));
    }
    tables_[tableName] = std::move(table);
    return true;
}

std::string Storage::tablePath(const std::string &name) const {
    return dataDir_ + "/" + name + ".csv";
}

bool Storage::persistTable(const Table &table, std::string &error) const {
    std::ofstream out(tablePath(table.name), std::ios::trunc);
    if (!out.is_open()) {
        error = "Failed to persist table " + table.name;
        return false;
    }
    for (size_t i = 0; i < table.columns.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << utils::csvEscape(table.columns[i]);
    }
    out << "\n";
    for (const auto &row : table.rows) {
        for (size_t i = 0; i < row.values.size(); ++i) {
            if (i > 0) {
                out << ",";
            }
            out << utils::csvEscape(row.values[i]);
        }
        out << "\n";
    }
    return true;
}

bool Storage::createTable(const std::string &name, const std::vector<std::string> &columns, std::string &error) {
    if (tables_.count(name)) {
        error = "Table already exists";
        return false;
    }
    if (columns.empty()) {
        error = "Column list cannot be empty";
        return false;
    }
    Table table;
    table.name = name;
    table.columns = columns;
    tables_[name] = table;
    return persistTable(tables_[name], error);
}

bool Storage::insertRow(const std::string &name, const std::vector<std::string> &values, std::string &error) {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        error = "Unknown table";
        return false;
    }
    if (values.size() != it->second.columns.size()) {
        error = "Value count does not match column count";
        return false;
    }
    it->second.rows.push_back(Row{values});
    return persistTable(it->second, error);
}

bool Storage::selectRows(const std::string &name, const std::vector<std::string> &columns,
                         const std::string &whereColumn, const std::string &whereValue,
                         bool hasWhere, QueryResult &result, std::string &error) const {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        error = "Unknown table";
        return false;
    }
    const Table &table = it->second;
    std::vector<int> columnIndices;
    if (columns.size() == 1 && columns.front() == "*") {
        result.columns = table.columns;
        columnIndices.resize(table.columns.size());
        for (size_t i = 0; i < table.columns.size(); ++i) {
            columnIndices[i] = static_cast<int>(i);
        }
    } else {
        result.columns = columns;
        columnIndices.reserve(columns.size());
        for (const auto &col : columns) {
            auto iter = std::find(table.columns.begin(), table.columns.end(), col);
            if (iter == table.columns.end()) {
                error = "Unknown column in SELECT: " + col;
                return false;
            }
            columnIndices.push_back(static_cast<int>(std::distance(table.columns.begin(), iter)));
        }
    }

    int whereIndex = -1;
    if (hasWhere) {
        auto iter = std::find(table.columns.begin(), table.columns.end(), whereColumn);
        if (iter == table.columns.end()) {
            error = "Unknown column in WHERE clause";
            return false;
        }
        whereIndex = static_cast<int>(std::distance(table.columns.begin(), iter));
    }

    for (const auto &row : table.rows) {
        if (hasWhere) {
            if (row.values[whereIndex] != whereValue) {
                continue;
            }
        }
        Row projected;
        for (int idx : columnIndices) {
            projected.values.push_back(row.values[idx]);
        }
        result.rows.push_back(std::move(projected));
    }
    return true;
}
