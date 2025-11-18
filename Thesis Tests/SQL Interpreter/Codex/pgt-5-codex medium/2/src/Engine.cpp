#include "Engine.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "Lexer.h"
#include "Parser.h"
#include "Utils.h"

Engine::Engine() = default;

void Engine::repl() {
    std::cout << "minisql> " << std::flush;
    std::string buffer;
    while (true) {
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }
        std::string trimmed = Utils::trim(line);
        if (trimmed == ".exit") {
            break;
        }
        buffer += line;
        buffer += '\n';
        std::size_t pos;
        while ((pos = buffer.find(';')) != std::string::npos) {
            std::string statement = buffer.substr(0, pos + 1);
            buffer.erase(0, pos + 1);
            buffer = Utils::trim(buffer);
            executeStatement(statement);
        }
        std::cout << "minisql> " << std::flush;
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Unable to open script: " << filename << '\n';
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    executeStatement(buffer.str());
}

std::string Engine::executeStatementWeb(const std::string &sql) {
    return executeStatementInternal(sql, true);
}

void Engine::executeStatement(const std::string &sql) {
    executeStatementInternal(sql, false);
}

std::string Engine::executeStatementInternal(const std::string &sql, bool returnOutput) {
    std::ostringstream output;
    try {
        Lexer lexer(sql);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto statements = parser.parseStatements();
        for (const auto &stmt : statements) {
            std::string result = executeParsedStatement(*stmt);
            if (returnOutput) {
                output << result;
            } else {
                std::cout << result;
            }
        }
        if (statements.empty()) {
            if (returnOutput) {
                output << "OK\n";
            } else {
                std::cout << "OK\n";
            }
        }
    } catch (const std::exception &ex) {
        if (returnOutput) {
            output << "ERROR: " << ex.what() << "\n";
        } else {
            std::cout << "ERROR: " << ex.what() << "\n";
        }
    }
    return output.str();
}

std::string Engine::executeParsedStatement(const Statement &stmt) {
    switch (stmt.type()) {
        case StatementType::CREATE_TABLE:
            return executeCreate(static_cast<const CreateTableStatement &>(stmt));
        case StatementType::INSERT:
            return executeInsert(static_cast<const InsertStatement &>(stmt));
        case StatementType::SELECT:
            return executeSelect(static_cast<const SelectStatement &>(stmt));
    }
    throw std::runtime_error("Unsupported statement");
}

std::string Engine::executeCreate(const CreateTableStatement &stmt) {
    if (!storage_.createTable(stmt.tableName, stmt.columns)) {
        throw std::runtime_error(storage_.getLastError());
    }
    return "OK\n";
}

std::string Engine::executeInsert(const InsertStatement &stmt) {
    if (!storage_.insertRow(stmt.tableName, stmt.values)) {
        throw std::runtime_error(storage_.getLastError());
    }
    return "OK\n";
}

std::string Engine::executeSelect(const SelectStatement &stmt) {
    const Table *table = storage_.getTable(stmt.tableName);
    if (!table) {
        throw std::runtime_error("Table not found: " + stmt.tableName);
    }
    int whereIndex = -1;
    if (stmt.hasWhere) {
        for (std::size_t i = 0; i < table->columns.size(); ++i) {
            if (Utils::toLower(table->columns[i]) == Utils::toLower(stmt.whereColumn)) {
                whereIndex = static_cast<int>(i);
                break;
            }
        }
        if (whereIndex == -1) {
            throw std::runtime_error("Unknown column in WHERE clause: " + stmt.whereColumn);
        }
    }
    std::vector<const Row *> matches;
    for (const auto &row : table->rows) {
        if (whereIndex == -1 || row.values[whereIndex] == stmt.whereValue) {
            matches.push_back(&row);
        }
    }
    std::ostringstream oss;
    std::vector<std::string> columns = table->columns;
    for (auto &column : columns) {
        column = Utils::trim(column);
    }
    oss << Utils::join(columns, " | ") << '\n';
    std::vector<std::string> separators(columns.size(), "----");
    oss << Utils::join(separators, "-+-") << '\n';
    for (const Row *row : matches) {
        oss << Utils::join(row->values, " | ") << '\n';
    }
    oss << "(" << matches.size() << " row(s) returned)\n";
    return oss.str();
}
