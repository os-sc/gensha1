#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#define word uint32_t
#define BLOCKSIZE 64

word constants[80];
word msg_schedule[80];
word hash_values[5];

word a;
word b;
word c;
word d;
word e;
word t;

word block_number = 1;
word block[BLOCKSIZE/sizeof(word)];

word rotl(word x, word n)
{
    return (x << n) | (x >> (sizeof(word) - n));
}

word ch(word x, word y, word z)
{
    return (x & y) ^ (~x & z);
}

word parity(word x, word y, word z)
{
    return x ^ y ^ z;
}

word maj(word x, word y, word z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

word f(word t, word x, word y, word z)
{
    if (t <= 19) return ch(x, y, z);
    if (t >= 20 && t <= 39) return parity(x, y, z);
    if (t >= 40 && t <= 59) return maj(x, y, z);
    if (t >= 60 && t <= 79) return parity(x, y, z);

    fprintf(stderr, "[ERROR] Inavlid constants array.\n");
    exit(EINVAL);
}

void print_empty_line() {
    printf("[-]\n");
}

void print_separator() {
    print_empty_line();
    printf("[-] NNNN AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD EEEEEEEE\n");
    print_empty_line();
}

void print_initial_hash_value() {
    printf("[*] IHV: %08x %08x %08x %08x %08x\n",
            hash_values[0],
            hash_values[1],
            hash_values[2],
            hash_values[3],
            hash_values[4]);
}

void print_digest(int t) {
    printf("[t] %04d %08x %08x %08x %08x %08x\n",
            t,
            hash_values[0],
            hash_values[1],
            hash_values[2],
            hash_values[3],
            hash_values[4]);
}

void print_block() {
    printf("[W] BLOCK CONTENTS\n");
    for (word i = 0; i < BLOCKSIZE/sizeof(word); i++)
        printf("[W] %04d %08x\n", i, block[i]);
}

void init_constants() {
    for(size_t i = 0; i < 80; i++) {
        if (i <= 19) constants[i] = 0x5a827999;
        if (i >= 20 && i <= 39) constants[i] = 0x6ed9eba1;
        if (i >= 40 && i <= 59) constants[i] = 0x8f1bbcdc;
        if (i >= 60 && i <= 79) constants[i] = 0xca62c1d6;
    }
}

void prepare_message_schedule() {
    for(word j = 0; j < BLOCKSIZE/sizeof(word); j++) {
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

void init_working_vars() {
    a = hash_values[0];
    b = hash_values[1];
    c = hash_values[2];
    d = hash_values[3];
    e = hash_values[4];
}

void cicle_working_vars() {
    for(size_t j = 0; j < 80; j++) {
        t = rotl(5, a) + f(j, b, d, c) + e + constants[j] + msg_schedule[j];
        e = d;
        d = c;
        c = rotl(30, b);
        b = a;
        a = t;
    }
}

bool read_block(FILE* file, word* block) {
    unsigned char tmp_block[BLOCKSIZE];
    size_t len = fread(tmp_block, 1, BLOCKSIZE, file);

    if (len < BLOCKSIZE) {
        // Add binary 1000 0000 as the next byte
        tmp_block[len] = (unsigned char)0x80;
        // Zero out the rest
        while(len < BLOCKSIZE)
            tmp_block[len++] = 0;
        memcpy(block, tmp_block, BLOCKSIZE);
        return false;
    }
    return true;
}

void init_hash_values() {
    hash_values[0] = 0x67452301;
    hash_values[1] = 0xefcdab89;
    hash_values[2] = 0x98badcfe;
    hash_values[3] = 0x10325476;
    hash_values[4] = 0xc3d2e1f0;
}

void compute_intermediate_hash_values() {
    hash_values[0] = a + hash_values[0];
    hash_values[1] = b + hash_values[1];
    hash_values[2] = c + hash_values[2];
    hash_values[3] = d + hash_values[3];
    hash_values[4] = e + hash_values[4];
}

void generate_sha1(FILE* file) {
    init_constants();
    init_hash_values();

    print_separator();
    print_initial_hash_value();
    print_separator();

    for (;;) {
        bool more = read_block(file, block);
        print_block();
        print_separator();

        prepare_message_schedule();
        init_working_vars();
        for (int t=0; t < 80; t++) {
            cicle_working_vars();
            print_digest(t);
        }
        print_separator();
        compute_intermediate_hash_values();
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
        printf("[*] Reading from %s\n", argv[1]);
        file = fopen(argv[1], "r");
    }

    generate_sha1(file);

    return EXIT_SUCCESS;
}

