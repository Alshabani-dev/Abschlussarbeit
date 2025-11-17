#include "Engine.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

Engine::Engine() {}

void Engine::repl() {
    std::cout << "minisql> ";
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == ".exit") {
            break;
        }

        if (!line.empty()) {
            executeStatement(line);
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
    while (std::getline(file, line)) {
        if (!line.empty()) {
            executeStatement(line);
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
    try {
        Parser parser;
        std::unique_ptr<Statement> stmt = parser.parse(sql);

        switch (stmt->type()) {
            case StatementType::CREATE_TABLE: {
                auto createStmt = dynamic_cast<CreateTableStatement*>(stmt.get());
                if (storage_.createTable(createStmt->tableName, createStmt->columns)) {
                    if (returnOutput) {
                        return "OK";
                    } else {
                        std::cout << "OK" << std::endl;
                    }
                } else {
                    if (returnOutput) {
                        return "Error: " + storage_.getLastError();
                    } else {
                        std::cerr << "Error: " << storage_.getLastError() << std::endl;
                    }
                }
                break;
            }

            case StatementType::INSERT: {
                auto insertStmt = dynamic_cast<InsertStatement*>(stmt.get());
                if (storage_.insertRow(insertStmt->tableName, insertStmt->values)) {
                    if (returnOutput) {
                        return "OK";
                    } else {
                        std::cout << "OK" << std::endl;
                    }
                } else {
                    if (returnOutput) {
                        return "Error: " + storage_.getLastError();
                    } else {
                        std::cerr << "Error: " << storage_.getLastError() << std::endl;
                    }
                }
                break;
            }

            case StatementType::SELECT: {
                auto selectStmt = dynamic_cast<SelectStatement*>(stmt.get());
                const Table *table = storage_.getTable(selectStmt->tableName);

                if (!table) {
                    if (returnOutput) {
                        return "Error: Table '" + selectStmt->tableName + "' not found";
                    } else {
                        std::cerr << "Error: Table '" << selectStmt->tableName << "' not found" << std::endl;
                    }
                    break;
                }

                std::ostringstream output;
                std::vector<Row> filteredRows;

                if (selectStmt->hasWhere) {
                    // Find column index
                    size_t colIndex = 0;
                    for (; colIndex < table->columns.size(); ++colIndex) {
                        if (Utils::toLower(table->columns[colIndex]) == Utils::toLower(selectStmt->whereColumn)) {
                            break;
                        }
                    }

                    if (colIndex >= table->columns.size()) {
                        if (returnOutput) {
                            return "Error: Column '" + selectStmt->whereColumn + "' not found";
                        } else {
                            std::cerr << "Error: Column '" << selectStmt->whereColumn << "' not found" << std::endl;
                        }
                        break;
                    }

                    // Filter rows
                    for (const auto &row : table->rows) {
                        if (row.values[colIndex] == selectStmt->whereValue) {
                            filteredRows.push_back(row);
                        }
                    }
                } else {
                    filteredRows = table->rows;
                }

                // Build output
                if (filteredRows.empty()) {
                    if (returnOutput) {
                        return "No rows returned";
                    } else {
                        std::cout << "No rows returned" << std::endl;
                    }
                } else {
                    // Header
                    output << Utils::join(table->columns, " | ") << "\n";
                    output << std::string(table->columns.size() * 4, '-') << "\n";

                    // Rows
                    for (const auto &row : filteredRows) {
                        output << Utils::join(row.values, " | ") << "\n";
                    }

                    output << "\n(" << filteredRows.size() << " row(s) returned)";

                    if (returnOutput) {
                        return output.str();
                    } else {
                        std::cout << output.str() << std::endl;
                    }
                }
                break;
            }
        }
    } catch (const std::exception &e) {
        if (returnOutput) {
            return "Error: " + std::string(e.what());
        } else {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    return "";
}
