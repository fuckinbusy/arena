#define SDL_MAIN_USE_CALLBACKS 1
#include "../include/SDL3/SDL.h"
#include "../include/SDL3/SDL_main.h"

#define ARENA_IMPLEMENTATION
#include "arena.h"

#include <stdio.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static Uint64 last_time = 0;

static Arena arena;
static bool use_arena   = true;
static bool window_busy = false;

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define NUM_POINTS ((WINDOW_WIDTH*WINDOW_HEIGHT)/6)
#define MAX_FRAMES 5000

static SDL_FPoint **points = NULL;

// Arrays to store frame times
static float frame_times[MAX_FRAMES];
static size_t frame_index = 0;

// Function to save CSV
static void save_fps_data(const char *filename, float *data, size_t count, const char *label) {
    FILE *f = fopen(filename, "w");
    if (!f) return;
    fprintf(f, "frame,%s_ms\n", label);
    for (size_t i = 0; i < count; ++i) {
        fprintf(f, "%zu,%.3f\n", i + 1, data[i]);
    }
    fclose(f);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Example Renderer Points", "1.0", "com.example.renderer-points");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Arena benchmark", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    arena = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_16MB,
        ARENA_CAPACITY_32MB,
        ARENA_GROWTH_CONTRACT_FIXED,
        0,
        ARENA_FLAG_DEBUG
    ));

    points = malloc(sizeof(SDL_FPoint*)*NUM_POINTS);

    last_time = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    switch (event->type) {
        case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
        case SDL_EVENT_KEY_DOWN:
            switch (event->key.key) {
                case SDLK_1: use_arena = true; break;
                case SDLK_2: use_arena = false; break;
                case SDLK_ESCAPE: return SDL_APP_SUCCESS;
            }
            break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED: window_busy = false; break;
        case SDL_EVENT_WINDOW_FOCUS_LOST: window_busy = true; break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    const Uint64 now = SDL_GetTicks();
    const float elapsed = ((float)(now - last_time)) / 1000.0f;
    last_time = now;

    if (window_busy) {
        SDL_Delay(10);
        return SDL_APP_CONTINUE;
    }

    // Allocation of points
    for (size_t i = 0; i < NUM_POINTS; ++i) {
        SDL_FPoint *p = NULL;
        if (use_arena) {
            p = arena_alloc_raw(&arena, sizeof(SDL_FPoint), ARENA_ALIGN_16B);
        } else {
            p = malloc(sizeof(SDL_FPoint));
        }
        p->x = (float)SDL_rand(WINDOW_WIDTH);
        p->y = (float)SDL_rand(WINDOW_HEIGHT);
        points[i] = p;
    }

    // Render
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    for (size_t i = 0; i < NUM_POINTS; ++i) {
        SDL_RenderPoint(renderer, points[i]->x, points[i]->y);
    }
    SDL_RenderPresent(renderer);

    // Record frame time in ms
    if (frame_index < MAX_FRAMES) {
        frame_times[frame_index++] = elapsed * 1000.0f;
    }

    // Reset allocations
    if (use_arena) {
        arena_reset(&arena);
    } else {
        for (size_t i = 0; i < NUM_POINTS; ++i) {
            free(points[i]);
        }
    }

    // Update FPS display every second
    static float fps_timer = 0;
    static size_t frame_counter = 0;
    fps_timer += elapsed;
    frame_counter++;
    if (fps_timer >= 1.0f) {
        char buffer[128] = {0};
        SDL_snprintf(buffer, sizeof buffer, "FPS: %zu (%s) (%zu points)", frame_counter, use_arena ? "arena" : "malloc", (size_t)NUM_POINTS);
        SDL_SetWindowTitle(window, buffer);
        fps_timer -= 1.0f;
        frame_counter = 0;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    // Save frame times
    save_fps_data(use_arena ? "arena.csv" : "malloc.csv", frame_times, frame_index, use_arena ? "arena" : "malloc");

    if (use_arena) {
        arena_destroy(&arena);
    } else {
        for (size_t i = 0; i < NUM_POINTS; ++i) {
            free(points[i]);
        }
    }
    free(points);
}
