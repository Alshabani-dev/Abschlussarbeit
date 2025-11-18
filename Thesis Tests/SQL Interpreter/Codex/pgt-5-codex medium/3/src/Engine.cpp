#include "Engine.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "Utils.h"

Engine::Engine() = default;

void Engine::repl() {
    std::string buffer;
    std::string line;
    std::cout << "mini-sql> " << std::flush;
    while (std::getline(std::cin, line)) {
        buffer += line;
        buffer.push_back('\n');
        std::string statement;
        while (extractStatementFromBuffer(buffer, statement)) {
            std::string trimmed = Utils::trim(statement);
            if (!trimmed.empty()) {
                executeStatement(trimmed);
            }
        }
        std::cout << "mini-sql> " << std::flush;
    }
}

void Engine::executeScript(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Unable to open script file: " << filename << std::endl;
        return;
    }
    std::ostringstream stream;
    stream << file.rdbuf();
    std::string buffer = stream.str();
    trimLeadingWhitespace(buffer);
    std::string statement;
    while (extractStatementFromBuffer(buffer, statement)) {
        std::string trimmed = Utils::trim(statement);
        if (!trimmed.empty()) {
            executeStatement(trimmed);
        }
    }
    if (!Utils::trim(buffer).empty()) {
        std::cerr << "ERROR: Incomplete statement at end of script" << std::endl;
    }
}

std::string Engine::executeStatementWeb(const std::string &sql) {
    try {
        return executeStatementInternal(sql, true);
    } catch (const std::exception &ex) {
        return std::string("ERROR: ") + ex.what();
    }
}

void Engine::executeStatement(const std::string &sql) {
    try {
        executeStatementInternal(sql, false);
    } catch (const std::exception &ex) {
        std::cout << "ERROR: " << ex.what() << std::endl;
    }
}

std::string Engine::executeStatementInternal(const std::string &sql, bool returnOutput) {
    Parser parser(sql);
    StatementPtr statement = parser.parseStatement();
    std::string result;

    switch (statement->type) {
        case StatementType::CreateTable: {
            auto *create = static_cast<CreateTableStatement *>(statement.get());
            result = storage_.createTable(create->tableName, create->columns);
            break;
        }
        case StatementType::Insert: {
            auto *insert = static_cast<InsertStatement *>(statement.get());
            result = storage_.insertRow(insert->tableName, insert->values);
            break;
        }
        case StatementType::Select: {
            auto *select = static_cast<SelectStatement *>(statement.get());
            std::optional<std::pair<std::string, std::string>> whereClause;
            if (select->hasWhere) {
                whereClause = std::make_pair(select->whereColumn, select->whereValue);
            }
            QueryResult queryResult = storage_.selectRows(select->tableName, select->columns, select->selectAll, whereClause);
            result = formatQueryResult(queryResult);
            break;
        }
    }

    if (!returnOutput) {
        std::cout << result << std::endl;
    }
    return result;
}

bool Engine::extractStatementFromBuffer(std::string &buffer, std::string &statement) const {
    bool inString = false;
    char delimiter = '\0';
    bool escape = false;
    for (size_t i = 0; i < buffer.size(); ++i) {
        char c = buffer[i];
        if (inString) {
            if (escape) {
                escape = false;
                continue;
            }
            if (c == '\\') {
                escape = true;
                continue;
            }
            if (c == delimiter) {
                inString = false;
                delimiter = '\0';
            }
            continue;
        }
        if (c == '\'' || c == '"') {
            inString = true;
            delimiter = c;
            continue;
        }
        if (c == ';') {
            statement = buffer.substr(0, i + 1);
            buffer.erase(0, i + 1);
            trimLeadingWhitespace(buffer);
            return true;
        }
    }
    return false;
}

void Engine::trimLeadingWhitespace(std::string &buffer) const {
    size_t index = 0;
    while (index < buffer.size() && std::isspace(static_cast<unsigned char>(buffer[index]))) {
        ++index;
    }
    buffer.erase(0, index);
}

std::string Engine::formatQueryResult(const QueryResult &result) const {
    std::ostringstream stream;
    auto formatRow = [](const std::vector<std::string> &values) {
        std::ostringstream rowStream;
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                rowStream << " | ";
            }
            rowStream << values[i];
        }
        return rowStream.str();
    };

    stream << formatRow(result.columns);
    for (const Row &row : result.rows) {
        stream << '\n' << formatRow(row.values);
    }
    return stream.str();
}
