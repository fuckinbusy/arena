/*
The main contract for this parser:
    1. JSON file must be valid.
    2. All numbers should be valid signed/unsigned integers -?[0..MAX_INT32].
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#define ARENA_IMPLEMENTATION
// #define ARENA_LOGGING
// #define ARENA_USE_STD_STRING
#include "arena.h"

#define MAX_FILENAME (int)255
static int INDENTS = 0;
#define INDENT_INC (INDENTS++)
#define INDENT_DEC (INDENTS--)

#define BENCHMARK_ITERATIONS 10000
#define RUNS 5

// #define LOGGING
#ifdef LOGGING
#define LOG(fmt, ...) \
do {\
    printf("[+] ");\
    for (size_t i = 0; i < INDENTS; ++i) {\
        putchar(' ');\
    }\
    printf(fmt"\n", ##__VA_ARGS__);\
} while (0)
#else
#define LOG(...) ((void)0)
#endif

typedef enum JsonType {
    JSON_NUM,
    JSON_BOOL,
    JSON_STR,
    JSON_ARR,
    JSON_OBJ,
    JSON_NULL
} JsonType;

typedef struct JsonValue { // 32 bytes total size
    JsonType type;
    union {
        int num;
        char *str;
        int bool_;

        struct {
            struct JsonValue **values;
            size_t size;
            size_t capacity;
        } array;

        struct {
            char **keys;
            struct JsonValue **values;
            size_t size;
            size_t capacity;
        } object;
    };
} JsonValue;

static Arena ARENA = {0};
static Arena FILE_ARENA = {0};

typedef struct JsonFile {
    char filename[MAX_FILENAME];
    size_t length;
    char buffer[];
} JsonFile;

JsonValue *json_value_alloc(JsonType type);
JsonFile *json_read_file(const char *filename);
int json_init(const char *filename, JsonFile **file);
void json_skip_whitespace(char **ptr);
void json_print(JsonFile *file);

JsonValue *json_parse(JsonFile *file);
JsonValue *json_parse_value(char **p);
JsonValue *json_parse_str(char **p);
JsonValue *json_parse_num(char **p);
JsonValue *json_parse_true(char **p);
JsonValue *json_parse_false(char **p);
JsonValue *json_parse_array(char **p);
JsonValue *json_parse_object(char **p);
JsonValue *json_parse_null(char **p);

JsonFile *json_read_file(const char *filename)
{
    if (!filename) return NULL;

    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    LOG("File size: %d | File name: %s\n", file_size, filename);

    JsonFile *json = (JsonFile*)arena_alloc_raw(&FILE_ARENA, sizeof(JsonFile)+file_size+1, ARENA_ALIGN_8B);
    if (!json) return NULL;

    strncpy(json->filename, filename, strlen(filename));
    fread(json->buffer, 1, file_size, file);
    
    json->length = file_size;
    json->buffer[file_size] = '\0';

    fclose(file);
    return json;
}

int json_init(const char *filename, JsonFile **file)
{
    *file = json_read_file(filename);
    return (*file) ? 1 : 0;
}

void json_skip_whitespace(char **ptr)
{
    int skipped = 0;
    while (isspace((unsigned char)**ptr)) {
        (*ptr)++;
        skipped++;
    }
    if (skipped > 0)
        LOG("Whitespaces skipped");
}

void json_print(JsonFile *file)
{
    if (!file) return;

    char *c = file->buffer;
    while (*c) {
        LOG("%c", *c);
        c++;
    }
    LOG("\n");

}

JsonValue *json_parse_value(char **p)
{
    if (!p) return NULL;

    json_skip_whitespace(p);

    char c = **p;

    switch (c) {
        case '[': return json_parse_array(p);
        case '{': return json_parse_object(p);
        case '"': return json_parse_str(p);
        case 't': return json_parse_true(p);
        case 'f': return json_parse_false(p);
        case 'n': return json_parse_null(p);
        default:
            if (**p == '-' || isdigit((unsigned char)**p)) return json_parse_num(p);
            return NULL;
    }

    return NULL;
}

JsonValue *json_parse(JsonFile *file) // returns true/false state
{
    if (!file || file->length == 0) return NULL;

    char *p = file->buffer;

    json_skip_whitespace(&p);
        
    JsonValue *root = json_parse_value(&p);
    // if (!root) return 0;
    
    json_skip_whitespace(&p);
    
    // if (*p != '\0') return NULL;

    return root;
}

JsonValue *json_parse_str(char **p)
{
    if (!(*p)) return NULL;

    ++(*p); // next char after "
    char *start = *p;

    LOG("Parsing string");
    
    while (**p && **p != '"') {
        (*p)++;
    }
    if (**p != '"') return NULL;

    size_t size = (*p) - start;

    JsonValue *str = (JsonValue*)arena_alloc_raw(&ARENA, sizeof(JsonValue), alignof(JsonValue));
    if (!str) return NULL;

    char *s = (char*)arena_alloc_raw(&ARENA, size + 1, ARENA_ALIGN_8B);
    s[size] = '\0';
    memcpy(s, start, size);
    str->type = JSON_STR;
    str->str = s;

    (*p)++;
    LOG("String parsed");

    return str;
}

JsonValue* json_parse_num(char** p)
{
    int sign = 1;
    if (**p == '-') {
        sign = -1;
        (*p)++;
        if (isdigit((unsigned char)**p) == 0) return NULL;
    };

    LOG("Parsing number");
    
    char *start = *p;

    while (isdigit((unsigned char)**p) != 0) {
        (*p)++;
    }

    int value = 0;
    while (start != *p) {
        value = (value * 10) + (*start - '0');
        start++;
    }

    value *= sign;

    JsonValue *v = arena_alloc_raw(&ARENA, sizeof(JsonValue), alignof(JsonValue));
    if (!v) return NULL;

    v->type = JSON_NUM;
    v->num = value;

    LOG("Number parsed");

    return v;
}

JsonValue* json_parse_true(char** p)
{
    if (strncmp(*p, "true", 4) != 0) return NULL;
    (*p) += 4;

    LOG("Parsing bool");
    
    JsonValue *v = arena_alloc_raw(&ARENA, sizeof(JsonValue), alignof(JsonValue));
    if (!v) return NULL;
    v->type = JSON_BOOL;
    v->bool_ = 1;
    
    LOG("Bool parsed");

    return v;
}

JsonValue* json_parse_false(char** p)
{
    if (strncmp(*p, "false", 5) != 0) return NULL;
    (*p) += 5;

    LOG("Parsing bool");

    JsonValue *v = arena_alloc_raw(&ARENA, sizeof(JsonValue), alignof(JsonValue));
    if (!v) return NULL;
    v->type = JSON_BOOL;
    v->bool_ = 0;

    LOG("Bool parsed");
    return v;
}

JsonValue *json_parse_array(char **p)
{
    if (**p == '[') (*p)++;
    json_skip_whitespace(p);

    LOG("Parsing array");
    INDENT_INC;

    JsonValue *root = arena_alloc_raw(&ARENA, sizeof(JsonValue), alignof(JsonValue));
    root->type = JSON_ARR;
    root->array.size = 0;
    root->array.capacity = 4;
    root->array.values = arena_alloc_raw(&ARENA, sizeof(JsonValue*)*root->array.capacity, alignof(JsonValue));
    if (!root) goto exit_;

    if (**p == ']') {
        (*p)++;
        goto exit_;
    }

    while (1) { // infinite loop (its okay because it will be interrupted anyway inside)
        json_skip_whitespace(p);

        JsonValue *v = json_parse_value(p);
        if (v == NULL) goto exit_;

        if (root->array.size >= root->array.capacity) {
            size_t new_cap = root->array.capacity * 2;
            void *tmp = arena_alloc_raw(&ARENA, new_cap * sizeof(JsonValue*), alignof(JsonValue*));
            arena_memcpy(tmp, root->array.values, root->array.size*sizeof(JsonValue*));
            if (!tmp) goto exit_;
            root->array.values = tmp;
            root->array.capacity = new_cap;
        }
        root->array.values[root->array.size++] = v;

        json_skip_whitespace(p);

        if (**p == ',') {
            (*p)++;
            continue;
        }

        if (**p == ']') {
            (*p)++;
            break;
        } else {
            (*p)++;
            goto exit_;
        }
    }

exit_:
    LOG("Array parsed");
    INDENT_DEC;
    return root;
}

JsonValue *json_parse_object(char **p)
{
    if (**p != '{') return NULL;
    (*p)++;
    json_skip_whitespace(p);
    
    LOG("Parsing object");
    INDENT_INC;

    JsonValue *root = arena_alloc_raw(&ARENA, sizeof(JsonValue), alignof(JsonValue));
    if (!root) goto exit_;
    root->type            = JSON_OBJ;
    root->object.size     = 0;
    root->object.capacity = 4;
    root->object.keys     = arena_alloc_raw(&ARENA, sizeof(char*) * root->object.capacity, alignof(char*));
    root->object.values   = arena_alloc_raw(&ARENA, sizeof(JsonValue*) * root->object.capacity, alignof(JsonValue*));
    if (!root->object.keys || !root->object.values) goto exit_;

    if (**p == '}') {
        (*p)++;
        goto exit_;
    }
    
    while (1) {
        json_skip_whitespace(p);

        if (**p != '"') goto exit_;
        (*p)++;

        char *start = *p;
        while (**p && **p != '"') {
            (*p)++;
        }
        if (**p != '"') goto exit_;

        size_t key_len = *p - start;
        (*p)++;

        json_skip_whitespace(p);

        if (**p != ':') goto exit_;
        (*p)++;

        json_skip_whitespace(p);
    
        JsonValue *v = json_parse_value(p);
        if (!v) goto exit_;

        char *key = arena_alloc_raw(&ARENA, key_len+1, alignof(1));
        arena_memcpy(key, start, key_len);
        key[key_len] = '\0';

        if (root->object.capacity <= root->object.size) {
            root->object.capacity *= 2;
            void *tmp_keys      = arena_alloc_raw(&ARENA, root->object.capacity * sizeof(char*), alignof(char*));
            void *tmp_values    = arena_alloc_raw(&ARENA, root->object.capacity * sizeof(JsonValue*), alignof(JsonValue*));
            arena_memcpy(tmp_keys, root->object.keys, root->object.size*sizeof(char*));
            arena_memcpy(tmp_values, root->object.values, root->object.size*sizeof(JsonValue*));    
            if (!tmp_keys || !tmp_values) goto exit_;
            root->object.keys   = tmp_keys;
            root->object.values = tmp_values;
        }

        root->object.values[root->object.size] = v;
        root->object.keys[root->object.size]   = key;
        root->object.size++;
        
        json_skip_whitespace(p);

        if (**p == ',') {
            (*p)++;
            continue;
        }

        if (**p == '}') {
            (*p)++;
            break;
        } else {
            (*p)++;
            goto exit_;
        }
    }

exit_:
    INDENT_DEC;
    LOG("Object parsed");
    return root;
}

JsonValue *json_parse_null(char **p)
{
    if (strncmp(*p, "null", 4) != 0) return NULL;
    (*p) += 4;

    JsonValue *v = arena_alloc_raw(&ARENA, sizeof(JsonValue), alignof(JsonValue));
    v->type = JSON_NULL;

    LOG("Null parsed");

    return v;
}

void json_free_value(JsonValue *value)
{
    if (!value) return;

    switch (value->type) {
        case JSON_ARR: {
            for (size_t i = 0; i < value->array.size; ++i) {
                json_free_value(value->array.values[i]);
                LOG("Array value freed: %p", value->array.values[i]);
            }
            free(value->array.values);
            LOG("Array values array freed: %p", value->array.values);
            free(value);
            LOG("Array freed: %p", value);
        } break;
        
        case JSON_OBJ: {
            for (size_t i = 0; i < value->object.size; ++i) {
                if (value->object.values) {
                    json_free_value(value->object.values[i]);
                    LOG("Object value freed: %p", value->object.values[i]);
                }
                if (value->object.keys) {
                    free(value->object.keys[i]);
                    LOG("Object key freed: %p", value->object.keys[i]);
                }
            }
            free(value->object.keys);
            LOG("Object keys array freed: %p", value->object.keys);
            free(value->object.values);
            LOG("Object values array freed: %p", value->object.values);
            free(value);
            LOG("Object freed: %p", value);
        } break;
        
        case JSON_BOOL: {
            free(value);
            LOG("Bool freed: %p", value);
        } break;
        
        case JSON_NULL: {
            free(value);
            LOG("Null freed: %p", value);
        } break;
        
        case JSON_STR: {
            free(value->str);
            LOG("String array freed: %p", value->str);
            free(value);
            LOG("String freed: %p", value);
        } break;
        
        case JSON_NUM: {
            free(value);
            LOG("Number freed: %p", value);
        } break;
    }
}

int main(void)
{
    const char *FILE = "j.json";

    ARENA = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_8KB,
        ARENA_CAPACITY_32MB,
        ARENA_GROWTH_CONTRACT_CHUNKY,
        ARENA_GROWTH_FACTOR_CHUNKY_1MB,
        0
    ));

    FILE_ARENA = arena_create_ex(arena_config_create(
        ARENA_CAPACITY_1MB,
        ARENA_CAPACITY_1MB,
        ARENA_GROWTH_CONTRACT_FIXED,
        0,
        0
    ));
    
    JsonFile *json_file = NULL;
    if (!json_init(FILE, &json_file)) {
        LOG("Json parser initialized");
        LOG("Couldnt read the json file!\n");
        return 1;
    }
    
    double time = 0;
    for (size_t r = 0; r < RUNS; ++r) {
        clock_t start = clock();
        for (size_t i = 0; i < BENCHMARK_ITERATIONS; ++i) {
            JsonValue *root = json_parse(json_file);
            arena_reset(&ARENA);
            LOG("File parsed\n");
        }
        clock_t end = clock();
        time += (double)(end - start);
    }
    time /= RUNS;
    
    arena_destroy(&ARENA);
    arena_destroy(&FILE_ARENA);
    
    double ms = time * 1000.0 / CLOCKS_PER_SEC;
    printf("ARENA-----------------------------\n");
    printf("Total: %.3f ms\n", ms);
    printf("Per parse: %.6f ms\n", ms / BENCHMARK_ITERATIONS);
    printf("ARENA-----------------------------\n");

    return 0;
}