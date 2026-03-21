#include <stdbool.h>
/*
 * ============================================================
 * BIT OPERATIONS PRACTICE
 * ============================================================
 * Compile:
 *   gcc -Wall -Wextra -o bit_test bit_ops.c && ./bit_test
 * ============================================================
 */

#include <stdint.h>
#include <stdio.h>

/* ============================================================
 * YOUR IMPLEMENTATIONS
 * ============================================================ */

// 1. return 1 if the nth bit of x is set, 0 otherwise
int bit_get(uint32_t x, int n) {
    if (x & (1u << n)) return 1;
    return 0;
}

// 2. return x with the nth bit set
uint32_t bit_set(uint32_t x, int n) {
    return x | (1u << n);
}

// 3. return x with the nth bit cleared
uint32_t bit_clear(uint32_t x, int n) {
    return x & ~(1u << n);
}

// 4. return x with the nth bit toggled
uint32_t bit_toggle(uint32_t x, int n) {
    return x ^ (1u << n);
}

// i am trash at this one apparently holyyy
// 5. return x with bits hi down to lo set to val
// example: bit_field_set(0xFFFFFFFF, 7, 4, 0xA) -> bits 7-4 become 1010
uint32_t bit_field_set(uint32_t x, int hi, int lo, uint32_t val) {
    int len = hi - lo + 1;
    uint32_t mask = ((1u << len) - 1) << lo;

    x &= ~mask;
    x |= ((val << lo) & mask);
    return x;
}

// 6. extract bits hi down to lo from x as an unsigned value
uint32_t bit_field_get(uint32_t x, int hi, int lo) {
    int len = hi - lo + 1;
    uint32_t mask = ((1u << len) - 1) << lo;
    return (x & mask) >> lo;
}

// 7. return the number of set bits in x, no loops no builtins
int popcount(uint32_t x) {
    uint8_t count = 0;
    while (x) {
        x &= (x-1);
        count++;
    }
    return count;
}

// 8. return 1 if x is a power of two, 0 otherwise
// must handle x=0 correctly
int is_power_of_two(uint32_t x) {
    if (!x) return 0;
    return (x & (x-1)) == 0;
}

// 9. round x up to the next power of two
// example: 5 -> 8, 8 -> 8, 0 -> 1
uint32_t next_pow2(uint32_t x) {
    if (x & (1u << 31)) return x;
    if (!x) return 1u;

    // find most set bit?
    uint8_t pos = 0;
    uint32_t temp = x;

    if (x > 0xFFFF) {
        x >>= 16;
        pos += 16;
    }
    if (x > 0xFF) {
        x >>= 8;
        pos += 8;
    }
    if (x > 0xF) {
        x >>= 4;
        pos += 4;
    }
    if (x > 0b11) {
        x >>= 2;
        pos += 2;
    }
    if (x > 0b1) {
        pos += 1;
    }

    if (is_power_of_two(temp)) return (1u << pos);
    return (1u << (pos+1));
}

// 10. reverse the bits of x
// example: 0x80000000 -> 0x00000001
uint32_t bit_reverse(uint32_t x) {
    x = ((x & 0xFFFF0000) >> 16) | ((x & 0x0000FFFF) << 16);
    x = ((x & 0xFF00FF00) >> 8) | ((x & 0x00FF00FF) << 8);
    x = ((x & 0xF0F0F0F0) >> 4) | ((x & 0x0F0F0F0F) << 4);
    x = ((x & 0xCCCCCCCC) >> 2) | ((x & 0x33333333) << 2);
    x = ((x & 0xAAAAAAAA) >> 1) | ((x & 0x55555555) << 1);
    return x;

    uint32_t ret = 0;
    for (uint8_t i = 0; i < 32; i++) {
        if (x & (1u << (31-i))) {
            ret |= (1u << i);
        }
    }
    return ret;
}

// 11. return x with bytes swapped (big endian to little endian)
// example: 0x12345678 -> 0x78563412
uint32_t byte_swap(uint32_t x) {
    x = ((x & 0xFFFF0000) >> 16) | ((x & 0x0000FFFF) << 16);
    x = ((x & 0xFF00FF00) >> 8) | ((x & 0x00FF00FF) << 8);
    return x;
}

// 12. return 1 if x has even parity, 0 if odd, no loops no builtins
int even_parity(uint32_t x) {
    return popcount(x) % 2 == 0;
}

// 13. return the position of the lowest set bit in x
// example: 0b1100 -> 2
// behavior for x=0 is undefined
int lowest_set_bit_pos(uint32_t x) {
    // i can do isolate lowest with trick
    // then use 32 case switch statement
    // but i don't feel like writing that
    for (uint8_t i = 0; i < 32; i++) {
        if (x & (1u << i)) return i;
    }
    return 0;
}

// 14. return x with only the lowest set bit kept, all others cleared
// example: 0b1100 -> 0b0100
uint32_t isolate_lowest_set_bit(uint32_t x) {
    return x & ~(x-1);
}

// 15. pack four bytes a, b, c, d into a single uint32_t
// a is the most significant byte
uint32_t pack_bytes(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return
    ((uint32_t)a) << 24 |
    ((uint32_t)b) << 16 |
    ((uint32_t)c) << 8 |
    ((uint32_t)d);
}

// 16. given a byte, return 1 if it has odd parity using only
// operations on the byte itself, no lookup table, no builtins
int byte_odd_parity(uint8_t x) {
    uint8_t count = 0;
    while (x) {
        x &= (x-1);
        count++;
    }
    return count%2 != 0;
}

// 17. return x with the bits in positions given by mask
// rotated left by one within the mask region
// example: mask=0b00111100, x has those bits as 1010 (bits 5-2)
// result has those bits as 0101 (rotated left by one within mask)
uint32_t masked_rotate_left(uint32_t x, uint32_t mask) {
    uint32_t bits = ((x & mask) << 1);
    if (bits & ~mask) {
        bits |= (~(mask-1)) & mask;
    }
    bits &= mask;
    x &= ~mask;
    x |= bits;
    return x;
}

// 18. given a 32 bit value, return the number of transitions
// between 0 and 1 or 1 and 0 in the bit pattern
// example: 0b00001111 has 1 transition
// example: 0b01010101 has 7 transitions
int bit_transitions(uint32_t x) {
    uint8_t count = 0;
    bool last = x & 1u;
    for (uint8_t i = 1; i < 32; i++) {
        bool val = (x & (1u << i)) ? true : false;
        if (val != last) {
            count++;
            last = !last;
        }
    }
    return count;
}

/* ============================================================
 * TEST HARNESS
 * ============================================================ */

static int passed = 0;
static int total  = 0;

#define CHECK(label, cond) do {             \
    total++;                                \
    if (cond) { passed++;                   \
        printf("PASS  %s\n", label); }      \
    else printf("FAIL  %s\n", label);       \
} while(0)

int main(void) {
    printf("\n=== Bit Operations – Test Suite ===\n\n");

    /* ---- 1. bit_get ---- */
    printf("-- bit_get --\n");
    CHECK("T01 bit_get bit 0 set",     bit_get(0x1, 0)        == 1);
    CHECK("T02 bit_get bit 0 clear",   bit_get(0x2, 0)        == 0);
    CHECK("T03 bit_get bit 31 set",    bit_get(0x80000000, 31)== 1);
    CHECK("T04 bit_get bit 31 clear",  bit_get(0x7FFFFFFF, 31)== 0);
    CHECK("T05 bit_get middle bit",    bit_get(0x00100000, 20)== 1);

    /* ---- 2. bit_set ---- */
    printf("\n-- bit_set --\n");
    CHECK("T06 bit_set bit 0",         bit_set(0x0, 0)        == 0x1);
    CHECK("T07 bit_set bit 31",        bit_set(0x0, 31)       == 0x80000000);
    CHECK("T08 bit_set already set",   bit_set(0x1, 0)        == 0x1);
    CHECK("T09 bit_set preserves",     bit_set(0xF0, 0)       == 0xF1);

    /* ---- 3. bit_clear ---- */
    printf("\n-- bit_clear --\n");
    CHECK("T10 bit_clear bit 0",       bit_clear(0x1, 0)      == 0x0);
    CHECK("T11 bit_clear bit 31",      bit_clear(0x80000000, 31) == 0x0);
    CHECK("T12 bit_clear already clr", bit_clear(0x0, 0)      == 0x0);
    CHECK("T13 bit_clear preserves",   bit_clear(0xF1, 0)     == 0xF0);

    /* ---- 4. bit_toggle ---- */
    printf("\n-- bit_toggle --\n");
    CHECK("T14 bit_toggle 0->1",       bit_toggle(0x0, 0)     == 0x1);
    CHECK("T15 bit_toggle 1->0",       bit_toggle(0x1, 0)     == 0x0);
    CHECK("T16 bit_toggle bit 4",      bit_toggle(0x00, 4)    == 0x10);
    CHECK("T17 bit_toggle preserves",  bit_toggle(0xFF, 4)    == 0xEF);

    /* ---- 5. bit_field_set ---- */
    printf("\n-- bit_field_set --\n");
    CHECK("T18 field_set bits 7-4",    bit_field_set(0xFFFFFFFF, 7, 4, 0xA) == 0xFFFFFFAF);
    CHECK("T19 field_set bits 3-0",    bit_field_set(0xFFFFFFFF, 3, 0, 0x5) == 0xFFFFFFF5);
    CHECK("T20 field_set bits 31-28",  bit_field_set(0x00000000, 31, 28, 0xA) == 0xA0000000);
    CHECK("T21 field_set clears bits", bit_field_set(0xFFFFFFFF, 7, 4, 0x0) == 0xFFFFFF0F);
    CHECK("T22 field_set val masked",  bit_field_set(0x00000000, 3, 0, 0xFF) == 0x0000000F);

    /* ---- 6. bit_field_get ---- */
    printf("\n-- bit_field_get --\n");
    CHECK("T23 field_get bits 7-4",    bit_field_get(0xAB, 7, 4)   == 0xA);
    CHECK("T24 field_get bits 3-0",    bit_field_get(0xAB, 3, 0)   == 0xB);
    CHECK("T25 field_get bits 31-28",  bit_field_get(0xA0000000, 31, 28) == 0xA);
    CHECK("T26 field_get single bit",  bit_field_get(0x10, 4, 4)   == 0x1);

    /* ---- 7. popcount ---- */
    printf("\n-- popcount --\n");
    CHECK("T27 popcount 0",            popcount(0x00000000) == 0);
    CHECK("T28 popcount 1",            popcount(0x00000001) == 1);
    CHECK("T29 popcount all ones",     popcount(0xFFFFFFFF) == 32);
    CHECK("T30 popcount 0xA",          popcount(0xA)        == 2);
    CHECK("T31 popcount 0xABCD1234",   popcount(0xABCD1234) == 15);

    /* ---- 8. is_power_of_two ---- */
    printf("\n-- is_power_of_two --\n");
    CHECK("T32 pow2 0 is false",       is_power_of_two(0)   == 0);
    CHECK("T33 pow2 1 is true",        is_power_of_two(1)   == 1);
    CHECK("T34 pow2 2 is true",        is_power_of_two(2)   == 1);
    CHECK("T35 pow2 3 is false",       is_power_of_two(3)   == 0);
    CHECK("T36 pow2 16 is true",       is_power_of_two(16)  == 1);
    CHECK("T37 pow2 15 is false",      is_power_of_two(15)  == 0);
    CHECK("T38 pow2 0x80000000 true",  is_power_of_two(0x80000000) == 1);

    /* ---- 9. next_pow2 ---- */
    printf("\n-- next_pow2 --\n");
    CHECK("T39 next_pow2 0 -> 1",      next_pow2(0)  == 1);
    CHECK("T40 next_pow2 1 -> 1",      next_pow2(1)  == 1);
    CHECK("T41 next_pow2 2 -> 2",      next_pow2(2)  == 2);
    CHECK("T42 next_pow2 3 -> 4",      next_pow2(3)  == 4);
    CHECK("T43 next_pow2 5 -> 8",      next_pow2(5)  == 8);
    CHECK("T44 next_pow2 8 -> 8",      next_pow2(8)  == 8);
    CHECK("T45 next_pow2 15 -> 16",    next_pow2(15) == 16);
    CHECK("T46 next_pow2 17 -> 32",    next_pow2(17) == 32);

    /* ---- 10. bit_reverse ---- */
    printf("\n-- bit_reverse --\n");
    CHECK("T47 reverse 0x80000000",    bit_reverse(0x80000000) == 0x00000001);
    CHECK("T48 reverse 0x00000001",    bit_reverse(0x00000001) == 0x80000000);
    CHECK("T49 reverse 0xFFFFFFFF",    bit_reverse(0xFFFFFFFF) == 0xFFFFFFFF);
    CHECK("T50 reverse 0x00000000",    bit_reverse(0x00000000) == 0x00000000);
    CHECK("T51 reverse 0xF0F0F0F0",    bit_reverse(0xF0F0F0F0) == 0x0F0F0F0F);

    /* ---- 11. byte_swap ---- */
    printf("\n-- byte_swap --\n");
    CHECK("T52 swap 0x12345678",       byte_swap(0x12345678) == 0x78563412);
    CHECK("T53 swap 0x00000001",       byte_swap(0x00000001) == 0x01000000);
    CHECK("T54 swap 0xAABBCCDD",       byte_swap(0xAABBCCDD) == 0xDDCCBBAA);
    CHECK("T55 swap 0xFFFFFFFF",       byte_swap(0xFFFFFFFF) == 0xFFFFFFFF);

    /* ---- 12. even_parity ---- */
    printf("\n-- even_parity --\n");
    CHECK("T56 even_parity 0 (even)",  even_parity(0x00000000) == 1);
    CHECK("T57 even_parity 1 (odd)",   even_parity(0x00000001) == 0);
    CHECK("T58 even_parity 3 (even)",  even_parity(0x00000003) == 1);
    CHECK("T59 even_parity all ones",  even_parity(0xFFFFFFFF) == 1);
    CHECK("T60 even_parity 0xA",       even_parity(0xA)        == 1);

    /* ---- 13. lowest_set_bit_pos ---- */
    printf("\n-- lowest_set_bit_pos --\n");
    CHECK("T61 lsb pos 0b0001",        lowest_set_bit_pos(0x1)  == 0);
    CHECK("T62 lsb pos 0b0010",        lowest_set_bit_pos(0x2)  == 1);
    CHECK("T63 lsb pos 0b1100",        lowest_set_bit_pos(0xC)  == 2);
    CHECK("T64 lsb pos 0x80000000",    lowest_set_bit_pos(0x80000000) == 31);
    CHECK("T65 lsb pos 0xFFFFFFFF",    lowest_set_bit_pos(0xFFFFFFFF) == 0);

    /* ---- 14. isolate_lowest_set_bit ---- */
    printf("\n-- isolate_lowest_set_bit --\n");
    CHECK("T66 isolate 0b1100",        isolate_lowest_set_bit(0xC)        == 0x4);
    CHECK("T67 isolate 0b0001",        isolate_lowest_set_bit(0x1)        == 0x1);
    CHECK("T68 isolate 0x80000000",    isolate_lowest_set_bit(0x80000000) == 0x80000000);
    CHECK("T69 isolate 0xFFFFFFFF",    isolate_lowest_set_bit(0xFFFFFFFF) == 0x1);

    /* ---- 15. pack_bytes ---- */
    printf("\n-- pack_bytes --\n");
    CHECK("T70 pack 0x12,0x34,0x56,0x78", pack_bytes(0x12,0x34,0x56,0x78) == 0x12345678);
    CHECK("T71 pack 0xFF,0x00,0xFF,0x00", pack_bytes(0xFF,0x00,0xFF,0x00) == 0xFF00FF00);
    CHECK("T72 pack 0x00,0x00,0x00,0x01", pack_bytes(0x00,0x00,0x00,0x01) == 0x00000001);
    CHECK("T73 pack 0x01,0x00,0x00,0x00", pack_bytes(0x01,0x00,0x00,0x00) == 0x01000000);

    /* ---- 16. byte_odd_parity ---- */
    printf("\n-- byte_odd_parity --\n");
    CHECK("T74 byte odd parity 0x00", byte_odd_parity(0x00) == 0);
    CHECK("T75 byte odd parity 0x01", byte_odd_parity(0x01) == 1);
    CHECK("T76 byte odd parity 0x03", byte_odd_parity(0x03) == 0);
    CHECK("T77 byte odd parity 0xFF", byte_odd_parity(0xFF) == 0);
    CHECK("T78 byte odd parity 0x07", byte_odd_parity(0x07) == 1);

    /* ---- 17. masked_rotate_left ---- */
    printf("\n-- masked_rotate_left --\n");
    /*
     * mask = 0b00111100 = 0x3C, bits 5-2
     * x    = 0b00101000 = 0x28, bits 5-2 are 1010
     * rotated left within mask: 1010 -> 0101
     * result bits 5-2 = 0101 -> 0b00010100 = 0x14
     * bits outside mask unchanged
     */
    CHECK("T79 masked_rotate basic",
          masked_rotate_left(0x28, 0x3C) == 0x14);
    /*
     * mask = 0x0F, bits 3-0
     * x    = 0xA5, bits 3-0 are 0101
     * rotated: 0101 -> 1010
     * result = 0xAA, upper bits unchanged
     */
    CHECK("T80 masked_rotate low nibble",
          masked_rotate_left(0xA5, 0x0F) == 0xAA);
    /* rotating all ones stays all ones */
    CHECK("T81 masked_rotate all ones",
          masked_rotate_left(0xFF, 0xFF) == 0xFF);
    /* rotating all zeros stays all zeros */
    CHECK("T82 masked_rotate all zeros",
          masked_rotate_left(0x00, 0xFF) == 0x00);

    /* ---- 18. bit_transitions ---- */
    printf("\n-- bit_transitions --\n");
    CHECK("T83 transitions 0x00000000",  bit_transitions(0x00000000) == 0);
    CHECK("T84 transitions 0xFFFFFFFF",  bit_transitions(0xFFFFFFFF) == 0);
    CHECK("T85 transitions 0b00001111",  bit_transitions(0x0000000F) == 1);
    CHECK("T86 transitions 0b01010101",  bit_transitions(0x55)       == 7);
    CHECK("T87 transitions 0b10000001",  bit_transitions(0x81)       == 3); // cuz we are passing in a uint32
    CHECK("T88 transitions 0b11001100",  bit_transitions(0xCC)       == 4);

    printf("\n=== %d / %d tests passed ===\n\n", passed, total);
    return (passed == total) ? 0 : 1;
}
