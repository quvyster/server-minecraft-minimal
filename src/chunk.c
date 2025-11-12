#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "server.h"

/* Простая генерация ландшафта (шум Перлина упрощённо) */
static int32_t get_terrain_height(int32_t x, int32_t z) {
    /* Упрощённая генерация: используем хеш для квазислучайности */
    int32_t h = 0;
    h = ((x * 73856093) ^ (z * 19349663)) & 0x7FFFFFFF;
    
    /* Базовая высота 64 блока */
    int32_t base_height = 64;
    int32_t variation = (h % 24) - 12;  /* ±12 блоков */
    
    return base_height + variation;
}

/* Создать чанк */
Chunk* chunk_create(int32_t x, int32_t z) {
    Chunk* chunk = malloc(sizeof(Chunk));
    if (!chunk) return NULL;
    
    memset(chunk, 0, sizeof(Chunk));
    chunk->x = x;
    chunk->z = z;
    chunk->last_accessed = (uint32_t)time(NULL);
    chunk->modified = false;
    
    printf("[CHUNK] Создан чанк: (%d, %d)\n", x, z);
    
    return chunk;
}

/* Удалить чанк */
void chunk_destroy(Chunk* chunk) {
    if (!chunk) return;
    
    printf("[CHUNK] Удалён чанк: (%d, %d)\n", chunk->x, chunk->z);
    free(chunk);
}

/* Генерировать чанк */
void chunk_generate(Chunk* chunk) {
    if (!chunk) return;
    
    int32_t chunk_x = chunk->x;
    int32_t chunk_z = chunk->z;
    
    /* Заполняем чанк блоками */
    for (int lx = 0; lx < CHUNK_SIZE; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE; lz++) {
            int32_t world_x = chunk_x * CHUNK_SIZE + lx;
            int32_t world_z = chunk_z * CHUNK_SIZE + lz;
            
            /* Генерируем высоту ландшафта */
            int32_t terrain_height = get_terrain_height(world_x, world_z);
            
            for (int y = 0; y < 256; y++) {
                uint8_t block_id;
                
                if (y == 0) {
                    /* Бедрок */
                    block_id = 7;
                } else if (y < terrain_height - 3) {
                    /* Камень */
                    block_id = 1;
                } else if (y < terrain_height) {
                    /* Грязь */
                    block_id = 3;
                } else if (y == terrain_height) {
                    /* Трава */
                    block_id = 2;
                } else if (y < 63) {
                    /* Под водой - вода */
                    block_id = 9;
                } else {
                    /* Воздух */
                    block_id = 0;
                }
                
                chunk->blocks[lx][y][lz] = block_id;
            }
        }
    }
    
    chunk->modified = true;
}

/* Получить блок из чанка */
static uint8_t chunk_get_block(Chunk* chunk, int lx, int ly, int lz) {
    if (!chunk) return 0;
    
    if (lx < 0 || lx >= CHUNK_SIZE || 
        ly < 0 || ly >= 256 || 
        lz < 0 || lz >= CHUNK_SIZE) {
        return 0;
    }
    
    return chunk->blocks[lx][ly][lz];
}

/* Установить блок в чанке */
static void chunk_set_block(Chunk* chunk, int lx, int ly, int lz, uint8_t block_id) {
    if (!chunk) return;
    
    if (lx < 0 || lx >= CHUNK_SIZE || 
        ly < 0 || ly >= 256 || 
        lz < 0 || lz >= CHUNK_SIZE) {
        return;
    }
    
    chunk->blocks[lx][ly][lz] = block_id;
    chunk->modified = true;
}

/* Получить или создать чанк */
Chunk* chunk_get_or_create(int32_t x, int32_t z) {
    pthread_rwlock_rdlock(&server_state.chunks_lock);
    
    /* Ищем существующий чанк */
    for (int i = 0; i < server_state.loaded_chunks; i++) {
        if (server_state.chunks[i].x == x && server_state.chunks[i].z == z) {
            server_state.chunks[i].last_accessed = (uint32_t)time(NULL);
            Chunk* result = &server_state.chunks[i];
            pthread_rwlock_unlock(&server_state.chunks_lock);
            return result;
        }
    }
    
    pthread_rwlock_unlock(&server_state.chunks_lock);
    
    /* Создаём новый чанк */
    pthread_rwlock_wrlock(&server_state.chunks_lock);
    
    if (server_state.loaded_chunks >= MAX_CHUNKS_LOADED) {
        /* Выгружаем самый старый чанк */
        uint32_t oldest_time = UINT32_MAX;
        int oldest_idx = -1;
        
        for (int i = 0; i < server_state.loaded_chunks; i++) {
            if (server_state.chunks[i].last_accessed < oldest_time) {
                oldest_time = server_state.chunks[i].last_accessed;
                oldest_idx = i;
            }
        }
        
        if (oldest_idx >= 0) {
            chunk_save(&server_state.chunks[oldest_idx]);
            
            /* Сдвигаем элементы */
            for (int i = oldest_idx; i < server_state.loaded_chunks - 1; i++) {
                server_state.chunks[i] = server_state.chunks[i + 1];
            }
            server_state.loaded_chunks--;
        }
    }
    
    if (server_state.loaded_chunks < MAX_CHUNKS_LOADED) {
        Chunk* new_chunk = &server_state.chunks[server_state.loaded_chunks];
        new_chunk->x = x;
        new_chunk->z = z;
        chunk_generate(new_chunk);
        server_state.loaded_chunks++;
        
        pthread_rwlock_unlock(&server_state.chunks_lock);
        return new_chunk;
    }
    
    pthread_rwlock_unlock(&server_state.chunks_lock);
    return NULL;
}

/* Получить блок по мировым координатам */
uint8_t block_get(int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y >= 256) return 0;
    
    int32_t chunk_x = x >> 4;  /* x / 16 */
    int32_t chunk_z = z >> 4;  /* z / 16 */
    
    int lx = x & 15;  /* x % 16 */
    int lz = z & 15;  /* z % 16 */
    
    Chunk* chunk = chunk_get_or_create(chunk_x, chunk_z);
    if (!chunk) return 0;
    
    return chunk_get_block(chunk, lx, y, lz);
}

/* Установить блок по мировым координатам */
void block_set(int32_t x, int32_t y, int32_t z, uint8_t block_id) {
    if (y < 0 || y >= 256) return;
    
    int32_t chunk_x = x >> 4;
    int32_t chunk_z = z >> 4;
    
    int lx = x & 15;
    int lz = z & 15;
    
    Chunk* chunk = chunk_get_or_create(chunk_x, chunk_z);
    if (!chunk) return;
    
    chunk_set_block(chunk, lx, y, lz, block_id);
    
    /* Отправляем обновление всем игрокам в радиусе */
    pthread_rwlock_rdlock(&server_state.players_lock);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (server_state.players[i].socket <= 0) continue;
        
        double dx = x - server_state.players[i].x;
        double dy = y - server_state.players[i].y;
        double dz = z - server_state.players[i].z;
        double dist_sq = dx*dx + dy*dy + dz*dz;
        
        /* В пределах рендер-дистанции */
        int render_dist = RENDER_DISTANCE * 16;
        if (dist_sq < render_dist * render_dist) {
            packet_send_block_change(&server_state.players[i], x, y, z, block_id);
        }
    }
    
    pthread_rwlock_unlock(&server_state.players_lock);
}

/* Сохранить чанк на диск */
void chunk_save(Chunk* chunk) {
    if (!chunk || !chunk->modified) return;
    
    char filename[128];
    snprintf(filename, sizeof(filename), "world/chunk_%d_%d.dat", chunk->x, chunk->z);
    
    FILE* f = fopen(filename, "wb");
    if (!f) {
        printf("[ERROR] Не удалось открыть файл для сохранения: %s\n", filename);
        return;
    }
    
    /* Сохраняем координаты чанка */
    fwrite(&chunk->x, sizeof(int32_t), 1, f);
    fwrite(&chunk->z, sizeof(int32_t), 1, f);
    
    /* Сохраняем данные блоков */
    fwrite(chunk->blocks, sizeof(chunk->blocks), 1, f);
    
    fclose(f);
    chunk->modified = false;
    
    if (DEBUG_LOG) {
        printf("[CHUNK] Сохранён чанк: (%d, %d) -> %s\n", chunk->x, chunk->z, filename);
    }
}

/* Загрузить чанк с диска */
void chunk_load(Chunk* chunk) {
    if (!chunk) return;
    
    char filename[128];
    snprintf(filename, sizeof(filename), "world/chunk_%d_%d.dat", chunk->x, chunk->z);
    
    FILE* f = fopen(filename, "rb");
    if (!f) {
        if (DEBUG_LOG) {
            printf("[CHUNK] Файл чанка не найден, генерируем новый: (%d, %d)\n", 
                   chunk->x, chunk->z);
        }
        chunk_generate(chunk);
        return;
    }
    
    /* Загружаем координаты (для проверки) */
    int32_t x, z;
    fread(&x, sizeof(int32_t), 1, f);
    fread(&z, sizeof(int32_t), 1, f);
    
    if (x != chunk->x || z != chunk->z) {
        printf("[ERROR] Рассинхронизация координат чанка!\n");
        fclose(f);
        return;
    }
    
    /* Загружаем данные блоков */
    fread(chunk->blocks, sizeof(chunk->blocks), 1, f);
    
    fclose(f);
    chunk->modified = false;
    
    if (DEBUG_LOG) {
        printf("[CHUNK] Загружен чанк: (%d, %d) <- %s\n", chunk->x, chunk->z, filename);
    }
}

/* Очистить всё что можно выгрузить (для экономии памяти) */
void chunk_cleanup_unused() {
    pthread_rwlock_wrlock(&server_state.chunks_lock);
    
    uint32_t current_time = (uint32_t)time(NULL);
    
    for (int i = 0; i < server_state.loaded_chunks; ) {
        uint32_t time_since_access = current_time - server_state.chunks[i].last_accessed;
        
        if (time_since_access > CHUNK_UNLOAD_TIMEOUT / 1000) {
            chunk_save(&server_state.chunks[i]);
            
            /* Удаляем чанк */
            for (int j = i; j < server_state.loaded_chunks - 1; j++) {
                server_state.chunks[j] = server_state.chunks[j + 1];
            }
            server_state.loaded_chunks--;
        } else {
            i++;
        }
    }
    
    pthread_rwlock_unlock(&server_state.chunks_lock);
}
