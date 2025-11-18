#ifndef PARSER_H
#define PARSER_H

#include <vector>

#include "Ast.h"
#include "Lexer.h"

class Parser {
public:
    explicit Parser(const std::vector<Token> &tokens);
    std::vector<StatementPtr> parseStatements();

private:
    std::vector<Token> tokens_;
    std::size_t position_;

    const Token &peek() const;
    const Token &previous() const;
    bool atEnd() const;
    const Token &advance();
    bool match(TokenType type);
    void expect(TokenType type, const std::string &message);
    StatementPtr parseStatement();
    StatementPtr parseCreateTable();
    StatementPtr parseInsert();
    StatementPtr parseSelect();
    std::string parseIdentifier(const std::string &context);
    std::string parseValue(const std::string &context);
    std::vector<std::string> parseIdentifierList();
    std::vector<std::string> parseValueList();
};

#endif // PARSER_H
