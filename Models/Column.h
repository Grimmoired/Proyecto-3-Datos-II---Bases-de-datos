//
// Created by j1p2p3a4 on 6/16/2026.
//

#ifndef UNTITLED2_COLUMN_H
#define UNTITLED2_COLUMN_H
#pragma once
#include <string>
#include <cstring>

enum class DataType {
    INTEGER,
    DOUBLE,
    VARCHAR,
    DATETIME
};

class Column {
private:
    char name[64];
    DataType type;
    int varcharLength;
    bool nullable;

public:
    Column() : type(DataType::INTEGER), varcharLength(0), nullable(true) {
        name[0] = '\0';
    }

    Column(const std::string& n, DataType t, int vlen, bool null) {
        strncpy(name, n.c_str(), 63);
        name[63] = '\0';
        type = t;
        varcharLength = vlen;
        nullable = null;
    }

    std::string getName() const { return std::string(name); }
    DataType getType() const { return type; }
    int getVarcharLength() const { return varcharLength; }
    bool isNullable() const { return nullable; }

    void setName(const std::string& n) { strncpy(name, n.c_str(), 63); name[63] = '\0'; }
    void setType(DataType t) { type = t; }
    void setVarcharLength(int vlen) { varcharLength = vlen; }
    void setNullable(bool null) { nullable = null; }
};
#endif //UNTITLED2_COLUMN_H
