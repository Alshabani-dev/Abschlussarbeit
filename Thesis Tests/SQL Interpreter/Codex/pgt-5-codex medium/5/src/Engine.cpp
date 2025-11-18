#include "Engine.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Utils.h"

Engine::Engine() = default;

void Engine::repl() {
    std::cout << "Minimal SQL Interpreter. Type EXIT; to quit." << std::endl;
    std::string buffer;
    std::string line;
    while (true) {
        std::cout << "sql> " << std::flush;
        if (!std::getline(std::cin, line)) {
            break;
        }
        buffer += line + '\n';
        if (!Utils::hasStatementTerminator(buffer)) {
            continue;
        }
        Utils::SplitResult split = Utils::splitSqlStatements(buffer);
        buffer = split.remainder;
        for (std::string stmt : split.statements) {
            stmt = Utils::trim(stmt);
            if (stmt.empty()) {
                continue;
            }
            std::string lowered = Utils::toLower(stmt);
            if (lowered == "exit" || lowered == "quit") {
                std::cout << "Bye!" << std::endl;
                return;
            }
            executeStatement(stmt);
        }
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        throw std::runtime_error("Unable to open script file: " + filename);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    Utils::SplitResult split = Utils::splitSqlStatements(buffer.str());
    std::vector<std::string> statements = Utils::joinStatements(split);
    for (const std::string &statement : statements) {
        if (statement.empty()) {
            continue;
        }
        executeStatement(statement);
    }
}

std::string Engine::executeStatementWeb(const std::string &sql) {
    std::string output = executeStatementInternal(sql, true);
    if (!output.empty() && output.back() == '\n') {
        output.pop_back();
    }
    return output;
}

void Engine::executeStatement(const std::string &sql) {
    executeStatementInternal(sql, false);
}

std::string Engine::executeStatementInternal(const std::string &sql, bool captureOutput) {
    std::ostringstream captured;
    Utils::SplitResult split = Utils::splitSqlStatements(sql);
    std::vector<std::string> statements = Utils::joinStatements(split);
    for (std::string statement : statements) {
        statement = Utils::trim(statement);
        if (statement.empty()) {
            continue;
        }
        std::string lowered = Utils::toLower(statement);
        if (lowered == "exit" || lowered == "quit") {
            if (!captureOutput) {
                std::cout << "Bye!" << std::endl;
                std::exit(0);
            }
            continue;
        }
        try {
            auto ast = parser_.parse(statement);
            std::string result = dispatchStatement(*ast);
            if (result.empty()) {
                continue;
            }
            if (captureOutput) {
                captured << result << '\n';
            } else {
                std::cout << result << std::endl;
            }
        } catch (const std::exception &ex) {
            if (captureOutput) {
                captured << "ERROR: " << ex.what() << '\n';
            } else {
                std::cout << "ERROR: " << ex.what() << std::endl;
            }
        }
    }
    return captured.str();
}

std::string Engine::dispatchStatement(const Statement &statement) {
    switch (statement.type) {
        case StatementType::CreateTable: {
            const auto &create = static_cast<const CreateTableStmt &>(statement);
            std::string error;
            if (!storage_.createTable(create.tableName, create.columns, error)) {
                return "ERROR: " + error;
            }
            return "OK";
        }
        case StatementType::Insert: {
            const auto &insert = static_cast<const InsertStmt &>(statement);
            std::string error;
            if (!storage_.insertRow(insert.tableName, insert.values, error)) {
                return "ERROR: " + error;
            }
            return "OK";
        }
        case StatementType::Select: {
            const auto &select = static_cast<const SelectStmt &>(statement);
            return handleSelect(select);
        }
        default:
            return "ERROR: Unsupported statement";
    }
}

std::string Engine::handleSelect(const SelectStmt &stmt) {
    std::vector<std::string> header;
    std::vector<Row> rows;
    std::string error;
    if (!storage_.selectRows(stmt, header, rows, error)) {
        return "ERROR: " + error;
    }
    if (header.empty()) {
        return "";
    }
    std::ostringstream out;
    for (size_t i = 0; i < header.size(); ++i) {
        if (i > 0) {
            out << " | ";
        }
        out << header[i];
    }
    out << '\n';
    for (const Row &row : rows) {
        for (size_t i = 0; i < row.values.size(); ++i) {
            if (i > 0) {
                out << " | ";
            }
            out << row.values[i];
        }
        out << '\n';
    }
    return out.str();
}
