#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <string>
#include <vector>

#include "Ast.h"
#include "Lexer.h"

class Parser {
public:
    explicit Parser(const std::string &input);

    StatementPtr parseStatement();

private:
    std::vector<Token> tokens_;
    size_t index_ = 0;

    const Token &peek() const;
    const Token &consume();
    bool match(TokenType type);
    const Token &expect(TokenType type, const std::string &message);
    void expectEnd();

    StatementPtr parseCreateTable();
    StatementPtr parseInsert();
    StatementPtr parseSelect();

    std::string parseIdentifier();
    std::vector<std::string> parseIdentifierList();
    std::vector<std::string> parseValueList();
    std::string parseValueToken();
};

#endif // PARSER_H
