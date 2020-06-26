#include <stdio.h>
#include <stdint.h>
#include "lalloc.h"
#include "util.h"

#define SIZE 20

#define ROUNDS 1000
int main() {
    uint16_t** a = malloc(sizeof(uint16_t*) * ROUNDS);
    uint16_t* b = malloc(sizeof(uint16_t*) * ROUNDS);

    b = lalloc(2);
    lfree(b);

    uint64_t elapsedal = 0;
    uint64_t elapsedfr = 0;
    uint64_t start;
    uint64_t end;
    start = utime();
    for (int i = 0; i < ROUNDS; i++) {
        a[i] = malloc(SIZE);
    }
    end = utime();
    elapsedal = end - start;
    for (int i = 0; i < ROUNDS; i++) {
        free(a[i]);
    }
    end = utime();
    elapsedfr = end - start;

    uint64_t blapsedal = 0;
    uint64_t blapsedfr = 0;
    start = utime();
    for (int i = 0; i < ROUNDS; i++) {
        a[i] = lalloc(SIZE);
    }
    end = utime();
    blapsedal = end - start;
    for (int i = 0; i < ROUNDS; i++) {
        lfree(a[i]);
    }
    end = utime();
    blapsedfr = end - start;

    printf("Alloc: \n");
    printf("%lu vs %lu\n", elapsedal, blapsedal);
    printf("%f vs %f\n", (elapsedal * 1000. * 2.81) / ((double) ROUNDS), (blapsedal * 1000. * 2.81) / ((double) ROUNDS));
    printf("\nFree: \n");
    printf("%lu vs %lu\n", elapsedfr, blapsedfr);
    printf("%f vs %f\n", (elapsedfr * 1000. * 2.81) / ((double) ROUNDS), (blapsedfr * 1000. * 2.81) / ((double) ROUNDS));

}

/*
int main() {
    printf("%lu\n", sizeof(struct bblk));
    struct bblk a;
    printf("%lu\n", sizeof(a.mem));
    printf("I want to move first\n");
    return 0;
}
 */
