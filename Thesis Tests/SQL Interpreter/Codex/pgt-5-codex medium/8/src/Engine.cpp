#include "Engine.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "Utils.h"

Engine::Engine() = default;

void Engine::repl() {
    std::cout << "Minimal SQL Interpreter. Type statements ending with ';'. Ctrl+D to exit.\n";
    std::string buffer;
    std::string line;
    while (true) {
        std::cout << "sql> " << std::flush;
        if (!std::getline(std::cin, line)) {
            break;
        }
        buffer += line;
        buffer += "\n";
        size_t pos;
        while ((pos = buffer.find(';')) != std::string::npos) {
            std::string statement = buffer.substr(0, pos + 1);
            executeStatement(statement);
            buffer.erase(0, pos + 1);
        }
    }
    if (!utils::trim(buffer).empty()) {
        executeStatement(buffer);
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        std::cerr << "Failed to open script: " << filename << "\n";
        return;
    }
    std::string buffer;
    std::string line;
    while (std::getline(in, line)) {
        buffer += line;
        buffer += "\n";
        size_t pos;
        while ((pos = buffer.find(';')) != std::string::npos) {
            std::string statement = buffer.substr(0, pos + 1);
            executeStatement(statement);
            buffer.erase(0, pos + 1);
        }
    }
    if (!utils::trim(buffer).empty()) {
        executeStatement(buffer);
    }
}

std::string Engine::executeStatementWeb(const std::string &sql) {
    return executeStatementInternal(sql, true);
}

void Engine::executeStatement(const std::string &sql) {
    std::string result = executeStatementInternal(sql, true);
    if (!result.empty()) {
        std::cout << result << "\n";
    }
}

std::string Engine::executeStatementInternal(const std::string &sql, bool /*returnOutput*/) {
    std::string trimmed = utils::trim(sql);
    if (trimmed.empty()) {
        return "";
    }
    try {
        Parser parser(trimmed);
        auto statement = parser.parseStatement();
        std::string error;
        switch (statement->kind) {
            case StatementKind::CreateTable: {
                auto *createStmt = static_cast<CreateTableStatement *>(statement.get());
                if (!storage_.createTable(createStmt->tableName, createStmt->columns, error)) {
                    return "ERROR: " + error;
                }
                return "OK";
            }
            case StatementKind::Insert: {
                auto *insertStmt = static_cast<InsertStatement *>(statement.get());
                if (!storage_.insertRow(insertStmt->tableName, insertStmt->values, error)) {
                    return "ERROR: " + error;
                }
                return "OK";
            }
            case StatementKind::Select: {
                auto *selectStmt = static_cast<SelectStatement *>(statement.get());
                QueryResult result;
                bool hasWhere = selectStmt->whereColumn.has_value() && selectStmt->whereValue.has_value();
                std::string whereColumn = hasWhere ? *selectStmt->whereColumn : "";
                std::string whereValue = hasWhere ? *selectStmt->whereValue : "";
                if (!storage_.selectRows(selectStmt->tableName, selectStmt->columns, whereColumn, whereValue,
                                         hasWhere, result, error)) {
                    return "ERROR: " + error;
                }
                std::ostringstream oss;
                if (result.columns.empty()) {
                    oss << "(no columns)\n";
                } else {
                    oss << utils::join(result.columns, " | ") << "\n";
                }
                for (const auto &row : result.rows) {
                    oss << utils::join(row.values, " | ") << "\n";
                }
                return oss.str();
            }
        }
    } catch (const std::exception &ex) {
        return std::string("ERROR: ") + ex.what();
    }
    return "ERROR: Unknown statement";
}
