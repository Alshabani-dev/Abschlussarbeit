#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <memory>
#include "Storage.h"
#include "Parser.h"

class Engine {
public:
    Engine();
    void repl();
    void executeScript(const std::string& filename);
    std::string executeStatementWeb(const std::string& sql);

private:
    Storage storage_;
    
    void executeStatement(const std::string& sql);
    std::string executeStatementInternal(const std::string& sql, bool returnOutput);
    
    std::string executeCreateTable(CreateTableStatement* stmt);
    std::string executeInsert(InsertStatement* stmt);
    std::string executeSelect(SelectStatement* stmt);
    
    std::string formatTableOutput(const Table* table, const std::vector<size_t>& rowIndices) const;
};

#endif // ENGINE_H
