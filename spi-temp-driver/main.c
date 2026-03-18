#include <stdint.h>
#include <stdio.h>
#include <math.h>

// =======================
// SPI Temperature Parser
// =======================
//
// Format (16 bits total):
// bit 15: fault flag
// bit 14: reserved
// bits 13:2: signed 12-bit temp (two's complement)
// bits 1:0: ignore
//
// scale: 0.25 °C per LSB
//
// return 1 on success, 0 on fault

int parse_temperature(const uint8_t rx[2], float* temperature_c)
{
    uint16_t reg = rx[1] | (((uint16_t)(rx[0])) << 8);

    /*
    // check fault
    if (reg & (1u << 15)) return 0;
    uint16_t temp_bits = (reg & ((1u << 14) - 1)) >> 2;
    // if negative
    if (temp_bits & (1u << 11)) {
        temp_bits |= (0xF << 12);
    }
    int16_t temp = (int16_t)temp_bits;
    *temperature_c = temp * 0.25f;
    return 1;
    */

    if (reg & (1u << 15)) return 0;
    uint16_t temp_bits = (reg >> 2) & 0xFFF;
    // check neg for sign ext
    /*
    if (temp_bits & (1u << 11)) {
        temp_bits |= 0xF000;
    }
    */
    // sign ext
    temp_bits = (int16_t)(temp_bits << 4) >> 4; // this sht beautiful
    *temperature_c = (int16_t)temp_bits * 0.25f;
    return 1;
}

int feq(float a, float b)
{
    return fabsf(a - b) < 1e-4f;
}

void run_test(const char* name, uint8_t rx[2], int expect_ok, float expect_temp)
{
    float temp = 0.0f;
    int ok = parse_temperature(rx, &temp);

    printf("%s:\n", name);

    if (ok != expect_ok) {
        printf("  ❌ FAIL (status mismatch) expected=%d got=%d\n", expect_ok, ok);
        return;
    }

    if (!ok) {
        printf("  ✅ PASS (fault detected correctly)\n");
        return;
    }

    if (!feq(temp, expect_temp)) {
        printf("  ❌ FAIL (temp mismatch) expected=%.2f got=%.2f\n",
               expect_temp, temp);
        return;
    }

    printf("  ✅ PASS (temp=%.2f)\n", temp);
}

int main()
{
    {
        uint8_t rx[2] = {0x01, 0x90};
        run_test("25.0 C", rx, 1, 25.0f);
    }

    {
        uint8_t rx[2] = {0x00, 0x00};
        run_test("0.0 C", rx, 1, 0.0f);
    }

    {
        uint8_t rx[2] = {0x3F, 0xF0};
        run_test("-1.0 C", rx, 1, -1.0f);
    }

    {
        uint8_t rx[2] = {0x80, 0x00};
        run_test("fault case", rx, 0, 0.0f);
    }

    {
        uint8_t rx[2] = {0x00, 0xA3};
        run_test("10.0 C (status bits)", rx, 1, 10.0f);
    }

    return 0;
}
