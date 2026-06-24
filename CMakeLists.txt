cmake_minimum_required(VERSION 3.20)
project(Proyecto_3_Bases_de_datos)

set(CMAKE_CXX_STANDARD 20)

set(SHARED_SOURCES
        Models/Column.h
        Models/Row.h
        Models/table.h
        StorageManager/SystemCatalog.cpp
        StorageManager/SystemCatalog.h
        StorageManager/StorageManager.cpp
        StorageManager/StorageManager.h
        Indexes/BST.h
        Indexes/BTree.h
        QueryProcessor/Parser.cpp
        QueryProcessor/Parser.h
        QueryProcessor/QueryProcessor.cpp
        QueryProcessor/QueryProcessor.h
)

# CLI
add_executable(tinysqldb_cli main.cpp ${SHARED_SOURCES})

# Server
add_executable(tinysqldb_server server.cpp ${SHARED_SOURCES})
target_link_libraries(tinysqldb_server ws2_32)  # necesario en Windows para sockets