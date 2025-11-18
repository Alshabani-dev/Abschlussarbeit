#include "Engine.h"

#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>

#include "Lexer.h"
#include "Utils.h"

Engine::Engine() = default;

void Engine::repl() {
    std::string buffer;
    std::string line;
    std::cout << "minisql> " << std::flush;
    while (std::getline(std::cin, line)) {
        buffer += line;
        buffer.push_back('\n');
        size_t semicolonPos = std::string::npos;
        while ((semicolonPos = buffer.find(';')) != std::string::npos) {
            std::string statement = Utils::trim(buffer.substr(0, semicolonPos));
            if (!statement.empty()) {
                executeStatement(statement);
            }
            buffer.erase(0, semicolonPos + 1);
        }
        std::cout << "minisql> " << std::flush;
    }
    std::string remaining = Utils::trim(buffer);
    if (!remaining.empty()) {
        executeStatement(remaining);
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open script file: " << filename << "\n";
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    auto statements = Utils::splitStatements(buffer.str());
    for (const auto &statement : statements) {
        if (statement.empty()) {
            continue;
        }
        executeStatement(statement);
    }
}

std::string Engine::executeStatementWeb(const std::string &sql) {
    return executeStatementInternal(sql, true);
}

void Engine::executeStatement(const std::string &sql) {
    std::string output = executeStatementInternal(sql, false);
    if (output.empty()) {
        return;
    }
    std::cout << output;
    if (output.back() != '\n') {
        std::cout << '\n';
    }
    std::cout.flush();
}

std::string Engine::executeStatementInternal(const std::string &sql, bool /*forWeb*/) {
    std::string trimmed = Utils::trim(sql);
    if (trimmed.empty()) {
        return "";
    }

    Lexer lexer(trimmed);
    std::vector<Token> tokens = lexer.tokenize();
    if (!lexer.getError().empty()) {
        return "ERROR: " + lexer.getError();
    }

    Parser parser(tokens);
    auto statement = parser.parseStatement();
    if (!statement) {
        return "ERROR: " + parser.getError();
    }

    return std::visit([this](auto &&stmt) -> std::string {
        using T = std::decay_t<decltype(stmt)>;
        if constexpr (std::is_same_v<T, CreateTableStatement>) {
            if (!storage_.createTable(stmt.tableName, stmt.columns)) {
                return "ERROR: " + storage_.getLastError();
            }
            return "OK";
        } else if constexpr (std::is_same_v<T, InsertStatement>) {
            if (!storage_.insertRow(stmt.tableName, stmt.values)) {
                return "ERROR: " + storage_.getLastError();
            }
            return "OK";
        } else if constexpr (std::is_same_v<T, SelectStatement>) {
            const Table *table = storage_.getTable(stmt.tableName);
            if (!table) {
                return "ERROR: Table does not exist: " + stmt.tableName;
            }
            auto findColumnIndex = [&](const std::string &name) -> std::optional<size_t> {
                std::string target = Utils::toLower(name);
                for (size_t i = 0; i < table->columns.size(); ++i) {
                    if (Utils::toLower(table->columns[i]) == target) {
                        return i;
                    }
                }
                return std::nullopt;
            };

            std::vector<size_t> columnIndexes;
            std::vector<std::string> columnNames;
            if (stmt.selectAll) {
                for (size_t i = 0; i < table->columns.size(); ++i) {
                    columnIndexes.push_back(i);
                    columnNames.push_back(table->columns[i]);
                }
            } else {
                for (const auto &column : stmt.columns) {
                    auto index = findColumnIndex(column);
                    if (!index) {
                        return "ERROR: Unknown column: " + column;
                    }
                    columnIndexes.push_back(*index);
                    columnNames.push_back(table->columns[*index]);
                }
            }

            size_t whereIndex = 0;
            if (stmt.hasWhere) {
                auto index = findColumnIndex(stmt.where.column);
                if (!index) {
                    return "ERROR: Unknown column in WHERE: " + stmt.where.column;
                }
                whereIndex = *index;
            }

            std::ostringstream out;
            for (size_t i = 0; i < columnNames.size(); ++i) {
                if (i > 0) {
                    out << " | ";
                }
                out << columnNames[i];
            }
            out << '\n';

            for (const auto &row : table->rows) {
                if (stmt.hasWhere) {
                    if (whereIndex >= row.values.size() || row.values[whereIndex] != stmt.where.value) {
                        continue;
                    }
                }
                for (size_t i = 0; i < columnIndexes.size(); ++i) {
                    if (i > 0) {
                        out << " | ";
                    }
                    size_t idx = columnIndexes[i];
                    if (idx < row.values.size()) {
                        out << row.values[idx];
                    }
                }
                out << '\n';
            }
            return out.str();
        }
        return std::string{};
    }, *statement);
}
