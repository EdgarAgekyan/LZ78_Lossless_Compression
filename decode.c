#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "trie.h"
#include "word.h"
#include "io.h"
#include "code.h"

#define OPTIONS "vi:o:h"

//Helper function to calculate bit length of specified code
int bit_length(uint16_t code) {

    int bit_length = 0;

    while (code > 0) {
        bit_length++;
        code = code >> 1;
    }

    return bit_length;
}

int main(int argc, char **argv) {

    int opt = 0;

    //Cases for user input
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
            printf("   Decompresses files with the LZ78 decompression algorithm.\n");
            printf("   Used with files compressed with the corresponding encoder.\n");
            printf("\n");
            printf("USAGE\n");
            printf("   ./decode [-vh] [-i input] [-o output]\n");
            printf("\n");
            printf("OPTIONS\n");
            printf("   -v          Display decompression statistics\n");
            printf("   -i input    Specify input to decompress (stdin by default)\n");
            printf("   -o output   Specify output of decompressed input (stdout by default)\n");
            printf("   -h          Display program usage\n");
            return 0;
        default: break;
        }
    }

    FileHeader header = { 0, 0 };
    read_header(input, &header);

    if (header.magic != MAGIC) {
        printf("Magic Number Doesn't Match");
        return 1;
    }

    fchmod(output, header.protection);

    WordTable *table = wt_create();

    uint8_t curr_sym = 0;
    uint16_t curr_code = 0;
    uint16_t next_code = START_CODE;
    while (read_pair(input, &curr_code, &curr_sym, bit_length(next_code))) {
        table[next_code] = word_append_sym(table[curr_code], curr_sym);
        write_word(output, table[next_code]);
        next_code = next_code + 1;
        if (next_code == MAX_CODE) {
            wt_reset(table);
            next_code = START_CODE;
        }
    }

    //Final flush words to output
    flush_words(output);

    //Free wordtable memory
    wt_delete(table);

    //Close our files
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
