#include "Engine.h"
#include "Lexer.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>

Engine::Engine() : storage_() {}

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
        
        // Skip empty lines
        if (line.empty()) {
            std::cout << "minisql> ";
            continue;
        }
        
        // Accumulate input until we see a semicolon
        buffer += line + " ";
        
        if (line.back() == ';') {
            try {
                executeStatement(buffer);
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << std::endl;
            }
            buffer.clear();
        }
        
        std::cout << "minisql> ";
    }
}

void Engine::executeScript(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        return;
    }
    
    std::string buffer;
    std::string line;
    
    while (std::getline(file, line)) {
        line = Utils::trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line.substr(0, 2) == "--") {
            continue;
        }
        
        buffer += line + " ";
        
        if (line.back() == ';') {
            try {
                executeStatement(buffer);
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
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
        return std::string("Error: ") + e.what();
    }
}

void Engine::executeStatement(const std::string& sql) {
    std::string output = executeStatementInternal(sql, false);
    if (!output.empty()) {
        std::cout << output << std::endl;
    }
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
    if (stmt->type() == StatementType::CREATE_TABLE) {
        auto createStmt = dynamic_cast<CreateTableStatement*>(stmt.get());
        if (storage_.createTable(createStmt->tableName, createStmt->columns)) {
            std::string result = "OK";
            if (!returnOutput) {
                std::cout << result << std::endl;
            }
            return result;
        } else {
            std::string error = "Error: " + storage_.getLastError();
            if (!returnOutput) {
                std::cout << error << std::endl;
            }
            return error;
        }
    }
    else if (stmt->type() == StatementType::INSERT) {
        auto insertStmt = dynamic_cast<InsertStatement*>(stmt.get());
        if (storage_.insertRow(insertStmt->tableName, insertStmt->values)) {
            std::string result = "OK";
            if (!returnOutput) {
                std::cout << result << std::endl;
            }
            return result;
        } else {
            std::string error = "Error: " + storage_.getLastError();
            if (!returnOutput) {
                std::cout << error << std::endl;
            }
            return error;
        }
    }
    else if (stmt->type() == StatementType::SELECT) {
        auto selectStmt = dynamic_cast<SelectStatement*>(stmt.get());
        const Table* table = storage_.getTable(selectStmt->tableName);
        
        if (!table) {
            std::string error = "Error: Table '" + selectStmt->tableName + "' does not exist";
            if (!returnOutput) {
                std::cout << error << std::endl;
            }
            return error;
        }
        
        return formatSelectResult(table, selectStmt);
    }
    
    return "";
}

std::string Engine::formatSelectResult(const Table* table, const SelectStatement* stmt) {
    std::ostringstream output;
    
    // Print header
    for (size_t i = 0; i < table->columns.size(); ++i) {
        output << table->columns[i];
        if (i < table->columns.size() - 1) {
            output << " | ";
        }
    }
    output << "\n";
    
    // Print separator
    for (size_t i = 0; i < table->columns.size(); ++i) {
        for (size_t j = 0; j < table->columns[i].size(); ++j) {
            output << "-";
        }
        if (i < table->columns.size() - 1) {
            output << "-+-";
        }
    }
    output << "\n";
    
    // Filter and print rows
    int rowCount = 0;
    for (const auto& row : table->rows) {
        bool includeRow = true;
        
        if (stmt->hasWhere) {
            // Find column index
            int columnIndex = -1;
            for (size_t i = 0; i < table->columns.size(); ++i) {
                if (table->columns[i] == stmt->whereColumn) {
                    columnIndex = i;
                    break;
                }
            }
            
            if (columnIndex == -1) {
                return "Error: Column '" + stmt->whereColumn + "' not found";
            }
            
            if (row.values[columnIndex] != stmt->whereValue) {
                includeRow = false;
            }
        }
        
        if (includeRow) {
            for (size_t i = 0; i < row.values.size(); ++i) {
                output << row.values[i];
                if (i < row.values.size() - 1) {
                    output << " | ";
                }
            }
            output << "\n";
            rowCount++;
        }
    }
    
    output << "\n(" << rowCount << " row(s) returned)";
    
    return output.str();
}
