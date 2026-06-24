//
// Created by j1p2p3a4 on 6/16/2026.
//

#ifndef UNTITLED2_TABLE_H
#define UNTITLED2_TABLE_H
#pragma once
#include <string>
#include <vector>
#include "Column.h"
#include "Row.h"

class Table {
private:
    std::string name;
    std::string databaseName;
    std::vector<Column> columns;

public:
    Table() {}

    Table(const std::string& n, const std::string& db) {
        name = n;
        databaseName = db;
    }

    std::string getName() const { return name; }
    std::string getDatabaseName() const { return databaseName; }
    std::vector<Column>& getColumns() { return columns; }
    const std::vector<Column>& getColumns() const { return columns; }

    void addColumn(const Column& col) { columns.push_back(col); }

    int getColumnIndex(const std::string& colName) const {
        for (int i = 0; i < (int)columns.size(); i++)
            if (columns[i].getName() == colName)
                return i;
        return -1;
    }
};
#endif //UNTITLED2_TABLE_H
