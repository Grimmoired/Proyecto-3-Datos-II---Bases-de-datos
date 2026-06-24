//
// Created by j1p2p3a4 on 6/16/2026.
//

#ifndef UNTITLED2_BTREE_H
#define UNTITLED2_BTREE_H
#pragma once
#include <string>
#include <vector>
#include <stdexcept>

static const int BTREE_ORDER = 3; // orden minimo, cada nodo tiene max 2*ORDER-1 keys

struct BTreeNode {
    std::vector<std::string> keys;
    std::vector<long long> offsets;
    std::vector<BTreeNode*> children;
    bool isLeaf;

    BTreeNode(bool leaf) {
        isLeaf = leaf;
    }
};

class BTree {
private:
    BTreeNode* root;
    int t; // grado minimo

    void splitChild(BTreeNode* parent, int i, BTreeNode* child) {
        BTreeNode* newNode = new BTreeNode(child->isLeaf);
        int mid = t - 1;

        for (int j = 0; j < t - 1; j++) {
            newNode->keys.push_back(child->keys[mid + 1 + j]);
            newNode->offsets.push_back(child->offsets[mid + 1 + j]);
        }
        if (!child->isLeaf) {
            for (int j = 0; j < t; j++)
                newNode->children.push_back(child->children[mid + 1 + j]);
        }

        parent->keys.insert(parent->keys.begin() + i, child->keys[mid]);
        parent->offsets.insert(parent->offsets.begin() + i, child->offsets[mid]);
        parent->children.insert(parent->children.begin() + i + 1, newNode);

        child->keys.resize(mid);
        child->offsets.resize(mid);
        if (!child->isLeaf)
            child->children.resize(t);
    }

    void insertNonFull(BTreeNode* node, const std::string& key, long long offset) {
        int i = (int)node->keys.size() - 1;

        // Verificar duplicado
        for (const auto& k : node->keys)
            if (k == key) throw std::runtime_error("Duplicate key in index: " + key);

        if (node->isLeaf) {
            node->keys.push_back("");
            node->offsets.push_back(0);
            while (i >= 0 && key < node->keys[i]) {
                node->keys[i + 1] = node->keys[i];
                node->offsets[i + 1] = node->offsets[i];
                i--;
            }
            node->keys[i + 1] = key;
            node->offsets[i + 1] = offset;
        } else {
            while (i >= 0 && key < node->keys[i]) i--;
            i++;
            if ((int)node->children[i]->keys.size() == 2 * t - 1) {
                splitChild(node, i, node->children[i]);
                if (key > node->keys[i]) i++;
            }
            insertNonFull(node->children[i], key, offset);
        }
    }

    long long searchNode(BTreeNode* node, const std::string& key) const {
        int i = 0;
        while (i < (int)node->keys.size() && key > node->keys[i]) i++;
        if (i < (int)node->keys.size() && key == node->keys[i])
            return node->offsets[i];
        if (node->isLeaf) return -1;
        return searchNode(node->children[i], key);
    }

    // Remover key del subarbol con raiz node
    void removeFromNode(BTreeNode* node, const std::string& key) {
        int i = 0;
        while (i < (int)node->keys.size() && key > node->keys[i]) i++;

        if (i < (int)node->keys.size() && node->keys[i] == key) {
            if (node->isLeaf) {
                node->keys.erase(node->keys.begin() + i);
                node->offsets.erase(node->offsets.begin() + i);
            } else {
                // Reemplazar con predecesor
                BTreeNode* pred = node->children[i];
                while (!pred->isLeaf) pred = pred->children.back();
                node->keys[i] = pred->keys.back();
                node->offsets[i] = pred->offsets.back();
                removeFromNode(node->children[i], pred->keys.back());
            }
        } else {
            if (node->isLeaf)
                throw std::runtime_error("Key not found in index: " + key);
            removeFromNode(node->children[i], key);
        }
    }

    void collectAll(BTreeNode* node, std::vector<std::pair<std::string, long long>>& out) const {
        if (node == nullptr) return;
        for (int i = 0; i < (int)node->keys.size(); i++) {
            if (!node->isLeaf) collectAll(node->children[i], out);
            out.push_back({node->keys[i], node->offsets[i]});
        }
        if (!node->isLeaf) collectAll(node->children.back(), out);
    }

    void destroyTree(BTreeNode* node) {
        if (node == nullptr) return;
        for (auto* child : node->children) destroyTree(child);
        delete node;
    }

public:
    BTree() {
        t = BTREE_ORDER;
        root = new BTreeNode(true);
    }

    ~BTree() { destroyTree(root); }

    void insert(const std::string& key, long long offset) {
        if ((int)root->keys.size() == 2 * t - 1) {
            BTreeNode* newRoot = new BTreeNode(false);
            newRoot->children.push_back(root);
            splitChild(newRoot, 0, root);
            root = newRoot;
        }
        insertNonFull(root, key, offset);
    }

    long long search(const std::string& key) const {
        return searchNode(root, key);
    }

    void remove(const std::string& key) {
        removeFromNode(root, key);
    }

    bool exists(const std::string& key) const {
        return searchNode(root, key) != -1;
    }

    std::vector<std::pair<std::string, long long>> getAll() const {
        std::vector<std::pair<std::string, long long>> result;
        collectAll(root, result);
        return result;
    }
};
#endif //UNTITLED2_BTREE_H
