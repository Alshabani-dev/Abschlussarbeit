#include "Engine.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "Utils.h"

namespace {

std::string formatRow(const std::vector<std::string> &values) {
    return Utils::join(values, " | ");
}

std::string formatSelectResult(const std::vector<std::string> &columns, const std::vector<Row> &rows) {
    std::ostringstream oss;
    oss << formatRow(columns) << "\n";
    for (const Row &row : rows) {
        oss << formatRow(row.values) << "\n";
    }
    oss << "(" << rows.size() << " row(s))";
    return oss.str();
}

} // namespace

Engine::Engine() = default;

void Engine::repl() {
    std::cout << "Minimal SQL Interpreter. Type SQL statements terminated with ';'." << std::endl;
    std::string buffer;
    while (true) {
        std::cout << "minisql> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }
        buffer += line;
        buffer += "\n";
        if (buffer.find(';') == std::string::npos) {
            continue;
        }
        executeStatement(buffer);
        buffer.clear();
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "ERROR: Unable to open script file: " << filename << std::endl;
        return;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    executeStatement(buffer.str());
}

void Engine::executeStatement(const std::string &sql) {
    try {
        executeStatementInternal(sql, false);
    } catch (const std::exception &ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
    }
}

std::string Engine::executeStatementWeb(const std::string &sql) {
    try {
        return executeStatementInternal(sql, true);
    } catch (const std::exception &ex) {
        return std::string("ERROR: ") + ex.what();
    }
}

std::string Engine::executeStatementInternal(const std::string &sql, bool returnOutput) {
    Parser parser(sql);
    std::vector<StatementPtr> statements = parser.parseStatements();

    std::ostringstream output;
    bool first = true;

    for (const auto &stmt : statements) {
        std::string message;
        switch (stmt->type) {
            case StatementType::CREATE_TABLE: {
                auto *createStmt = static_cast<CreateTableStatement *>(stmt.get());
                std::string error;
                if (!storage_.createTable(createStmt->tableName, createStmt->columns, error)) {
                    message = "ERROR: " + error;
                } else {
                    message = "OK";
                }
                break;
            }
            case StatementType::INSERT: {
                auto *insertStmt = static_cast<InsertStatement *>(stmt.get());
                std::string error;
                if (!storage_.insertRow(insertStmt->tableName, insertStmt->values, error)) {
                    message = "ERROR: " + error;
                } else {
                    message = "OK";
                }
                break;
            }
            case StatementType::SELECT: {
                auto *selectStmt = static_cast<SelectStatement *>(stmt.get());
                std::vector<std::string> columns;
                std::vector<Row> rows;
                std::string error;
                if (!storage_.selectRows(*selectStmt, columns, rows, error)) {
                    message = "ERROR: " + error;
                } else {
                    message = formatSelectResult(columns, rows);
                }
                break;
            }
        }

        if (!message.empty()) {
            if (!first) {
                output << "\n";
            }
            output << message;
            first = false;
        }
    }

    if (returnOutput) {
        return output.str();
    }

    std::string finalOutput = output.str();
    if (!finalOutput.empty()) {
        std::cout << finalOutput << std::endl;
    }
    return "";
}
