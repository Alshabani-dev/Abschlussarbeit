#include <iostream>
#include "Lexer.h"

int main() {
    Lexer lexer;
    std::string testSql = "CREATE TABLE users (id, name, age);";

    std::vector<Token> tokens = lexer.tokenize(testSql);

    std::cout << "Tokens:" << std::endl;
    for (const auto& token : tokens) {
        std::cout << "Type: " << static_cast<int>(token.type)
                  << ", Value: '" << token.value << "'"
                  << ", Position: " << token.position << std::endl;
    }

    return 0;
}
