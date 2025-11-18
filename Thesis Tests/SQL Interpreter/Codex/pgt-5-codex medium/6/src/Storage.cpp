#include "Storage.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "Utils.h"

Storage::Storage(const std::string &dataDir) : dataDir_(dataDir) {
    std::filesystem::create_directories(dataDir_);
    loadTables();
}

std::string Storage::normalize(const std::string &value) const {
    return Utils::toLower(Utils::trim(value));
}

Table *Storage::findTable(const std::string &name) {
    auto it = tables_.find(normalize(name));
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

const Table *Storage::findTableConst(const std::string &name) const {
    auto it = tables_.find(normalize(name));
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

bool Storage::createTable(const std::string &name, const std::vector<std::string> &columns, std::string &error) {
    if (columns.empty()) {
        error = "Table must have at least one column";
        return false;
    }
    if (findTable(name) != nullptr) {
        error = "Table already exists";
        return false;
    }
    Table table;
    table.name = normalize(name);
    table.columns = columns;
    tables_[table.name] = table;
    saveTable(tables_.at(table.name));
    return true;
}

bool Storage::insertRow(const std::string &name, const std::vector<std::string> &values, std::string &error) {
    Table *table = findTable(name);
    if (!table) {
        error = "Table not found";
        return false;
    }
    if (values.size() != table->columns.size()) {
        error = "Column count does not match values";
        return false;
    }
    Row row;
    row.values = values;
    table->rows.push_back(row);
    saveTable(*table);
    return true;
}

bool Storage::selectRows(const SelectStatement &statement,
                         std::vector<std::string> &resultColumns,
                         std::vector<Row> &resultRows,
                         std::string &error) const {
    const Table *table = findTableConst(statement.tableName);
    if (!table) {
        error = "Table not found";
        return false;
    }

    std::unordered_map<std::string, size_t> columnIndex;
    for (size_t i = 0; i < table->columns.size(); ++i) {
        columnIndex[normalize(table->columns[i])] = i;
    }

    std::vector<size_t> selectedIndexes;
    if (statement.selectAll) {
        for (size_t i = 0; i < table->columns.size(); ++i) {
            selectedIndexes.push_back(i);
        }
        resultColumns = table->columns;
    } else {
        for (const auto &col : statement.columns) {
            auto it = columnIndex.find(normalize(col));
            if (it == columnIndex.end()) {
                error = "Unknown column in SELECT: " + col;
                return false;
            }
            selectedIndexes.push_back(it->second);
            resultColumns.push_back(table->columns[it->second]);
        }
    }

    int whereIndex = -1;
    std::string whereValueNormalized;
    if (statement.hasWhere) {
        auto it = columnIndex.find(normalize(statement.whereColumn));
        if (it == columnIndex.end()) {
            error = "Unknown column in WHERE clause";
            return false;
        }
        whereIndex = static_cast<int>(it->second);
        whereValueNormalized = statement.whereValue;
    }

    for (const Row &row : table->rows) {
        if (whereIndex >= 0) {
            if (row.values[whereIndex] != whereValueNormalized) {
                continue;
            }
        }
        Row projected;
        for (size_t index : selectedIndexes) {
            projected.values.push_back(row.values[index]);
        }
        resultRows.push_back(std::move(projected));
    }

    return true;
}

void Storage::loadTables() {
    for (const auto &entry : std::filesystem::directory_iterator(dataDir_)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() != ".csv") {
            continue;
        }
        std::ifstream file(entry.path());
        if (!file.is_open()) {
            continue;
        }
        std::string header;
        if (!std::getline(file, header)) {
            continue;
        }
        Table table;
        table.name = normalize(entry.path().stem().string());
        table.columns = parseCsvLine(header);
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) {
                continue;
            }
            std::vector<std::string> values = parseCsvLine(line);
            Row row;
            row.values = values;
            table.rows.push_back(row);
        }
        tables_[table.name] = table;
    }
}

std::vector<std::string> Storage::parseCsvLine(const std::string &line) const {
    std::vector<std::string> values;
    std::string value;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    value.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                value.push_back(c);
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                values.push_back(value);
                value.clear();
            } else {
                value.push_back(c);
            }
        }
    }
    values.push_back(value);
    return values;
}

std::string Storage::buildCsvLine(const std::vector<std::string> &values) const {
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            oss << ',';
        }
        bool needsQuotes = values[i].find_first_of(",\"\n") != std::string::npos;
        if (needsQuotes) {
            oss << '"';
            for (char c : values[i]) {
                if (c == '"') {
                    oss << "\"\"";
                } else {
                    oss << c;
                }
            }
            oss << '"';
        } else {
            oss << values[i];
        }
    }
    return oss.str();
}

void Storage::saveTable(const Table &table) const {
    std::filesystem::path path = std::filesystem::path(dataDir_) / (table.name + ".csv");
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to write table: " + table.name);
    }
    file << buildCsvLine(table.columns) << "\n";
    for (const Row &row : table.rows) {
        file << buildCsvLine(row.values) << "\n";
    }
}
