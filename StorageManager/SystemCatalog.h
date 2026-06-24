//
// Created by j1p2p3a4 on 6/16/2026.
//

#ifndef UNTITLED2_SYSTEMCATALOG_H
#define UNTITLED2_SYSTEMCATALOG_H



#pragma once
#include <string>
#include <vector>
#include "../Models/Table.h"

// Metadata de un indice guardada en el catalog
struct IndexEntry {
    char indexName[64];
    char tableName[64];
    char columnName[64];
    char dbName[64];
    char type[8]; // "BTREE" o "BST"
};

class SystemCatalog {
private:
    std::string catalogPath;  // carpeta raiz del catalog

    std::string dbsFilePath() const;
    std::string tablesFilePath() const;
    std::string columnsFilePath() const;
    std::string indexesFilePath() const;

public:
    SystemCatalog(const std::string& path);

    void init();  // crea la carpeta y archivos si no existen

    // Databases
    bool databaseExists(const std::string& dbName) const;
    void addDatabase(const std::string& dbName);
    std::vector<std::string> getDatabases() const;

    // Tables
    bool tableExists(const std::string& dbName, const std::string& tableName) const;
    void addTable(const Table& table);
    void removeTable(const std::string& dbName, const std::string& tableName);
    std::vector<Table> getTables(const std::string& dbName) const;
    Table getTable(const std::string& dbName, const std::string& tableName) const;

    // Indexes
    bool indexExists(const std::string& dbName, const std::string& tableName) const;
    void addIndex(const IndexEntry& entry);
    void removeIndex(const std::string& dbName, const std::string& tableName);
    std::vector<IndexEntry> getIndexes(const std::string& dbName) const;
    IndexEntry getIndex(const std::string& dbName, const std::string& tableName) const;
};


#endif //UNTITLED2_SYSTEMCATALOG_H
