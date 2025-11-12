#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include "globals.h"

/* Структура для игрока */
typedef struct {
    int32_t entity_id;
    char username[20];
    uint8_t uuid[16];
    
    /* Позиция и ориентация */
    double x, y, z;
    float yaw, pitch;
    bool on_ground;
    
    /* Состояние */
    int32_t health;
    float experience;
    int32_t level;
    
    /* Сетевые данные */
    int socket;
    uint8_t protocol_state;  /* 0=handshake, 1=status, 2=login, 3=play */
    char ip[16];
    int port;
    
    /* Временные отметки */
    uint64_t last_keep_alive;
    uint64_t join_time;
    
    /* Инвентарь (упрощённо) */
    uint16_t inventory[MAX_INVENTORY_SIZE];
    uint8_t selected_slot;
    
    /* Загруженные чанки */
    int32_t loaded_chunks[MAX_CHUNKS_LOADED * 2];
    int loaded_chunk_count;
    
    /* Флаги */
    bool spawn_position_sent;
    bool ready;
    
} Player;

/* Структура для чанка */
typedef struct {
    int32_t x, z;  /* координаты чанка */
    uint8_t blocks[CHUNK_SIZE][256][CHUNK_SIZE];  /* [x][y][z] блоки */
    uint32_t last_accessed;
    bool modified;
    uint8_t light_data[CHUNK_SIZE * CHUNK_SIZE * 256 / 2];  /* упрощённо */
} Chunk;

/* Глобальное состояние сервера */
typedef struct {
    bool running;
    int server_socket;
    uint32_t current_tick;
    uint64_t server_time;
    
    /* Игроки */
    Player players[MAX_PLAYERS];
    int active_players;
    pthread_rwlock_t players_lock;
    
    /* Чанки */
    Chunk* chunks;
    int loaded_chunks;
    pthread_rwlock_t chunks_lock;
    
    /* Потоки */
    pthread_t tick_thread;
    pthread_t save_thread;
    pthread_t network_thread;
    
    /* Статистика */
    uint64_t total_ticks;
    uint32_t ticks_with_lag;
    
} ServerState;

extern ServerState server_state;

/* Функции сервера */
bool server_init();
void server_shutdown();
void server_tick();
void server_save_world();

/* Функции игроков */
Player* player_create(const char* username, const uint8_t* uuid);
void player_destroy(Player* player);
void player_broadcast_position(Player* player);
void player_send_chunk(Player* player, int32_t chunk_x, int32_t chunk_z);
void player_set_position(Player* player, double x, double y, double z, float yaw, float pitch);

/* Функции чанков */
Chunk* chunk_create(int32_t x, int32_t z);
void chunk_destroy(Chunk* chunk);
Chunk* chunk_get_or_create(int32_t x, int32_t z);
void chunk_generate(Chunk* chunk);
void chunk_save(Chunk* chunk);
void chunk_load(Chunk* chunk);

/* Функции блоков */
uint8_t block_get(int32_t x, int32_t y, int32_t z);
void block_set(int32_t x, int32_t y, int32_t z, uint8_t block_id);

#endif /* SERVER_H */
