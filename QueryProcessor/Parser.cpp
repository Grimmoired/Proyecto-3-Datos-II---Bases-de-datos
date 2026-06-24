//
// Created by j1p2p3a4 on 6/16/2026.
//
#include "Parser.h"
#include <cctype>
#include <stdexcept>
#include <algorithm>
#include <unordered_set>


std::string Parser::toUpper(const std::string& s) const {
    std::string r = s;
    for (auto& c : r) c = toupper(c);
    return r;
}

static bool isKeyword(const std::string& s) {
    static const std::unordered_set<std::string> keywords = {
        "CREATE","DATABASE","SET","TABLE","AS","DROP","INDEX","ON","OF","TYPE",
        "INSERT","INTO","VALUES","SELECT","FROM","WHERE","ORDER","BY","ASC","DESC",
        "UPDATE","DELETE","BTREE","BST","INTEGER","DOUBLE","VARCHAR","DATETIME",
        "LIKE","NOT","NULL","PRIMARY","KEY"
    };
    return keywords.count(s) > 0;
}

std::vector<Token> Parser::tokenize(const std::string& sql) const {
    std::vector<Token> result;
    int i = 0;
    int n = (int)sql.size();

    while (i < n) {
        // Espacios
        if (isspace(sql[i])) { i++; continue; }

        // Comentario de linea
        if (i + 1 < n && sql[i] == '/' && sql[i+1] == '/') {
            while (i < n && sql[i] != '\n') i++;
            continue;
        }

        // String literal
        if (sql[i] == '"' || sql[i] == '\'') {
            char delim = sql[i++];
            std::string val;
            while (i < n && sql[i] != delim) val += sql[i++];
            if (i < n) i++; // consumir cierre
            result.push_back({TokenType::STRING, val});
            continue;
        }

        // Simbolos
        if (sql[i] == '(' || sql[i] == ')' || sql[i] == ',' ||
            sql[i] == ';' || sql[i] == '*' || sql[i] == '=' ||
            sql[i] == '<' || sql[i] == '>') {
            // Detectar ==
            if (sql[i] == '=' && i + 1 < n && sql[i+1] == '=') {
                result.push_back({TokenType::SYMBOL, "="});
                i += 2;
            } else {
                result.push_back({TokenType::SYMBOL, std::string(1, sql[i])});
                i++;
            }
            continue;
        }

        // Numero
        if (isdigit(sql[i]) || (sql[i] == '-' && i + 1 < n && isdigit(sql[i+1]))) {
            std::string val;
            if (sql[i] == '-') val += sql[i++];
            while (i < n && (isdigit(sql[i]) || sql[i] == '.')) val += sql[i++];
            result.push_back({TokenType::NUMBER, val});
            continue;
        }

        // Identificador o keyword
        if (isalpha(sql[i]) || sql[i] == '_') {
            std::string val;
            while (i < n && (isalnum(sql[i]) || sql[i] == '_')) val += sql[i++];
            std::string up = val;
            for (auto& c : up) c = toupper(c);
            if (isKeyword(up))
                result.push_back({TokenType::KEYWORD, up});
            else
                result.push_back({TokenType::IDENTIFIER, val});
            continue;
        }

        // Wildcard con asterisco en LIKE *texto*
        if (sql[i] == '*') {
            result.push_back({TokenType::SYMBOL, "*"});
            i++;
            continue;
        }

        i++; // caracter desconocido, ignorar
    }

    result.push_back({TokenType::END, ""});
    return result;
}

Token Parser::peek() const {
    if (pos < (int)tokens.size()) return tokens[pos];
    return {TokenType::END, ""};
}

Token Parser::consume() {
    Token t = peek();
    pos++;
    return t;
}

Token Parser::expect(const std::string& value) {
    Token t = consume();
    std::string up = t.value;
    for (auto& c : up) c = toupper(c);
    std::string expUp = value;
    for (auto& c : expUp) c = toupper(c);
    if (up != expUp)
        throw std::runtime_error("Expected '" + value + "' but got '" + t.value + "'");
    return t;
}

bool Parser::match(const std::string& value) const {
    std::string up = peek().value;
    for (auto& c : up) c = toupper(c);
    std::string valUp = value;
    for (auto& c : valUp) c = toupper(c);
    return up == valUp;
}

bool Parser::matchType(TokenType t) const {
    return peek().type == t;
}

ParsedStatement Parser::parse(const std::string& sql) {
    tokens = tokenize(sql);
    pos = 0;

    if (peek().type == TokenType::END)
        throw std::runtime_error("Empty statement");

    std::string kw = toUpper(peek().value);

    if (kw == "CREATE") {
        consume();
        std::string next = toUpper(peek().value);
        if (next == "DATABASE") return parseCreateDatabase();
        if (next == "TABLE")    return parseCreateTable();
        if (next == "INDEX")    return parseCreateIndex();
        throw std::runtime_error("Unknown CREATE target: " + next);
    }
    if (kw == "SET")    { consume(); return parseSetDatabase(); }
    if (kw == "DROP")   { consume(); return parseDropTable(); }
    if (kw == "INSERT") { consume(); return parseInsert(); }
    if (kw == "SELECT") { consume(); return parseSelect(); }
    if (kw == "UPDATE") { consume(); return parseUpdate(); }
    if (kw == "DELETE") { consume(); return parseDelete(); }

    throw std::runtime_error("Unknown statement: " + peek().value);
}

ParsedStatement Parser::parseCreateDatabase() {
    expect("DATABASE");
    ParsedStatement s;
    s.type = StatementType::CREATE_DATABASE;
    s.databaseName = consume().value;
    return s;
}

ParsedStatement Parser::parseSetDatabase() {
    expect("DATABASE");
    ParsedStatement s;
    s.type = StatementType::SET_DATABASE;
    s.databaseName = consume().value;
    return s;
}

ParsedStatement Parser::parseDropTable() {
    expect("TABLE");
    ParsedStatement s;
    s.type = StatementType::DROP_TABLE;
    s.tableName = consume().value;
    return s;
}

ParsedStatement Parser::parseCreateTable() {
    expect("TABLE");
    ParsedStatement s;
    s.type = StatementType::CREATE_TABLE;
    s.tableName = consume().value;

    // Soporte para AS opcional
    if (match("AS")) consume();

    expect("(");
    while (!match(")") && peek().type != TokenType::END) {
        ParsedStatement::ColDef col;
        col.varcharLen = 0;
        col.nullable = true;

        col.name = consume().value;
        col.typeName = toUpper(consume().value);

        if (col.typeName == "VARCHAR") {
            expect("(");
            col.varcharLen = std::stoi(consume().value);
            expect(")");
        }

        // Nullable opcional
        if (match("NOT")) {
            consume();
            if (match("NULL")) { consume(); col.nullable = false; }
        } else if (match("NULL")) {
            consume(); col.nullable = true;
        }

        s.columnDefs.push_back(col);

        if (match(",")) consume();
    }
    expect(")");
    return s;
}

ParsedStatement Parser::parseCreateIndex() {
    expect("INDEX");
    ParsedStatement s;
    s.type = StatementType::CREATE_INDEX;
    s.indexName = consume().value;
    expect("ON");
    s.tableName = consume().value;
    expect("(");
    s.indexColumn = consume().value;
    expect(")");
    expect("OF");
    expect("TYPE");
    s.indexType = toUpper(consume().value);
    return s;
}

ParsedStatement Parser::parseInsert() {
    expect("INTO");
    ParsedStatement s;
    s.type = StatementType::INSERT;
    s.tableName = consume().value;
    expect("VALUES");
    expect("(");

    while (!match(")") && peek().type != TokenType::END) {
        Token t = consume();
        s.insertValues.push_back(t.value);
        if (match(",")) consume();
    }
    expect(")");
    return s;
}

ParsedStatement Parser::parseSelect() {
    ParsedStatement s;
    s.type = StatementType::SELECT;
    s.selectAll = false;

    if (match("*")) {
        consume();
        s.selectAll = true;
    } else {
        while (!match("FROM") && peek().type != TokenType::END) {
            s.selectColumns.push_back(consume().value);
            if (match(",")) consume();
        }
    }

    expect("FROM");
    s.tableName = consume().value;

    if (match("WHERE")) {
        consume();
        s.where = parseWhere();
    }

    if (match("ORDER")) {
        consume();
        expect("BY");
        s.orderByColumn = consume().value;
        if (match("ASC") || match("DESC"))
            s.orderByDir = toUpper(consume().value);
        else
            s.orderByDir = "ASC";
    }

    return s;
}

ParsedStatement Parser::parseUpdate() {
    ParsedStatement s;
    s.type = StatementType::UPDATE;
    s.tableName = consume().value;
    expect("SET");
    s.updateColumn = consume().value;
    expect("=");
    s.updateValue = consume().value;

    if (match("WHERE")) {
        consume();
        s.where = parseWhere();
    }

    return s;
}

ParsedStatement Parser::parseDelete() {
    expect("FROM");
    ParsedStatement s;
    s.type = StatementType::DELETE_ROWS;
    s.tableName = consume().value;

    if (match("WHERE")) {
        consume();
        s.where = parseWhere();
    }

    return s;
}

WhereClause Parser::parseWhere() {
    WhereClause w;
    w.present = true;
    w.column = consume().value;

    Token opToken = consume();
    w.op = toUpper(opToken.value);

    // LIKE: valor puede venir con asteriscos pegados
    if (w.op == "LIKE") {
        std::string val;
        // consumir tokens hasta espacio/fin (puede ser *texto* como un solo token o varios)
        while (peek().type != TokenType::END &&
               !match("ORDER") && !match("AND") && !match(";")) {
            val += consume().value;
        }
        // Quitar asteriscos externos para guardar solo el patron
        if (!val.empty() && val.front() == '*') val = val.substr(1);
        if (!val.empty() && val.back() == '*') val.pop_back();
        w.value = val;
    } else {
        w.value = consume().value;
    }

    return w;
}