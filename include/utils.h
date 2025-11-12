#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

/* === МАТЕМАТИКА === */
double distance_2d(double x1, double z1, double x2, double z2);
double distance_3d(double x1, double y1, double z1, double x2, double y2, double z2);
int fast_sqrt(int x);

/* === ВРЕМЯ === */
uint64_t get_millis();
uint64_t get_micros();

/* === СЛУЧАЙНЫЕ ЧИСЛА === */
uint32_t xorshift32(uint32_t* state);
int random_int(int min, int max);
float random_float();

/* === СТРОКИ === */
char* string_duplicate(const char* str);
void string_trim(char* str);
int string_starts_with(const char* str, const char* prefix);
int string_ends_with(const char* str, const char* suffix);

/* === ХЕШИРОВАНИЕ === */
uint32_t hash_string(const char* str);
uint32_t hash_combine(uint32_t h1, uint32_t h2);

/* === СЖАТИЕ === */
size_t compress_rle(const uint8_t* src, size_t src_len,
                   uint8_t* dst, size_t dst_len);
size_t decompress_rle(const uint8_t* src, size_t src_len,
                     uint8_t* dst, size_t dst_len);

/* === ОТЛАДКА === */
void print_hex(const uint8_t* data, size_t len);
void print_memory_stats();

/* === МАКРОСЫ === */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, min, max) MAX(min, MIN(x, max))
#define ABS(x) ((x) < 0 ? -(x) : (x))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define UNUSED(x) (void)(x)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

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

#endif /* UTILS_H */
