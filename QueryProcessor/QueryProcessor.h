//
// Created by j1p2p3a4 on 6/16/2026.
//

#ifndef UNTITLED2_QUERYPROCESSOR_H
#define UNTITLED2_QUERYPROCESSOR_H



#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "../Models/Row.h"
#include "../Models/Table.h"
#include "../StorageManager/SystemCatalog.h"
#include "../StorageManager/StorageManager.h"
#include "../Indexes/BST.h"
#include "../Indexes/BTree.h"
#include "Parser.h"

// Resultado de ejecutar una sentencia
struct QueryResult {
    bool success;
    std::string message;
    std::vector<std::string> columns;  // encabezados para SELECT
    std::vector<Row> rows;             // filas para SELECT
};

// Indice en memoria asociado a una tabla
struct IndexHandle {
    std::string type;       // "BTREE" o "BST"
    std::string column;
    BST*   bst;
    BTree* btree;

    IndexHandle() : bst(nullptr), btree(nullptr) {}
};

class QueryProcessor {
private:
    SystemCatalog catalog;
    StorageManager storage;
    Parser parser;
    std::string currentDb;

    // indices en memoria: clave = dbName + "." + tableName
    std::unordered_map<std::string, IndexHandle> indexes;

    std::string indexKey(const std::string& db, const std::string& table) const;

    // Ejecutores por tipo
    QueryResult execCreateDatabase(const ParsedStatement& s);
    QueryResult execSetDatabase(const ParsedStatement& s);
    QueryResult execCreateTable(const ParsedStatement& s);
    QueryResult execDropTable(const ParsedStatement& s);
    QueryResult execCreateIndex(const ParsedStatement& s);
    QueryResult execInsert(const ParsedStatement& s);
    QueryResult execSelect(const ParsedStatement& s);
    QueryResult execUpdate(const ParsedStatement& s);
    QueryResult execDelete(const ParsedStatement& s);

    // Helpers
    bool matchesWhere(const Row& row, const Table& table, const WhereClause& where) const;
    void quicksort(std::vector<Row>& rows, int colIdx, bool asc, int lo, int hi) const;
    void loadIndexesFromCatalog();
    void insertIntoIndex(const std::string& db, const std::string& table,
                         const std::string& key, long long offset);
    void removeFromIndex(const std::string& db, const std::string& table,
                         const std::string& key);
    long long searchIndex(const std::string& db, const std::string& table,
                          const std::string& key) const;
    DataType parseDataType(const std::string& typeName, int& varcharLen) const;

public:
    QueryProcessor(const std::string& catalogPath, const std::string& dataPath);

    QueryResult execute(const std::string& sql);
    std::string getCurrentDb() const { return currentDb; }
};



#endif //UNTITLED2_QUERYPROCESSOR_H
