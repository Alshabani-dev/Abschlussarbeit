#include "Engine.h"
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
            currentCommand += line;
            if (!currentCommand.empty() && currentCommand.back() == ';') {
                currentCommand.pop_back();
                executeStatement(currentCommand);
                currentCommand.clear();
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

        // Simple parsing for basic SQL commands
        if (sqlCopy.find("CREATE TABLE") == 0) {
            // Extract table name and columns
            size_t start = sqlCopy.find("(");
            size_t end = sqlCopy.find(")");
            if (start == std::string::npos || end == std::string::npos) {
                output << "Error: Invalid CREATE TABLE syntax" << std::endl;
            } else {
                // Extract table name
                std::string tableName = sqlCopy.substr(12, start - 12);
                tableName.erase(std::remove_if(tableName.begin(), tableName.end(), ::isspace), tableName.end());

                // Extract columns
                std::string columnsPart = sqlCopy.substr(start + 1, end - start - 1);

                // Split columns
                std::vector<std::string> columns;
                size_t pos = 0;
                while (pos < columnsPart.length()) {
                    // Skip whitespace
                    while (pos < columnsPart.length() && isspace(columnsPart[pos])) {
                        pos++;
                    }

                    if (pos >= columnsPart.length()) break;

                    // Find comma or end of string
                    size_t commaPos = columnsPart.find(",", pos);
                    if (commaPos == std::string::npos) {
                        commaPos = columnsPart.length();
                    }

                    // Extract column name
                    std::string col = columnsPart.substr(pos, commaPos - pos);
                    col.erase(std::remove_if(col.begin(), col.end(), ::isspace), col.end());
                    if (!col.empty()) {
                        columns.push_back(col);
                    }

                    pos = commaPos + 1;
                }

                // Create table
                if (storage_.createTable(tableName, columns)) {
                    output << "OK" << std::endl;
                } else {
                    output << "Error: " << storage_.getLastError() << std::endl;
                }
            }
        }
        else if (sqlCopy.find("INSERT INTO") == 0) {
            // Extract table name and values
            size_t intoPos = sqlCopy.find("INTO");
            size_t valuesPos = sqlCopy.find("VALUES");
            if (intoPos == std::string::npos || valuesPos == std::string::npos) {
                output << "Error: Invalid INSERT syntax" << std::endl;
            } else {
                // Extract table name
                std::string tableName = sqlCopy.substr(11, intoPos - 11);
                tableName.erase(std::remove_if(tableName.begin(), tableName.end(), ::isspace), tableName.end());

                // Extract values
                std::string valuesPart = sqlCopy.substr(valuesPos + 6);
                size_t parenStart = valuesPart.find("(");
                size_t parenEnd = valuesPart.find(")");
                if (parenStart == std::string::npos || parenEnd == std::string::npos) {
                    output << "Error: Invalid VALUES syntax" << std::endl;
                } else {
                    valuesPart = valuesPart.substr(parenStart + 1, parenEnd - parenStart - 1);

                    // Split values
                    std::vector<std::string> values;
                    size_t pos = 0;
                    while (pos < valuesPart.length()) {
                        // Skip whitespace
                        while (pos < valuesPart.length() && isspace(valuesPart[pos])) {
                            pos++;
                        }

                        if (pos >= valuesPart.length()) break;

                        // Find comma or end of string
                        size_t commaPos = valuesPart.find(",", pos);
                        if (commaPos == std::string::npos) {
                            commaPos = valuesPart.length();
                        }

                        // Extract value
                        std::string val = valuesPart.substr(pos, commaPos - pos);
                        // Trim whitespace from value
                        size_t valStart = val.find_first_not_of(" \t");
                        size_t valEnd = val.find_last_not_of(" \t");
                        if (valStart != std::string::npos) {
                            val = val.substr(valStart, valEnd - valStart + 1);
                            values.push_back(val);
                        }

                        pos = commaPos + 1;
                    }

                    // Insert row
                    if (storage_.insertRow(tableName, values)) {
                        output << "OK" << std::endl;
                    } else {
                        output << "Error: " << storage_.getLastError() << std::endl;
                    }
                }
            }
        }
        else if (sqlCopy.find("SELECT") == 0) {
            // Extract table name and optional WHERE clause
            size_t fromPos = sqlCopy.find("FROM");
            if (fromPos == std::string::npos) {
                output << "Error: Invalid SELECT syntax" << std::endl;
            } else {
                // Extract table name
                std::string tablePart = sqlCopy.substr(fromPos + 4);
                size_t wherePos = tablePart.find("WHERE");
                if (wherePos != std::string::npos) {
                    tablePart = tablePart.substr(0, wherePos);
                }

                tablePart.erase(std::remove_if(tablePart.begin(), tablePart.end(), ::isspace), tablePart.end());

                const Table* table = storage_.getTable(tablePart);
                if (!table) {
                    output << "Error: Table not found: " << tablePart << std::endl;
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
            }
        }
        else {
            output << "Error: Unsupported SQL command" << std::endl;
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
