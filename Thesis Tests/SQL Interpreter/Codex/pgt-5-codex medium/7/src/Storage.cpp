#include "Storage.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "Utils.h"

namespace {

const char *kDataDir = "data";

}

Storage::Storage() {
    std::filesystem::create_directories(kDataDir);
    loadFromDisk();
}

bool Storage::createTable(const CreateTableStatement &stmt, std::string &error) {
    if (tables_.count(stmt.tableName) > 0) {
        error = "Table already exists: " + stmt.tableName;
        return false;
    }
    if (stmt.columns.empty()) {
        error = "Table must have at least one column";
        return false;
    }

    Table table;
    table.columns = stmt.columns;
    tables_[stmt.tableName] = table;
    persistTable(stmt.tableName, tables_[stmt.tableName]);
    return true;
}

bool Storage::insertRow(const InsertStatement &stmt, std::string &error) {
    auto it = tables_.find(stmt.tableName);
    if (it == tables_.end()) {
        error = "Unknown table: " + stmt.tableName;
        return false;
    }
    Table &table = it->second;
    if (table.columns.size() != stmt.values.size()) {
        error = "Expected " + std::to_string(table.columns.size()) + " values";
        return false;
    }

    Row row{stmt.values};
    table.rows.push_back(row);
    persistTable(stmt.tableName, table);
    return true;
}

bool Storage::select(const SelectStatement &stmt, QueryResult &result, std::string &error) {
    auto it = tables_.find(stmt.tableName);
    if (it == tables_.end()) {
        error = "Unknown table: " + stmt.tableName;
        return false;
    }

    const Table &table = it->second;
    result.columns.clear();
    result.rows.clear();
    std::vector<int> columnIndexes;
    if (stmt.columns.empty()) {
        for (size_t i = 0; i < table.columns.size(); ++i) {
            columnIndexes.push_back(static_cast<int>(i));
        }
        result.columns = table.columns;
    } else {
        result.columns = stmt.columns;
        for (const std::string &column : stmt.columns) {
            auto pos = std::find(table.columns.begin(), table.columns.end(), column);
            if (pos == table.columns.end()) {
                error = "Unknown column: " + column;
                return false;
            }
            columnIndexes.push_back(static_cast<int>(pos - table.columns.begin()));
        }
    }

    int whereIndex = -1;
    std::string whereValue;
    if (stmt.where.has_value()) {
        const std::string &whereColumn = stmt.where->column;
        auto pos = std::find(table.columns.begin(), table.columns.end(), whereColumn);
        if (pos == table.columns.end()) {
            error = "Unknown column in WHERE: " + whereColumn;
            return false;
        }
        whereIndex = static_cast<int>(pos - table.columns.begin());
        whereValue = stmt.where->value;
    }

    for (const Row &row : table.rows) {
        if (whereIndex >= 0 && row.values[whereIndex] != whereValue) {
            continue;
        }
        Row projected;
        for (int index : columnIndexes) {
            projected.values.push_back(row.values[index]);
        }
        result.rows.push_back(projected);
    }
    return true;
}

void Storage::loadFromDisk() {
    namespace fs = std::filesystem;
    if (!fs::exists(kDataDir)) {
        return;
    }
    for (const auto &entry : fs::directory_iterator(kDataDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        std::string filename = entry.path().filename().string();
        if (entry.path().extension() != ".csv") {
            continue;
        }
        std::string tableName = filename.substr(0, filename.size() - 4);
        loadTable(entry.path().string(), tableName);
    }
}

void Storage::loadTable(const std::string &path, const std::string &name) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return;
    }
    std::string line;
    if (!std::getline(file, line)) {
        return;
    }
    Table table;
    table.columns = splitCsvLine(line);
    for (std::string &column : table.columns) {
        column = toLower(column);
    }
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> values = splitCsvLine(line);
        table.rows.push_back(Row{values});
    }
    tables_[name] = table;
}

void Storage::persistTable(const std::string &name, const Table &table) {
    std::filesystem::create_directories(kDataDir);
    std::string path = std::string(kDataDir) + "/" + name + ".csv";

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "Failed to persist table: " << name << std::endl;
        return;
    }

    std::vector<std::string> escapedColumns;
    for (const std::string &column : table.columns) {
        escapedColumns.push_back(csvEscape(column));
    }
    file << join(escapedColumns, ",") << "\n";

    for (const Row &row : table.rows) {
        std::vector<std::string> escapedValues;
        for (const std::string &value : row.values) {
            escapedValues.push_back(csvEscape(value));
        }
        file << join(escapedValues, ",") << "\n";
    }
}
