#include "Engine.h"
#include "Lexer.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

Engine::Engine() {}

void Engine::repl() {
    std::cout << "MiniSQL Interpreter (type '.exit' to quit)\n";
    
    std::string buffer;
    
    while (true) {
        std::string line;
        
        if (buffer.empty()) {
            std::cout << "minisql> ";
        } else {
            std::cout << "      -> ";
        }
        
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        // Check for special commands
        std::string trimmed = Utils::trim(line);
        if (trimmed == ".exit" || trimmed == ".quit") {
            break;
        }
        
        // Accumulate input until we see a semicolon
        buffer += line + " ";
        
        if (trimmed.find(';') != std::string::npos) {
            // Execute the statement
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
        
        // Check if statement is complete (ends with semicolon)
        if (trimmed.find(';') != std::string::npos) {
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
    std::string output = executeStatementInternal(sql, false);
    if (!output.empty()) {
        std::cout << output << "\n";
    }
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
        std::unique_ptr<Statement> stmt = parser.parse();
        
        // Execute based on statement type
        std::string result;
        
        switch (stmt->type()) {
            case StatementType::CREATE_TABLE:
                result = executeCreateTable(dynamic_cast<CreateTableStatement*>(stmt.get()));
                break;
                
            case StatementType::INSERT:
                result = executeInsert(dynamic_cast<InsertStatement*>(stmt.get()));
                break;
                
            case StatementType::SELECT:
                result = executeSelect(dynamic_cast<SelectStatement*>(stmt.get()));
                break;
        }
        
        if (returnOutput) {
            return result;
        } else {
            std::cout << result << "\n";
            return "";
        }
        
    } catch (const std::exception& e) {
        std::string error = std::string("Error: ") + e.what();
        if (returnOutput) {
            return error;
        } else {
            std::cerr << error << "\n";
            return "";
        }
    }
}

std::string Engine::executeCreateTable(CreateTableStatement* stmt) {
    if (storage_.createTable(stmt->tableName, stmt->columns)) {
        return "OK";
    } else {
        return "Error: " + storage_.getLastError();
    }
}

std::string Engine::executeInsert(InsertStatement* stmt) {
    if (storage_.insertRow(stmt->tableName, stmt->values)) {
        return "OK";
    } else {
        return "Error: " + storage_.getLastError();
    }
}

std::string Engine::executeSelect(SelectStatement* stmt) {
    const Table* table = storage_.getTable(stmt->tableName);
    
    if (!table) {
        return "Error: Table '" + stmt->tableName + "' does not exist";
    }
    
    std::vector<size_t> matchingRows;
    
    if (stmt->hasWhere) {
        // Find column index
        auto it = std::find(table->columns.begin(), table->columns.end(), stmt->whereColumn);
        if (it == table->columns.end()) {
            return "Error: Column '" + stmt->whereColumn + "' not found in table '" + stmt->tableName + "'";
        }
        size_t columnIndex = std::distance(table->columns.begin(), it);
        
        // Filter rows
        for (size_t i = 0; i < table->rows.size(); i++) {
            if (table->rows[i].values[columnIndex] == stmt->whereValue) {
                matchingRows.push_back(i);
            }
        }
    } else {
        // All rows
        for (size_t i = 0; i < table->rows.size(); i++) {
            matchingRows.push_back(i);
        }
    }
    
    return formatTableOutput(table, matchingRows);
}

std::string Engine::formatTableOutput(const Table* table, const std::vector<size_t>& rowIndices) const {
    std::ostringstream oss;
    
    // Calculate column widths
    std::vector<size_t> widths;
    for (const auto& col : table->columns) {
        widths.push_back(col.length());
    }
    
    for (size_t idx : rowIndices) {
        const Row& row = table->rows[idx];
        for (size_t i = 0; i < row.values.size() && i < widths.size(); i++) {
            widths[i] = std::max(widths[i], row.values[i].length());
        }
    }
    
    // Print header
    for (size_t i = 0; i < table->columns.size(); i++) {
        if (i > 0) oss << " | ";
        oss << table->columns[i];
        // Pad to width
        for (size_t j = table->columns[i].length(); j < widths[i]; j++) {
            oss << " ";
        }
    }
    oss << "\n";
    
    // Print separator
    for (size_t i = 0; i < table->columns.size(); i++) {
        if (i > 0) oss << "-+-";
        for (size_t j = 0; j < widths[i]; j++) {
            oss << "-";
        }
    }
    oss << "\n";
    
    // Print rows
    for (size_t idx : rowIndices) {
        const Row& row = table->rows[idx];
        for (size_t i = 0; i < row.values.size(); i++) {
            if (i > 0) oss << " | ";
            oss << row.values[i];
            // Pad to width
            for (size_t j = row.values[i].length(); j < widths[i]; j++) {
                oss << " ";
            }
        }
        oss << "\n";
    }
    
    // Print summary
    oss << "\n(" << rowIndices.size() << " row(s) returned)";
    
    return oss.str();
}
