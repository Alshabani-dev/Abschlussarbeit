#include "Engine.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include "Utils.h"

Engine::Engine() {}

void Engine::repl() {
    std::cout << "minisql> ";
    std::string line;

    while (std::getline(std::cin, line)) {
        if (line == ".exit") {
            break;
        }

        if (!line.empty()) {
            try {
                executeStatement(line);
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }

        std::cout << "minisql> ";
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::string line;
    std::string currentStatement;

    while (std::getline(file, line)) {
        // Skip comments
        if (line.empty() || (line.size() >= 2 && line.substr(0, 2) == "--")) {
            continue;
        }

        currentStatement += line;

        // Check if the statement ends with a semicolon
        size_t semicolonPos = currentStatement.find(';');
        if (semicolonPos != std::string::npos) {
            std::string statement = currentStatement.substr(0, semicolonPos);
            statement = Utils::trim(statement);

            if (!statement.empty()) {
                try {
                    executeStatement(statement);
                } catch (const std::exception& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                }
            }

            // Remove the processed statement
            currentStatement = currentStatement.substr(semicolonPos + 1);
        }
    }

    // If there's any remaining statement without semicolon, try to execute it
    if (!currentStatement.empty()) {
        currentStatement = Utils::trim(currentStatement);
        if (!currentStatement.empty()) {
            try {
                executeStatement(currentStatement);
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }
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
        Parser parser;
        std::unique_ptr<Statement> stmt = parser.parse(sql);

        if (!stmt) {
            return "Error: Empty statement";
        }

        switch (stmt->type()) {
            case StatementType::CREATE_TABLE: {
                auto createStmt = dynamic_cast<CreateTableStatement*>(stmt.get());
                if (storage_.createTable(createStmt->tableName, createStmt->columns)) {
                    output << "OK";
                } else {
                    output << "Error: " << storage_.getLastError();
                }
                break;
            }

            case StatementType::INSERT: {
                auto insertStmt = dynamic_cast<InsertStatement*>(stmt.get());
                if (storage_.insertRow(insertStmt->tableName, insertStmt->values)) {
                    output << "OK";
                } else {
                    output << "Error: " << storage_.getLastError();
                }
                break;
            }

            case StatementType::SELECT: {
                auto selectStmt = dynamic_cast<SelectStatement*>(stmt.get());
                const Table* table = storage_.getTable(selectStmt->tableName);

                if (!table) {
                    output << "Error: Table '" << selectStmt->tableName << "' not found";
                    break;
                }

                // Print header
                for (size_t i = 0; i < table->columns.size(); ++i) {
                    if (i > 0) output << " | ";
                    output << table->columns[i];
                }
                output << "\n";

                // Print separator
                for (size_t i = 0; i < table->columns.size(); ++i) {
                    if (i > 0) output << "-+-";
                    output << "---";
                }
                output << "\n";

                // Filter and print rows
                int rowCount = 0;
                for (const auto& row : table->rows) {
                    if (selectStmt->hasWhere) {
                        // Find column index
                        auto colIt = std::find(table->columns.begin(), table->columns.end(),
                                              selectStmt->whereColumn);
                        if (colIt == table->columns.end()) {
                            output << "Error: Column '" << selectStmt->whereColumn << "' not found";
                            break;
                        }

                        size_t colIndex = std::distance(table->columns.begin(), colIt);
                        if (row.values[colIndex] != selectStmt->whereValue) {
                            continue;
                        }
                    }

                    // Print row
                    for (size_t i = 0; i < row.values.size(); ++i) {
                        if (i > 0) output << " | ";
                        output << row.values[i];
                    }
                    output << "\n";
                    rowCount++;
                }

                output << "\n(" << rowCount << " row(s) returned)";
                break;
            }
        }
    } catch (const std::exception& e) {
        output << "Error: " << e.what();
    }

    if (returnOutput) {
        return output.str();
    } else {
        std::cout << output.str() << std::endl;
        return "";
    }
}
