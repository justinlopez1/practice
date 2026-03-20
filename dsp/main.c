/*
 * ============================================================
 * PROBLEM: DSP Control Register Interface
 * ============================================================
 *
 * CONTEXT
 * -------
 * You are writing a driver for a DSP chip connected over SPI.
 * The chip exposes a single 32-bit control register that you
 * read and write in full over SPI. Your driver needs to expose
 * clean field-level accessors so the rest of the firmware
 * never has to think about bit positions.
 *
 * THE REGISTER LAYOUT (32 bits, all others reserved/zero)
 *
 *   bits 31–28  GAIN        4 bits   unsigned, output gain 0–15
 *   bits 27–24  CHANNEL     4 bits   unsigned, active channel 0–15
 *   bits 23–16  SAMPLE_RATE 8 bits   unsigned, encoded sample rate
 *   bits 15–12  reserved            must always be written as 0
 *   bits 11–8   EQ_BAND     4 bits   unsigned, EQ band select 0–15
 *   bits  7–4   FLAGS       4 bits   individual flag bits (see below)
 *   bits  3–0   MODE        4 bits   unsigned, operating mode 0–15
 *
 * FLAGS field (bits 7–4), individual bits:
 *   bit 7   MUTE        1 = muted
 *   bit 6   BYPASS      1 = bypass DSP processing
 *   bit 5   CLIP        1 = clipping detected (read-only in hw, writable here)
 *   bit 4   LOCK        1 = PLL locked
 *
 * YOUR TASK
 * ---------
 * Implement the following functions. The register value is
 * passed by value for reads and returned as a new uint32_t
 * for writes — simulating the real pattern where you read
 * the full register over SPI, modify fields, and write it back.
 *
 * READ FUNCTIONS (extract a field from a register value):
 *
 *   uint8_t  reg_get_gain       (uint32_t reg);
 *   uint8_t  reg_get_channel    (uint32_t reg);
 *   uint8_t  reg_get_sample_rate(uint32_t reg);
 *   uint8_t  reg_get_eq_band    (uint32_t reg);
 *   uint8_t  reg_get_flags      (uint32_t reg);
 *   uint8_t  reg_get_mode       (uint32_t reg);
 *
 * WRITE FUNCTIONS (return a new register value with field set):
 *
 *   uint32_t reg_set_gain       (uint32_t reg, uint8_t val);
 *   uint32_t reg_set_channel    (uint32_t reg, uint8_t val);
 *   uint32_t reg_set_sample_rate(uint32_t reg, uint8_t val);
 *   uint32_t reg_set_eq_band    (uint32_t reg, uint8_t val);
 *   uint32_t reg_set_flags      (uint32_t reg, uint8_t val);
 *   uint32_t reg_set_mode       (uint32_t reg, uint8_t val);
 *
 * FLAG BIT HELPERS (operate on the FLAGS field directly):
 *
 *   uint32_t reg_set_flag  (uint32_t reg, uint8_t flag_bit);
 *   uint32_t reg_clear_flag(uint32_t reg, uint8_t flag_bit);
 *   int      reg_get_flag  (uint32_t reg, uint8_t flag_bit);
 *
 *   flag_bit is the absolute bit position in the register
 *   (i.e. 7 for MUTE, 6 for BYPASS, 5 for CLIP, 4 for LOCK).
 *
 * CHECKSUM FUNCTION:
 *
 *   uint8_t reg_checksum(uint32_t reg);
 *
 *   Returns the XOR of all four bytes of the register value.
 *   This is appended to SPI transactions so the receiver can
 *   verify the transfer wasn't corrupted.
 *
 * PARITY FUNCTION:
 *
 *   int reg_parity(uint32_t reg);
 *
 *   Returns 1 if the number of set bits in reg is odd,
 *   0 if even. (odd parity bit value)
 *   Implement this without any loops.
 *
 * CONSTRAINTS
 *   - No magic numbers in your implementation — define masks
 *     and shifts as named constants at the top
 *   - set functions must not corrupt adjacent fields
 *   - set functions must write reserved bits as 0
 *   - val is masked to the field width before being written
 *     (i.e. if someone passes gain=255, only the low 4 bits
 *     are used — don't corrupt adjacent fields)
 *
 * ============================================================
 * YOUR IMPLEMENTATION GOES BELOW
 * ============================================================
 */

#include <stdint.h>
#include <stdio.h>

/*
*   bits 31–28  GAIN        4 bits   unsigned, output gain 0–15
*   bits 27–24  CHANNEL     4 bits   unsigned, active channel 0–15
*   bits 23–16  SAMPLE_RATE 8 bits   unsigned, encoded sample rate
*   bits 15–12  reserved            must always be written as 0
*   bits 11–8   EQ_BAND     4 bits   unsigned, EQ band select 0–15
*   bits  7–4   FLAGS       4 bits   individual flag bits (see below)
*   bits  3–0   MODE        4 bits   unsigned, operating mode 0–15
*
* FLAGS field (bits 7–4), individual bits:
*   bit 7   MUTE        1 = muted
*   bit 6   BYPASS      1 = bypass DSP processing
*   bit 5   CLIP        1 = clipping detected (read-only in hw, writable here)
*   bit 4   LOCK        1 = PLL locked
*/

/* --- field definitions --- */
/* TODO: define your masks and shifts here */


/* --- read functions --- */
uint8_t  reg_get_gain       (uint32_t reg) { (void)reg; return 0; }
uint8_t  reg_get_channel    (uint32_t reg) { (void)reg; return 0; }
uint8_t  reg_get_sample_rate(uint32_t reg) { (void)reg; return 0; }
uint8_t  reg_get_eq_band    (uint32_t reg) { (void)reg; return 0; }
uint8_t  reg_get_flags      (uint32_t reg) { (void)reg; return 0; }
uint8_t  reg_get_mode       (uint32_t reg) { (void)reg; return 0; }

/* --- write functions --- */
uint32_t reg_set_gain       (uint32_t reg, uint8_t val) { (void)reg; (void)val; return 0; }
uint32_t reg_set_channel    (uint32_t reg, uint8_t val) { (void)reg; (void)val; return 0; }
uint32_t reg_set_sample_rate(uint32_t reg, uint8_t val) { (void)reg; (void)val; return 0; }
uint32_t reg_set_eq_band    (uint32_t reg, uint8_t val) { (void)reg; (void)val; return 0; }
uint32_t reg_set_flags      (uint32_t reg, uint8_t val) { (void)reg; (void)val; return 0; }
uint32_t reg_set_mode       (uint32_t reg, uint8_t val) { (void)reg; (void)val; return 0; }

/* --- flag helpers --- */
uint32_t reg_set_flag  (uint32_t reg, uint8_t flag_bit) { (void)reg; (void)flag_bit; return 0; }
uint32_t reg_clear_flag(uint32_t reg, uint8_t flag_bit) { (void)reg; (void)flag_bit; return 0; }
int      reg_get_flag  (uint32_t reg, uint8_t flag_bit) { (void)reg; (void)flag_bit; return 0; }

/* --- checksum --- */
uint8_t reg_checksum(uint32_t reg) { (void)reg; return 0; }

/* --- parity --- */
int reg_parity(uint32_t reg) { (void)reg; return 0; }

/*
 * ============================================================
 * TEST HARNESS
 * ============================================================
 * Compile:
 *   gcc -Wall -Wextra -o reg_test reg_interface.c && ./reg_test
 * ============================================================
 */

static int passed = 0;
static int total  = 0;

#define CHECK(label, cond) do {             \
    total++;                                \
    if (cond) { passed++;                   \
        printf("PASS  %s\n", label); }      \
    else printf("FAIL  %s\n", label);       \
} while(0)

int main(void) {
    printf("\n=== DSP Register Interface – Test Suite ===\n\n");

    /* ---- basic get on known register value ---- */
    printf("-- Field extraction --\n");

    /*
     * Build a register by hand so we know exactly what's in it:
     *
     *   GAIN        = 0xA  (1010) → bits 31-28
     *   CHANNEL     = 0x3  (0011) → bits 27-24
     *   SAMPLE_RATE = 0x2C (44)   → bits 23-16
     *   reserved    = 0x0         → bits 15-12
     *   EQ_BAND     = 0x7  (0111) → bits 11-8
     *   FLAGS       = 0x9  (1001) → bits 7-4  (MUTE=1, BYPASS=0, CLIP=0, LOCK=1)
     *   MODE        = 0x5  (0101) → bits 3-0
     *
     *   0xA3 2C 07 95
     *   = 1010_0011_0010_1100_0000_0111_1001_0101
     */
    uint32_t reg = 0xA32C0795;

    CHECK("T01 get gain",        reg_get_gain(reg)        == 0xA);
    CHECK("T02 get channel",     reg_get_channel(reg)     == 0x3);
    CHECK("T03 get sample_rate", reg_get_sample_rate(reg) == 0x2C);
    CHECK("T04 get eq_band",     reg_get_eq_band(reg)     == 0x7);
    CHECK("T05 get flags",       reg_get_flags(reg)       == 0x9);
    CHECK("T06 get mode",        reg_get_mode(reg)        == 0x5);

    /* ---- individual flag bits ---- */
    printf("\n-- Flag bits --\n");
    CHECK("T07 MUTE set",        reg_get_flag(reg, 7) == 1);
    CHECK("T08 BYPASS clear",    reg_get_flag(reg, 6) == 0);
    CHECK("T09 CLIP clear",      reg_get_flag(reg, 5) == 0);
    CHECK("T10 LOCK set",        reg_get_flag(reg, 4) == 1);

    /* ---- set functions preserve adjacent fields ---- */
    printf("\n-- Field writes --\n");
    uint32_t r2 = reg_set_gain(reg, 0x5);
    CHECK("T11 set gain value",          reg_get_gain(r2)        == 0x5);
    CHECK("T11 set gain preserves ch",   reg_get_channel(r2)     == 0x3);
    CHECK("T11 set gain preserves sr",   reg_get_sample_rate(r2) == 0x2C);
    CHECK("T11 set gain preserves mode", reg_get_mode(r2)        == 0x5);

    uint32_t r3 = reg_set_mode(reg, 0xF);
    CHECK("T12 set mode value",           reg_get_mode(r3)    == 0xF);
    CHECK("T12 set mode preserves flags", reg_get_flags(r3)   == 0x9);
    CHECK("T12 set mode preserves gain",  reg_get_gain(r3)    == 0xA);

    uint32_t r4 = reg_set_sample_rate(reg, 0xFF);
    CHECK("T13 set sample_rate value",      reg_get_sample_rate(r4) == 0xFF);
    CHECK("T13 set sample_rate preserves gain", reg_get_gain(r4)    == 0xA);
    CHECK("T13 set sample_rate preserves mode", reg_get_mode(r4)    == 0x5);

    /* ---- val masking — oversized value must not corrupt neighbors ---- */
    printf("\n-- Overflow masking --\n");
    uint32_t r5 = reg_set_gain(reg, 0xFF);   /* 0xFF masked to 4 bits = 0xF */
    CHECK("T14 gain overflow masked",         reg_get_gain(r5)    == 0xF);
    CHECK("T14 gain overflow no channel leak",reg_get_channel(r5) == 0x3);

    uint32_t r6 = reg_set_mode(reg, 0xFF);   /* 0xFF masked to 4 bits = 0xF */
    CHECK("T15 mode overflow masked",         reg_get_mode(r6)    == 0xF);
    CHECK("T15 mode overflow no flag leak",   reg_get_flags(r6)   == 0x9);

    /* ---- reserved bits must be zero after any set ---- */
    printf("\n-- Reserved bits --\n");
    uint32_t r7 = reg_set_eq_band(reg, 0xF);
    CHECK("T16 reserved bits zero after set", (r7 & 0x0000F000) == 0);

    uint32_t r8 = reg_set_sample_rate(reg, 0xAB);
    CHECK("T17 reserved bits zero after sr set", (r8 & 0x0000F000) == 0);

    /* ---- flag set/clear helpers ---- */
    printf("\n-- Flag set/clear --\n");
    uint32_t r9 = reg_set_flag(reg, 6);      /* set BYPASS */
    CHECK("T18 set_flag BYPASS",             reg_get_flag(r9, 6) == 1);
    CHECK("T18 set_flag preserves MUTE",     reg_get_flag(r9, 7) == 1);
    CHECK("T18 set_flag preserves LOCK",     reg_get_flag(r9, 4) == 1);
    CHECK("T18 set_flag preserves mode",     reg_get_mode(r9)    == 0x5);

    uint32_t r10 = reg_clear_flag(reg, 7);   /* clear MUTE */
    CHECK("T19 clear_flag MUTE",             reg_get_flag(r10, 7) == 0);
    CHECK("T19 clear_flag preserves LOCK",   reg_get_flag(r10, 4) == 1);
    CHECK("T19 clear_flag preserves mode",   reg_get_mode(r10)    == 0x5);

    /* ---- round-trip: set all fields then read back ---- */
    printf("\n-- Round trip --\n");
    uint32_t r11 = 0;
    r11 = reg_set_gain(r11,        0xB);
    r11 = reg_set_channel(r11,     0x4);
    r11 = reg_set_sample_rate(r11, 0x80);
    r11 = reg_set_eq_band(r11,     0xC);
    r11 = reg_set_flags(r11,       0x6);
    r11 = reg_set_mode(r11,        0x2);
    CHECK("T20 round-trip gain",        reg_get_gain(r11)        == 0xB);
    CHECK("T20 round-trip channel",     reg_get_channel(r11)     == 0x4);
    CHECK("T20 round-trip sample_rate", reg_get_sample_rate(r11) == 0x80);
    CHECK("T20 round-trip eq_band",     reg_get_eq_band(r11)     == 0xC);
    CHECK("T20 round-trip flags",       reg_get_flags(r11)       == 0x6);
    CHECK("T20 round-trip mode",        reg_get_mode(r11)        == 0x2);
    CHECK("T20 reserved bits zero",     (r11 & 0x0000F000)       == 0);

    /* ---- checksum ---- */
    printf("\n-- Checksum --\n");
    /*
     * 0xA32C0795
     * byte3 = 0xA3, byte2 = 0x2C, byte1 = 0x07, byte0 = 0x95
     * 0xA3 ^ 0x2C ^ 0x07 ^ 0x95 = ?
     * 0xA3 ^ 0x2C = 0x8F
     * 0x8F ^ 0x07 = 0x88
     * 0x88 ^ 0x95 = 0x1D
     */
    CHECK("T21 checksum known value",   reg_checksum(0xA32C0795) == 0x1D);
    CHECK("T22 checksum zero reg",      reg_checksum(0x00000000) == 0x00);
    CHECK("T23 checksum all ones",      reg_checksum(0xFFFFFFFF) == 0x00);
    CHECK("T24 checksum single byte",   reg_checksum(0x000000AB) == 0xAB);

    /* ---- parity ---- */
    printf("\n-- Parity --\n");
    CHECK("T25 parity 0x1 (1 bit, odd)",      reg_parity(0x00000001) == 1);
    CHECK("T26 parity 0x3 (2 bits, even)",    reg_parity(0x00000003) == 0);
    CHECK("T27 parity 0x7 (3 bits, odd)",     reg_parity(0x00000007) == 1);
    CHECK("T28 parity 0x0 (0 bits, even)",    reg_parity(0x00000000) == 0);
    CHECK("T29 parity 0xFFFFFFFF (32 bits, even)", reg_parity(0xFFFFFFFF) == 0);
    CHECK("T30 parity 0xA32C0795",            reg_parity(0xA32C0795) ==
                                              (__builtin_popcount(0xA32C0795) & 1));

    printf("\n=== %d / %d tests passed ===\n\n", passed, total);
    return (passed == total) ? 0 : 1;
}
