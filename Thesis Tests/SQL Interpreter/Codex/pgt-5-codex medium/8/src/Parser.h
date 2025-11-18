#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <stdexcept>
#include <string>

#include "Ast.h"
#include "Lexer.h"

class Parser {
public:
    explicit Parser(const std::string &input);
    std::unique_ptr<Statement> parseStatement();

private:
    Lexer lexer_;
    Token current_;

    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    void expect(TokenType type, const std::string &message);
    bool matchKeyword(const std::string &keyword);
    void expectKeyword(const std::string &keyword, const std::string &message);
    std::unique_ptr<Statement> parseCreate();
    std::unique_ptr<Statement> parseInsert();
    std::unique_ptr<Statement> parseSelect();
    std::string parseIdentifier();
    std::string parseLiteral();
};

#endif // PARSER_H
