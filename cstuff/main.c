#include <stdio.h>

void rotate_right(int *arr, unsigned int n, unsigned int k) {
    if (n <= 1) return;
    // to rotate right k times we can
    // 1 2 3 4
    k = k % n;
    int moved = 0;

    for (int i = 0; moved < n;i++) {
        int temp = arr[i];
        int idx = i;
        int start = i;
        while (1) {
            int next_idx = (idx+k)%n;
            int temp2 = arr[next_idx];
            arr[next_idx] = temp;
            temp = temp2;
            idx = next_idx;
            moved++;
            if (start == idx || moved >= n) break;
        }
    }
}

static void print_array(const int *arr, unsigned int n) {
    printf("[");
    for (unsigned int i = 0; i < n; i++) {
        printf("%d", arr[i]);
        if (i + 1 < n) printf(", ");
    }
    printf("]\n");
}

int main(void) {
    /* Normal case */
    int a1[] = {1,2,3,4,5,6,7};
    rotate_right(a1, 7, 3);
    print_array(a1, 7);   // expected: [5,6,7,1,2,3,4]

    /* k > n */
    int a2[] = {10,20,30,40};
    rotate_right(a2, 4, 6);
    print_array(a2, 4);   // expected: [30,40,10,20]

    /* k == 0 */
    int a3[] = {1,2,3};
    rotate_right(a3, 3, 0);
    print_array(a3, 3);   // expected: [1,2,3]

    /* n == 1 */
    int a4[] = {42};
    rotate_right(a4, 1, 5);
    print_array(a4, 1);   // expected: [42]

    /* n == 0 (should not crash) */
    rotate_right(NULL, 0, 10);

    return 0;
}
