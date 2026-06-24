//
// Created by j1p2p3a4 on 6/16/2026.
//

#include "SystemCatalog.h"
#include <fstream>
#include <filesystem>
#include <cstring>
#include <stdexcept>

namespace fs = std::filesystem;

// Estructura fija que se serializa a disco para cada DB
struct DBEntry {
    char name[64];
};

// Estructura fija que se serializa a disco para cada tabla
struct TableEntry {
    char name[64];
    char dbName[64];
    int columnCount;
};

// Estructura fija para cada columna en disco
struct ColumnEntry {
    char tableName[64];
    char dbName[64];
    char colName[64];
    int type;           // cast de DataType
    int varcharLength;
    bool nullable;
};


SystemCatalog::SystemCatalog(const std::string& path) {
    catalogPath = path;
}

std::string SystemCatalog::dbsFilePath() const     { return catalogPath + "/SystemDatabases.bin"; }
std::string SystemCatalog::tablesFilePath() const  { return catalogPath + "/SystemTables.bin"; }
std::string SystemCatalog::columnsFilePath() const { return catalogPath + "/SystemColumns.bin"; }
std::string SystemCatalog::indexesFilePath() const { return catalogPath + "/SystemIndexes.bin"; }

void SystemCatalog::init() {
    fs::create_directories(catalogPath);
    // Crear archivos vacios si no existen
    for (auto& p : {dbsFilePath(), tablesFilePath(), columnsFilePath(), indexesFilePath()}) {
        if (!fs::exists(p)) {
            std::ofstream f(p, std::ios::binary);
        }
    }
}


bool SystemCatalog::databaseExists(const std::string& dbName) const {
    std::ifstream f(dbsFilePath(), std::ios::binary);
    DBEntry e;
    while (f.read(reinterpret_cast<char*>(&e), sizeof(DBEntry))) {
        if (std::string(e.name) == dbName) return true;
    }
    return false;
}

void SystemCatalog::addDatabase(const std::string& dbName) {
    if (databaseExists(dbName)) return;
    std::ofstream f(dbsFilePath(), std::ios::binary | std::ios::app);
    DBEntry e;
    strncpy(e.name, dbName.c_str(), 63);
    e.name[63] = '\0';
    f.write(reinterpret_cast<char*>(&e), sizeof(DBEntry));
}

std::vector<std::string> SystemCatalog::getDatabases() const {
    std::vector<std::string> result;
    std::ifstream f(dbsFilePath(), std::ios::binary);
    DBEntry e;
    while (f.read(reinterpret_cast<char*>(&e), sizeof(DBEntry)))
        result.push_back(std::string(e.name));
    return result;
}


bool SystemCatalog::tableExists(const std::string& dbName, const std::string& tableName) const {
    std::ifstream f(tablesFilePath(), std::ios::binary);
    TableEntry e;
    while (f.read(reinterpret_cast<char*>(&e), sizeof(TableEntry))) {
        if (std::string(e.dbName) == dbName && std::string(e.name) == tableName)
            return true;
    }
    return false;
}

void SystemCatalog::addTable(const Table& table) {
    if (tableExists(table.getDatabaseName(), table.getName())) return;

    // Escribir entrada de tabla
    {
        std::ofstream f(tablesFilePath(), std::ios::binary | std::ios::app);
        TableEntry e;
        strncpy(e.name, table.getName().c_str(), 63);       e.name[63] = '\0';
        strncpy(e.dbName, table.getDatabaseName().c_str(), 63); e.dbName[63] = '\0';
        e.columnCount = (int)table.getColumns().size();
        f.write(reinterpret_cast<char*>(&e), sizeof(TableEntry));
    }

    // Escribir columnas
    {
        std::ofstream f(columnsFilePath(), std::ios::binary | std::ios::app);
        for (const auto& col : table.getColumns()) {
            ColumnEntry ce;
            strncpy(ce.tableName, table.getName().c_str(), 63);       ce.tableName[63] = '\0';
            strncpy(ce.dbName, table.getDatabaseName().c_str(), 63);  ce.dbName[63] = '\0';
            strncpy(ce.colName, col.getName().c_str(), 63);           ce.colName[63] = '\0';
            ce.type = (int)col.getType();
            ce.varcharLength = col.getVarcharLength();
            ce.nullable = col.isNullable();
            f.write(reinterpret_cast<char*>(&ce), sizeof(ColumnEntry));
        }
    }
}

void SystemCatalog::removeTable(const std::string& dbName, const std::string& tableName) {
    // Reescribir SystemTables sin la entrada eliminada
    std::vector<TableEntry> tables;
    {
        std::ifstream f(tablesFilePath(), std::ios::binary);
        TableEntry e;
        while (f.read(reinterpret_cast<char*>(&e), sizeof(TableEntry))) {
            if (!(std::string(e.dbName) == dbName && std::string(e.name) == tableName))
                tables.push_back(e);
        }
    }
    {
        std::ofstream f(tablesFilePath(), std::ios::binary | std::ios::trunc);
        for (auto& e : tables)
            f.write(reinterpret_cast<char*>(&e), sizeof(TableEntry));
    }

    // Reescribir SystemColumns sin las columnas de esa tabla
    std::vector<ColumnEntry> cols;
    {
        std::ifstream f(columnsFilePath(), std::ios::binary);
        ColumnEntry ce;
        while (f.read(reinterpret_cast<char*>(&ce), sizeof(ColumnEntry))) {
            if (!(std::string(ce.dbName) == dbName && std::string(ce.tableName) == tableName))
                cols.push_back(ce);
        }
    }
    {
        std::ofstream f(columnsFilePath(), std::ios::binary | std::ios::trunc);
        for (auto& ce : cols)
            f.write(reinterpret_cast<char*>(&ce), sizeof(ColumnEntry));
    }
}

std::vector<Table> SystemCatalog::getTables(const std::string& dbName) const {
    std::vector<Table> result;
    std::ifstream f(tablesFilePath(), std::ios::binary);
    TableEntry e;
    while (f.read(reinterpret_cast<char*>(&e), sizeof(TableEntry))) {
        if (std::string(e.dbName) == dbName) {
            Table t(std::string(e.name), std::string(e.dbName));
            // Cargar columnas
            std::ifstream fc(columnsFilePath(), std::ios::binary);
            ColumnEntry ce;
            while (fc.read(reinterpret_cast<char*>(&ce), sizeof(ColumnEntry))) {
                if (std::string(ce.dbName) == dbName && std::string(ce.tableName) == e.name) {
                    Column col(std::string(ce.colName), (DataType)ce.type, ce.varcharLength, ce.nullable);
                    t.addColumn(col);
                }
            }
            result.push_back(t);
        }
    }
    return result;
}

Table SystemCatalog::getTable(const std::string& dbName, const std::string& tableName) const {
    Table t(tableName, dbName);
    std::ifstream f(columnsFilePath(), std::ios::binary);
    ColumnEntry ce;
    while (f.read(reinterpret_cast<char*>(&ce), sizeof(ColumnEntry))) {
        if (std::string(ce.dbName) == dbName && std::string(ce.tableName) == tableName) {
            Column col(std::string(ce.colName), (DataType)ce.type, ce.varcharLength, ce.nullable);
            t.addColumn(col);
        }
    }
    return t;
}

bool SystemCatalog::indexExists(const std::string& dbName, const std::string& tableName) const {
    std::ifstream f(indexesFilePath(), std::ios::binary);
    IndexEntry e;
    while (f.read(reinterpret_cast<char*>(&e), sizeof(IndexEntry))) {
        if (std::string(e.dbName) == dbName && std::string(e.tableName) == tableName)
            return true;
    }
    return false;
}

void SystemCatalog::addIndex(const IndexEntry& entry) {
    std::ofstream f(indexesFilePath(), std::ios::binary | std::ios::app);
    f.write(reinterpret_cast<const char*>(&entry), sizeof(IndexEntry));
}

void SystemCatalog::removeIndex(const std::string& dbName, const std::string& tableName) {
    std::vector<IndexEntry> entries;
    {
        std::ifstream f(indexesFilePath(), std::ios::binary);
        IndexEntry e;
        while (f.read(reinterpret_cast<char*>(&e), sizeof(IndexEntry))) {
            if (!(std::string(e.dbName) == dbName && std::string(e.tableName) == tableName))
                entries.push_back(e);
        }
    }
    {
        std::ofstream f(indexesFilePath(), std::ios::binary | std::ios::trunc);
        for (auto& e : entries)
            f.write(reinterpret_cast<const char*>(&e), sizeof(IndexEntry));
    }
}

std::vector<IndexEntry> SystemCatalog::getIndexes(const std::string& dbName) const {
    std::vector<IndexEntry> result;
    std::ifstream f(indexesFilePath(), std::ios::binary);
    IndexEntry e;
    while (f.read(reinterpret_cast<char*>(&e), sizeof(IndexEntry))) {
        if (std::string(e.dbName) == dbName)
            result.push_back(e);
    }
    return result;
}

IndexEntry SystemCatalog::getIndex(const std::string& dbName, const std::string& tableName) const {
    std::ifstream f(indexesFilePath(), std::ios::binary);
    IndexEntry e;
    while (f.read(reinterpret_cast<char*>(&e), sizeof(IndexEntry))) {
        if (std::string(e.dbName) == dbName && std::string(e.tableName) == tableName)
            return e;
    }
    throw std::runtime_error("Index not found: " + tableName);
}