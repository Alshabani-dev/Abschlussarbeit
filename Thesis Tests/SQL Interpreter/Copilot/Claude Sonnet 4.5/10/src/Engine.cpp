#include "Engine.h"
#include "Lexer.h"
#include "Ast.h"
#include <iostream>
#include <fstream>
#include <sstream>

Engine::Engine() {}

void Engine::repl() {
    std::cout << "minisql> ";
    std::string line;
    std::string buffer;
    
    while (std::getline(std::cin, line)) {
        // Check for exit command
        if (line == ".exit" || line == "exit" || line == "quit") {
            break;
        }
        
        // Accumulate lines until we see a semicolon
        buffer += line + " ";
        
        // Check if buffer contains semicolon
        if (buffer.find(';') != std::string::npos) {
            executeStatement(buffer);
            buffer.clear();
        }
        
        std::cout << "minisql> ";
    }
}

void Engine::executeScript(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filename << "'" << std::endl;
        return;
    }
    
    std::string line;
    std::string buffer;
    
    while (std::getline(file, line)) {
        buffer += line + " ";
        
        // Execute when we find a semicolon
        if (buffer.find(';') != std::string::npos) {
            executeStatement(buffer);
            buffer.clear();
        }
    }
    
    // Execute any remaining buffer
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
    std::ostringstream output;
    
    // Tokenize
    Lexer lexer(sql);
    std::vector<Token> tokens = lexer.tokenize();
    
    // Parse
    Parser parser(tokens);
    std::unique_ptr<Statement> stmt = parser.parse();
    
    if (!stmt) {
        std::string error = "Parse error: " + parser.getError();
        if (returnOutput) {
            return error;
        } else {
            std::cout << error << std::endl;
            return "";
        }
    }
    
    // Execute based on statement type
    if (stmt->type() == StatementType::CREATE_TABLE) {
        auto* createStmt = static_cast<CreateTableStatement*>(stmt.get());
        
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
                std::cout << error << std::endl;
            }
        }
    }
    else if (stmt->type() == StatementType::INSERT) {
        auto* insertStmt = static_cast<InsertStatement*>(stmt.get());
        
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
                std::cout << error << std::endl;
            }
        }
    }
    else if (stmt->type() == StatementType::SELECT) {
        auto* selectStmt = static_cast<SelectStatement*>(stmt.get());
        
        const Table* table = storage_.getTable(selectStmt->tableName);
        if (!table) {
            std::string error = "Error: Table '" + selectStmt->tableName + "' does not exist";
            if (returnOutput) {
                return error;
            } else {
                std::cout << error << std::endl;
            }
        } else {
            std::string result = formatSelectResult(table, selectStmt);
            if (returnOutput) {
                return result;
            } else {
                std::cout << result;
            }
        }
    }
    
    return "";
}

std::string Engine::formatSelectResult(const Table* table, const SelectStatement* selectStmt) {
    std::ostringstream output;
    
    // Find matching rows
    std::vector<const Row*> matchingRows;
    
    if (!selectStmt->hasWhere) {
        // No WHERE clause - return all rows
        for (const auto& row : table->rows) {
            matchingRows.push_back(&row);
        }
    } else {
        // Find column index for WHERE clause
        int columnIndex = -1;
        for (size_t i = 0; i < table->columns.size(); i++) {
            if (table->columns[i] == selectStmt->whereColumn) {
                columnIndex = static_cast<int>(i);
                break;
            }
        }
        
        if (columnIndex == -1) {
            return "Error: Column '" + selectStmt->whereColumn + "' does not exist in table '" + selectStmt->tableName + "'";
        }
        
        // Filter rows
        for (const auto& row : table->rows) {
            if (row.values[columnIndex] == selectStmt->whereValue) {
                matchingRows.push_back(&row);
            }
        }
    }
    
    // Print header
    for (size_t i = 0; i < table->columns.size(); i++) {
        if (i > 0) {
            output << " | ";
        }
        output << table->columns[i];
    }
    output << "\n";
    
    // Print separator
    for (size_t i = 0; i < table->columns.size(); i++) {
        if (i > 0) {
            output << "-+-";
        }
        output << std::string(table->columns[i].length(), '-');
    }
    output << "\n";
    
    // Print rows
    for (const auto* row : matchingRows) {
        for (size_t i = 0; i < row->values.size(); i++) {
            if (i > 0) {
                output << " | ";
            }
            output << row->values[i];
        }
        output << "\n";
    }
    
    // Print summary
    output << "\n(" << matchingRows.size() << " row(s) returned)\n";
    
    return output.str();
}
