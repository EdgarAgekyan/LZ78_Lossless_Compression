#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "io.h"
#include "code.h"
#include "endian.h"

//static uint8_t buffer[BLOCK];
//static uint8_t* buf_ptr = buffer;
static int buf_total = 0;

static uint8_t pair_buffer[BLOCK] = { 0 };

//We also need byte variable and bit variable to keep track of position
static int buf_bit = 0;
static int buf_byte = 0;

static int buf_size = 0;

static uint8_t buf[BLOCK] = { 0 };
static int loc = 0;

uint64_t total_syms = 0;
uint64_t total_bits = 0;

void check_buf(int outfile) {
    //If we reach the end of the bit and the end of the buffer,
    //flush the pair to outfile
    //reset positions
    if (buf_bit >= 7 && buf_byte >= BLOCK - 1) {
        flush_pairs(outfile);
        buf_bit = 0;
        buf_byte = 0;
    }
    //Else if the bit is only at the end, we reset the bit and increment byte
    else if (buf_bit >= 7) {
        buf_bit = 0;
        buf_byte++;
    }
    //Else, we just increment bit
    else {
        buf_bit++;
    }
}

void check_write(int infile) {
    if (buf_bit >= 7 && buf_byte >= buf_size - 1) {
        memset(pair_buffer, 0, sizeof(pair_buffer));
        buf_size = read_bytes(infile, pair_buffer, BLOCK);

        buf_bit = 0;
        buf_byte = 0;
    }
    //Else if the bit is only at the end, we reset the bit and increment byte
    else if (buf_bit >= 7) {
        buf_bit = 0;
        buf_byte++;
    }
    //Else, we just increment bit
    else {
        buf_bit++;
    }
}

int read_bytes(int infile, uint8_t *buf, int to_read) {

    ssize_t read_data = 0;
    ssize_t bytes_left = to_read;
    ssize_t total_read = 0;

    do {
        buf += read_data;
        read_data = read(infile, buf, bytes_left);
        total_read += read_data;
        bytes_left -= read_data;
    } while (read_data > 0 && bytes_left > 0);

    return total_read;
}

int write_bytes(int outfile, uint8_t *buf, int to_write) {

    ssize_t read_data = 0;
    ssize_t bytes_left = to_write;
    ssize_t total_read = 0;

    do {
        buf += read_data;
        read_data = write(outfile, buf, bytes_left);
        total_read += read_data;
        bytes_left -= read_data;
    } while (read_data > 0 && bytes_left > 0);

    return total_read;
}

void read_header(int infile, FileHeader *header) {
    read_bytes(infile, (uint8_t *) header, sizeof(FileHeader));

    if (big_endian()) {
        header->magic = swap32(header->magic);
        header->protection = swap16(header->protection);
    }

    total_bits += (sizeof(FileHeader) * 8);
}

void write_header(int outfile, FileHeader *header) {

    write_bytes(outfile, (uint8_t *) header, sizeof(FileHeader));

    if (big_endian()) {
        header->magic = swap32(header->magic);
        header->protection = swap16(header->protection);
    }
    total_bits += (sizeof(FileHeader) * 8);
}

bool read_sym(int infile, uint8_t *sym) {

    if (buf_total == 0 || loc == buf_total) {
        buf_total = read_bytes(infile, buf, BLOCK);
        loc = 0;
        if (buf_total == 0) {
            return false;
        }
        *sym = buf[loc];
        loc++;

        total_syms++;
        return true;
    } else {
        //increment pointer
        //make sym = pointer
        *sym = buf[loc];
        loc++;
        total_syms++;
        return true;
    }
}

void write_pair(int outfile, uint16_t code, uint8_t sym, int bitlen) {

    //Loop through bitlen
    for (int i = 0; i < bitlen; i++) {
        //If the LSB ... is 1, we insert a 1 into our buffer at position bit
        //and call helper function to check and increment
        if ((code & (1 << i)) != 0) {
            //if we find a 1, we insert it
            pair_buffer[buf_byte] = pair_buffer[buf_byte] | (1 << buf_bit);
            check_buf(outfile);
        } else {
            pair_buffer[buf_byte] = pair_buffer[buf_byte] & ~(1 << buf_bit);
            check_buf(outfile);
        }
    }

    //Loop through every uint16_tvalue
    for (int i = 0; i < 8; i++) {
        //If the LSB ... is 1, we insert a 1 into our buffer at position bit
        //And call our helper function
        if ((sym & (1 << i)) != 0) {
            pair_buffer[buf_byte] = pair_buffer[buf_byte] | (1 << buf_bit);
            check_buf(outfile);
        } else {
            pair_buffer[buf_byte] = pair_buffer[buf_byte] & ~(1 << buf_bit);
            check_buf(outfile);
        }
    }
    total_bits += (bitlen + 8);
}

void flush_pairs(int outfile) {
    //buf_byte is the index, so that + 1
    write_bytes(outfile, pair_buffer, buf_byte + 1);
    //If this is called, we want to write out our entire buffer
    memset(pair_buffer, 0, sizeof(pair_buffer));
    buf_size = 0;
    //Set our buffer to all 0s
}

//
// Read bitlen bits of a code into *code, and then a full 8-bit symbol into *sym, from infile.
// Return true if the complete pair was read and false otherwise.
//
// Like write_pair, this function must read the least significant bit of each input byte first, and
// will store those bits into the LSB of *code and of *sym first.
//
// It may be useful to write a helper function that reads a single bit from a file usi
bool read_pair(int infile, uint16_t *code, uint8_t *sym, int bitlen) {

    total_bits += (bitlen + 8);

    //If our buf_size is 0, that means our buffer is empty
    if (buf_size == 0) {
        //So we call read_bytes to store file input into buffer
        //4096 bytes
        //buf_size becomes the amount successfully read
        buf_size = read_bytes(infile, pair_buffer, BLOCK);
        buf_byte = 0;
        buf_bit = 0;
    }

    //Loop through bitlen
    for (int i = 0; i < bitlen; i++) {
        //
        if ((pair_buffer[buf_byte] & (1 << buf_bit)) != 0) {

            *code = *code | (1 << i);
            check_write(infile);
            if (buf_size == 0) {
                buf_byte = 0;
                buf_bit = 0;
                memset(pair_buffer, 0, sizeof(pair_buffer));
                return false;
            }
        } else {
            //buf_bit++;
            *code = *code & ~(1 << i);
            check_write(infile);
            if (buf_size == 0) {
                buf_byte = 0;
                buf_bit = 0;
                memset(pair_buffer, 0, sizeof(pair_buffer));
                return false;
            }
        }
    }

    //Loop through every uint16_tvalue
    for (int i = 0; i < 8; i++) {
        //If the LSB ... is 1, we insert a 1 into our buffer at position bit
        //And call our helper function
        if ((pair_buffer[buf_byte] & (1 << buf_bit)) != 0) {
            *sym = *sym | (1 << i);
            check_write(infile);
            if (buf_size == 0) {
                buf_byte = 0;
                buf_bit = 0;
                memset(pair_buffer, 0, sizeof(pair_buffer));
                return false;
            }
        } else {
            *sym = *sym & ~(1 << i);
            check_write(infile);
            if (buf_size == 0) {
                buf_byte = 0;
                buf_bit = 0;
                memset(pair_buffer, 0, sizeof(pair_buffer));
                return false;
            }
        }
    }

    if (*code == STOP_CODE) {
        return false;
    }

    return true;
}

void write_word(int outfile, Word *w) {
    for (int i = 0; i < (int) w->len; i++) {
        buf[loc] = w->syms[i];
        loc++;
        if (loc == (int) w->len) {
            write_bytes(outfile, buf, loc);
            memset(buf, 0, sizeof(buf));
            loc = 0;
        }
    }
    total_syms = total_syms + w->len;
}

void flush_words(int outfile) {
    //changed loc-1 to loc
    write_bytes(outfile, buf, loc);
    memset(buf, 0, sizeof(buf));
}
