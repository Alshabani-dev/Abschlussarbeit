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
    std::ostringstream output;

    try {
        Lexer lexer(sql);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        std::unique_ptr<Statement> stmt = parser.parse();

        switch (stmt->type()) {
            case StatementType::CREATE_TABLE: {
                auto createStmt = dynamic_cast<CreateTableStatement*>(stmt.get());
                if (storage_.createTable(createStmt->tableName, createStmt->columns)) {
                    output << "OK\n";
                } else {
                    output << "Error: " << storage_.getLastError() << "\n";
                }
                break;
            }
            case StatementType::INSERT: {
                auto insertStmt = dynamic_cast<InsertStatement*>(stmt.get());
                if (storage_.insertRow(insertStmt->tableName, insertStmt->values)) {
                    output << "OK\n";
                } else {
                    output << "Error: " << storage_.getLastError() << "\n";
                }
                break;
            }
            case StatementType::SELECT: {
                auto selectStmt = dynamic_cast<SelectStatement*>(stmt.get());
                const Table* table = storage_.getTable(selectStmt->tableName);
                if (!table) {
                    output << "Error: Table not found: " << selectStmt->tableName << "\n";
                    break;
                }

                // Print header
                for (size_t i = 0; i < table->columns.size(); i++) {
                    if (i > 0) output << " | ";
                    output << table->columns[i];
                }
                output << "\n";

                // Print separator
                for (size_t i = 0; i < table->columns.size(); i++) {
                    if (i > 0) output << "-+-";
                    output << "---";
                }
                output << "\n";

                // Filter and print rows
                int rowCount = 0;
                for (const auto& row : table->rows) {
                    bool matches = true;

                    if (selectStmt->hasWhere) {
                        // Find column index
                        int colIndex = -1;
                        for (size_t i = 0; i < table->columns.size(); i++) {
                            if (Utils::toLower(table->columns[i]) == Utils::toLower(selectStmt->whereColumn)) {
                                colIndex = i;
                                break;
                            }
                        }

                        if (colIndex >= 0) {
                            matches = (row.values[colIndex] == selectStmt->whereValue);
                        } else {
                            matches = false;
                        }
                    }

                    if (matches) {
                        for (size_t i = 0; i < row.values.size(); i++) {
                            if (i > 0) output << " | ";
                            output << row.values[i];
                        }
                        output << "\n";
                        rowCount++;
                    }
                }

                output << "\n(" << rowCount << " row(s) returned)\n";
                break;
            }
        }
    } catch (const std::exception& e) {
        output << "Error: " << e.what() << "\n";
    }

    if (returnOutput) {
        return output.str();
    } else {
        std::cout << output.str();
        return "";
    }
}
