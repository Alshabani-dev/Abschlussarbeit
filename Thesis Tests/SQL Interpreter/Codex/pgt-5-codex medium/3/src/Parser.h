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
    StatementPtr parseCreateTable();
    StatementPtr parseInsert();
    StatementPtr parseSelect();

    std::vector<std::string> parseParenthesizedIdentifierList();
    std::vector<std::string> parsePlainIdentifierList();
    std::vector<std::string> parseValueList();

    Token expectToken(TokenType type, const std::string &message);
    Token expectIdentifier(const std::string &message);
    std::string parseValueToken(const Token &token, const std::string &contextMessage);

    void expectKeyword(const std::string &keyword, const std::string &message);
    bool peekKeyword(const std::string &keyword) const;

    std::string peekKeywordText() const;

    Lexer lexer_;
};

#endif // PARSER_H
