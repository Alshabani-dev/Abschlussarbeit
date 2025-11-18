#ifndef PARSER_H
#define PARSER_H

#include <optional>
#include <string>
#include <vector>

#include "Ast.h"
#include "Lexer.h"

class Parser {
public:
    explicit Parser(const std::vector<Token> &tokens);

    std::optional<Statement> parseStatement();
    std::string getError() const;

private:
    const std::vector<Token> &tokens_;
    size_t position_;
    std::string error_;

    const Token &peek() const;
    const Token &advance();
    bool match(TokenType type);
    bool expect(TokenType type, const std::string &message);
    std::string parseIdentifier(const std::string &context);
    std::string parseValue(const std::string &context);

    std::optional<Statement> parseCreateTable();
    std::optional<Statement> parseInsert();
    std::optional<Statement> parseSelect();
};

#endif // PARSER_H
