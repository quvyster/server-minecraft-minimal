#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* Утилиты для сервера */

/* === МАТЕМАТИКА === */

double distance_2d(double x1, double z1, double x2, double z2) {
    double dx = x2 - x1;
    double dz = z2 - z1;
    return sqrt(dx * dx + dz * dz);
}

double distance_3d(double x1, double y1, double z1, 
                   double x2, double y2, double z2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double dz = z2 - z1;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

/* Быстрое целое число */
int fast_sqrt(int x) {
    if (x < 0) return 0;
    if (x == 0) return 0;
    
    int result = x;
    int xn = (result + 1) >> 1;
    
    while (xn < result) {
        result = xn;
        xn = (result + x / result) >> 1;
    }
    
    return result;
}

/* === ВРЕМЯ === */

uint64_t get_millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

uint64_t get_micros() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/* === СЛУЧАЙНЫЕ ЧИСЛА === */

uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *state = x;
}

int random_int(int min, int max) {
    static uint32_t seed = 0;
    if (seed == 0) {
        seed = time(NULL);
    }
    uint32_t val = xorshift32(&seed);
    return min + (val % (max - min + 1));
}

float random_float() {
    static uint32_t seed = 0;
    if (seed == 0) {
        seed = time(NULL);
    }
    return (float)xorshift32(&seed) / (float)0xFFFFFFFFU;
}

/* === СТРОКИ === */

char* string_duplicate(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* dup = malloc(len + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

void string_trim(char* str) {
    if (!str) return;
    
    int len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\n')) {
        str[--len] = '\0';
    }
    
    int start = 0;
    while (str[start] == ' ') {
        start++;
    }
    
    if (start > 0) {
        memmove(str, str + start, len - start + 1);
    }
}

int string_starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) return 0;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

int string_ends_with(const char* str, const char* suffix) {
    if (!str || !suffix) return 0;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

/* === ХЕШИРОВАНИЕ === */

uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash;
}

uint32_t hash_combine(uint32_t h1, uint32_t h2) {
    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
}

/* === СЖАТИЕ === */

/* Простое RLE сжатие */
size_t compress_rle(const uint8_t* src, size_t src_len,
                   uint8_t* dst, size_t dst_len) {
    size_t dst_pos = 0;
    size_t src_pos = 0;
    
    while (src_pos < src_len && dst_pos + 2 < dst_len) {
        uint8_t byte = src[src_pos];
        uint8_t count = 1;
        
        while (src_pos + count < src_len && 
               src[src_pos + count] == byte && 
               count < 255) {
            count++;
        }
        
        dst[dst_pos++] = byte;
        dst[dst_pos++] = count;
        src_pos += count;
    }
    
    return dst_pos;
}

size_t decompress_rle(const uint8_t* src, size_t src_len,
                     uint8_t* dst, size_t dst_len) {
    size_t dst_pos = 0;
    size_t src_pos = 0;
    
    while (src_pos + 1 < src_len && dst_pos < dst_len) {
        uint8_t byte = src[src_pos++];
        uint8_t count = src[src_pos++];
        
        for (uint8_t i = 0; i < count && dst_pos < dst_len; i++) {
            dst[dst_pos++] = byte;
        }
    }
    
    return dst_pos;
}

/* === ОТЛАДКА === */

void print_hex(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (i % 16 == 0) printf("\n");
        printf("%02X ", data[i]);
    }
    printf("\n");
}

void print_memory_stats() {
    printf("[MEMORY] Статистика памяти:\n");
    #ifdef __linux__
    FILE* f = fopen("/proc/self/status", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                printf("  RSS: %s", line + 7);
            } else if (strncmp(line, "VmPeak:", 7) == 0) {
                printf("  Peak: %s", line + 8);
            }
        }
        fclose(f);
    }
    #endif
}

/* === АССЕРТЫ === */

#ifdef DEBUG_LOG
#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "[ASSERT FAILED] %s:%d - %s\n", \
                    __FILE__, __LINE__, msg); \
            abort(); \
        } \
    } while(0)
#else
#define ASSERT(cond, msg) do {} while(0)
#endif

/* === МАКРОСЫ === */

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max) MAX(min, MIN(x, max))
#define ABS(x) ((x) < 0 ? -(x) : (x))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define UNUSED(x) (void)(x)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
