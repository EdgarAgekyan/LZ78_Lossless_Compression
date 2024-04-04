#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "trie.h"
#include "word.h"
#include "io.h"
#include "code.h"

#define OPTIONS "vi:o:h"

//Helper function used to calculate how many bits are in our code
int bit_length(uint16_t code) {

    int bit_length = 0;

    //We loop until our code is 0, meaning it has no more bits in it left
    while (code > 0) {
        //Increment bit length
        bit_length++;
        //Bitwise shift right 1
        code = code >> 1;
    }

    //Once we complete looping, we return the length
    return bit_length;
}

int main(int argc, char **argv) {

    int opt = 0;

    //Cases for user to specify
    int case_v = 0;
    int case_i = 0;
    int case_o = 0;

    int input = STDIN_FILENO;
    int output = STDOUT_FILENO;

    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 'v': case_v = 1; break;
        case 'i':
            case_i = 1;
            input = open(optarg, O_RDONLY);
            if (input == -1) {
                printf("Error opening");
                return 1;
            }
            break;
        case 'o':
            case_o = 1;
            output = open(optarg, O_WRONLY | O_CREAT | O_TRUNC);
            if (output == -1) {
                printf("Error opening");
                return 1;
            }
            break;
        case 'h':
            printf("SYNOPSIS\n");
            printf("   Compresses files using the LZ78 compression algorithm.\n");
            printf("   Compressed files are decompressed with the corresponding decoder.\n");
            printf("\n");
            printf("USAGE\n");
            printf("   ./encode [-vh] [-i input] [-o output]\n");
            printf("\n");
            printf("OPTIONS\n");
            printf("   -v          Display compression statistics\n");
            printf("   -i input    Specify input to compress (stdin by default)\n");
            printf("   -o output   Specify output of compressed input (stdout by default)\n");
            printf("   -h          Display program help and usage\n");
            return 0;
        default: break;
        }
    }

    struct stat stats;

    fstat(input, &stats);

    FileHeader header = { 0, 0 };

    header.magic = MAGIC;
    header.protection = stats.st_mode;
    fchmod(output, header.protection);
    write_header(output, &header);

    TrieNode *node = trie_create();
    TrieNode *curr_node = node;

    uint16_t next_code = START_CODE;

    TrieNode *prev_node = NULL;

    uint8_t prev_sym = 0;
    uint8_t curr_sym = 0;

    while (read_sym(input, &curr_sym)) {
        TrieNode *next_node = trie_step(curr_node, curr_sym);

        if (next_node != NULL) {
            prev_node = curr_node;
            curr_node = next_node;

        } else {
            write_pair(output, curr_node->code, curr_sym, bit_length(next_code));
            curr_node->children[curr_sym] = trie_node_create(next_code);
            curr_node = node;
            next_code = next_code + 1;
        }

        if (next_code == MAX_CODE) {
            trie_reset(node);
            curr_node = node;
            next_code = START_CODE;
        }
        prev_sym = curr_sym;
    }

    if (curr_node != node) {
        write_pair(output, prev_node->code, prev_sym, bit_length(next_code));
        next_code = (next_code + 1) % MAX_CODE;
    }
    write_pair(output, STOP_CODE, 0, bit_length(next_code));
    flush_pairs(output);

    //Free trie
    trie_delete(node);

    //Close files
    if (case_i == 1) {
        close(input);
    }

    if (case_o == 1) {
        close(output);
    }

    //Verbose case
    if (case_v == 1) {

        uint64_t bytes;
        double space_saving;

        if (total_bits % 8 == 0) {
            bytes = total_bits / 8;
        } else {
            bytes = total_bits / 8 + 1;
        }

        space_saving = 100.0 * (1.0 - (double) bytes / (double) total_syms);

        printf("Compressed file size: %" PRIu64 " bytes\n", bytes);
        printf("Uncompressed file size: %" PRIu64 " bytes\n", total_syms);
        printf("Compression ratio: %.2f%%\n", space_saving);
    }

    return 0;
}
