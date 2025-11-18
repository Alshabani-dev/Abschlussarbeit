#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <vector>

#include "Ast.h"
#include "Storage.h"

class Engine {
public:
    Engine();

    void repl();
    void executeScript(const std::string &filename);
    std::string executeStatementWeb(const std::string &sql);

private:
    Storage storage_;
    std::string buffer_;

    void executeStatement(const std::string &sql);
    std::string executeStatementInternal(const std::string &sql);

    std::vector<std::string> extractStatementsFromBuffer(std::string &buffer);
    std::string handleCreate(const Statement &stmtBase);
    std::string handleInsert(const Statement &stmtBase);
    std::string handleSelect(const Statement &stmtBase);
    std::string formatSelectResult(const Table &table, const std::vector<const Row *> &rows) const;
};

#endif // ENGINE_H
