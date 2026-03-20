#include <stdint.h>
#include <stdio.h>

// =======================
// Packet Parser (UART/CAN style)
// =======================
//
// You are given a byte stream that may contain multiple packets.
//
// Packet format:
//   [0xAA][LEN][DATA...][CHECKSUM]
//
// Where:
//   - 0xAA is the header (1 byte)
//   - LEN is number of DATA bytes (1 byte)
//   - DATA is LEN bytes
//   - CHECKSUM is 1 byte
//
// Checksum rule:
//   checksum = (LEN + sum(DATA bytes)) & 0xFF
//
// Requirements:
//
// Implement:
//   int parse_packet(const uint8_t* buf, size_t len,
//                    uint8_t* out_data, size_t* out_len);
//
// Behavior:
//   - Find the FIRST valid packet in buf
//   - Copy DATA into out_data
//   - Set *out_len
//   - Return 1 if a valid packet is found
//   - Return 0 otherwise
//
// Edge cases:
//   - ignore garbage before header
//   - incomplete packet → return 0
//   - bad checksum → skip and continue searching
//   - multiple packets → only return the FIRST valid one
//
// Constraints:
//   - no dynamic allocation
//   - do not read out of bounds
//   - assume out_data is large enough

int parse_packet(const uint8_t* buf, size_t len,
                 uint8_t* out_data, size_t* out_len)
{
    *out_len = 0;
    // TODO: implement
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == 0xAA) {
            size_t curr_idx = i + 1;

            // read len
            uint8_t data_len =  buf[curr_idx++];

            if (len < i + 1 + 1 + data_len + 1) {
                return 0;
            }

            // read data and copy into buffer
            uint8_t total = data_len;
            for (size_t j = 0; j < data_len; j++) {
                uint8_t val = buf[curr_idx++];
                out_data[j] = val;
                total += val;
            }

            // check checksum
            uint8_t cs = buf[curr_idx];

            // printf("%zu\n", curr_idx);
            // printf("%u\n", cs);

            if (cs == total) {
                *out_len = data_len;
                return 1;
            }
        }
    }

    return 0;
}

// =======================
// Test Harness
// =======================

void print_result(int ok, uint8_t* data, size_t len)
{
    printf("ok=%d len=%zu data=", ok, len);
    for (size_t i = 0; i < len; i++) {
        printf("%u ", data[i]);
    }
    printf("\n");
}

int main(void)
{
    uint8_t out[256];
    size_t out_len = 0;

    // valid packet: AA 03 10 20 30 checksum
    uint8_t test1[] = {
        0xAA, 0x03, 0x10, 0x20, 0x30,
        (0x03 + 0x10 + 0x20 + 0x30) & 0xFF
    };

    int ok1 = parse_packet(test1, sizeof(test1), out, &out_len);
    print_result(ok1, out, out_len);

    // garbage before packet
    uint8_t test2[] = {
        0x00, 0xFF, 0xAA, 0x02, 0x05, 0x06,
        (0x02 + 0x05 + 0x06) & 0xFF
    };

    int ok2 = parse_packet(test2, sizeof(test2), out, &out_len);
    print_result(ok2, out, out_len);

    // bad checksum → should skip
    uint8_t test3[] = {
        0xAA, 0x02, 0x01, 0x02, 0x00,  // bad checksum
        0xAA, 0x01, 0x09,
        (0x01 + 0x09) & 0xFF
    };

    int ok3 = parse_packet(test3, sizeof(test3), out, &out_len);
    print_result(ok3, out, out_len);

    // incomplete packet
    uint8_t test4[] = {
        0xAA, 0x05, 1,2,3
    };

    int ok4 = parse_packet(test4, sizeof(test4), out, &out_len);
    print_result(ok4, out, out_len);

    return 0;
}
