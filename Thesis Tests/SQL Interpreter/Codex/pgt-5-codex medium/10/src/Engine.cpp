#include "Engine.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "Utils.h"

Engine::Engine() = default;

void Engine::repl() {
    std::cout << "MiniSQL ready. Type SQL statements ending with ';'." << std::endl;
    std::string buffer;
    std::string line;
    std::cout << "> " << std::flush;
    while (std::getline(std::cin, line)) {
        buffer += line;
        buffer.push_back('\n');

        auto statements = utils::splitStatements(buffer);
        buffer.clear();
        for (const auto &stmt : statements) {
            if (stmt.find(';') == std::string::npos) {
                buffer = stmt;
                continue;
            }
            try {
                executeStatement(stmt);
            } catch (const std::exception &ex) {
                std::cerr << "Error: " << ex.what() << std::endl;
            }
        }
        std::cout << "> " << std::flush;
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream input(filename);
    if (!input.is_open()) {
        std::cerr << "Unable to open script file: " << filename << std::endl;
        return;
    }
    std::stringstream buffer;
    buffer << input.rdbuf();
    auto statements = utils::splitStatements(buffer.str());
    for (const auto &stmt : statements) {
        if (stmt.find(';') == std::string::npos) {
            continue;
        }
        try {
            executeStatement(stmt);
        } catch (const std::exception &ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
        }
    }
}

std::string Engine::executeStatementWeb(const std::string &sql) {
    try {
        return executeStatementInternal(sql, true);
    } catch (const std::exception &ex) {
        return std::string("Error: ") + ex.what();
    }
}

void Engine::executeStatement(const std::string &sql) {
    executeStatementInternal(sql, false);
}

std::string Engine::executeStatementInternal(const std::string &sql, bool returnOutput) {
    Parser parser(sql);
    StatementPtr stmt = parser.parseStatement();
    std::ostringstream oss;
    auto emitOk = [&](const std::string &text) {
        if (returnOutput) {
            oss << text << '\n';
        } else {
            std::cout << text << std::endl;
        }
    };

    if (auto createStmt = dynamic_cast<CreateTableStatement *>(stmt.get())) {
        std::string error;
        if (!storage_.createTable(createStmt->tableName, createStmt->columns, error)) {
            throw std::runtime_error(error);
        }
        emitOk("OK");
    } else if (auto insertStmt = dynamic_cast<InsertStatement *>(stmt.get())) {
        std::string error;
        if (!storage_.insertInto(insertStmt->tableName, insertStmt->values, error)) {
            throw std::runtime_error(error);
        }
        emitOk("OK");
    } else if (auto selectStmt = dynamic_cast<SelectStatement *>(stmt.get())) {
        std::string error;
        auto result = storage_.selectFrom(*selectStmt, error);
        if (!result) {
            throw std::runtime_error(error);
        }

        std::ostringstream tableStream;
        tableStream << utils::join(result->columns, " | ") << "\n";
        for (const auto &row : result->rows) {
            tableStream << utils::join(row.values, " | ") << "\n";
        }
        if (returnOutput) {
            oss << tableStream.str();
        } else {
            std::cout << tableStream.str();
        }
    }

    return oss.str();
}
