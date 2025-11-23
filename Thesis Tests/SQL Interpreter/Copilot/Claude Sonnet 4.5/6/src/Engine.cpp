#include "Engine.h"
#include "Lexer.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>

Engine::Engine() {}

void Engine::repl() {
    std::cout << "MiniSQL Interactive Shell\n";
    std::cout << "Type .exit to quit\n\n";
    
    std::string input;
    std::string buffer;
    
    while (true) {
        if (buffer.empty()) {
            std::cout << "minisql> ";
        } else {
            std::cout << "      -> ";
        }
        
        if (!std::getline(std::cin, input)) {
            break; // EOF
        }
        
        input = Utils::trim(input);
        
        // Check for exit command
        if (input == ".exit" || input == ".quit") {
            break;
        }
        
        if (input.empty()) {
            continue;
        }
        
        // Accumulate input until we see a semicolon
        buffer += " " + input;
        
        if (input.back() == ';' || input.find(';') != std::string::npos) {
            try {
                executeStatement(buffer);
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << "\n";
            }
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
    
    std::string line;
    std::string buffer;
    
    while (std::getline(file, line)) {
        line = Utils::trim(line);
        
        if (line.empty() || line[0] == '#') {
            continue; // Skip empty lines and comments
        }
        
        buffer += " " + line;
        
        if (line.back() == ';' || line.find(';') != std::string::npos) {
            try {
                executeStatement(buffer);
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
            }
            buffer.clear();
        }
    }
    
    file.close();
}

std::string Engine::executeStatementWeb(const std::string& sql) {
    try {
        return executeStatementInternal(sql, true);
    } catch (const std::exception& e) {
        return "Error: " + std::string(e.what());
    }
}

void Engine::executeStatement(const std::string& sql) {
    std::string result = executeStatementInternal(sql, true);
    std::cout << result;
}

std::string Engine::executeStatementInternal(const std::string& sql, bool returnOutput) {
    std::string trimmedSql = Utils::trim(sql);
    if (trimmedSql.empty()) {
        return "";
    }
    
    // Tokenize
    Lexer lexer(trimmedSql);
    std::vector<Token> tokens = lexer.tokenize();
    
    // Parse
    Parser parser(tokens);
    std::unique_ptr<Statement> stmt = parser.parseStatement();
    
    // Execute based on statement type
    std::string result;
    
    switch (stmt->type()) {
        case StatementType::CREATE_TABLE:
            result = executeCreateTable(static_cast<CreateTableStatement*>(stmt.get()));
            break;
        case StatementType::INSERT:
            result = executeInsert(static_cast<InsertStatement*>(stmt.get()));
            break;
        case StatementType::SELECT:
            result = executeSelect(static_cast<SelectStatement*>(stmt.get()));
            break;
    }
    
    return result;
}

std::string Engine::executeCreateTable(CreateTableStatement* stmt) {
    if (storage_.createTable(stmt->tableName, stmt->columns)) {
        return "OK\n";
    } else {
        return "Error: " + storage_.getLastError() + "\n";
    }
}

std::string Engine::executeInsert(InsertStatement* stmt) {
    if (storage_.insertRow(stmt->tableName, stmt->values)) {
        return "OK\n";
    } else {
        return "Error: " + storage_.getLastError() + "\n";
    }
}

std::string Engine::executeSelect(SelectStatement* stmt) {
    const Table* table = storage_.getTable(stmt->tableName);
    
    if (!table) {
        return "Error: Table not found: " + stmt->tableName + "\n";
    }
    
    std::ostringstream result;
    
    // Print header
    for (size_t i = 0; i < table->columns.size(); ++i) {
        if (i > 0) result << " | ";
        result << table->columns[i];
    }
    result << "\n";
    
    // Print separator
    for (size_t i = 0; i < table->columns.size(); ++i) {
        if (i > 0) result << "-+-";
        for (size_t j = 0; j < table->columns[i].length(); ++j) {
            result << "-";
        }
    }
    result << "\n";
    
    // Filter and print rows
    size_t rowCount = 0;
    for (const auto& row : table->rows) {
        bool shouldPrint = true;
        
        // Apply WHERE filter if present
        if (stmt->hasWhere) {
            shouldPrint = false;
            
            // Find column index
            for (size_t i = 0; i < table->columns.size(); ++i) {
                if (table->columns[i] == stmt->whereColumn) {
                    if (row.values[i] == stmt->whereValue) {
                        shouldPrint = true;
                    }
                    break;
                }
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
    
    result << "\n(" << rowCount << " row(s) returned)\n";
    
    return result.str();
}
