#pragma once

#include <sqlite3.h>

#include <string>
#include <vector>

struct Row {
    int id;
    std::string a;
    std::string b;
};

// Wraps the sqlite3 handle for the two fixed tables A and B
class Database {
public:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Returns "" on success, an error description otherwise.
    std::string insert(const std::string& table, int id, const std::string& name);
    std::string truncate(const std::string& table);

    std::vector<Row> intersection();
    std::vector<Row> symmetricDifference();

private:
    static bool isValidTable(const std::string& table);
    std::string exec(const std::string& sql);
    std::vector<Row> query(const char* sql);

    sqlite3* db_ = nullptr;
};

void runServer(int port);
