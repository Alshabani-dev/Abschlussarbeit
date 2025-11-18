#include "Storage.h"

#include <filesystem>
#include <fstream>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "Utils.h"

#ifndef DATA_DIRECTORY
#define DATA_DIRECTORY "./data"
#endif

namespace {
std::string tableFilePath(const std::string &tableName) {
    std::filesystem::path base(DATA_DIRECTORY);
    base /= tableName + ".csv";
    return base.string();
}
} // namespace

Storage::Storage() {
    ensureDataDirectory();
    loadExistingTables();
}

void Storage::ensureDataDirectory() {
    std::filesystem::create_directories(DATA_DIRECTORY);
}

void Storage::loadExistingTables() {
    namespace fs = std::filesystem;
    for (const auto &entry : fs::directory_iterator(DATA_DIRECTORY)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".csv") {
            continue;
        }
        loadTableFromFile(entry.path().string(), entry.path().stem().string());
    }
}

void Storage::loadTableFromFile(const std::string &path, const std::string &tableName) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return;
    }
    std::string line;
    if (!std::getline(file, line)) {
        return;
    }
    Table table;
    table.columns = parseCsvLine(line);
    while (std::getline(file, line)) {
        auto values = parseCsvLine(line);
        if (!values.empty()) {
            Row row;
            row.values = std::move(values);
            table.rows.push_back(std::move(row));
        }
    }
    tables_[normalizeName(tableName)] = std::move(table);
}

void Storage::createTable(const CreateTableStatement &stmt) {
    if (stmt.columns.empty()) {
        throw std::runtime_error("Table must have at least one column");
    }
    std::string name = normalizeName(stmt.tableName);
    if (tables_.find(name) != tables_.end()) {
        throw std::runtime_error("Table already exists: " + stmt.tableName);
    }
    Table table;
    table.columns = stmt.columns;
    tables_[name] = table;
    saveTable(name);
}

void Storage::insertRow(const InsertStatement &stmt) {
    std::string name = normalizeName(stmt.tableName);
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        throw std::runtime_error("Unknown table: " + stmt.tableName);
    }
    Table &table = it->second;
    if (table.columns.size() != stmt.values.size()) {
        throw std::runtime_error("Column/value count mismatch");
    }
    Row row;
    row.values = stmt.values;
    table.rows.push_back(row);
    saveTable(name);
}

SelectResult Storage::select(const SelectStatement &stmt) const {
    std::string name = normalizeName(stmt.tableName);
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        throw std::runtime_error("Unknown table: " + stmt.tableName);
    }
    const Table &table = it->second;
    if (table.columns.empty()) {
        return {};
    }
    std::unordered_map<std::string, std::size_t> columnIndex;
    for (std::size_t i = 0; i < table.columns.size(); ++i) {
        columnIndex[utils::toLower(table.columns[i])] = i;
    }

    std::vector<std::size_t> selectedIndexes;
    SelectResult result;

    if (stmt.selectAll || stmt.columns.empty()) {
        result.columns = table.columns;
        selectedIndexes.resize(table.columns.size());
        std::iota(selectedIndexes.begin(), selectedIndexes.end(), 0);
    } else {
        for (const auto &columnName : stmt.columns) {
            auto itIdx = columnIndex.find(utils::toLower(columnName));
            if (itIdx == columnIndex.end()) {
                throw std::runtime_error("Unknown column: " + columnName);
            }
            selectedIndexes.push_back(itIdx->second);
            result.columns.push_back(table.columns[itIdx->second]);
        }
    }

    std::optional<std::size_t> whereIndex;
    std::string whereValue;
    if (stmt.where.has_value()) {
        auto itIdx = columnIndex.find(utils::toLower(stmt.where->column));
        if (itIdx == columnIndex.end()) {
            throw std::runtime_error("Unknown column in WHERE: " + stmt.where->column);
        }
        whereIndex = itIdx->second;
        whereValue = stmt.where->value;
    }

    for (const auto &row : table.rows) {
        if (whereIndex.has_value()) {
            if (row.values.size() <= *whereIndex || row.values[*whereIndex] != whereValue) {
                continue;
            }
        }
        Row projected;
        projected.values.reserve(selectedIndexes.size());
        for (auto index : selectedIndexes) {
            if (index >= row.values.size()) {
                projected.values.emplace_back();
            } else {
                projected.values.push_back(row.values[index]);
            }
        }
        result.rows.push_back(std::move(projected));
    }
    return result;
}

void Storage::saveTable(const std::string &tableName) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        return;
    }
    const Table &table = it->second;
    std::ofstream file(tableFilePath(tableName));
    if (!file.is_open()) {
        throw std::runtime_error("Unable to persist table: " + tableName);
    }
    file << toCsvLine(table.columns) << "\n";
    for (const auto &row : table.rows) {
        file << toCsvLine(row.values) << "\n";
    }
}

std::string Storage::normalizeName(const std::string &name) const {
    return utils::toLower(name);
}

std::vector<std::string> Storage::parseCsvLine(const std::string &line) const {
    std::vector<std::string> values;
    std::string current;
    bool inQuotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
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
                values.push_back(current);
                current.clear();
            } else {
                current.push_back(c);
            }
        }
    }
    values.push_back(current);
    return values;
}

std::string Storage::toCsvLine(const std::vector<std::string> &values) const {
    std::ostringstream oss;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            oss << ',';
        }
        const auto &value = values[i];
        bool needsQuoting = value.find_first_of(",""\n") != std::string::npos;
        if (needsQuoting) {
            oss << '"';
            for (char c : value) {
                if (c == '"') {
                    oss << '"';
                }
                oss << c;
            }
            oss << '"';
        } else {
            oss << value;
        }
    }
    return oss.str();
}
