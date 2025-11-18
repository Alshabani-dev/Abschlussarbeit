#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <string>
#include <vector>

#include "Ast.h"
#include "Lexer.h"

class Parser {
public:
    Parser() = default;
    StatementPtr parse(const std::string &sql);

private:
    std::vector<Token> tokens_;
    size_t pos_ = 0;

    const Token &current() const;
    const Token &consume();
    bool match(TokenType type);
    void expect(TokenType type, const std::string &message);

    StatementPtr parseStatement();
    StatementPtr parseCreate();
    StatementPtr parseInsert();
    StatementPtr parseSelect();

    std::string parseIdentifier();
    std::string parseValue();
};

#endif // PARSER_H
