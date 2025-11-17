#include "Engine.h"
#include "Lexer.h"
#include "Parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

Engine::Engine() {
    std::cout << "SQL Engine initialized" << std::endl;
}

void Engine::repl() {
    std::cout << "Minimal SQL Interpreter (type '.exit' to quit)" << std::endl;

    std::string input;
    while (true) {
        std::cout << "minisql> ";
        std::getline(std::cin, input);

        if (input == ".exit") {
            break;
        }

        if (!input.empty()) {
            executeStatement(input);
        }
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::string line;
    std::string currentCommand;

    while (std::getline(file, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start != std::string::npos) {
            size_t end = line.find_last_not_of(" \t");
            line = line.substr(start, end - start + 1);
        }

        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        // Check for exit command
        if (line == ".exit") {
            break;
        }

        // Check if line ends with semicolon
        if (line.back() == ';') {
            // Remove semicolon and execute
            line.pop_back();
            executeStatement(line);
        } else {
            // Accumulate multi-line commands
            if (!currentCommand.empty()) {
                currentCommand += " " + line;
            } else {
                currentCommand = line;
            }
        }
    }

    // Execute any remaining command
    if (!currentCommand.empty()) {
        executeStatement(currentCommand);
    }
}

void Engine::executeStatement(const std::string &sql) {
    executeStatementInternal(sql, false);
}

std::string Engine::executeStatementInternal(const std::string &sql, bool returnOutput) {
    std::ostringstream output;

    try {
        // Make a copy of the SQL command
        std::string sqlCopy = sql;

        // Remove semicolon if present
        if (!sqlCopy.empty() && sqlCopy.back() == ';') {
            sqlCopy.pop_back();
        }

        // Trim whitespace from the entire command
        size_t start = sqlCopy.find_first_not_of(" \t");
        if (start != std::string::npos) {
            size_t end = sqlCopy.find_last_not_of(" \t");
            sqlCopy = sqlCopy.substr(start, end - start + 1);
        }

        // Check for exit command
        if (sqlCopy == ".exit") {
            return returnOutput ? "" : (std::cout << "Exiting..." << std::endl, "");
        }

        // Use Lexer and Parser to process the SQL command
        Lexer lexer(sqlCopy);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(tokens);
        std::unique_ptr<Statement> stmt = parser.parse();

        if (!stmt) {
            output << "Error: Failed to parse SQL command" << std::endl;
        } else {
            switch (stmt->type()) {
                case StatementType::CREATE_TABLE: {
                    auto createStmt = dynamic_cast<CreateTableStatement*>(stmt.get());
                    if (storage_.createTable(createStmt->tableName, createStmt->columns)) {
                        output << "OK" << std::endl;
                    } else {
                        output << "Error: " << storage_.getLastError() << std::endl;
                    }
                    break;
                }
                case StatementType::INSERT: {
                    auto insertStmt = dynamic_cast<InsertStatement*>(stmt.get());
                    if (storage_.insertRow(insertStmt->tableName, insertStmt->values)) {
                        output << "OK" << std::endl;
                    } else {
                        output << "Error: " << storage_.getLastError() << std::endl;
                    }
                    break;
                }
                case StatementType::SELECT: {
                    auto selectStmt = dynamic_cast<SelectStatement*>(stmt.get());
                    const Table* table = storage_.getTable(selectStmt->tableName);
                    if (!table) {
                        output << "Error: Table not found: " << selectStmt->tableName << std::endl;
                    } else {
                        // Print header
                        for (size_t i = 0; i < table->columns.size(); ++i) {
                            if (i > 0) output << " | ";
                            output << table->columns[i];
                        }
                        output << std::endl;

                        // Print separator
                        for (size_t i = 0; i < table->columns.size(); ++i) {
                            if (i > 0) output << "-+-";
                            output << "---";
                        }
                        output << std::endl;

                        // Print rows
                        for (const auto& row : table->rows) {
                            for (size_t i = 0; i < row.values.size(); ++i) {
                                if (i > 0) output << " | ";
                                output << row.values[i];
                            }
                            output << std::endl;
                        }

                        output << "(" << table->rows.size() << " row(s) returned)" << std::endl;
                    }
                    break;
                }
                default:
                    output << "Error: Unsupported SQL command" << std::endl;
            }
        }
    } catch (const std::exception& e) {
        output << "Error: " << e.what() << std::endl;
    }

    if (returnOutput) {
        return output.str();
    } else {
        std::cout << output.str();
        return "";
    }
}

std::string Engine::executeStatementWeb(const std::string &sql) {
    return executeStatementInternal(sql, true);
}
