//
// Created by j1p2p3a4 on 6/16/2026.
//

#ifndef UNTITLED2_STORAGEMANAGER_H
#define UNTITLED2_STORAGEMANAGER_H



#pragma once
#include <string>
#include <vector>
#include "../Models/Table.h"
#include "../Models/Row.h"

class StorageManager {
private:
    std::string dataPath;  // carpeta raiz donde viven las carpetas de cada DB

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
    void writeAllRows(const Table& table, const std::vector<Row>& rows);

    int countRows(const Table& table) const;
};
#endif //UNTITLED2_STORAGEMANAGER_H
