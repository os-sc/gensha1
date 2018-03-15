#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#define BLOCKSIZE 64

uint32_t rotl(uint32_t x, uint32_t n)
{
    return (x << n) | (x >> (sizeof(uint32_t) - n));
}

uint32_t ch(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (~x & z);
}

uint32_t parity(uint32_t x, uint32_t y, uint32_t z)
{
    return x ^ y ^ z;
}

uint32_t maj(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

uint32_t f(uint32_t t, uint32_t x, uint32_t y, uint32_t z)
{
    if (t <= 19) return ch(x, y, z);
    if (t >= 20 && t <= 39) return parity(x, y, z);
    if (t >= 40 && t <= 59) return maj(x, y, z);
    if (t >= 60 && t <= 79) return parity(x, y, z);

    fprintf(stderr, "[ERROR] Inavlid constants array.\n");
    exit(EINVAL);
}

void print_separator() {
    printf("[-]\n");
    printf("[-]==================================================\n");
    printf("[-]\n");
}

void print_initial_hash_value(uint32_t* hash_values) {
    printf("[*] IHV: %08x %08x %08x %08x %08x\n",
            hash_values[0],
            hash_values[1],
            hash_values[2],
            hash_values[3],
            hash_values[4]);
}

void print_digest_header() {
    printf("[t] ---- AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD EEEEEEEE\n");
}

void print_digest(int t, uint32_t* hash_values) {
    printf("[t] %04d %08x %08x %08x %08x %08x\n",
            t,
            hash_values[0],
            hash_values[1],
            hash_values[2],
            hash_values[3],
            hash_values[4]);
}

void print_block(uint32_t* block, uint32_t block_number) {
    printf("[W] B#%02d CONTENT:\n", block_number);
    for (uint32_t i = 0; i < BLOCKSIZE/sizeof(uint32_t); i++)
        printf("[W] %04d %08x\n", i, block[i]);
}

void init_constants(uint32_t* constants) {
    for(size_t i = 0; i < 80; i++) {
        if (i <= 19) constants[i] = 0x5a827999;
        if (i >= 20 && i <= 39) constants[i] = 0x6ed9eba1;
        if (i >= 40 && i <= 59) constants[i] = 0x8f1bbcdc;
        if (i >= 60 && i <= 79) constants[i] = 0xca62c1d6;
    }
}

void prepare_message_schedule(uint32_t* msg_schedule) {
    for(uint32_t j = 0; j < BLOCKSIZE/sizeof(uint32_t); j++) {
        if (j <= 15)
            msg_schedule[j] = msg_schedule[j];
        else
            msg_schedule[j] =
                rotl(1, msg_schedule[j-3]
                        ^ msg_schedule[j-8]
                        ^ msg_schedule[j-14]
                        ^ msg_schedule[j-16]);
    }
}

void init_working_vars(uint32_t* hash_values, uint32_t* working_vars) {
    working_vars[0] = hash_values[0];
    working_vars[1] = hash_values[1];
    working_vars[2] = hash_values[2];
    working_vars[3] = hash_values[3];
    working_vars[4] = hash_values[4];
}

void cycle_working_vars(uint32_t* msg_schedule, uint32_t* constants, uint32_t* working_vars) {
    for(size_t j = 0; j < 80; j++) {
        uint32_t tmp = rotl(5, working_vars[0])
            + f(j, working_vars[1], working_vars[2], working_vars[3])
            + working_vars[4]
            + constants[j]
            + msg_schedule[j];

        working_vars[4] = working_vars[3];
        working_vars[3] = working_vars[2];
        working_vars[2] = rotl(30, working_vars[1]);
        working_vars[1] = working_vars[0];
        working_vars[0] = tmp;
    }
}

bool read_block(FILE* file, uint32_t block[16]) {
    unsigned char* char_block = (unsigned char*) block;
    size_t len = fread(char_block, 1, BLOCKSIZE, file);
    if (len < BLOCKSIZE) {
        // Add 1000 0000 as the next byte
        char_block[len] = 0x80;
        // Zero out the rest
        while(len < BLOCKSIZE)
            char_block[len++] = 0;
        return false;
    }
    return true;
}

void init_hash_values(uint32_t* hash_values) {
    hash_values[0] = 0x67452301;
    hash_values[1] = 0xefcdab89;
    hash_values[2] = 0x98badcfe;
    hash_values[3] = 0x10325476;
    hash_values[4] = 0xc3d2e1f0;
}

void compute_intermediate_hash_values(uint32_t* hash_values, uint32_t* working_vars) {
    hash_values[0] = working_vars[0] + hash_values[0];
    hash_values[1] = working_vars[0] + hash_values[1];
    hash_values[2] = working_vars[0] + hash_values[2];
    hash_values[3] = working_vars[0] + hash_values[3];
    hash_values[4] = working_vars[0] + hash_values[4];
}

void generate_sha1(FILE* file) {
    uint32_t block_number = 1;
    uint32_t block[BLOCKSIZE/sizeof(uint32_t)];

    uint32_t constants[80];
    uint32_t msg_schedule[80];

    uint32_t hash_values[5];
    uint32_t working_vars[5];

    init_constants(constants);
    init_hash_values(hash_values);

    print_separator(constants);
    print_initial_hash_value(hash_values);
    print_separator();

    for (;;) {
        bool more = read_block(file, block);
        print_block(block, block_number);
        print_separator();

        prepare_message_schedule(msg_schedule);
        init_working_vars(hash_values, working_vars);
        print_digest_header();
        for (int t=0; t < 80; t++) {
            cycle_working_vars(msg_schedule, constants, working_vars);
            print_digest(t, hash_values);
        }
        print_separator();
        compute_intermediate_hash_values(hash_values, working_vars);
        block_number++;
        if (!more)
            break;
    }
}

int main(int argc, char **argv) {
    FILE* file;
    if (argc < 2) {
        printf("[*] Reading from stdin\n");
        file = stdin;
    } else {
        printf("[*] Reading from file %s\n", argv[1]);
        file = fopen(argv[1], "r");
    }

    generate_sha1(file);

    return EXIT_SUCCESS;
}

