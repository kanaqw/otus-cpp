#include "database.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

// ---- Database -------------------------------------------------------------
//
// See example/ for the sqlite3 C API patterns (sqlite3_open, sqlite3_exec,
// row callbacks) this follows, using prepared statements instead of
// sqlite3_exec + C callback for parameter binding and result extraction.

Database::Database() {
    if (sqlite3_open(":memory:", &db_) != SQLITE_OK) {
        throw std::runtime_error(std::string("sqlite3_open: ") + sqlite3_errmsg(db_));
    }
    exec("CREATE TABLE A (id INTEGER PRIMARY KEY, name TEXT NOT NULL)");
    exec("CREATE TABLE B (id INTEGER PRIMARY KEY, name TEXT NOT NULL)");
}

Database::~Database() { sqlite3_close(db_); }

bool Database::isValidTable(const std::string& table) { return table == "A" || table == "B"; }

std::string Database::exec(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string err = errMsg ? errMsg : "unknown sqlite error";
        sqlite3_free(errMsg);
        return err;
    }
    return "";
}

std::vector<Row> Database::query(const char* sql) {
    std::vector<Row> rows;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db_));
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Row row;
        row.id = sqlite3_column_int(stmt, 0);
        const unsigned char* a = sqlite3_column_text(stmt, 1);
        const unsigned char* b = sqlite3_column_text(stmt, 2);
        row.a = a ? reinterpret_cast<const char*>(a) : "";
        row.b = b ? reinterpret_cast<const char*>(b) : "";
        rows.push_back(std::move(row));
    }
    sqlite3_finalize(stmt);
    return rows;
}

std::string Database::insert(const std::string& table, int id, const std::string& name) {
    if (!isValidTable(table)) return "unknown table " + table;

    sqlite3_stmt* stmt = nullptr;
    std::string sql = "INSERT INTO " + table + " (id, name) VALUES (?, ?)";
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::string("prepare failed: ") + sqlite3_errmsg(db_);
    }
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_CONSTRAINT) return "duplicate " + std::to_string(id);
    if (rc != SQLITE_DONE) return std::string("insert failed: ") + sqlite3_errmsg(db_);
    return "";
}

std::string Database::truncate(const std::string& table) {
    if (!isValidTable(table)) return "unknown table " + table;
    return exec("DELETE FROM " + table);
}

std::vector<Row> Database::intersection() {
    static const char* kSql =
        "SELECT A.id, A.name, B.name "
        "FROM A INNER JOIN B ON A.id = B.id "
        "ORDER BY A.id ASC";
    return query(kSql);
}

std::vector<Row> Database::symmetricDifference() {
    static const char* kSql =
        "SELECT id, a_name, b_name FROM ("
        "  SELECT A.id AS id, A.name AS a_name, '' AS b_name "
        "  FROM A LEFT JOIN B ON A.id = B.id WHERE B.id IS NULL "
        "  UNION ALL "
        "  SELECT B.id AS id, '' AS a_name, B.name AS b_name "
        "  FROM B LEFT JOIN A ON A.id = B.id WHERE A.id IS NULL "
        ") ORDER BY id ASC";
    return query(kSql);
}

// ---- protocol ---------------------------------------------------------

namespace {

std::string rowsToResponse(const std::vector<Row>& rows) {
    std::string out;
    for (const auto& row : rows) {
        out += std::to_string(row.id) + "," + row.a + "," + row.b + "\n";
    }
    out += "OK\n";
    return out;
}

bool parseId(const std::string& s, int& out) {
    if (s.empty()) return false;
    try {
        size_t pos = 0;
        out = std::stoi(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

std::string handleCommand(const std::string& rawLine, Database& db) {
    std::string line = rawLine;
    while (!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream ss(line);
    std::string cmd;
    ss >> cmd;

    if (cmd.empty()) {
        return "ERR empty command\n";
    }

    if (cmd == "INSERT") {
        std::string table, idStr, name;
        if (!(ss >> table >> idStr >> name)) {
            return "ERR invalid INSERT syntax\n";
        }
        int id;
        if (!parseId(idStr, id)) return "ERR invalid id " + idStr + "\n";
        std::string err = db.insert(table, id, name);
        return err.empty() ? "OK\n" : "ERR " + err + "\n";
    }

    if (cmd == "TRUNCATE") {
        std::string table;
        if (!(ss >> table)) return "ERR invalid TRUNCATE syntax\n";
        std::string err = db.truncate(table);
        return err.empty() ? "OK\n" : "ERR " + err + "\n";
    }

    if (cmd == "INTERSECTION") {
        return rowsToResponse(db.intersection());
    }

    if (cmd == "SYMMETRIC_DIFFERENCE") {
        return rowsToResponse(db.symmetricDifference());
    }

    return "ERR unknown command " + cmd + "\n";
}

// ---- networking ---------------------------------------------------------

bool setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

struct Client {
    int fd;
    std::string inbuf;
    std::string outbuf;
};

}  // namespace

void runServer(int port) {
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        std::perror("socket");
        return;
    }

    int yes = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::perror("bind");
        close(listenFd);
        return;
    }

    if (listen(listenFd, SOMAXCONN) < 0) {
        std::perror("listen");
        close(listenFd);
        return;
    }

    if (!setNonBlocking(listenFd)) {
        std::perror("fcntl");
        close(listenFd);
        return;
    }

    std::unordered_map<int, Client> clients;
    Database db;

    std::cerr << "join_server listening on port " << port << "\n";

    while (true) {
        fd_set readfds, writefds;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(listenFd, &readfds);
        int maxFd = listenFd;

        for (auto& [fd, client] : clients) {
            FD_SET(fd, &readfds);
            if (!client.outbuf.empty()) FD_SET(fd, &writefds);
            if (fd > maxFd) maxFd = fd;
        }

        int ready = select(maxFd + 1, &readfds, &writefds, nullptr, nullptr);
        if (ready < 0) {
            if (errno == EINTR) continue;
            std::perror("select");
            break;
        }

        if (FD_ISSET(listenFd, &readfds)) {
            while (true) {
                sockaddr_in clientAddr{};
                socklen_t clientLen = sizeof(clientAddr);
                int clientFd = accept(listenFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
                if (clientFd < 0) {
                    break;  // no more pending connections (EAGAIN/EWOULDBLOCK)
                }
                setNonBlocking(clientFd);
                clients[clientFd] = Client{clientFd, "", ""};
            }
        }

        std::vector<int> toClose;

        for (auto& [fd, client] : clients) {
            if (FD_ISSET(fd, &readfds)) {
                char buf[4096];
                while (true) {
                    ssize_t n = recv(fd, buf, sizeof(buf), 0);
                    if (n > 0) {
                        client.inbuf.append(buf, static_cast<size_t>(n));
                        if (static_cast<size_t>(n) < sizeof(buf)) break;
                    } else if (n == 0) {
                        toClose.push_back(fd);
                        break;
                    } else {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        toClose.push_back(fd);
                        break;
                    }
                }

                size_t pos;
                while ((pos = client.inbuf.find('\n')) != std::string::npos) {
                    std::string line = client.inbuf.substr(0, pos);
                    client.inbuf.erase(0, pos + 1);
                    client.outbuf += handleCommand(line, db);
                }
            }
        }

        for (auto& [fd, client] : clients) {
            if (!client.outbuf.empty() && FD_ISSET(fd, &writefds)) {
                ssize_t n = send(fd, client.outbuf.data(), client.outbuf.size(), 0);
                if (n > 0) {
                    client.outbuf.erase(0, static_cast<size_t>(n));
                } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    toClose.push_back(fd);
                }
            }
        }

        for (int fd : toClose) {
            close(fd);
            clients.erase(fd);
        }
    }

    close(listenFd);
}
