#include "Engine.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "Parser.h"
#include "Utils.h"

Engine::Engine() = default;

void Engine::repl() {
    std::cout << "minisql> " << std::flush;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::string trimmed = Utils::trim(line);
        if (trimmed == ".exit") {
            break;
        }
        buffer_ += line;
        buffer_ += '\n';
        auto statements = extractStatementsFromBuffer(buffer_);
        for (const auto &stmt : statements) {
            executeStatement(stmt);
        }
        std::cout << "minisql> " << std::flush;
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Failed to open script file: " << filename << '\n';
        return;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string content = ss.str();
    auto tmpBuffer = content;
    auto statements = extractStatementsFromBuffer(tmpBuffer);
    for (const auto &stmt : statements) {
        executeStatement(stmt);
    }
    if (!Utils::trim(tmpBuffer).empty()) {
        std::cerr << "Warning: script ended without a semicolon; last statement ignored." << '\n';
    }
}

std::string Engine::executeStatementWeb(const std::string &sql) {
    std::string statement = Utils::trim(sql);
    if (!statement.empty() && statement.back() != ';') {
        statement.push_back(';');
    }
    return executeStatementInternal(statement);
}

void Engine::executeStatement(const std::string &sql) {
    std::string output = executeStatementInternal(sql);
    if (!output.empty()) {
        std::cout << output << '\n';
    }
}

std::string Engine::executeStatementInternal(const std::string &sql) {
    if (Utils::trim(sql).empty()) {
        return {};
    }
    try {
        Parser parser(sql);
        StatementPtr stmt = parser.parseStatement();
        switch (stmt->type()) {
            case StatementType::CREATE_TABLE:
                return handleCreate(*stmt);
            case StatementType::INSERT:
                return handleInsert(*stmt);
            case StatementType::SELECT:
                return handleSelect(*stmt);
        }
    } catch (const std::exception &ex) {
        return std::string("Error: ") + ex.what();
    }
    return "Error: Unknown statement";
}

std::vector<std::string> Engine::extractStatementsFromBuffer(std::string &buffer) {
    std::vector<std::string> statements;
    std::string current;
    bool inQuotes = false;
    for (size_t i = 0; i < buffer.size(); ++i) {
        char c = buffer[i];
        current += c;
        if (c == '"') {
            if (i + 1 < buffer.size() && buffer[i + 1] == '"') {
                current += buffer[i + 1];
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ';' && !inQuotes) {
            std::string trimmed = Utils::trim(current);
            if (!trimmed.empty()) {
                statements.push_back(trimmed);
            }
            current.clear();
        }
    }
    buffer = current;
    return statements;
}

std::string Engine::handleCreate(const Statement &stmtBase) {
    const auto &stmt = static_cast<const CreateTableStatement &>(stmtBase);
    if (!storage_.createTable(stmt.tableName, stmt.columns)) {
        return "Error: " + storage_.getLastError();
    }
    return "OK";
}

std::string Engine::handleInsert(const Statement &stmtBase) {
    const auto &stmt = static_cast<const InsertStatement &>(stmtBase);
    if (!storage_.insertRow(stmt.tableName, stmt.values)) {
        return "Error: " + storage_.getLastError();
    }
    return "OK";
}

std::string Engine::handleSelect(const Statement &stmtBase) {
    const auto &stmt = static_cast<const SelectStatement &>(stmtBase);
    const Table *table = storage_.getTable(stmt.tableName);
    if (!table) {
        return "Error: Table does not exist";
    }
    std::vector<const Row *> rows;
    if (stmt.hasWhere) {
        auto it = std::find(table->columns.begin(), table->columns.end(), stmt.whereColumn);
        if (it == table->columns.end()) {
            return "Error: Unknown column in WHERE clause";
        }
        size_t columnIndex = static_cast<size_t>(std::distance(table->columns.begin(), it));
        for (const auto &row : table->rows) {
            if (columnIndex < row.values.size() && row.values[columnIndex] == stmt.whereValue) {
                rows.push_back(&row);
            }
        }
    } else {
        for (const auto &row : table->rows) {
            rows.push_back(&row);
        }
    }
    return formatSelectResult(*table, rows);
}

std::string Engine::formatSelectResult(const Table &table, const std::vector<const Row *> &rows) const {
    if (table.columns.empty()) {
        return "(0 row(s) returned)";
    }
    std::vector<size_t> widths(table.columns.size(), 0);
    for (size_t i = 0; i < table.columns.size(); ++i) {
        widths[i] = std::max(widths[i], table.columns[i].size());
    }
    for (const auto *row : rows) {
        for (size_t i = 0; i < row->values.size(); ++i) {
            widths[i] = std::max(widths[i], row->values[i].size());
        }
    }
    std::ostringstream out;
    for (size_t i = 0; i < table.columns.size(); ++i) {
        if (i > 0) {
            out << " | ";
        }
        out << std::setw(static_cast<int>(widths[i])) << table.columns[i];
    }
    out << '\n';
    for (size_t i = 0; i < table.columns.size(); ++i) {
        if (i > 0) {
            out << "-+-";
        }
        out << std::string(widths[i], '-');
    }
    out << '\n';
    for (const auto *row : rows) {
        for (size_t i = 0; i < row->values.size(); ++i) {
            if (i > 0) {
                out << " | ";
            }
            out << std::setw(static_cast<int>(widths[i])) << row->values[i];
        }
        out << '\n';
    }
    out << '(' << rows.size() << " row(s) returned)";
    return out.str();
}
