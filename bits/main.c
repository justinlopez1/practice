//
// Created by justin on 2/23/26.
//

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* Extract up to 32 bits */
uint32_t get_bits(const uint8_t *buf, size_t buf_len,
                  size_t bit_offset, size_t bit_len) {
    if (bit_len > 32) return 0;
    if (buf == NULL) return 0;
    if (buf_len == 0) return 0;
    if (bit_offset+bit_len > buf_len*8) return 0;

    uint32_t ret = 0;

    for (size_t i = 0; i < bit_len; i++) {
        size_t idx = (i+bit_offset)/8;
        size_t pos = (i+bit_offset)%8;
        uint32_t val = (buf[idx] >> pos) & 1u;
        ret |= (val << i);
    }

    return ret;
}

/* Insert up to 32 bits (writes bit_len LSBs of value into the buffer) */
int set_bits(uint8_t *buf, size_t buf_len,
             size_t bit_offset, size_t bit_len, uint32_t value) {
    if (bit_len > 32) return -1;
    if (buf == NULL) return 0;
    if (buf_len == 0) return 0;
    if (bit_offset+bit_len > buf_len*8) return -1;

    for (size_t i = 0; i < bit_len; i++) {
        size_t idx = (i+bit_offset)/8;
        size_t pos = (i+bit_offset)%8;
        if ((value >> i) & 1u) {
            buf[idx] |= (1u << pos);
        }
        else {
            buf[idx] &= ~(1u << pos);
        }
    }

    return 0;
}
/* helper */
void test(const char *name,
          const uint8_t *buf,
          size_t len,
          size_t offset,
          size_t bits,
          uint32_t expected)
{
    uint32_t result = get_bits(buf, len, offset, bits);

    printf("%s: result=%u (0x%X), expected=%u (0x%X) %s\n",
           name,
           result, result,
           expected, expected,
           (result == expected) ? "PASS" : "FAIL");
}

int main(void)
{
    /* -------------------------------------------------- */
    printf("\n=== Test 1: single byte extraction ===\n");
    {
        uint8_t b[] = {0b10110010};

        // read bits 2..4 (3 bits)
        // bits = 0,0,1 → 0b100 = 4
        test("basic extract", b, 1, 2, 3, 4);
    }

    /* -------------------------------------------------- */
    printf("\n=== Test 2: whole byte ===\n");
    {
        uint8_t b[] = {0xAB};

        test("full byte", b, 1, 0, 8, 0xAB);
    }

    /* -------------------------------------------------- */
    printf("\n=== Test 3: cross byte boundary ===\n");
    {
        uint8_t b[] = {
            0b10110010,
            0b01101100
        };

        // start at bit6, read 5 bits
        // expected = 0b10010 = 18
        test("cross boundary", b, 2, 6, 5, 18);
    }

    /* -------------------------------------------------- */
    printf("\n=== Test 4: middle of second byte ===\n");
    {
        uint8_t b[] = {0x00, 0b11010110};

        // read bits 9..12 (inside byte1)
        // bits: 1,1,0,1 → 0b1011 = 11
        test("second byte", b, 2, 9, 4, 11);
    }

    /* -------------------------------------------------- */
    printf("\n=== Test 5: bit_len == 0 ===\n");
    {
        uint8_t b[] = {0xFF};

        test("zero length", b, 1, 3, 0, 0);
    }

    /* -------------------------------------------------- */
    printf("\n=== Test 6: 32-bit read ===\n");
    {
        uint8_t b[] = {0x78, 0x56, 0x34, 0x12};

        // little-endian bit stream
        test("32-bit read", b, 4, 0, 32, 0x12345678);
    }

    /* -------------------------------------------------- */
    printf("\n=== Test 7: out of bounds ===\n");
    {
        uint8_t b[] = {0xFF};

        // exceeds buffer
        test("out of range", b, 1, 6, 4, 0);
    }

    return 0;
}
