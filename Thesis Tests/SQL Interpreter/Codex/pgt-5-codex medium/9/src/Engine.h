#ifndef ENGINE_H
#define ENGINE_H

#include <string>

#include "Parser.h"
#include "Storage.h"

class Engine {
public:
    Engine();

    void repl();
    void executeScript(const std::string &filename);
    std::string executeStatementWeb(const std::string &sql);

private:
    Storage storage_;

    void executeStatement(const std::string &sql);
    std::string executeStatementInternal(const std::string &sql, bool forWeb);
};

#endif // ENGINE_H
