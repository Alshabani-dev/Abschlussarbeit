#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <memory>
#include "Storage.h"
#include "Parser.h"

class Engine {
public:
    Engine();
    void repl();                                    // interactive mode
    void executeScript(const std::string &filename); // execute from file
    std::string executeStatementWeb(const std::string &sql); // execute for web interface

private:
    Storage storage_;

    void executeStatement(const std::string &sql);
    std::string executeStatementInternal(const std::string &sql, bool returnOutput);
};

#endif // ENGINE_H
