# Proyecto #3 - Bases de datos.

Curso de Algoritmos y Estructuras de Datos II (CE 2103)
Instituto Tecnológico de Costa Rica — I Semestre 2026

Este proyecto consistio en la elaboracion de un motor de bases de datos relacional sencillo implementado en C++, apoyado de un cliente web elaborado con ReactJS.

---

## Arquitectura

Cliente React (localhost:5173)
        ↓ HTTP POST /query
Servidor C++ (localhost:8080)
        ↓
  QueryProcessor → Parser → StorageManager → Disco
                          → Índices (BST / BTree)

---

## Dependencias

### Servidor (C++)
- C++20
- cpp-httplib — header-only, incluido en el proyecto (httplib.h)
- nlohmann/json — header-only, incluido en el proyecto (json.hpp)
- MinGW-w64 (Windows) o GCC (Linux/Mac)
- CMake 3.20+

### Cliente (React)
- Node.js 18+
- Vite + React (instalado con npm install)

---

## Compilación del servidor

Primero abra el proyecto en CLion, CMake se encarga de configurar los targets automáticamente.

Targets disponibles:
- tinysqldb_cli       — interfaz por línea de comandos
- tinysqldb_server    — servidor HTTP
- tinysqldb_benchmark — benchmark de índices

---

## Ejecución

### 1. Servidor C++

Primero hay que ejecutar el target tinysqldb_server desde CLion o desde la terminal:

    tinysqldb_server.exe

El servidor queda en modo escucha en http://localhost:8080

Los datos se almacenan en:
- ./catalog/  — metadata (bases de datos, tablas, columnas, índices)
- ./data/     — archivos binarios de cada tabla

### 2. Cliente React

    cd client
    npm install      (solo la primera vez)
    npm run dev

Inserte la URL http://localhost:5173 en su navegador web

---

## Uso

Escriba sentencias SQL separadas por ; en el area de texto desginada y presione Ejecutar.

Ejemplo:

    CREATE DATABASE Universidad;
    SET DATABASE Universidad;
    CREATE TABLE Estudiante (
        ID INTEGER,
        Nombre VARCHAR(30),
        PrimerApellido VARCHAR(30),
        FechaNacimiento DATETIME
    );
    INSERT INTO Estudiante VALUES(2025102303, "Bryan", "Abarca", "2006-16-11 23:11:53");
    SELECT * FROM Estudiante;

---

## Comandos y funciones SQL soportadas:

CREATE DATABASE <name>                                      — Crea una base de datos
SET DATABASE <name>                                         — Establece el contexto
CREATE TABLE <name> (...)                                   — Crea una tabla
DROP TABLE <name>                                           — Elimina una tabla vacía
INSERT INTO <table> VALUES(...)                             — Inserta una fila
SELECT * | <cols> FROM <table>                              — Selecciona una columna de una tabla
  [WHERE ...] [ORDER BY <col> ASC|DESC]                     — Ordena una serie de columnas basandose en algun criterio elegido
UPDATE <table> SET <col> = <val> [WHERE ...]                — Actualiza una columna a un nuevo valor elegido
DELETE FROM <table> [WHERE ...]                             — Elimina un elemento de una tabla
CREATE INDEX <name> ON <table>(<col>) OF TYPE BTREE|BST     — Crea un indice para un elemento de una tabla

Tipos de dato: INTEGER, DOUBLE, VARCHAR(n), DATETIME
Operadores WHERE: =, <, >, NOT, LIKE

---

## Estructura del proyecto

---

## Scripts de prueba

A continuacion una serie de scripts que pueden usarse para probar los comandos y funcionalidades de la base de datos, deben de ejecutarse en orden de aparicion:

-- =============================================
-- SCRIPT 1: Setup base
-- =============================================

CREATE DATABASE Universidad;
SET DATABASE Universidad;

CREATE TABLE Estudiante (
    ID INTEGER,
    Nombre VARCHAR(30),
    PrimerApellido VARCHAR(30),
    SegundoApellido VARCHAR(30),
    FechaNacimiento DATETIME
);

-- =============================================
-- SCRIPT 2: Inserts exitosos
-- =============================================

INSERT INTO Estudiante VALUES(1, "Isaac", "Ramirez", "Herrera", "2000-01-01 01:02:00");
INSERT INTO Estudiante VALUES(2, "Juan", "Ramirez", "X", "2001-05-10 00:00:00");
INSERT INTO Estudiante VALUES(3, "Pedro", "Herrera", "Y", "1999-03-15 00:00:00");
INSERT INTO Estudiante VALUES(4, "Maria", "Lopez", "Z", "2002-07-20 00:00:00");
INSERT INTO Estudiante VALUES(5, "Ana", "Mora", "W", "2000-11-30 00:00:00");

-- =============================================
-- SCRIPT 3: SELECT basicos
-- =============================================

SELECT * FROM Estudiante;
SELECT Nombre, PrimerApellido FROM Estudiante;
SELECT * FROM Estudiante WHERE ID = 3;
SELECT * FROM Estudiante WHERE PrimerApellido LIKE Ramirez;
SELECT * FROM Estudiante ORDER BY Nombre ASC;
SELECT * FROM Estudiante ORDER BY Nombre DESC;

-- =============================================
-- SCRIPT 4: UPDATE y DELETE
-- =============================================

UPDATE Estudiante SET Nombre = "Felipe" WHERE ID = 2;
SELECT * FROM Estudiante WHERE ID = 2;

DELETE FROM Estudiante WHERE ID = 5;
SELECT * FROM Estudiante;

-- =============================================
-- SCRIPT 5: Indices
-- =============================================

CREATE INDEX Estudiante_Id ON Estudiante(ID) OF TYPE BTREE;
SELECT * FROM Estudiante WHERE ID = 3;

-- =============================================
-- SCRIPT 6: CASOS DE ERROR
-- =============================================

-- Error: base de datos ya existe
CREATE DATABASE Universidad;

-- Error: tabla ya existe
CREATE TABLE Estudiante (ID INTEGER);

-- Error: base de datos no existe
SET DATABASE NoExiste;

-- Error: tabla no existe
SELECT * FROM NoExiste;

-- Error: duplicado en indice
INSERT INTO Estudiante VALUES(1, "Andres", "Ramirez", "Z", "2000-01-01 00:00:00");

-- Error: tipo de dato incorrecto
INSERT INTO Estudiante VALUES("texto", "Luis", "Castro", "Q", "2000-01-01 00:00:00");

-- Error: cantidad de valores incorrecta
INSERT INTO Estudiante VALUES(6, "Luis");

-- Error: DROP tabla con datos
DROP TABLE Estudiante;

-- Error: columna inexistente en WHERE
SELECT * FROM Estudiante WHERE ColumnaFalsa = 1;

-- Error: columna inexistente en ORDER BY
SELECT * FROM Estudiante ORDER BY ColumnaFalsa ASC;

-- =============================================
-- SCRIPT 7: BST index
-- =============================================

CREATE DATABASE Prueba;
SET DATABASE Prueba;

CREATE TABLE Producto (
    ID INTEGER,
    Nombre VARCHAR(50),
    Precio DOUBLE
);

INSERT INTO Producto VALUES(10, "Laptop", "1500.99");
INSERT INTO Producto VALUES(20, "Mouse", "25.50");
INSERT INTO Producto VALUES(30, "Teclado", "75.00");

CREATE INDEX Producto_Id ON Producto(ID) OF TYPE BST;

SELECT * FROM Producto WHERE ID = 20;

-- Error: segundo indice en misma tabla
CREATE INDEX Producto_Id2 ON Producto(Nombre) OF TYPE BST;

-- =============================================
-- SCRIPT 8: DROP exitoso (tabla vacia)
-- =============================================

CREATE TABLE Temporal (ID INTEGER, Valor VARCHAR(10));
DROP TABLE Temporal;
