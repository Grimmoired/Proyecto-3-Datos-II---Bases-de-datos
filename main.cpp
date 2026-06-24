#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include "QueryProcessor/QueryProcessor.h"

// Imprime el resultado de un SELECT como tabla en consola
void printResult(const QueryResult& result) {
    if (!result.success) {
        std::cout << "[ERROR] " << result.message << "\n";
        return;
    }

    std::cout << "[OK] " << result.message << "\n";

    if (result.columns.empty()) return;

    // Calcular ancho de cada columna
    std::vector<int> widths;
    for (const auto& col : result.columns)
        widths.push_back((int)col.size());

    for (const auto& row : result.rows)
        for (int i = 0; i < row.fieldCount(); i++)
            widths[i] = std::max(widths[i], (int)row.getField(i).size());

    // Header
    for (int i = 0; i < (int)result.columns.size(); i++)
        std::cout << "| " << std::left << std::setw(widths[i]) << result.columns[i] << " ";
    std::cout << "|\n";

    // Separador
    for (int i = 0; i < (int)result.columns.size(); i++)
        std::cout << "+-" << std::string(widths[i], '-') << "-";
    std::cout << "+\n";

    // Filas
    for (const auto& row : result.rows) {
        for (int i = 0; i < row.fieldCount(); i++)
            std::cout << "| " << std::left << std::setw(widths[i]) << row.getField(i) << " ";
        std::cout << "|\n";
    }
}

// Divide un script por ; respetando strings entre comillas
std::vector<std::string> splitStatements(const std::string& script) {
    std::vector<std::string> stmts;
    std::string current;
    bool inString = false;
    char strChar = 0;

    for (int i = 0; i < (int)script.size(); i++) {
        char c = script[i];

        if (!inString && (c == '"' || c == '\'')) {
            inString = true;
            strChar = c;
            current += c;
        } else if (inString && c == strChar) {
            inString = false;
            current += c;
        } else if (!inString && c == ';') {
            // Ignorar comentarios de linea
            std::string trimmed;
            for (auto& ch : current)
                if (ch != '\r') trimmed += ch;
            // trim
            int s = 0, e = (int)trimmed.size() - 1;
            while (s <= e && isspace(trimmed[s])) s++;
            while (e >= s && isspace(trimmed[e])) e--;
            trimmed = trimmed.substr(s, e - s + 1);
            if (!trimmed.empty()) stmts.push_back(trimmed);
            current.clear();
        } else {
            current += c;
        }
    }

    // Ultima sentencia sin ;
    std::string trimmed;
    for (auto& ch : current) if (ch != '\r') trimmed += ch;
    int s = 0, e = (int)trimmed.size() - 1;
    while (s <= e && isspace(trimmed[s])) s++;
    while (e >= s && isspace(trimmed[e])) e--;
    trimmed = trimmed.substr(s, e - s + 1);
    if (!trimmed.empty()) stmts.push_back(trimmed);

    return stmts;
}

int main() {
    QueryProcessor qp("./catalog", "./data");

    std::cout << "TinySQLDb - escribe sentencias SQL (termina con ; para ejecutar)\n";
    std::cout << "Escribe 'exit' para salir.\n\n";

    std::string line, buffer;

    while (true) {
        std::cout << (buffer.empty() ? "sql> " : "  -> ");
        if (!std::getline(std::cin, line)) break;

        if (line == "exit") break;

        buffer += " " + line;

        // Ejecutar cuando hay al menos un ;
        if (buffer.find(';') != std::string::npos) {
            auto stmts = splitStatements(buffer);
            for (const auto& stmt : stmts) {
                std::cout << "\n>> " << stmt << "\n";
                auto result = qp.execute(stmt);
                printResult(result);
            }
            buffer.clear();
        }
    }

    std::cout << "Bye.\n";
    return 0;
}