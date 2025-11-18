#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <vector>

#include "Ast.h"
#include "Lexer.h"

class Parser {
public:
    explicit Parser(const std::string &input);
    StatementPtr parseStatement();

private:
    std::vector<Token> tokens_;
    size_t position_ = 0;

    const Token &peek() const;
    const Token &get();
    bool matchKeyword(const std::string &keyword);
    void expect(TokenType type, const std::string &message);
    void expectKeyword(const std::string &keyword);
    bool isEnd() const;

    std::unique_ptr<CreateTableStatement> parseCreateTable();
    std::unique_ptr<InsertStatement> parseInsert();
    std::unique_ptr<SelectStatement> parseSelect();
    std::vector<std::string> parseIdentifierList();
};

#endif // PARSER_H
