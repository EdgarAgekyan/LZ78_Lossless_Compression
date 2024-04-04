#include <stdio.h>
#include <stdlib.h>
#include "word.h"
#include "code.h"

Word *word_create(uint8_t *syms, uint32_t len) {

    Word *word = (Word *) calloc(1, sizeof(Word));
    word->syms = (uint8_t *) calloc(len, sizeof(uint8_t));
    word->len = len;

    for (int i = 0; i < (int) len; i++) {
        word->syms[i] = syms[i];
    }

    if (word != NULL) {
        return word;
    }
    return NULL;
}

Word *word_append_sym(Word *w, uint8_t sym) {

    //Create new word, passing in the current word's syms and

    //Create a temporary array equal to the size of Word's syms
    uint8_t *temp_syms = (uint8_t *) calloc(w->len + 1, sizeof(uint8_t));

    //Make temporary array equal to Word's
    //Loop through word's array and make equal
    for (int i = 0; i < (int) w->len; i++) {
        temp_syms[i] = w->syms[i];
    }

    //After copying, we have the last element left, the new sym
    temp_syms[w->len] = sym;

    Word *word = word_create(temp_syms, (w->len) + 1);

    //Free temporary array from heap since we no longer need it
    free(temp_syms);
    temp_syms = NULL;

    return word;
}

//This deletes a Word, which has a syms array and a len
void word_delete(Word *w) {

    //Check to make sure the input is not NULL and continue
    if (w != NULL) {
        free(w->syms);
        w->syms = NULL;
        //Once we free the syms array, we can finally free the Word and set to NULL
        free(w);
        w = NULL;
    }
}

WordTable *wt_create(void) {

    //We create a WordTable array with a size of MAX_CODE
    WordTable *table = (WordTable *) calloc(MAX_CODE, sizeof(Word *));

    //Initialize the first Word at index EMPTY_CODE
    table[EMPTY_CODE] = word_create(NULL, 0);
    return table;
}

void wt_reset(WordTable *wt) {

    if (wt != NULL) {
        //Loop through all the
        for (int i = 0; i < MAX_CODE; i++) {
            if (wt[i] != NULL) {
                if (i != EMPTY_CODE) {
                    word_delete(wt[i]);
                    wt[i] = NULL;
                }
            }
        }
    }
}

void wt_delete(WordTable *wt) {

    if (wt != NULL) {
        for (int i = 0; i < MAX_CODE; i++) {
            if (wt[i] != NULL) {
                word_delete(wt[i]);
                wt[i] = NULL;
            }
        }
        free(wt);
        wt = NULL;
    }
}
