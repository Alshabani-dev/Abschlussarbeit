#include <iostream>
#include "Parser.h"
#include "Lexer.h"

int main() {
    std::string sql = "CREATE TABLE users (id, name, age);";

    Parser parser;
    try {
        std::unique_ptr<Statement> stmt = parser.parse(sql);
        if (stmt && stmt->type() == StatementType::CREATE_TABLE) {
            auto createStmt = dynamic_cast<CreateTableStatement*>(stmt.get());
            std::cout << "Parsed CREATE TABLE statement successfully!" << std::endl;
            std::cout << "Table name: " << createStmt->tableName << std::endl;
            std::cout << "Columns: ";
            for (const auto& col : createStmt->columns) {
                std::cout << col << " ";
            }
            std::cout << std::endl;
        } else {
            std::cout << "Failed to parse statement" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
