//
// Created by j1p2p3a4 on 6/16/2026.
//

#ifndef UNTITLED2_BST_H
#define UNTITLED2_BST_H

#pragma once
#include <string>
#include <vector>
#include <stdexcept>

struct BSTNode {
    std::string key;
    long long diskOffset;
    BSTNode* left;
    BSTNode* right;

    BSTNode(const std::string& k, long long offset) {
        key = k;
        diskOffset = offset;
        left = nullptr;
        right = nullptr;
    }
};

class BST {
private:
    BSTNode* root;

    BSTNode* insertNode(BSTNode* node, const std::string& key, long long offset) {
        if (node == nullptr)
            return new BSTNode(key, offset);
        if (key < node->key)
            node->left = insertNode(node->left, key, offset);
        else if (key > node->key)
            node->right = insertNode(node->right, key, offset);
        else
            throw std::runtime_error("Duplicate key in index: " + key);
        return node;
    }

    long long searchNode(BSTNode* node, const std::string& key) const {
        if (node == nullptr) return -1;
        if (key == node->key) return node->diskOffset;
        if (key < node->key)  return searchNode(node->left, key);
        return searchNode(node->right, key);
    }

    BSTNode* removeNode(BSTNode* node, const std::string& key) {
        if (node == nullptr) return nullptr;
        if (key < node->key) {
            node->left = removeNode(node->left, key);
        } else if (key > node->key) {
            node->right = removeNode(node->right, key);
        } else {
            if (node->left == nullptr) {
                BSTNode* tmp = node->right;
                delete node;
                return tmp;
            }
            if (node->right == nullptr) {
                BSTNode* tmp = node->left;
                delete node;
                return tmp;
            }
            // Sucesor inorden
            BSTNode* succ = node->right;
            while (succ->left != nullptr) succ = succ->left;
            node->key = succ->key;
            node->diskOffset = succ->diskOffset;
            node->right = removeNode(node->right, succ->key);
        }
        return node;
    }

    void destroyTree(BSTNode* node) {
        if (node == nullptr) return;
        destroyTree(node->left);
        destroyTree(node->right);
        delete node;
    }

    void collectAll(BSTNode* node, std::vector<std::pair<std::string, long long>>& out) const {
        if (node == nullptr) return;
        collectAll(node->left, out);
        out.push_back({node->key, node->diskOffset});
        collectAll(node->right, out);
    }

public:
    BST() { root = nullptr; }

    ~BST() { destroyTree(root); }

    void insert(const std::string& key, long long offset) {
        root = insertNode(root, key, offset);
    }

    long long search(const std::string& key) const {
        return searchNode(root, key);
    }

    void remove(const std::string& key) {
        root = removeNode(root, key);
    }

    bool exists(const std::string& key) const {
        return searchNode(root, key) != -1;
    }

    // Retorna todos los pares (key, offset) en orden
    std::vector<std::pair<std::string, long long>> getAll() const {
        std::vector<std::pair<std::string, long long>> result;
        collectAll(root, result);
        return result;
    }
};



#endif //UNTITLED2_BST_H
