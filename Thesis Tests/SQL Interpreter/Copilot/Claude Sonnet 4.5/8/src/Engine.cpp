#include "Engine.h"
#include "Lexer.h"
#include "Ast.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

Engine::Engine() {}

void Engine::repl() {
    std::cout << "minisql> ";
    std::string line;
    std::string buffer;
    
    while (std::getline(std::cin, line)) {
        line = Utils::trim(line);
        
        // Check for exit command
        if (line == ".exit" || line == "exit" || line == "quit") {
            break;
        }
        
        if (line.empty()) {
            std::cout << "minisql> ";
            continue;
        }
        
        // Accumulate input until we see a semicolon
        buffer += line + " ";
        
        if (line.back() == ';') {
            executeStatement(buffer);
            buffer.clear();
        }
        
        std::cout << "minisql> ";
    }
}

void Engine::executeScript(const std::string& filename) {
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file '" << filename << "'" << std::endl;
        return;
    }
    
    std::string line;
    std::string buffer;
    
    while (std::getline(file, line)) {
        line = Utils::trim(line);
        
        if (line.empty() || line[0] == '#') {
            continue; // Skip empty lines and comments
        }
        
        buffer += line + " ";
        
        if (line.back() == ';') {
            executeStatement(buffer);
            buffer.clear();
        }
    }
    
    // Execute any remaining statement
    if (!buffer.empty()) {
        executeStatement(buffer);
    }
    
    file.close();
}

std::string Engine::executeStatementWeb(const std::string& sql) {
    return executeStatementInternal(sql, true);
}

void Engine::executeStatement(const std::string& sql) {
    executeStatementInternal(sql, false);
}

std::string Engine::executeStatementInternal(const std::string& sql, bool returnOutput) {
    std::string trimmedSql = Utils::trim(sql);
    
    if (trimmedSql.empty()) {
        return returnOutput ? "" : "";
    }
    
    // Tokenize
    Lexer lexer(trimmedSql);
    std::vector<Token> tokens = lexer.tokenize();
    
    // Parse
    Parser parser(tokens);
    auto stmt = parser.parseStatement();
    
    if (!stmt) {
        std::string error = "Parse error: " + parser.getError();
        if (returnOutput) {
            return "Error: " + error;
        } else {
            std::cerr << error << std::endl;
            return "";
        }
    }
    
    // Execute based on statement type
    if (stmt->type() == StatementType::CREATE_TABLE) {
        auto createStmt = dynamic_cast<CreateTableStatement*>(stmt.get());
        if (storage_.createTable(createStmt->tableName, createStmt->columns)) {
            std::string result = "OK";
            if (returnOutput) {
                return result;
            } else {
                std::cout << result << std::endl;
            }
        } else {
            std::string error = "Error: " + storage_.getLastError();
            if (returnOutput) {
                return error;
            } else {
                std::cerr << error << std::endl;
            }
        }
    } else if (stmt->type() == StatementType::INSERT) {
        auto insertStmt = dynamic_cast<InsertStatement*>(stmt.get());
        if (storage_.insertRow(insertStmt->tableName, insertStmt->values)) {
            std::string result = "OK";
            if (returnOutput) {
                return result;
            } else {
                std::cout << result << std::endl;
            }
        } else {
            std::string error = "Error: " + storage_.getLastError();
            if (returnOutput) {
                return error;
            } else {
                std::cerr << error << std::endl;
            }
        }
    } else if (stmt->type() == StatementType::SELECT) {
        auto selectStmt = dynamic_cast<SelectStatement*>(stmt.get());
        const Table* table = storage_.getTable(selectStmt->tableName);
        
        if (!table) {
            std::string error = "Error: Table '" + selectStmt->tableName + "' does not exist";
            if (returnOutput) {
                return error;
            } else {
                std::cerr << error << std::endl;
            }
        } else {
            std::string result = formatSelectResult(table, selectStmt->whereColumn, 
                                                    selectStmt->whereValue, selectStmt->hasWhere);
            if (returnOutput) {
                return result;
            } else {
                std::cout << result << std::endl;
            }
        }
    }
    
    return "";
}

std::string Engine::formatSelectResult(const Table* table, const std::string& whereColumn, 
                                       const std::string& whereValue, bool hasWhere) {
    std::ostringstream result;
    
    // Find column index for WHERE clause
    int whereColIndex = -1;
    if (hasWhere) {
        for (size_t i = 0; i < table->columns.size(); ++i) {
            if (table->columns[i] == whereColumn) {
                whereColIndex = static_cast<int>(i);
                break;
            }
        }
        
        if (whereColIndex == -1) {
            return "Error: Column '" + whereColumn + "' does not exist";
        }
    }
    
    // Print header
    for (size_t i = 0; i < table->columns.size(); ++i) {
        if (i > 0) result << " | ";
        result << table->columns[i];
    }
    result << "\n";
    
    // Print separator
    for (size_t i = 0; i < table->columns.size(); ++i) {
        if (i > 0) result << "-+-";
        result << std::string(table->columns[i].size(), '-');
    }
    result << "\n";
    
    // Print rows (with filtering if WHERE clause)
    int rowCount = 0;
    for (const auto& row : table->rows) {
        bool shouldPrint = true;
        
        if (hasWhere) {
            if (whereColIndex >= 0 && whereColIndex < static_cast<int>(row.values.size())) {
                shouldPrint = (row.values[whereColIndex] == whereValue);
            } else {
                shouldPrint = false;
            }
        }
        
        if (shouldPrint) {
            for (size_t i = 0; i < row.values.size(); ++i) {
                if (i > 0) result << " | ";
                result << row.values[i];
            }
            result << "\n";
            rowCount++;
        }
    }
    
    result << "\n(" << rowCount << " row(s) returned)";
    
    return result.str();
}
