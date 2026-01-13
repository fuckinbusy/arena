#include <stdio.h>
#include <time.h>

#define ARENA_IMPLEMENTATION
#include "arena.h"

#define ITERATIONS (size_t)1000000
#define RUNS       (size_t)5

typedef struct { float x, y, z; } FVec3;

static void test_arena()
{
    Arena arena = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_1KB,
        ARENA_CAPACITY_128MB,
        ARENA_GROWTH_CONTRACT_CHUNKY,
        ARENA_GROWTH_FACTOR_CHUNKY_2MB,
        ARENA_FLAG_NONE
    ));
    
    FVec3 **vectors = malloc(ITERATIONS*sizeof(FVec3*));
    size_t allocations = ITERATIONS;

    double time = 0;
    for (size_t r = 0; r < RUNS; ++r) {
        long t = clock();
        for (size_t i = 0; i < ITERATIONS; ++i) {
            FVec3 *p = arena_alloc_raw(&arena, sizeof(FVec3), alignof(FVec3));
            if (!p) break;
            vectors[i] = p;
        }
        long t2 = clock();
        time += (double)(t2 - t); 
        arena_reset(&arena);
    }
    time /= RUNS;

    double time_result = time * 1000.0 / CLOCKS_PER_SEC;
    printf("Arena: %.3f ms (Allocations: %zu) (Memory used: %0.1f MB)\n", time_result, allocations, (double)arena.reserved / (1024*1024));

    arena_destroy(&arena);
    free(vectors);
}

static void test_malloc()
{
    FVec3 **vectors = malloc(ITERATIONS*sizeof(FVec3*));
    size_t allocations = ITERATIONS;

    double time = 0;
    for (size_t r = 0; r < RUNS; ++r) {
        long t = clock();
        for (size_t i = 0; i < ITERATIONS; ++i) {
            FVec3 *p = malloc(sizeof(FVec3));
            if (!p) break;
            vectors[i] = p;
        }
        long t2 = clock();
        time += (double)(t2 - t);

        for (size_t i = 0; i < ITERATIONS; ++i) {
            free(vectors[i]);
        }
    }
    time /= RUNS;

    double time_result = time * 1000.0 / CLOCKS_PER_SEC;
    printf("Malloc: %.3f ms (Allocations: %zu)\n", time_result, allocations);


    free(vectors);
}

int main(int argc, char const *argv[])
{
    printf("Arena vs malloc test\nTotal iterations number: %zu\nRuns: %zu\n", ITERATIONS, RUNS);
    
    test_arena();
    test_malloc();

    return 0;
}
