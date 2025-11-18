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

bool Storage::createTable(const std::string &name, const std::vector<std::string> &columns) {
    lastError_.clear();
    if (columns.empty()) {
        lastError_ = "Cannot create a table without columns";
        return false;
    }
    std::string key = normalize(name);
    if (tables_.count(key) > 0U) {
        lastError_ = "Table already exists: " + name;
        return false;
    }
    Table table;
    table.name = name;
    table.columns = columns;
    tables_[key] = table;
    if (!saveTable(key)) {
        tables_.erase(key);
        return false;
    }
    lastError_.clear();
    return true;
}

bool Storage::insertRow(const std::string &tableName, const std::vector<std::string> &values) {
    lastError_.clear();
    std::string key = normalize(tableName);
    auto it = tables_.find(key);
    if (it == tables_.end()) {
        lastError_ = "Table does not exist: " + tableName;
        return false;
    }
    if (values.size() != it->second.columns.size()) {
        lastError_ = "Value count does not match columns";
        return false;
    }
    Row row;
    row.values = values;
    it->second.rows.push_back(row);
    if (!saveTable(key)) {
        it->second.rows.pop_back();
        return false;
    }
    lastError_.clear();
    return true;
}

const Table *Storage::getTable(const std::string &name) const {
    std::string key = normalize(name);
    auto it = tables_.find(key);
    if (it == tables_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::string Storage::getLastError() const {
    return lastError_;
}

void Storage::loadAllTables() {
    if (!ensureDataDir()) {
        return;
    }
    if (!fs::exists(dataDir_)) {
        return;
    }
    for (const auto &entry : fs::directory_iterator(dataDir_)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        auto path = entry.path();
        if (path.extension() == ".csv") {
            std::string tableName = path.stem().string();
            loadTableFromFile(tableName, path.string());
        }
    }
}

bool Storage::loadTableFromFile(const std::string &tableName, const std::string &filePath) {
    lastError_.clear();
    std::ifstream file(filePath);
    if (!file.is_open()) {
        lastError_ = "Failed to open table file: " + filePath;
        return false;
    }
    std::string header;
    if (!std::getline(file, header)) {
        return true;
    }
    std::vector<std::string> columns = parseCsvLine(header);
    if (columns.empty()) {
        lastError_ = "Invalid header in file: " + filePath;
        return false;
    }
    Table table;
    table.name = tableName;
    table.columns = columns;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> values = parseCsvLine(line);
        if (values.size() != columns.size()) {
            continue;
        }
        Row row;
        row.values = values;
        table.rows.push_back(row);
    }

    tables_[normalize(tableName)] = table;
    return true;
}

bool Storage::saveTable(const std::string &tableName) {
    auto it = tables_.find(tableName);
    if (it == tables_.end()) {
        return false;
    }
    if (!ensureDataDir()) {
        return false;
    }
    fs::path path = fs::path(dataDir_) / (it->second.name + ".csv");
    std::ofstream file(path.string(), std::ios::trunc);
    if (!file.is_open()) {
        lastError_ = "Failed to write table file: " + path.string();
        return false;
    }

    for (size_t i = 0; i < it->second.columns.size(); ++i) {
        if (i > 0) {
            file << ",";
        }
        file << escapeCsv(it->second.columns[i]);
    }
    file << "\n";

    for (const auto &row : it->second.rows) {
        for (size_t col = 0; col < row.values.size(); ++col) {
            if (col > 0) {
                file << ",";
            }
            file << escapeCsv(row.values[col]);
        }
        file << "\n";
    }
    lastError_.clear();
    return true;
}

std::string Storage::normalize(const std::string &name) const {
    return Utils::toLower(name);
}

bool Storage::ensureDataDir() {
    std::error_code ec;
    if (!fs::exists(dataDir_, ec)) {
        fs::create_directories(dataDir_, ec);
    }
    if (ec) {
        lastError_ = "Failed to create data directory: " + ec.message();
        return false;
    }
    return true;
}

std::vector<std::string> Storage::parseCsvLine(const std::string &line) {
    std::vector<std::string> values;
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
                values.push_back(current);
                current.clear();
            } else if (c == '\r') {
                continue;
            } else {
                current.push_back(c);
            }
        }
    }
    values.push_back(current);
    return values;
}

std::string Storage::escapeCsv(const std::string &value) {
    bool needsQuotes = value.find_first_of(",\"\n\r") != std::string::npos;
    std::string escaped = value;
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.insert(pos, "\"");
        pos += 2;
        needsQuotes = true;
    }
    if (needsQuotes) {
        return "\"" + escaped + "\"";
    }
    return escaped;
}
