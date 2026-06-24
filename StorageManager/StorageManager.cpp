//
// Created by j1p2p3a4 on 6/16/2026.
//

#include "StorageManager.h"
#include <fstream>
#include <filesystem>
#include <cstring>
#include <stdexcept>

namespace fs = std::filesystem;

// Tamaños fijos por tipo en disco
static const int SIZE_INTEGER  = 4;
static const int SIZE_DOUBLE   = 8;
static const int SIZE_DATETIME = 20; // "YYYY-MM-DD HH:MM:SS\0"

// Cifrado XOR simple
static const char XOR_KEY = 0x5A;

StorageManager::StorageManager(const std::string& path) {
    dataPath = path;
}

std::string StorageManager::tablePath(const std::string& dbName, const std::string& tableName) const {
    return dataPath + "/" + dbName + "/" + tableName + ".bin";
}

void StorageManager::encryptBuffer(char* buf, int size) const {
    for (int i = 0; i < size; i++)
        buf[i] ^= XOR_KEY;
}

void StorageManager::decryptBuffer(char* buf, int size) const {
    // XOR es simetrico
    encryptBuffer(buf, size);
}

int StorageManager::rowSize(const Table& table) const {
    int size = 0;
    for (const auto& col : table.getColumns()) {
        switch (col.getType()) {
            case DataType::INTEGER:  size += SIZE_INTEGER; break;
            case DataType::DOUBLE:   size += SIZE_DOUBLE; break;
            case DataType::DATETIME: size += SIZE_DATETIME; break;
            case DataType::VARCHAR:  size += col.getVarcharLength() + 1; break;
        }
    }
    return size;
}

void StorageManager::writeField(char* buf, int& offset, const std::string& value, const Column& col) const {
    switch (col.getType()) {
        case DataType::INTEGER: {
            int v = std::stoi(value);
            memcpy(buf + offset, &v, SIZE_INTEGER);
            offset += SIZE_INTEGER;
            break;
        }
        case DataType::DOUBLE: {
            double v = std::stod(value);
            memcpy(buf + offset, &v, SIZE_DOUBLE);
            offset += SIZE_DOUBLE;
            break;
        }
        case DataType::VARCHAR: {
            int len = col.getVarcharLength() + 1;
            memset(buf + offset, 0, len);
            strncpy(buf + offset, value.c_str(), len - 1);
            offset += len;
            break;
        }
        case DataType::DATETIME: {
            memset(buf + offset, 0, SIZE_DATETIME);
            strncpy(buf + offset, value.c_str(), SIZE_DATETIME - 1);
            offset += SIZE_DATETIME;
            break;
        }
    }
}

std::string StorageManager::readField(const char* buf, int& offset, const Column& col) const {
    std::string result;
    switch (col.getType()) {
        case DataType::INTEGER: {
            int v;
            memcpy(&v, buf + offset, SIZE_INTEGER);
            offset += SIZE_INTEGER;
            result = std::to_string(v);
            break;
        }
        case DataType::DOUBLE: {
            double v;
            memcpy(&v, buf + offset, SIZE_DOUBLE);
            offset += SIZE_DOUBLE;
            result = std::to_string(v);
            break;
        }
        case DataType::VARCHAR: {
            int len = col.getVarcharLength() + 1;
            result = std::string(buf + offset, strnlen(buf + offset, len));
            offset += len;
            break;
        }
        case DataType::DATETIME: {
            result = std::string(buf + offset, strnlen(buf + offset, SIZE_DATETIME));
            offset += SIZE_DATETIME;
            break;
        }
    }
    return result;
}


void StorageManager::createDatabase(const std::string& dbName) {
    fs::create_directories(dataPath + "/" + dbName);
}

void StorageManager::dropTable(const std::string& dbName, const std::string& tableName) {
    std::string path = tablePath(dbName, tableName);
    if (fs::exists(path))
        fs::remove(path);
}

void StorageManager::insertRow(const Table& table, const Row& row) {
    if (row.fieldCount() != (int)table.getColumns().size())
        throw std::runtime_error("Field count mismatch on insert");

    int rsize = rowSize(table);
    std::vector<char> buf(rsize, 0);

    int offset = 0;
    for (int i = 0; i < (int)table.getColumns().size(); i++)
        writeField(buf.data(), offset, row.getField(i), table.getColumns()[i]);

    encryptBuffer(buf.data(), rsize);

    std::ofstream f(tablePath(table.getDatabaseName(), table.getName()),
                    std::ios::binary | std::ios::app);
    if (!f) throw std::runtime_error("Cannot open table file for writing");
    f.write(buf.data(), rsize);
}

std::vector<Row> StorageManager::readAllRows(const Table& table) const {
    std::vector<Row> result;
    int rsize = rowSize(table);
    if (rsize == 0) return result;

    std::ifstream f(tablePath(table.getDatabaseName(), table.getName()), std::ios::binary);
    if (!f) return result;

    std::vector<char> buf(rsize);
    long long pos = 0;
    while (f.read(buf.data(), rsize)) {
        decryptBuffer(buf.data(), rsize);
        std::vector<std::string> fields;
        int offset = 0;
        for (const auto& col : table.getColumns())
            fields.push_back(readField(buf.data(), offset, col));

        Row r(fields);
        r.setDiskOffset(pos);
        result.push_back(r);
        pos += rsize;

        encryptBuffer(buf.data(), rsize); // restaurar para proxima lectura
    }
    return result;
}

void StorageManager::writeAllRows(const Table& table, const std::vector<Row>& rows) {
    int rsize = rowSize(table);
    std::ofstream f(tablePath(table.getDatabaseName(), table.getName()),
                    std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("Cannot open table file for writing");

    for (const auto& row : rows) {
        std::vector<char> buf(rsize, 0);
        int offset = 0;
        for (int i = 0; i < (int)table.getColumns().size(); i++)
            writeField(buf.data(), offset, row.getField(i), table.getColumns()[i]);
        encryptBuffer(buf.data(), rsize);
        f.write(buf.data(), rsize);
    }
}

int StorageManager::countRows(const Table& table) const {
    std::string path = tablePath(table.getDatabaseName(), table.getName());
    if (!fs::exists(path)) return 0;
    int rsize = rowSize(table);
    if (rsize == 0) return 0;
    uintmax_t fileSize = fs::file_size(path);
    return (int)(fileSize / rsize);
}