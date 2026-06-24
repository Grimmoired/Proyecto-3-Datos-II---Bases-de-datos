//
// Created by j1p2p3a4 on 6/16/2026.
//

#include "QueryProcessor.h"
#include <stdexcept>
#include <algorithm>
#include <sstream>


QueryProcessor::QueryProcessor(const std::string& catalogPath, const std::string& dataPath)
    : catalog(catalogPath), storage(dataPath) {
    catalog.init();
    loadIndexesFromCatalog();
}

QueryResult QueryProcessor::execute(const std::string& sql) {
    try {
        ParsedStatement s = parser.parse(sql);
        switch (s.type) {
            case StatementType::CREATE_DATABASE: return execCreateDatabase(s);
            case StatementType::SET_DATABASE:    return execSetDatabase(s);
            case StatementType::CREATE_TABLE:    return execCreateTable(s);
            case StatementType::DROP_TABLE:      return execDropTable(s);
            case StatementType::CREATE_INDEX:    return execCreateIndex(s);
            case StatementType::INSERT:          return execInsert(s);
            case StatementType::SELECT:          return execSelect(s);
            case StatementType::UPDATE:          return execUpdate(s);
            case StatementType::DELETE_ROWS:     return execDelete(s);
            default: return {false, "Unknown statement type", {}, {}};
        }
    } catch (const std::exception& e) {
        return {false, std::string("Error: ") + e.what(), {}, {}};
    }
}


std::string QueryProcessor::indexKey(const std::string& db, const std::string& table) const {
    return db + "." + table;
}

DataType QueryProcessor::parseDataType(const std::string& typeName, int& varcharLen) const {
    if (typeName == "INTEGER")  return DataType::INTEGER;
    if (typeName == "DOUBLE")   return DataType::DOUBLE;
    if (typeName == "DATETIME") return DataType::DATETIME;
    if (typeName == "VARCHAR")  return DataType::VARCHAR;
    throw std::runtime_error("Unknown data type: " + typeName);
}

void QueryProcessor::loadIndexesFromCatalog() {
    for (const auto& dbName : catalog.getDatabases()) {
        auto idxEntries = catalog.getIndexes(dbName);
        for (const auto& entry : idxEntries) {
            std::string tname = std::string(entry.tableName);
            std::string col   = std::string(entry.columnName);
            std::string type  = std::string(entry.type);
            std::string key   = indexKey(dbName, tname);

            Table table = catalog.getTable(dbName, tname);
            int colIdx = table.getColumnIndex(col);
            if (colIdx < 0) continue;

            IndexHandle h;
            h.type   = type;
            h.column = col;
            h.bst    = nullptr;
            h.btree  = nullptr;

            if (type == "BST")   h.bst   = new BST();
            if (type == "BTREE") h.btree = new BTree();

            // Cargar datos existentes en el indice
            auto rows = storage.readAllRows(table);
            for (const auto& row : rows) {
                std::string val = row.getField(colIdx);
                if (type == "BST")   h.bst->insert(val, row.getDiskOffset());
                if (type == "BTREE") h.btree->insert(val, row.getDiskOffset());
            }

            indexes[key] = h;
        }
    }
}

void QueryProcessor::insertIntoIndex(const std::string& db, const std::string& table,
                                     const std::string& key, long long offset) {
    auto it = indexes.find(indexKey(db, table));
    if (it == indexes.end()) return;
    auto& h = it->second;
    if (h.type == "BST")   h.bst->insert(key, offset);
    if (h.type == "BTREE") h.btree->insert(key, offset);
}

void QueryProcessor::removeFromIndex(const std::string& db, const std::string& table,
                                     const std::string& key) {
    auto it = indexes.find(indexKey(db, table));
    if (it == indexes.end()) return;
    auto& h = it->second;
    if (h.type == "BST")   h.bst->remove(key);
    if (h.type == "BTREE") h.btree->remove(key);
}

long long QueryProcessor::searchIndex(const std::string& db, const std::string& table,
                                      const std::string& key) const {
    auto it = indexes.find(indexKey(db, table));
    if (it == indexes.end()) return -1;
    const auto& h = it->second;
    if (h.type == "BST")   return h.bst->search(key);
    if (h.type == "BTREE") return h.btree->search(key);
    return -1;
}

bool QueryProcessor::matchesWhere(const Row& row, const Table& table,
                                   const WhereClause& where) const {
    if (!where.present) return true;
    int idx = table.getColumnIndex(where.column);
    if (idx < 0) throw std::runtime_error("Column not found: " + where.column);
    std::string val = row.getField(idx);

    if (where.op == "=")  return val == where.value;
    if (where.op == ">")  return val > where.value;
    if (where.op == "<")  return val < where.value;
    if (where.op == "NOT") return val != where.value;
    if (where.op == "LIKE") {
        return val.find(where.value) != std::string::npos;
    }
    return false;
}

void QueryProcessor::quicksort(std::vector<Row>& rows, int colIdx, bool asc, int lo, int hi) const {
    if (lo >= hi) return;
    std::string pivot = rows[(lo + hi) / 2].getField(colIdx);
    int i = lo, j = hi;
    while (i <= j) {
        while (asc ? rows[i].getField(colIdx) < pivot : rows[i].getField(colIdx) > pivot) i++;
        while (asc ? rows[j].getField(colIdx) > pivot : rows[j].getField(colIdx) < pivot) j--;
        if (i <= j) { std::swap(rows[i], rows[j]); i++; j--; }
    }
    quicksort(rows, colIdx, asc, lo, j);
    quicksort(rows, colIdx, asc, i, hi);
}

QueryResult QueryProcessor::execCreateDatabase(const ParsedStatement& s) {
    if (catalog.databaseExists(s.databaseName))
        return {false, "Database already exists: " + s.databaseName, {}, {}};
    catalog.addDatabase(s.databaseName);
    storage.createDatabase(s.databaseName);
    return {true, "Database created: " + s.databaseName, {}, {}};
}

QueryResult QueryProcessor::execSetDatabase(const ParsedStatement& s) {
    if (!catalog.databaseExists(s.databaseName))
        return {false, "Database not found: " + s.databaseName, {}, {}};
    currentDb = s.databaseName;
    return {true, "Database set to: " + s.databaseName, {}, {}};
}


QueryResult QueryProcessor::execCreateTable(const ParsedStatement& s) {
    if (currentDb.empty()) return {false, "No database selected", {}, {}};
    if (catalog.tableExists(currentDb, s.tableName))
        return {false, "Table already exists: " + s.tableName, {}, {}};

    Table table(s.tableName, currentDb);
    for (const auto& def : s.columnDefs) {
        int vlen = def.varcharLen;
        DataType dt = parseDataType(def.typeName, vlen);
        Column col(def.name, dt, vlen, def.nullable);
        table.addColumn(col);
    }
    catalog.addTable(table);
    return {true, "Table created: " + s.tableName, {}, {}};
}

QueryResult QueryProcessor::execDropTable(const ParsedStatement& s) {
    if (currentDb.empty()) return {false, "No database selected", {}, {}};
    if (!catalog.tableExists(currentDb, s.tableName))
        return {false, "Table not found: " + s.tableName, {}, {}};

    Table table = catalog.getTable(currentDb, s.tableName);
    if (storage.countRows(table) > 0)
        return {false, "Cannot drop table with data: " + s.tableName, {}, {}};

    catalog.removeTable(currentDb, s.tableName);
    storage.dropTable(currentDb, s.tableName);
    return {true, "Table dropped: " + s.tableName, {}, {}};
}

QueryResult QueryProcessor::execCreateIndex(const ParsedStatement& s) {
    if (currentDb.empty()) return {false, "No database selected", {}, {}};
    if (!catalog.tableExists(currentDb, s.tableName))
        return {false, "Table not found: " + s.tableName, {}, {}};
    if (catalog.indexExists(currentDb, s.tableName))
        return {false, "Table already has an index: " + s.tableName, {}, {}};

    Table table = catalog.getTable(currentDb, s.tableName);
    int colIdx = table.getColumnIndex(s.indexColumn);
    if (colIdx < 0) return {false, "Column not found: " + s.indexColumn, {}, {}};

    // Verificar duplicados en datos existentes
    auto rows = storage.readAllRows(table);
    std::unordered_map<std::string, int> seen;
    for (const auto& row : rows) {
        std::string val = row.getField(colIdx);
        if (seen.count(val)) return {false, "Duplicate values in column, cannot create index", {}, {}};
        seen[val]++;
    }

    // Crear indice en memoria
    IndexHandle h;
    h.type   = s.indexType;
    h.column = s.indexColumn;
    h.bst    = nullptr;
    h.btree  = nullptr;

    if (s.indexType == "BST")   h.bst   = new BST();
    if (s.indexType == "BTREE") h.btree = new BTree();

    for (const auto& row : rows) {
        std::string val = row.getField(colIdx);
        if (s.indexType == "BST")   h.bst->insert(val, row.getDiskOffset());
        if (s.indexType == "BTREE") h.btree->insert(val, row.getDiskOffset());
    }

    indexes[indexKey(currentDb, s.tableName)] = h;

    // Registrar en catalogo
    IndexEntry entry;
    strncpy(entry.indexName, s.indexName.c_str(), 63);  entry.indexName[63] = '\0';
    strncpy(entry.tableName, s.tableName.c_str(), 63);  entry.tableName[63] = '\0';
    strncpy(entry.columnName, s.indexColumn.c_str(), 63); entry.columnName[63] = '\0';
    strncpy(entry.dbName, currentDb.c_str(), 63);        entry.dbName[63] = '\0';
    strncpy(entry.type, s.indexType.c_str(), 7);         entry.type[7] = '\0';
    catalog.addIndex(entry);

    return {true, "Index created: " + s.indexName, {}, {}};
}

QueryResult QueryProcessor::execInsert(const ParsedStatement& s) {
    if (currentDb.empty()) return {false, "No database selected", {}, {}};
    if (!catalog.tableExists(currentDb, s.tableName))
        return {false, "Table not found: " + s.tableName, {}, {}};

    Table table = catalog.getTable(currentDb, s.tableName);
    if ((int)s.insertValues.size() != (int)table.getColumns().size())
        return {false, "Value count does not match column count", {}, {}};

    // Validar tipos
    for (int i = 0; i < (int)table.getColumns().size(); i++) {
        const Column& col = table.getColumns()[i];
        const std::string& val = s.insertValues[i];
        try {
            if (col.getType() == DataType::INTEGER) std::stoi(val);
            if (col.getType() == DataType::DOUBLE)  std::stod(val);
        } catch (...) {
            return {false, "Type mismatch on column: " + col.getName(), {}, {}};
        }
    }

    // Verificar duplicado en indice si existe
    std::string ikey = indexKey(currentDb, s.tableName);
    if (indexes.count(ikey)) {
        auto& h = indexes[ikey];
        int colIdx = table.getColumnIndex(h.column);
        std::string val = s.insertValues[colIdx];
        bool dup = (h.type == "BST")   ? h.bst->exists(val)
                 : (h.type == "BTREE") ? h.btree->exists(val) : false;
        if (dup) return {false, "Duplicate key value: " + val, {}, {}};
    }

    // Calcular offset antes de insertar
    int rowSz = storage.countRows(table);
    // El offset real es rowSz * rowSize, pero insertRow hace append
    // Lee el file size despues del insert para obtener el offset correcto
    Row row(s.insertValues);
    storage.insertRow(table, row);

    // El offset de la fila recien insertada es (conteo anterior) * rowSize
    // Lo recalculamos leyendo todas las filas
    auto allRows = storage.readAllRows(table);
    long long newOffset = allRows.back().getDiskOffset();

    // Actualizar indice
    if (indexes.count(ikey)) {
        auto& h = indexes[ikey];
        int colIdx = table.getColumnIndex(h.column);
        insertIntoIndex(currentDb, s.tableName, s.insertValues[colIdx], newOffset);
    }

    return {true, "Row inserted", {}, {}};
}

QueryResult QueryProcessor::execSelect(const ParsedStatement& s) {
    if (currentDb.empty()) return {false, "No database selected", {}, {}};
    if (!catalog.tableExists(currentDb, s.tableName))
        return {false, "Table not found: " + s.tableName, {}, {}};

    Table table = catalog.getTable(currentDb, s.tableName);
    std::vector<Row> result;

    // Usar indice si WHERE aplica sobre columna indexada
    std::string ikey = indexKey(currentDb, s.tableName);
    bool usedIndex = false;

    if (s.where.present && indexes.count(ikey) && s.where.op == "=") {
        auto& h = indexes[ikey];
        if (h.column == s.where.column) {
            long long offset = searchIndex(currentDb, s.tableName, s.where.value);
            if (offset >= 0) {
                auto allRows = storage.readAllRows(table);
                for (const auto& row : allRows)
                    if (row.getDiskOffset() == offset)
                        result.push_back(row);
            }
            usedIndex = true;
        }
    }

    if (!usedIndex) {
        auto allRows = storage.readAllRows(table);
        for (const auto& row : allRows)
            if (matchesWhere(row, table, s.where))
                result.push_back(row);
    }

    // ORDER BY
    if (!s.orderByColumn.empty()) {
        int colIdx = table.getColumnIndex(s.orderByColumn);
        if (colIdx < 0) return {false, "Column not found: " + s.orderByColumn, {}, {}};
        bool asc = (s.orderByDir != "DESC");
        quicksort(result, colIdx, asc, 0, (int)result.size() - 1);
    }

    // Columnas a retornar
    std::vector<std::string> headers;
    std::vector<int> colIdxs;
    if (s.selectAll) {
        for (int i = 0; i < (int)table.getColumns().size(); i++) {
            headers.push_back(table.getColumns()[i].getName());
            colIdxs.push_back(i);
        }
    } else {
        for (const auto& cname : s.selectColumns) {
            int idx = table.getColumnIndex(cname);
            if (idx < 0) return {false, "Column not found: " + cname, {}, {}};
            headers.push_back(cname);
            colIdxs.push_back(idx);
        }
    }

    // Proyectar columnas
    std::vector<Row> projected;
    for (const auto& row : result) {
        std::vector<std::string> vals;
        for (int i : colIdxs) vals.push_back(row.getField(i));
        projected.push_back(Row(vals));
    }

    return {true, std::to_string(projected.size()) + " row(s) found", headers, projected};
}

QueryResult QueryProcessor::execUpdate(const ParsedStatement& s) {
    if (currentDb.empty()) return {false, "No database selected", {}, {}};
    if (!catalog.tableExists(currentDb, s.tableName))
        return {false, "Table not found: " + s.tableName, {}, {}};

    Table table = catalog.getTable(currentDb, s.tableName);
    int updateColIdx = table.getColumnIndex(s.updateColumn);
    if (updateColIdx < 0) return {false, "Column not found: " + s.updateColumn, {}, {}};

    auto rows = storage.readAllRows(table);
    int count = 0;
    std::string ikey = indexKey(currentDb, s.tableName);

    for (auto& row : rows) {
        if (matchesWhere(row, table, s.where)) {
            // Si la columna actualizada esta indexada, actualizar indice
            if (indexes.count(ikey) && indexes[ikey].column == s.updateColumn) {
                removeFromIndex(currentDb, s.tableName, row.getField(updateColIdx));
                // El offset no cambia porque reescribimos todo
                insertIntoIndex(currentDb, s.tableName, s.updateValue, row.getDiskOffset());
            }
            // Mutar el campo directamente no es posible con getField/setField
            // Reconstruimos la fila
            std::vector<std::string> fields;
            for (int i = 0; i < row.fieldCount(); i++)
                fields.push_back(i == updateColIdx ? s.updateValue : row.getField(i));
            row = Row(fields);
            count++;
        }
    }

    storage.writeAllRows(table, rows);
    return {true, std::to_string(count) + " row(s) updated", {}, {}};
}

QueryResult QueryProcessor::execDelete(const ParsedStatement& s) {
    if (currentDb.empty()) return {false, "No database selected", {}, {}};
    if (!catalog.tableExists(currentDb, s.tableName))
        return {false, "Table not found: " + s.tableName, {}, {}};

    Table table = catalog.getTable(currentDb, s.tableName);
    auto rows = storage.readAllRows(table);
    std::string ikey = indexKey(currentDb, s.tableName);
    std::vector<Row> remaining;
    int count = 0;

    for (const auto& row : rows) {
        if (matchesWhere(row, table, s.where)) {
            // Remover del indice
            if (indexes.count(ikey)) {
                int colIdx = table.getColumnIndex(indexes[ikey].column);
                removeFromIndex(currentDb, s.tableName, row.getField(colIdx));
            }
            count++;
        } else {
            remaining.push_back(row);
        }
    }

    storage.writeAllRows(table, remaining);
    return {true, std::to_string(count) + " row(s) deleted", {}, {}};
}
