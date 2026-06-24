//
// Created by j1p2p3a4 on 6/16/2026.
//

#ifndef UNTITLED2_ROW_H
#define UNTITLED2_ROW_H
#pragma once
#include <string>
#include <vector>

class Row {
private:
    std::vector<std::string> fields;
    long long diskOffset;

public:
    Row() : diskOffset(-1) {}

    Row(const std::vector<std::string>& values) {
        fields = values;
        diskOffset = -1;
    }

    void addField(const std::string& val) { fields.push_back(val); }
    std::string getField(int i) const { return fields[i]; }
    int fieldCount() const { return (int)fields.size(); }

    long long getDiskOffset() const { return diskOffset; }
    void setDiskOffset(long long offset) { diskOffset = offset; }
};
#endif //UNTITLED2_ROW_H
