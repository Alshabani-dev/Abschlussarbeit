#include "Engine.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "Lexer.h"

Engine::Engine() = default;

void Engine::repl() {
    std::string buffer;
    std::string line;
    std::cout << "mini-sql> " << std::flush;
    while (std::getline(std::cin, line)) {
        buffer.append(line);
        buffer.push_back('\n');
        std::size_t pos;
        while ((pos = buffer.find(';')) != std::string::npos) {
            std::string statement = buffer.substr(0, pos + 1);
            executeStatement(statement);
            buffer.erase(0, pos + 1);
        }
        std::cout << "mini-sql> " << std::flush;
    }
    if (!buffer.empty()) {
        executeStatement(buffer);
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open script: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    executeStatement(buffer.str());
}

std::string Engine::executeStatementWeb(const std::string &sql) {
    try {
        std::string response = executeStatementInternal(sql, true);
        if (response.empty()) {
            return "OK";
        }
        return response;
    } catch (const std::exception &ex) {
        return std::string("Error: ") + ex.what();
    }
}

void Engine::executeStatement(const std::string &sql) {
    try {
        std::string output = executeStatementInternal(sql, false);
        if (!output.empty()) {
            std::cout << output << std::endl;
        }
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}

std::string Engine::executeStatementInternal(const std::string &sql, bool returnOutput) {
    Lexer lexer(sql);
    Parser parser(lexer.tokenize());
    auto statements = parser.parseStatements();
    std::ostringstream oss;
    bool first = true;
    for (const auto &stmt : statements) {
        std::string result = executeSingleStatement(*stmt);
        if (!result.empty()) {
            if (returnOutput) {
                if (!first) {
                    oss << '\n';
                }
                oss << result;
            } else {
                std::cout << result << std::endl;
            }
            first = false;
        }
    }
    if (returnOutput) {
        return oss.str();
    }
    return {};
}

std::string Engine::executeSingleStatement(const Statement &stmt) {
    switch (stmt.kind) {
    case StatementKind::CreateTable:
        storage_.createTable(static_cast<const CreateTableStatement &>(stmt));
        return "OK";
    case StatementKind::Insert:
        storage_.insertRow(static_cast<const InsertStatement &>(stmt));
        return "OK";
    case StatementKind::Select: {
        auto result = storage_.select(static_cast<const SelectStatement &>(stmt));
        return formatResult(result);
    }
    }
    return {};
}

std::string Engine::formatResult(const SelectResult &result) const {
    if (result.columns.empty()) {
        return "(no columns)";
    }
    auto formatRow = [](const std::vector<std::string> &values) {
        std::ostringstream row;
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                row << " | ";
            }
            row << values[i];
        }
        return row.str();
    };

    std::ostringstream oss;
    std::string header = formatRow(result.columns);
    oss << header << '\n';
    oss << std::string(header.size(), '-') << '\n';
    for (const auto &row : result.rows) {
        oss << formatRow(row.values) << '\n';
    }
    return oss.str();
}
