#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define ITERS 100000 // for -O2
#define SIZE  524288

volatile size_t sink;

size_t slow_strlen(const char *s) {
    size_t n = 0;
    while (s[n] != 0) n++;
    return n;
}

#define HAS_ZERO(v) (((v) - 0x0101010101010101ull) & ~(v) & 0x8080808080808080ull)
size_t fast_strlen(const char *str)
{
    const char *p = str;
    while ((uintptr_t)p & 7) {
        if (*p == 0) return p - str;
        p++;
    }

    const uint64_t *word = (uint64_t*)p;
    while (1) {
        uint64_t value = *word;
        if (HAS_ZERO(value)) break;
        word++;
    }

    const char *byte = (const char*)word;
    for (int i = 0; i < 8; ++i) {
        if (byte[i] == 0) return (byte + i) - str;
    }

    return 0;
}

size_t asm_fast_strlen(const char *s) {
    return __builtin_strlen(s);
}

int main() {
    static char buf[SIZE + 1];
    memset(buf, 'A', SIZE);
    buf[SIZE] = 0;

    clock_t t1 = clock();
    for (int i = 0; i < ITERS; i++)
        sink = slow_strlen(buf);
    clock_t t2 = clock();
    
    printf("slow: %f\n", (double)(t2 - t1) / CLOCKS_PER_SEC);
    t2 = clock();

    for (int i = 0; i < ITERS; i++)
        sink = fast_strlen(buf);
    clock_t t3 = clock();
    
    printf("fast: %f\n", (double)(t3 - t2) / CLOCKS_PER_SEC);
    t3 = clock();

    for (int i = 0; i < ITERS; i++)
        sink = asm_fast_strlen(buf);
    clock_t t4 = clock();
    
    printf("asm:  %f\n", (double)(t4 - t3) / CLOCKS_PER_SEC);

    /*
    so, after doing some simple tests it comes out
    that slow version is actually slow only on O0 and O1
    and there is almost no difference on O2
    HOWEVER, it is actually faster on O3

    but, it can depend on CPU and platform
    */
}
