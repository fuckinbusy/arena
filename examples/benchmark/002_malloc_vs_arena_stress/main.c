#include <stdio.h>
#include <time.h>

#define ARENA_IMPLEMENTATION
#include "arena.h"

#define ITERATIONS (size_t)1000000

typedef struct { float x, y, z; } FVec3;

int main(int argc, char const *argv[])
{
    printf("Arena vs malloc test\nTotal iterations number: %zu\n", ITERATIONS);
    
    Arena arena = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_8MB,
        ARENA_CAPACITY_64MB,
        ARENA_ALIGN_CACHELINE,
        ARENA_GROWTH_CONTRACT_FIXED,
        ARENA_GROWTH_FACTOR_NONE,
        ARENA_FLAG_DEBUG
    ));
    
    FVec3 **vectors = malloc(ITERATIONS*sizeof(FVec3*));
    
    size_t allocations = 0;

    long t = clock();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        FVec3 *p = arena_alloc_raw(&arena, sizeof(FVec3), alignof(FVec3));
        if (!p) break;
        vectors[i] = p;
        allocations++;
    }
    long t2 = clock();

    float time_result = (float)(t2 - t) * 1000.0 / CLOCKS_PER_SEC;
    printf("Arena: %.3f ms (Allocations: %zu) (Memory used: %0.1f MB)\n", time_result, allocations, (double)arena.offset / (1024*1024));
    arena_reset(&arena);
    arena_destroy(&arena);
    arena_memset(vectors, 0, sizeof(FVec3*)*ITERATIONS);
    
    t = clock();
    allocations = 0;
    for (size_t i = 0; i < ITERATIONS; ++i) {
        FVec3 *p = malloc(sizeof(FVec3));
        if (!p) break;
        vectors[i] = p;
        allocations++;
    }

    for (size_t i = 0; i < ITERATIONS; ++i) {
        free(vectors[i]);
    }

    t2 = clock();

    time_result = (float)(t2 - t) * 1000.0 / CLOCKS_PER_SEC;
    printf("Malloc: %.3f ms (Allocations: %zu)\n", time_result, allocations);

    free(vectors);
    return 0;
}
