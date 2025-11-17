#include <iostream>
#include "Lexer.h"
#include "Utils.h"

int main() {
    Lexer lexer;
    std::string sql = "CREATE TABLE users (id, name, age);";

    std::vector<Token> tokens = lexer.tokenize(sql);

    std::cout << "Tokens:" << std::endl;
    for (const auto& token : tokens) {
        std::cout << "Type: " << static_cast<int>(token.type)
                  << ", Value: '" << token.value << "'"
                  << ", Position: " << token.position << std::endl;
    }

    return 0;
}
