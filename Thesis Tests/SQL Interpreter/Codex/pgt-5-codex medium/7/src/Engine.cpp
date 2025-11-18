#include "Engine.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "Utils.h"

namespace {

std::string formatResult(const QueryResult &result) {
    std::ostringstream oss;
    if (!result.columns.empty()) {
        oss << join(result.columns, " | ") << "\n";
    }
    for (const Row &row : result.rows) {
        oss << join(row.values, " | ") << "\n";
    }
    if (result.rows.empty()) {
        oss << "(no rows)\n";
    }
    return oss.str();
}

bool isQuitCommand(const std::string &line) {
    std::string lower = toLower(trim(line));
    return lower == "quit" || lower == "exit" || lower == "\\q";
}

} // namespace

Engine::Engine() = default;

void Engine::repl() {
    std::string buffer;
    std::cout << "Minimal SQL Interpreter. Type QUIT to exit.\n";
    while (true) {
        std::cout << "sql> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }
        if (buffer.empty() && isQuitCommand(line)) {
            break;
        }
        buffer += line;
        buffer.push_back('\n');

        while (true) {
            size_t pos = buffer.find(';');
            if (pos == std::string::npos) {
                break;
            }
            std::string statement = buffer.substr(0, pos + 1);
            buffer.erase(0, pos + 1);
            statement = trim(statement);
            if (!statement.empty()) {
                executeStatement(statement);
            }
        }
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open script: " << filename << std::endl;
        return;
    }
    std::string buffer;
    std::string line;
    while (std::getline(file, line)) {
        buffer += line;
        buffer.push_back('\n');
        while (true) {
            size_t pos = buffer.find(';');
            if (pos == std::string::npos) {
                break;
            }
            std::string statement = buffer.substr(0, pos + 1);
            buffer.erase(0, pos + 1);
            statement = trim(statement);
            if (!statement.empty()) {
                executeStatement(statement);
            }
        }
    }
    buffer = trim(buffer);
    if (!buffer.empty()) {
        executeStatement(buffer + ";");
    }
}

std::string Engine::executeStatementWeb(const std::string &sql) {
    return executeStatementInternal(sql, true);
}

void Engine::executeStatement(const std::string &sql) {
    executeStatementInternal(sql, false);
}

std::string Engine::executeStatementInternal(const std::string &sql, bool returnOutput) {
    std::string result;
    try {
        std::string statement = trim(sql);
        if (!statement.empty() && statement.back() != ';') {
            statement.push_back(';');
        }
        Parser parser(statement);
        Statement stmt = parser.parseStatement();
        result = std::visit(
            [&](auto &&value) -> std::string {
                using T = std::decay_t<decltype(value)>;
                std::string error;
                if constexpr (std::is_same_v<T, CreateTableStatement>) {
                    if (storage_.createTable(value, error)) {
                        return "OK";
                    }
                } else if constexpr (std::is_same_v<T, InsertStatement>) {
                    if (storage_.insertRow(value, error)) {
                        return "OK";
                    }
                } else if constexpr (std::is_same_v<T, SelectStatement>) {
                    QueryResult queryResult;
                    if (storage_.select(value, queryResult, error)) {
                        return formatResult(queryResult);
                    }
                }
                return "ERROR: " + error;
            },
            stmt.value);
    } catch (const std::exception &ex) {
        result = std::string("ERROR: ") + ex.what();
    }

    if (!returnOutput) {
        std::cout << result << std::endl;
    }
    return result;
}
