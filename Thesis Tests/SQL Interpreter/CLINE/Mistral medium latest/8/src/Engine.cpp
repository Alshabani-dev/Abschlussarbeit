#include "Engine.h"
#include "Lexer.h"
#include "Parser.h"
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
    while (std::getline(file, line)) {
        if (!line.empty()) {
            try {
                executeStatement(line);
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
        // Tokenize
        Lexer lexer(sql);
        std::vector<Token> tokens = lexer.tokenize();

        // Parse
        Parser parser(tokens);
        std::unique_ptr<Statement> stmt = parser.parse();

        if (!stmt) {
            if (returnOutput) {
                return "Empty statement";
            }
            return "";
        }

        // Execute
        switch (stmt->type()) {
            case StatementType::CREATE_TABLE: {
                auto createStmt = dynamic_cast<CreateTableStatement*>(stmt.get());
                bool success = storage_.createTable(createStmt->tableName, createStmt->columns);
                if (success) {
                    if (returnOutput) {
                        return "OK";
                    }
                    std::cout << "OK" << std::endl;
                } else {
                    std::string error = storage_.getLastError();
                    if (returnOutput) {
                        return "Error: " + error;
                    }
                    std::cerr << "Error: " << error << std::endl;
                }
                break;
            }
            case StatementType::INSERT: {
                auto insertStmt = dynamic_cast<InsertStatement*>(stmt.get());
                bool success = storage_.insertRow(insertStmt->tableName, insertStmt->values);
                if (success) {
                    if (returnOutput) {
                        return "OK";
                    }
                    std::cout << "OK" << std::endl;
                } else {
                    std::string error = storage_.getLastError();
                    if (returnOutput) {
                        return "Error: " + error;
                    }
                    std::cerr << "Error: " << error << std::endl;
                }
                break;
            }
            case StatementType::SELECT: {
                auto selectStmt = dynamic_cast<SelectStatement*>(stmt.get());
                const Table* table = storage_.getTable(selectStmt->tableName);

                if (!table) {
                    std::string error = "Table not found: " + selectStmt->tableName;
                    if (returnOutput) {
                        return "Error: " + error;
                    }
                    std::cerr << "Error: " << error << std::endl;
                    break;
                }

                // Filter rows if WHERE clause exists
                std::vector<Row> filteredRows;
                if (selectStmt->hasWhere) {
                    // Find column index
                    int colIndex = -1;
                    for (size_t i = 0; i < table->columns.size(); i++) {
                        if (Utils::toLower(table->columns[i]) == Utils::toLower(selectStmt->whereColumn)) {
                            colIndex = i;
                            break;
                        }
                    }

                    if (colIndex == -1) {
                        std::string error = "Column not found: " + selectStmt->whereColumn;
                        if (returnOutput) {
                            return "Error: " + error;
                        }
                        std::cerr << "Error: " << error << std::endl;
                        break;
                    }

                    // Filter rows
                    for (const auto& row : table->rows) {
                        if (row.values[colIndex] == selectStmt->whereValue) {
                            filteredRows.push_back(row);
                        }
                    }
                } else {
                    filteredRows = table->rows;
                }

                // Print results
                if (returnOutput) {
                    output << "| ";
                    for (size_t i = 0; i < table->columns.size(); i++) {
                        output << table->columns[i];
                        if (i < table->columns.size() - 1) {
                            output << " | ";
                        }
                    }
                    output << " |\n";

                    for (const auto& row : filteredRows) {
                        output << "| ";
                        for (size_t i = 0; i < row.values.size(); i++) {
                            output << row.values[i];
                            if (i < row.values.size() - 1) {
                                output << " | ";
                            }
                        }
                        output << " |\n";
                    }

                    output << "(" << filteredRows.size() << " row(s) returned)";
                    return output.str();
                } else {
                    // Print header
                    std::cout << "| ";
                    for (size_t i = 0; i < table->columns.size(); i++) {
                        std::cout << table->columns[i];
                        if (i < table->columns.size() - 1) {
                            std::cout << " | ";
                        }
                    }
                    std::cout << " |\n";

                    // Print rows
                    for (const auto& row : filteredRows) {
                        std::cout << "| ";
                        for (size_t i = 0; i < row.values.size(); i++) {
                            std::cout << row.values[i];
                            if (i < row.values.size() - 1) {
                                std::cout << " | ";
                            }
                        }
                        std::cout << " |\n";
                    }

                    std::cout << "(" << filteredRows.size() << " row(s) returned)" << std::endl;
                }
                break;
            }
        }
    } catch (const std::exception& e) {
        std::string error = e.what();
        if (returnOutput) {
            return "Error: " + error;
        }
        std::cerr << "Error: " << error << std::endl;
    }

    return "";
}
