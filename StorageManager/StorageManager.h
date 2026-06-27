#pragma once
#include <string>
#include <vector>
#include "../Models/Table.h"
#include "../Models/Row.h"

class StorageManager {
private:
    std::string dataPath;

    std::string tablePath(const std::string& dbName, const std::string& tableName) const;
    int rowSize(const Table& table) const;

    void encryptBuffer(char* buf, int size) const;
    void decryptBuffer(char* buf, int size) const;

    void writeField(char* buf, int& offset, const std::string& value, const Column& col) const;
    std::string readField(const char* buf, int& offset, const Column& col) const;

public:
    StorageManager(const std::string& path);

    void createDatabase(const std::string& dbName);
    void dropTable(const std::string& dbName, const std::string& tableName);

    void insertRow(const Table& table, const Row& row);
    std::vector<Row> readAllRows(const Table& table) const;
    Row readRowAtOffset(const Table& table, long long offset) const;
    void writeAllRows(const Table& table, const std::vector<Row>& rows);

    int countRows(const Table& table) const;
    long long getFileSize(const std::string& dbName, const std::string& tableName) const;
};
