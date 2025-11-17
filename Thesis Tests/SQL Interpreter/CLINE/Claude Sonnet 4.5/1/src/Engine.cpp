#include "Engine.h"
#include "Lexer.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

Engine::Engine() {}

void Engine::repl() {
    std::cout << "MiniSQL Interpreter v1.0\n";
    std::cout << "Type SQL commands or '.exit' to quit.\n\n";
    
    std::string buffer;
    
    while (true) {
        std::cout << "minisql> ";
        std::string line;
        
        if (!std::getline(std::cin, line)) {
            // EOF or error reading input
            break;
        }
        
        if (line == ".exit" || line == ".quit") {
            break;
        }
        
        if (line.empty()) {
            continue;
        }
        
        buffer += line + " ";
        
        // Check if statement is complete (ends with semicolon)
        if (line.find(';') != std::string::npos) {
            executeStatement(buffer);
            buffer.clear();
        }
    }
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
        line = Utils::trim(line);
        
        if (line.empty() || line[0] == '#' || line.substr(0, 2) == "--") {
            continue; // Skip comments and empty lines
        }
        
        buffer += line + " ";
        
        // Check if statement is complete
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
    std::string result = executeStatementInternal(sql, false);
    if (!result.empty()) {
        std::cout << result << "\n";
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
    
    if (!lexer.getError().empty()) {
        std::string error = "Lexer error: " + lexer.getError();
        if (!returnOutput) {
            std::cerr << error << "\n";
        }
        return error;
    }
    
    // Parse
    Parser parser(tokens);
    std::unique_ptr<Statement> stmt = parser.parseStatement();
    
    if (parser.hasError()) {
        std::string error = "Parse error: " + parser.getError();
        if (!returnOutput) {
            std::cerr << error << "\n";
        }
        return error;
    }
    
    if (!stmt) {
        std::string error = "Error: Failed to parse statement";
        if (!returnOutput) {
            std::cerr << error << "\n";
        }
        return error;
    }
    
    // Execute
    std::string result;
    switch (stmt->type()) {
        case StatementType::CREATE_TABLE:
            result = handleCreateTable(static_cast<CreateTableStatement*>(stmt.get()));
            break;
        case StatementType::INSERT:
            result = handleInsert(static_cast<InsertStatement*>(stmt.get()));
            break;
        case StatementType::SELECT:
            result = handleSelect(static_cast<SelectStatement*>(stmt.get()));
            break;
        default:
            result = "Error: Unknown statement type";
    }
    
    return result;
}

std::string Engine::handleCreateTable(const CreateTableStatement* stmt) {
    if (storage_.createTable(stmt->tableName, stmt->columns)) {
        return "OK";
    } else {
        return "Error: " + storage_.getLastError();
    }
}

std::string Engine::handleInsert(const InsertStatement* stmt) {
    if (storage_.insertRow(stmt->tableName, stmt->values)) {
        return "OK";
    } else {
        return "Error: " + storage_.getLastError();
    }
}

std::string Engine::handleSelect(const SelectStatement* stmt) {
    const Table* table = storage_.getTable(stmt->tableName);
    
    if (!table) {
        return "Error: Table '" + stmt->tableName + "' does not exist";
    }
    
    std::vector<size_t> matchingRows;
    
    if (stmt->hasWhere) {
        // Find column index
        auto it = std::find(table->columns.begin(), table->columns.end(), stmt->whereColumn);
        if (it == table->columns.end()) {
            return "Error: Column '" + stmt->whereColumn + "' does not exist";
        }
        size_t columnIndex = std::distance(table->columns.begin(), it);
        
        // Filter rows
        for (size_t i = 0; i < table->rows.size(); ++i) {
            if (table->rows[i].values[columnIndex] == stmt->whereValue) {
                matchingRows.push_back(i);
            }
        }
    } else {
        // All rows
        for (size_t i = 0; i < table->rows.size(); ++i) {
            matchingRows.push_back(i);
        }
    }
    
    return formatSelectResult(table, matchingRows);
}

std::string Engine::formatSelectResult(const Table* table, const std::vector<size_t>& rowIndices) {
    std::ostringstream oss;
    
    if (table->columns.empty()) {
        return "Empty table";
    }
    
    // Calculate column widths
    std::vector<size_t> widths(table->columns.size());
    for (size_t i = 0; i < table->columns.size(); ++i) {
        widths[i] = table->columns[i].length();
    }
    
    for (size_t rowIdx : rowIndices) {
        const Row& row = table->rows[rowIdx];
        for (size_t i = 0; i < row.values.size() && i < widths.size(); ++i) {
            widths[i] = std::max(widths[i], row.values[i].length());
        }
    }
    
    // Print header
    for (size_t i = 0; i < table->columns.size(); ++i) {
        if (i > 0) oss << " | ";
        oss << std::left << std::setw(widths[i]) << table->columns[i];
    }
    oss << "\n";
    
    // Print separator
    for (size_t i = 0; i < table->columns.size(); ++i) {
        if (i > 0) oss << "-+-";
        oss << std::string(widths[i], '-');
    }
    oss << "\n";
    
    // Print rows
    for (size_t rowIdx : rowIndices) {
        const Row& row = table->rows[rowIdx];
        for (size_t i = 0; i < row.values.size(); ++i) {
            if (i > 0) oss << " | ";
            oss << std::left << std::setw(widths[i]) << row.values[i];
        }
        oss << "\n";
    }
    
    oss << "\n(" << rowIndices.size() << " row(s) returned)";
    
    return oss.str();
}
