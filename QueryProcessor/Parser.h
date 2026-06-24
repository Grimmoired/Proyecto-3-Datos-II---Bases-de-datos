//
// Created by j1p2p3a4 on 6/16/2026.
//

#ifndef UNTITLED2_PARSER_H
#define UNTITLED2_PARSER_H



#pragma once
#include <string>
#include <vector>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    NUMBER,
    STRING,      // literal entre comillas
    SYMBOL,      // ( ) , ; = < > *
    END
};

struct Token {
    TokenType type;
    std::string value;
};

// Tipos de sentencia reconocidos
enum class StatementType {
    CREATE_DATABASE,
    SET_DATABASE,
    CREATE_TABLE,
    DROP_TABLE,
    CREATE_INDEX,
    INSERT,
    SELECT,
    UPDATE,
    DELETE_ROWS,
    UNKNOWN
};

// Condicion WHERE
struct WhereClause {
    std::string column;
    std::string op;      // >, <, =, LIKE, NOT
    std::string value;
    bool present;

    WhereClause() { present = false; }
};

// Resultado del parse, estructura generica que el QueryProcessor interpreta
struct ParsedStatement {
    StatementType type;

    // CREATE DATABASE / SET DATABASE
    std::string databaseName;

    // CREATE TABLE
    std::string tableName;
    // columnas: nombre, tipo, varcharLen, nullable
    struct ColDef {
        std::string name;
        std::string typeName;
        int varcharLen;
        bool nullable;
    };
    std::vector<ColDef> columnDefs;

    // CREATE INDEX
    std::string indexName;
    std::string indexColumn;
    std::string indexType; // BTREE o BST

    // INSERT
    std::vector<std::string> insertValues;

    // SELECT
    bool selectAll;
    std::vector<std::string> selectColumns;
    std::string orderByColumn;
    std::string orderByDir; // ASC o DESC

    // UPDATE
    std::string updateColumn;
    std::string updateValue;

    // WHERE (SELECT, UPDATE, DELETE)
    WhereClause where;

    ParsedStatement() {
        type = StatementType::UNKNOWN;
        selectAll = false;
    }
};

class Parser {
private:
    std::vector<Token> tokens;
    int pos;

    Token peek() const;
    Token consume();
    Token expect(const std::string& value);
    bool match(const std::string& value) const;
    bool matchType(TokenType t) const;

    std::string toUpper(const std::string& s) const;
    std::vector<Token> tokenize(const std::string& sql) const;

    ParsedStatement parseCreateDatabase();
    ParsedStatement parseSetDatabase();
    ParsedStatement parseCreateTable();
    ParsedStatement parseCreateIndex();
    ParsedStatement parseDropTable();
    ParsedStatement parseInsert();
    ParsedStatement parseSelect();
    ParsedStatement parseUpdate();
    ParsedStatement parseDelete();
    WhereClause parseWhere();

public:
    ParsedStatement parse(const std::string& sql);
};

#endif //UNTITLED2_PARSER_H
