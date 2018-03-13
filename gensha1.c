#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#define BLOCKSIZE 64
#define WORDSIZE 4

#define print_horizontal_line(); printf("\n====================================================\n\n");


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

void set_constants(uint32_t* arr) {
    for(size_t i = 0; i < 80; i++) {
        if (i >=  0 && i <= 19) arr[i] = 0x5a827999;
        if (i >= 20 && i <= 39) arr[i] = 0x6ed9eba1;
        if (i >= 40 && i <= 59) arr[i] = 0x8f1bbcdc;
        if (i >= 60 && i <= 79) arr[i] = 0xca62c1d6;
    }
}

uint32_t f(uint32_t t, uint32_t x, uint32_t y, uint32_t z)
{
    if (t >=  0 && t <= 19) return ch(x, y, z);
    if (t >= 20 && t <= 39) return parity(x, y, z);
    if (t >= 40 && t <= 59) return maj(x, y, z);
    if (t >= 60 && t <= 79) return parity(x, y, z);

    printf("[ERROR] Inavlid constants array.\n");
    exit(EINVAL);
}

int main(int argc, char **argv)
{
    uint32_t msg_schedule[80];
    uint32_t hash_values[5];

    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t temp;

    // Set Constants
    uint32_t constants[80];
    set_constants(constants);

    // Set initial hash value
    hash_values[0] = 0x67452301;
    hash_values[1] = 0xefcdab89;
    hash_values[2] = 0x98badcfe;
    hash_values[3] = 0x10325476;
    hash_values[4] = 0xc3d2e1f0;

    print_horizontal_line();
    printf("Initial hash value:\n\n");
    printf("        %08x %08x %08x %08x %08x\n",
        hash_values[0],
        hash_values[1],
        hash_values[2],
        hash_values[3],
        hash_values[4]);
    print_horizontal_line();
    printf("            A        B        C        D        E   \n");

    // Parse the message
    bool done_reading = false;
    unsigned char msg_block[BLOCKSIZE];
    int iteration_counter = 0;
    for(uint32_t t = 1; true; t++) {
        size_t len = fread(msg_block, 1, 64, stdin);
        if (len < sizeof(msg_block)) {
            done_reading = true;
            // Add binary 1000 0000 as the next byte
            msg_block[len] = (unsigned char)0x80;
            // Zero out the rest
            for (size_t j = len + 1; j < BLOCKSIZE/sizeof(char); j++)
                msg_block[j] = 0;
        }

        // Prepare the message schedule
        for(size_t j = 0; j < BLOCKSIZE/WORDSIZE; j++) {
            if (j >= 0 && j <= 15)
                msg_schedule[j] = msg_schedule[j];
            else
                msg_schedule[j] = rotl(1,
                        msg_schedule[j-3]
                        ^ msg_schedule[j-8]
                        ^ msg_schedule[j-14]
                        ^ msg_schedule[j-16]);
        }

        // Initialize the working variables
        a = hash_values[0];
        b = hash_values[1];
        c = hash_values[2];
        d = hash_values[3];
        e = hash_values[4];

        // Mess with the working variables
        for(size_t j = 0; j < BLOCKSIZE/WORDSIZE; j++) {
            temp = rotl(5, a) + f(j, b, d, c) + e + constants[j] + msg_schedule[j];
            e = d;
            d = c;
            c = rotl(30, b);
            b = a;
            a = temp;
        }

        // Compute intermediate hash values
        hash_values[0] = a + hash_values[0];
        hash_values[1] = b + hash_values[1];
        hash_values[2] = c + hash_values[2];
        hash_values[3] = d + hash_values[3];
        hash_values[4] = e + hash_values[4];

        printf("t=%04d: %08x %08x %08x %08x %08x\n",
                iteration_counter,
                hash_values[0],
                hash_values[1],
                hash_values[2],
                hash_values[3],
                hash_values[4]);
        iteration_counter++;

        if(done_reading)
            break;
    }

    print_horizontal_line();
    printf("Digest:   ");
    for (size_t i = 0; i < BLOCKSIZE/WORDSIZE; i++) {
        printf("%x", hash_values[i]);
    }
    printf("\n");
    return EXIT_SUCCESS;
}

