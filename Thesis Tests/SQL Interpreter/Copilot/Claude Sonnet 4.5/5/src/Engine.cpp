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
    std::string input;
    std::string buffer;
    
    while (std::getline(std::cin, input)) {
        if (input == ".exit") {
            break;
        }
        
        buffer += input + " ";
        
        // Check if statement is complete (ends with semicolon)
        if (buffer.find(';') != std::string::npos) {
            executeStatement(buffer);
            buffer.clear();
            std::cout << "minisql> ";
        } else {
            std::cout << "      -> ";
        }
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
        
        // Check if statement is complete
        if (buffer.find(';') != std::string::npos) {
            executeStatement(buffer);
            buffer.clear();
        }
    }
    
    // Execute remaining buffer if any
    if (!buffer.empty()) {
        executeStatement(buffer);
    }
    
    file.close();
}

std::string Engine::executeStatementWeb(const std::string& sql) {
    return executeStatementInternal(sql, true);
}

void Engine::executeStatement(const std::string& sql) {
    std::string result = executeStatementInternal(sql, false);
    if (!result.empty()) {
        std::cout << result << std::endl;
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
    
    if (!stmt) {
        std::string error = "Parse error: " + parser.getErrorMessage();
        if (returnOutput) {
            return error;
        } else {
            std::cerr << error << std::endl;
            return "";
        }
    }
    
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
            
        default:
            result = "Error: Unknown statement type";
            break;
    }
    
    return result;
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
    
    if (!stmt->hasWhere) {
        // Return all rows
        for (size_t i = 0; i < table->rows.size(); ++i) {
            matchingRows.push_back(i);
        }
    } else {
        // Filter rows based on WHERE clause
        // Find column index
        auto it = std::find(table->columns.begin(), table->columns.end(), stmt->whereColumn);
        
        if (it == table->columns.end()) {
            return "Error: Column '" + stmt->whereColumn + "' not found in table '" + stmt->tableName + "'";
        }
        
        size_t columnIndex = std::distance(table->columns.begin(), it);
        
        // Filter rows
        for (size_t i = 0; i < table->rows.size(); ++i) {
            if (table->rows[i].values[columnIndex] == stmt->whereValue) {
                matchingRows.push_back(i);
            }
        }
    }
    
    return formatSelectResult(table, matchingRows);
}

std::string Engine::formatSelectResult(const Table* table, const std::vector<size_t>& rowIndices) {
    if (rowIndices.empty()) {
        return "(0 row(s) returned)";
    }
    
    std::ostringstream oss;
    
    // Calculate column widths
    std::vector<size_t> widths;
    for (const std::string& col : table->columns) {
        widths.push_back(col.length());
    }
    
    for (size_t rowIdx : rowIndices) {
        const Row& row = table->rows[rowIdx];
        for (size_t i = 0; i < row.values.size() && i < widths.size(); ++i) {
            widths[i] = std::max(widths[i], row.values[i].length());
        }
    }
    
    // Print header
    for (size_t i = 0; i < table->columns.size(); ++i) {
        if (i > 0) {
            oss << " | ";
        }
        oss << table->columns[i];
        // Pad to width
        for (size_t j = table->columns[i].length(); j < widths[i]; ++j) {
            oss << " ";
        }
    }
    oss << "\n";
    
    // Print separator
    for (size_t i = 0; i < table->columns.size(); ++i) {
        if (i > 0) {
            oss << "-+-";
        }
        for (size_t j = 0; j < widths[i]; ++j) {
            oss << "-";
        }
    }
    oss << "\n";
    
    // Print data rows
    for (size_t rowIdx : rowIndices) {
        const Row& row = table->rows[rowIdx];
        for (size_t i = 0; i < row.values.size(); ++i) {
            if (i > 0) {
                oss << " | ";
            }
            oss << row.values[i];
            // Pad to width
            for (size_t j = row.values[i].length(); j < widths[i]; ++j) {
                oss << " ";
            }
        }
        oss << "\n";
    }
    
    oss << "\n(" << rowIndices.size() << " row(s) returned)";
    
    return oss.str();
}
