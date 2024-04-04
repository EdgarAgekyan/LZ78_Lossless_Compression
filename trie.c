#include <stdio.h>
#include <stdlib.h>
#include "trie.h"
#include "code.h"

//Constructor for TrieNode
TrieNode *trie_node_create(uint16_t index) {
    //Initialize a TrieNode struct pointer using calloc
    TrieNode *trie = (TrieNode *) calloc(1, sizeof(TrieNode));

    //Node's code is set to code
    trie->code = index;

    //Make each child pointer NULL
    for (int i = 0; i < ALPHABET; i++) {
        trie->children[i] = NULL;
    }

    return trie;
}

void trie_node_delete(TrieNode *n) {
    if (n != NULL) {
        free(n);
        n = NULL;
    }
}

TrieNode *trie_create(void) {
    TrieNode *trie = trie_node_create(EMPTY_CODE);

    //If not successful, return NULL
    if (trie != NULL) {
        return trie;
    }
    return NULL;
}

void trie_reset(TrieNode *root) {

    //Free each child of n
    //trie_node_delete(root);

    for (int i = 0; i < ALPHABET; i++) {
        trie_delete(root->children[i]);
        root->children[i] = NULL;
    }
}

void trie_delete(TrieNode *n) {

    if (n != NULL) {
        for (int i = 0; i < ALPHABET; i++) {
            if (n->children[i] != NULL) {
                trie_delete(n->children[i]);
                n->children[i] = NULL;
            }
        }
        trie_node_delete(n);
    }
}

TrieNode *trie_step(TrieNode *n, uint8_t sym) {
    if (n->children[sym] != NULL) {
        return n->children[sym];
    }
    return NULL;
}
