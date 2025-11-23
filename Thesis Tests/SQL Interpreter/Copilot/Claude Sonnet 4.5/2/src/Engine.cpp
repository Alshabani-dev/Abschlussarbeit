#include "Engine.h"
#include "Lexer.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>

Engine::Engine() {}

void Engine::executeStatement(const std::string &sql) {
    std::string output = executeStatementInternal(sql, true);
    std::cout << output;
}

std::string Engine::executeStatementInternal(const std::string &sql, bool returnOutput) {
    std::ostringstream oss;
    
    // Tokenize
    Lexer lexer(sql);
    std::vector<Token> tokens = lexer.tokenize();
    
    // Parse
    Parser parser(tokens);
    auto stmt = parser.parseStatement();
    
    if (!stmt) {
        std::string error = "ERROR: " + parser.getLastError() + "\n";
        if (returnOutput) {
            return error;
        }
        std::cout << error;
        return "";
    }
    
    // Execute based on statement type
    switch (stmt->type()) {
        case StatementType::CREATE_TABLE: {
            auto* createStmt = static_cast<CreateTableStatement*>(stmt.get());
            if (storage_.createTable(createStmt->tableName, createStmt->columns)) {
                oss << "OK\n";
            } else {
                oss << "ERROR: " << storage_.getLastError() << "\n";
            }
            break;
        }
        
        case StatementType::INSERT: {
            auto* insertStmt = static_cast<InsertStatement*>(stmt.get());
            if (storage_.insertRow(insertStmt->tableName, insertStmt->values)) {
                oss << "OK\n";
            } else {
                oss << "ERROR: " << storage_.getLastError() << "\n";
            }
            break;
        }
        
        case StatementType::SELECT: {
            auto* selectStmt = static_cast<SelectStatement*>(stmt.get());
            const Table* table = storage_.getTable(selectStmt->tableName);
            
            if (!table) {
                oss << "ERROR: Table '" << selectStmt->tableName << "' does not exist\n";
            } else {
                oss << formatSelectResults(table, selectStmt->hasWhere,
                                          selectStmt->whereColumn,
                                          selectStmt->whereValue);
            }
            break;
        }
    }
    
    return oss.str();
}

std::string Engine::formatSelectResults(const Table* table, bool hasWhere,
                                        const std::string& whereColumn,
                                        const std::string& whereValue) {
    std::ostringstream oss;
    
    // Find column index for WHERE clause if needed
    int whereColIdx = -1;
    if (hasWhere) {
        for (size_t i = 0; i < table->columns.size(); ++i) {
            if (Utils::toLower(table->columns[i]) == Utils::toLower(whereColumn)) {
                whereColIdx = static_cast<int>(i);
                break;
            }
        }
        
        if (whereColIdx == -1) {
            return "ERROR: Column '" + whereColumn + "' not found\n";
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
    
    // Print rows
    int rowCount = 0;
    for (const auto& row : table->rows) {
        // Apply WHERE filter if needed
        if (hasWhere) {
            if (whereColIdx >= static_cast<int>(row.values.size()) ||
                row.values[whereColIdx] != whereValue) {
                continue;
            }
        }
        
        for (size_t i = 0; i < row.values.size(); ++i) {
            if (i > 0) oss << " | ";
            oss << row.values[i];
        }
        oss << "\n";
        rowCount++;
    }
    
    oss << "\n(" << rowCount << " row(s) returned)\n";
    return oss.str();
}

void Engine::repl() {
    std::cout << "minisql> ";
    std::string line;
    std::string buffer;
    
    while (std::getline(std::cin, line)) {
        // Trim the line
        line = Utils::trim(line);
        
        // Check for exit command
        if (line == ".exit" || line == ".quit") {
            break;
        }
        
        // Skip empty lines
        if (line.empty()) {
            std::cout << "minisql> ";
            continue;
        }
        
        // Accumulate lines until we see a semicolon
        buffer += line + " ";
        
        if (line.back() == ';') {
            // Execute the complete statement
            executeStatement(buffer);
            buffer.clear();
        }
        
        std::cout << "minisql> ";
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open file '" << filename << "'\n";
        return;
    }
    
    std::string line;
    std::string buffer;
    
    while (std::getline(file, line)) {
        line = Utils::trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line.substr(0, 2) == "--") {
            continue;
        }
        
        buffer += line + " ";
        
        if (line.back() == ';') {
            executeStatement(buffer);
            buffer.clear();
        }
    }
    
    // Execute any remaining buffered statement
    if (!buffer.empty()) {
        executeStatement(buffer);
    }
    
    file.close();
}

std::string Engine::executeStatementWeb(const std::string &sql) {
    return executeStatementInternal(sql, true);
}
