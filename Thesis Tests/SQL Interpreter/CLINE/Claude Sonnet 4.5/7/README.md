# Project: Minimal C++ SQL Interpreter

**Role:** You are a **senior C++ developer**.
**Task:** Design and implement a small SQL-like engine in **pure C++ (no external libraries)**.
**Goal:** Build a command-line program that can create in-memory tables, insert rows, run simple SELECT queries with WHERE filters, and provide a web interface for SQL execution.

---

## Table of Contents

1. [Introduction](#introduction)
2. [Requirements](#requirements)
3. [Project Structure](#project-structure)
4. [File Templates](#file-templates)
5. [Architecture Overview](#architecture-overview)
6. [Build and Execution](#build-and-execution)
7. [Supported SQL Subset](#supported-sql-subset)
8. [Parser and AST](#parser-and-ast)
9. [Storage Layer](#storage-layer)
10. [Query Execution](#query-execution)
11. [Testing & Error Prevention](#testing--error-prevention)
12. [Common Issues and Fixes](#common-issues-and-fixes)

---

## Introduction

This project implements a **minimal SQL interpreter** in C++:

- Read SQL commands from **stdin**, a script file, or a **web interface**.
- Parse a limited SQL subset into an **Abstract Syntax Tree (AST)**.
- Store table data in simple in-memory structures.
- **Automatically persist tables** to CSV files in the `data/` directory.
- Execute basic queries with `SELECT ... FROM ... WHERE ...`.
- Serve a **web-based UI** using a pure C++ HTTP server (no external libraries).

The focus is on **parsing**, **data modeling**, **execution**, and **persistence**, not performance or full SQL compatibility.

---

## Requirements

- C++17 or newer
- CMake
- Linux/Unix or Windows
- **No external libraries** (only C++ standard library and POSIX sockets for HTTP server)

---

## Project Structure

```text
project-root/
├── src/
│   ├── main.cpp
│   ├── Lexer.cpp
│   ├── Lexer.h
│   ├── Parser.cpp
│   ├── Parser.h
│   ├── Ast.h
│   ├── Engine.cpp
│   ├── Engine.h
│   ├── Storage.cpp
│   ├── Storage.h
│   ├── HttpServer.cpp
│   ├── HttpServer.h
│   └── Utils.h
├── data/
│   └── (CSV files per table - auto-generated)
├── CMakeLists.txt
└── README.md
```

- **Lexer:** Converts SQL text into tokens.
- **Parser:** Builds AST nodes from the token stream.
- **AST:** Structs/classes representing statements (CREATE, INSERT, SELECT).
- **Engine:** Coordinates parsing, execution, and user interaction (REPL, script, and web modes).
- **Storage:** Holds tables/rows in memory and **automatically persists them to CSV files** in the `data/` directory.
- **HttpServer:** Pure C++ HTTP server (using POSIX sockets) that serves a web interface for executing SQL commands.
- **Utils:** Helper functions for string manipulation and data processing.

---

## File Templates

### `Engine.h`

```cpp
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
```

### `Storage.h`

```cpp
#ifndef STORAGE_H
#define STORAGE_H

#include <string>
#include <vector>
#include <unordered_map>

struct Row {
    std::vector<std::string> values;
};

struct Table {
    std::vector<std::string> columns;
    std::vector<Row> rows;
};

class Storage {
public:
    Storage();  // Loads existing tables from data/ directory
    ~Storage(); // Saves all tables to data/ directory
    
    bool createTable(const std::string &name, const std::vector<std::string> &columns);
    bool insertRow(const std::string &tableName, const std::vector<std::string> &values);
    const Table* getTable(const std::string &name) const;
    
    std::string getLastError() const;

private:
    std::unordered_map<std::string, Table> tables_;
    std::string lastError_;
    std::string dataDir_;
    
    void loadAllTables();  // Load CSV files on startup
    bool saveTable(const std::string &tableName);  // Save table to CSV
};

#endif // STORAGE_H
```

---

## Architecture Overview

```text
+---------------------+
|   User (CLI/Web)    |
+----------+----------+
           |
           v
+----------+----------+
|        Engine       |
| - REPL / script     |
| - Web interface     |
| - Error handling    |
+----------+----------+
           |
           v
+----------+----------+      +-----------------+
|        Parser       | ---> |       AST       |
| - Uses Lexer        |      | (CREATE/INSERT/ |
+----------+----------+      |  SELECT nodes)  |
           |                 +-----------------+
           v
+----------+----------+      +-----------------+
|        Storage      | <--> |   data/*.csv    |
| - Tables & rows     |      | (Auto-persist)  |
| - CSV auto-save     |      +-----------------+
+---------------------+

+---------------------+
|     HttpServer      |
| - Pure C++ sockets  |
| - Embedded HTML/CSS |
| - REST API          |
+---------------------+
```

- **Engine** reads SQL, invokes **Parser**, and calls **Storage**.
- **Parser** depends on **Lexer** and produces AST nodes.
- **Storage** manages tables/rows in memory and **automatically persists to CSV files**.
- **HttpServer** provides a web interface using pure C++ (no external libraries).

---

## Build and Execution

### CMake Build

```bash
mkdir -p build
cd build
cmake ..
make
```

This should produce a binary, e.g. `minisql`.

### Run in Interactive Mode (REPL)

```bash
./build/minisql
```

Example session:

```text
minisql> CREATE TABLE users (id, name, age);
OK
minisql> INSERT INTO users VALUES (1, Alice, 30);
OK
minisql> INSERT INTO users VALUES (2, Bob, 25);
OK
minisql> SELECT * FROM users WHERE age = 25;
id | name | age
---+------+----
2  | Bob  | 25

(1 row(s) returned)
minisql> .exit
```

### Run a Script File

```bash
./build/minisql script.sql
```

### Run Web Server Mode

```bash
./build/minisql --web        # Default port 8080
./build/minisql --web 3000   # Custom port
```

Then open your browser to: **http://localhost:8080**

The web interface provides:
- Modern, responsive UI with gradient design
- SQL command input with syntax examples
- Real-time execution results
- Error handling and display
- Ctrl+Enter shortcut to execute commands

---

## Supported SQL Subset

The interpreter supports a **very small subset of SQL** with the following forms:

1. **CREATE TABLE**

```sql
CREATE TABLE table_name (col1, col2, col3);
```

- Column names are simple identifiers (no types).
- No primary keys, types, or constraints.
- **Automatically persisted to `data/table_name.csv`**

2. **INSERT INTO**

```sql
INSERT INTO table_name VALUES (val1, val2, val3);
```

- Values are parsed as **strings**.
- Basic support for quoted strings: `"Alice"`.
- **Automatically saves to CSV file after each insert**

3. **SELECT with optional WHERE**

```sql
SELECT * FROM table_name;
SELECT * FROM table_name WHERE column = value;
```

- Only `*` is supported (no column lists).
- Only `=` comparisons.
- WHERE value may be identifier or quoted string.

---

## Parser and AST

### Lexer Responsibilities

- Read input string and produce tokens:
  - Keywords: `CREATE`, `TABLE`, `INSERT`, `INTO`, `VALUES`, `SELECT`, `FROM`, `WHERE`
  - Symbols: `(`, `)`, `,`, `;`, `*`, `=`
  - Identifiers (table/column names)
  - String literals (e.g., "Alice")
  - Numeric literals (treated as strings internally)

### AST Structures (Example)

```cpp
enum class StatementType {
    CREATE_TABLE,
    INSERT,
    SELECT
};

struct Statement {
    virtual ~Statement() = default;
    virtual StatementType type() const = 0;
};

struct CreateTableStatement : Statement {
    std::string tableName;
    std::vector<std::string> columns;
    StatementType type() const override { return StatementType::CREATE_TABLE; }
};

struct InsertStatement : Statement {
    std::string tableName;
    std::vector<std::string> values;
    StatementType type() const override { return StatementType::INSERT; }
};

struct SelectStatement : Statement {
    std::string tableName;
    bool hasWhere = false;
    std::string whereColumn;
    std::string whereValue;
    StatementType type() const override { return StatementType::SELECT; }
};
```

The **Parser** consumes tokens and creates one of these statement types. The **Engine** then dispatches the correct execution logic based on `type()`.

---

## Storage Layer

The storage is intentionally simple but **persistent**:

- Each table:
  - List of column names (`std::vector<std::string>`)
  - List of rows (`std::vector<Row>`)
  - **Automatically saved to CSV file** in `data/` directory

- Each row:
  - List of values as strings (same order as columns)

**Persistence Features:**
- **Auto-save**: Every CREATE TABLE and INSERT operation immediately saves to disk
- **Auto-load**: Existing tables automatically load from CSV files on startup
- **CSV Format**: Human-readable, easy to inspect and edit
- **Proper escaping**: Handles commas, quotes, and newlines in data

Example in-memory layout for:

```sql
CREATE TABLE users (id, name, age);
INSERT INTO users VALUES (1, Alice, 30);
INSERT INTO users VALUES (2, Bob, 25);
```

**In Memory:**
```text
Table "users"
columns: ["id", "name", "age"]
rows:
  ["1", "Alice", "30"]
  ["2", "Bob", "25"]
```

**On Disk (data/users.csv):**
```csv
id,name,age
1,Alice,30
2,Bob,25
```

---

## Query Execution

### CREATE TABLE

- Validate that table does **not** already exist.
- Store column list in `Storage`.
- **Immediately save table structure to CSV file**.
- Return `OK` or an error message.

### INSERT

- Check that table exists.
- Check that the number of values matches the number of columns.
- Append row to table rows.
- **Immediately save updated table to CSV file**.

### SELECT

- Check that table exists.
- If no WHERE:
  - Return all rows.
- If WHERE:
  - Locate column index from column name.
  - Filter rows where `row[columnIndex] == whereValue`.
- Print header row and then matching rows in a simple pipe-separated format.

---

## Testing & Error Prevention

Suggested manual and scripted tests:

1. **Basic Creation & Insert**

```bash
echo "CREATE TABLE t (a, b); INSERT INTO t VALUES (1, 2);" | ./build/minisql
```

2. **SELECT All Rows**

```bash
echo "CREATE TABLE t (a, b); INSERT INTO t VALUES (1, 2); SELECT * FROM t;" | ./build/minisql
```

3. **SELECT with WHERE**

```bash
echo "CREATE TABLE t (a, b); INSERT INTO t VALUES (1, 2); INSERT INTO t VALUES (3, 4); SELECT * FROM t WHERE a = 3;" | ./build/minisql
```

4. **Persistence Test**

```bash
# Create data
echo "CREATE TABLE users (id, name); INSERT INTO users VALUES (1, Alice);" | ./build/minisql
# Restart and verify data persists
echo "SELECT * FROM users;" | ./build/minisql
```

5. **Web Interface Test**

```bash
./build/minisql --web 8080
# Open http://localhost:8080 and execute SQL commands
```

6. **Error Cases**

- Creating existing table.
- Inserting into non-existing table.
- Wrong number of values in INSERT.
- WHERE with unknown column.

---

## Common Issues and Fixes

1. **Ambiguous Tokenization**
   - **Problem:** Keywords and identifiers not distinguished correctly.
   - **Fix:** Normalize keywords to upper-case in the lexer and compare against a keyword list.

2. **Missing Semicolon Handling**
   - **Problem:** REPL waits indefinitely or mixes two statements.
   - **Fix:** Treat `;` as the statement terminator. Buffer input until a semicolon is seen.

3. **Unclear Error Messages**
   - **Problem:** Parser failures are hard to debug.
   - **Fix:** Include current token and expected token type in error strings.

4. **WHERE Clause Parsing Bugs**
   - **Problem:** `SELECT` works without WHERE but fails with it.
   - **Fix:** Make WHERE optional in grammar and explicitly check if `WHERE` keyword exists before parsing condition.

5. **Column-Value Mismatch**
   - **Problem:** INSERT succeeds even if number of values does not match.
   - **Fix:** Always compare `values.size()` to `columns.size()`; return an error if different.

6. **Case Sensitivity Confusion**
   - **Problem:** `users` and `USERS` treated differently.
   - **Fix:** Decide on a policy (e.g., normalize all identifiers to lower-case internally).

7. **Raw String Literal Issues in HTTP Server**
   - **Problem:** JavaScript code in HTML causes C++ compilation errors.
   - **Fix:** Use custom delimiter for raw string literals: `R"HTML(...)HTML"` instead of `R"(...)"`.

8. **CSV Data Corruption**
   - **Problem:** Commas or quotes in data break CSV parsing.
   - **Fix:** Implement proper CSV escaping (wrap in quotes, double-escape internal quotes).

---

*End of README: All core requirements, web interface, and persistence features are implemented in pure C++.*
