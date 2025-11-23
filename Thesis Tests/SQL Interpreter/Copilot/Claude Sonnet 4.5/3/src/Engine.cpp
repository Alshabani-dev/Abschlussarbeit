#include "Engine.h"
#include "Lexer.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>

Engine::Engine() : storage_() {}

void Engine::repl() {
    std::cout << "MinSQL Interpreter v1.0\n";
    std::cout << "Type SQL commands or .exit to quit\n\n";
    
    std::string buffer;
    
    while (true) {
        std::cout << "minisql> ";
        std::string line;
        
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        // Check for exit command
        std::string trimmed = Utils::trim(line);
        if (trimmed == ".exit" || trimmed == ".quit") {
            break;
        }
        
        // Accumulate lines until we see a semicolon
        buffer += line + " ";
        
        if (line.find(';') != std::string::npos) {
            executeStatement(buffer);
            buffer.clear();
        }
    }
    
    std::cout << "Goodbye!\n";
}

void Engine::executeScript(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: " << filename << "\n";
        return;
    }
    
    std::string buffer;
    std::string line;
    
    while (std::getline(file, line)) {
        buffer += line + " ";
        
        if (line.find(';') != std::string::npos) {
            executeStatement(buffer);
            buffer.clear();
        }
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
    std::string trimmed = Utils::trim(sql);
    if (trimmed.empty()) {
        return "";
    }
    
    try {
        // Tokenize
        Lexer lexer(trimmed);
        std::vector<Token> tokens = lexer.tokenize();
        
        // Parse
        Parser parser(tokens);
        std::unique_ptr<Statement> stmt = parser.parseStatement();
        
        // Execute based on statement type
        if (stmt->type() == StatementType::CREATE_TABLE) {
            auto* createStmt = static_cast<CreateTableStatement*>(stmt.get());
            
            if (storage_.createTable(createStmt->tableName, createStmt->columns)) {
                std::string msg = "OK";
                if (!returnOutput) {
                    std::cout << msg << "\n";
                }
                return msg;
            } else {
                std::string msg = "Error: " + storage_.getLastError();
                if (!returnOutput) {
                    std::cerr << msg << "\n";
                }
                return msg;
            }
        }
        else if (stmt->type() == StatementType::INSERT) {
            auto* insertStmt = static_cast<InsertStatement*>(stmt.get());
            
            if (storage_.insertRow(insertStmt->tableName, insertStmt->values)) {
                std::string msg = "OK";
                if (!returnOutput) {
                    std::cout << msg << "\n";
                }
                return msg;
            } else {
                std::string msg = "Error: " + storage_.getLastError();
                if (!returnOutput) {
                    std::cerr << msg << "\n";
                }
                return msg;
            }
        }
        else if (stmt->type() == StatementType::SELECT) {
            auto* selectStmt = static_cast<SelectStatement*>(stmt.get());
            
            const Table* table = storage_.getTable(selectStmt->tableName);
            if (!table) {
                std::string msg = "Error: Table '" + selectStmt->tableName + "' does not exist";
                if (!returnOutput) {
                    std::cerr << msg << "\n";
                }
                return msg;
            }
            
            std::string result = formatSelectResults(table, selectStmt);
            if (!returnOutput) {
                std::cout << result;
            }
            return result;
        }
        
    } catch (const std::exception& e) {
        std::string msg = std::string("Error: ") + e.what();
        if (!returnOutput) {
            std::cerr << msg << "\n";
        }
        return msg;
    }
    
    return "";
}

std::string Engine::formatSelectResults(const Table* table, const SelectStatement* stmt) {
    std::ostringstream oss;
    
    // Filter rows if WHERE clause exists
    std::vector<const Row*> matchingRows;
    
    if (stmt->hasWhere) {
        // Find column index
        int colIndex = -1;
        for (size_t i = 0; i < table->columns.size(); ++i) {
            if (Utils::toLower(table->columns[i]) == Utils::toLower(stmt->whereColumn)) {
                colIndex = static_cast<int>(i);
                break;
            }
        }
        
        if (colIndex == -1) {
            return "Error: Column '" + stmt->whereColumn + "' not found in table\n";
        }
        
        // Filter rows
        for (const auto& row : table->rows) {
            if (row.values[colIndex] == stmt->whereValue) {
                matchingRows.push_back(&row);
            }
        }
    } else {
        // No WHERE clause, include all rows
        for (const auto& row : table->rows) {
            matchingRows.push_back(&row);
        }
    }
    
    // Print header
    for (size_t i = 0; i < table->columns.size(); ++i) {
        if (i > 0) oss << " | ";
        oss << table->columns[i];
    }
    oss << "\n";
    
    // Print separator
    for (size_t i = 0; i < table->columns.size(); ++i) {
        if (i > 0) oss << "-+-";
        for (size_t j = 0; j < table->columns[i].length(); ++j) {
            oss << "-";
        }
    }
    oss << "\n";
    
    // Print matching rows
    for (const Row* row : matchingRows) {
        for (size_t i = 0; i < row->values.size(); ++i) {
            if (i > 0) oss << " | ";
            oss << row->values[i];
        }
        oss << "\n";
    }
    
    oss << "\n(" << matchingRows.size() << " row(s) returned)\n";
    
    return oss.str();
}
