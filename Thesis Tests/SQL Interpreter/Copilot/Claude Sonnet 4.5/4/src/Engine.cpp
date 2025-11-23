#include "Engine.h"
#include "Lexer.h"
#include "Parser.h"
#include "Ast.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>

Engine::Engine() {}

void Engine::repl() {
    std::cout << "minisql> Interactive mode. Type .exit to quit.\n";
    
    std::string buffer;
    
    while (true) {
        std::cout << "minisql> ";
        std::string line;
        
        if (!std::getline(std::cin, line)) {
            break; // EOF
        }
        
        // Check for special commands
        std::string trimmed = Utils::trim(line);
        if (trimmed == ".exit" || trimmed == ".quit") {
            break;
        }
        
        if (trimmed.empty()) {
            continue;
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
        std::cerr << "Error: Could not open file '" << filename << "'\n";
        return;
    }
    
    std::string buffer;
    std::string line;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        std::string trimmed = Utils::trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed.substr(0, 2) == "--") {
            continue;
        }
        
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
    std::ostringstream output;
    
    // Tokenize
    Lexer lexer(sql);
    std::vector<Token> tokens = lexer.tokenize();
    
    if (!lexer.getError().empty()) {
        std::string error = "Lexer error: " + lexer.getError();
        if (returnOutput) {
            return error;
        } else {
            std::cerr << error << "\n";
            return "";
        }
    }
    
    // Parse
    Parser parser(tokens);
    auto stmt = parser.parseStatement();
    
    if (!stmt) {
        std::string error = "Parser error: " + parser.getError();
        if (returnOutput) {
            return error;
        } else {
            std::cerr << error << "\n";
            return "";
        }
    }
    
    // Execute based on statement type
    switch (stmt->type()) {
        case StatementType::CREATE_TABLE: {
            auto* createStmt = static_cast<CreateTableStatement*>(stmt.get());
            if (storage_.createTable(createStmt->tableName, createStmt->columns)) {
                output << "OK";
            } else {
                output << "Error: " << storage_.getLastError();
            }
            break;
        }
        
        case StatementType::INSERT: {
            auto* insertStmt = static_cast<InsertStatement*>(stmt.get());
            if (storage_.insertRow(insertStmt->tableName, insertStmt->values)) {
                output << "OK";
            } else {
                output << "Error: " << storage_.getLastError();
            }
            break;
        }
        
        case StatementType::SELECT: {
            auto* selectStmt = static_cast<SelectStatement*>(stmt.get());
            const Table* table = storage_.getTable(selectStmt->tableName);
            
            if (!table) {
                output << "Error: Table '" << selectStmt->tableName << "' does not exist";
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
                for (size_t j = 0; j < table->columns[i].length(); ++j) {
                    output << "-";
                }
            }
            output << "\n";
            
            // Filter and print rows
            size_t rowCount = 0;
            for (const Row& row : table->rows) {
                bool includeRow = true;
                
                if (selectStmt->hasWhere) {
                    // Find column index
                    auto it = std::find(table->columns.begin(), table->columns.end(), 
                                       selectStmt->whereColumn);
                    if (it == table->columns.end()) {
                        output.str("");
                        output << "Error: Column '" << selectStmt->whereColumn << "' does not exist";
                        if (returnOutput) {
                            return output.str();
                        } else {
                            std::cerr << output.str() << "\n";
                            return "";
                        }
                    }
                    
                    size_t colIndex = std::distance(table->columns.begin(), it);
                    if (row.values[colIndex] != selectStmt->whereValue) {
                        includeRow = false;
                    }
                }
                
                if (includeRow) {
                    for (size_t i = 0; i < row.values.size(); ++i) {
                        if (i > 0) output << " | ";
                        output << row.values[i];
                    }
                    output << "\n";
                    rowCount++;
                }
            }
            
            output << "\n(" << rowCount << " row(s) returned)";
            break;
        }
    }
    
    if (returnOutput) {
        return output.str();
    } else {
        std::cout << output.str() << "\n";
        return "";
    }
}
