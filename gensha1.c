#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


int32_t rotl(int32_t x, int32_t n)
{
    return (x << n) | (x >> sizeof(int32_t) - n);
}

int32_t ch(int32_t x, int32_t y, int32_t z) {
    return (x & y) ^ (~x & z);
}

int32_t parity(int32_t x, int32_t y, int32_t z) {
    return x ^ y ^ z;
}

int32_t maj(int32_t x, int32_t y, int32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

int32_t f(int32_t t, int32_t x, int32_t y, int32_t z) {
    if (t >=  0 && t <= 19) return ch(x, y, z);
    if (t >= 20 && t <= 39) return parity(x, y, z);
    if (t >= 40 && t <= 59) return maj(x, y, z);
    if (t >= 60 && t <= 79) return parity(x, y, z);
}

int main(int argc, char **argv)
{
    int32_t msg_schedule[80];
    int32_t hash_values[5];

    int32_t a;
    int32_t b;
    int32_t c;
    int32_t d;
    int32_t e;
    int32_t temp;

    // Set Constants
    int32_t constants[80];
    for(int i = 0; i < sizeof(constants); i++) {
        if (i >=  0 && i <= 19) constants[i] = 0x5a827999;
        if (i >= 20 && i <= 39) constants[i] = 0x6ed9eba1;
        if (i >= 40 && i <= 59) constants[i] = 0x8f1bbcdc;
        if (i >= 60 && i <= 79) constants[i] = 0xca62c1d6;
    }

    // Set initial hash value
    hash_values[0] = 0x67452301;
    hash_values[1] = 0xefcdab89;
    hash_values[2] = 0x98badcfe;
    hash_values[3] = 0x10325476;
    hash_values[4] = 0xc3d2e1f0;

    // Parse the message
    bool done_reading = false;
    char buf[64];
    for(;;) {
        size_t len = fread(buf, 1, 64, stdin);
        // Pad the last part and stop reading
        if (len < sizeof(buf)) {
            done_reading = true;
            // Add binary 1000 0000 as the next byte
            buf[len] = 0x80;
            // Zero out the rest
            for (int i = len + 1; i < sizeof(buf); i++)
                buf[i] = 0;
        }

        // Prepare the message schedule
        for(int t = 0; t < sizeof(msg_schedule); t++) {
            if (t >= 0 && t <= 15)
                msg_schedule[t] = msg_schedule[t];
            else
                msg_schedule[t] = rotl(1,
                        msg_schedule[t-3]
                        ^ msg_schedule[t-8]
                        ^ msg_schedule[t-14]
                        ^ msg_schedule[t-16]);
        }

        // Initialize the working variables
        a = hash_values[0];
        b = hash_values[1];
        c = hash_values[2];
        d = hash_values[3];
        e = hash_values[4];

        // Mess with the working variables
        for(int t = 0; t < sizeof(msg_schedule); t++) {
            temp = rotl(5, a) + f(t, b, d, c) + e + constants[t] + msg_schedule[t];
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

        if(done_reading)
            break;
    }
}

