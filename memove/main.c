//
// Created by justin on 2/23/26.
//

#include "stdio.h"
#include "stdint.h"

// forward overlap needs to copy from right to left
// backwards overlap needs to copy from left to right
// no overlap it doesn't matter
void* my_memmove(void* dest, const void* src, size_t n) {
    if (n == 0 || dest == src) return dest;

    uint8_t* d = dest;
    const uint8_t* s = src;

    // explicit check for forward overlap
    // copy from right to left
    if ((d > s) && (d < s+n)) {
        d += n;
        s += n;
        for (size_t i = 0; i < n; i++) {
            d--;
            s--;
            *d = *s;
        }
    }
    // copy from left to right
    else {
        for (size_t i = 0; i < n; i++) {
            *d = *s;
            d++;
            s++;
        }
    }

    return dest;
}

/* helper to print buffers */
void print_buffer(const char *label, const char *buf, size_t n)
{
    printf("%s: ", label);
    for (size_t i = 0; i < n; i++)
        printf("%c ", buf[i]);
    printf("\n");
}

int main(void)
{
    printf("=== Test 1: Non-overlapping copy ===\n");
    {
        char src[]  = {'A','B','C','D','E'};
        char dest[5] = {0};

        my_memmove(dest, src, 5);

        print_buffer("dest", dest, 5);
        // expected: A B C D E
    }

    printf("\n=== Test 2: Overlap (dest > src) ===\n");
    {
        char buffer[] = {'A','B','C','D','E','F'};

        my_memmove(buffer + 2, buffer, 4);

        print_buffer("buffer", buffer, 6);
        // expected: A B A B C D
    }

    printf("\n=== Test 3: Overlap (dest < src) ===\n");
    {
        char buffer[] = {'A','B','C','D','E','F'};

        my_memmove(buffer, buffer + 2, 4);

        print_buffer("buffer", buffer, 6);
        // expected: C D E F E F
    }

    printf("\n=== Test 4: dest == src ===\n");
    {
        char buffer[] = {'A','B','C','D'};

        my_memmove(buffer, buffer, 4);

        print_buffer("buffer", buffer, 4);
        // expected unchanged
    }

    printf("\n=== Test 5: n == 0 ===\n");
    {
        char buffer[] = {'A','B','C','D'};

        my_memmove(buffer + 1, buffer, 0);

        print_buffer("buffer", buffer, 4);
        // expected unchanged
    }

    return 0;
}
