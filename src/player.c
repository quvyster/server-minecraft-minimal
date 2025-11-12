#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "server.h"
#include "protocol.h"

/* Создать игрока */
Player* player_create(const char* username, const uint8_t* uuid) {
    Player* player = malloc(sizeof(Player));
    if (!player) return NULL;
    
    memset(player, 0, sizeof(Player));
    strncpy(player->username, username, sizeof(player->username) - 1);
    
    if (uuid) {
        memcpy(player->uuid, uuid, 16);
    }
    
    /* Спаун позиция */
    player->x = SPAWN_X + 0.5;
    player->y = SPAWN_Y;
    player->z = SPAWN_Z + 0.5;
    player->yaw = 0.0f;
    player->pitch = 0.0f;
    
    /* Здоровье */
    player->health = 20;
    player->experience = 0.0f;
    player->level = 0;
    
    /* Инвентарь */
    player->selected_slot = 0;
    for (int i = 0; i < MAX_INVENTORY_SIZE; i++) {
        player->inventory[i] = 0;
    }
    
    /* Временные отметки */
    player->last_keep_alive = time(NULL) * 1000;
    player->join_time = time(NULL) * 1000;
    
    /* Флаги */
    player->spawn_position_sent = false;
    player->ready = false;
    player->on_ground = true;
    
    printf("[PLAYER] Создан игрок: %s\n", username);
    
    return player;
}

/* Удалить игрока */
void player_destroy(Player* player) {
    if (!player) return;
    
    if (player->socket > 0) {
        close(player->socket);
        player->socket = 0;
    }
    
    printf("[PLAYER] Удалён игрок: %s (ID=%d)\n", player->username, player->entity_id);
}

/* Отправить позицию игрока остальным */
void player_broadcast_position(Player* player) {
    if (!player || !player->ready) return;
    
    pthread_rwlock_rdlock(&server_state.players_lock);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Player* target = &server_state.players[i];
        
        if (target->socket <= 0 || !target->ready || target->entity_id == player->entity_id) {
            continue;
        }
        
        /* Проверяем расстояние */
        double dx = player->x - target->x;
        double dz = player->z - target->z;
        double distance_sq = dx * dx + dz * dz;
        
        /* Рендер дистанция в квадрате */
        int render_range = RENDER_DISTANCE * 16;
        int render_range_sq = render_range * render_range;
        
        if (distance_sq > render_range_sq) {
            continue;
        }
        
        /* Отправляем обновление позиции */
        packet_send_entity_move_relative(target, player);
    }
    
    pthread_rwlock_unlock(&server_state.players_lock);
}

/* Установить позицию игрока */
void player_set_position(Player* player, double x, double y, double z, 
                        float yaw, float pitch) {
    if (!player) return;
    
    player->x = x;
    player->y = y;
    player->z = z;
    player->yaw = yaw;
    player->pitch = pitch;
}

/* Отправить чанк игроку */
void player_send_chunk(Player* player, int32_t chunk_x, int32_t chunk_z) {
    if (!player || !player->ready) return;
    
    pthread_rwlock_rdlock(&server_state.chunks_lock);
    
    /* Ищем чанк в памяти */
    Chunk* chunk = NULL;
    for (int i = 0; i < server_state.loaded_chunks; i++) {
        if (server_state.chunks[i].x == chunk_x && 
            server_state.chunks[i].z == chunk_z) {
            chunk = &server_state.chunks[i];
            break;
        }
    }
    
    pthread_rwlock_unlock(&server_state.chunks_lock);
    
    if (!chunk) {
        /* Генерируем чанк если его нет */
        pthread_rwlock_wrlock(&server_state.chunks_lock);
        
        if (server_state.loaded_chunks < MAX_CHUNKS_LOADED) {
            chunk = &server_state.chunks[server_state.loaded_chunks];
            chunk->x = chunk_x;
            chunk->z = chunk_z;
            chunk_generate(chunk);
            server_state.loaded_chunks++;
        }
        
        pthread_rwlock_unlock(&server_state.chunks_lock);
    }
    
    if (chunk) {
        packet_send_chunk_data(player, chunk);
    }
}

/* Загрузить чанки вокруг игрока */
void player_load_chunks_around(Player* player) {
    if (!player) return;
    
    int center_chunk_x = (int)player->x >> 4;
    int center_chunk_z = (int)player->z >> 4;
    
    int range = RENDER_DISTANCE;
    
    for (int cx = center_chunk_x - range; cx <= center_chunk_x + range; cx++) {
        for (int cz = center_chunk_z - range; cz <= center_chunk_z + range; cz++) {
            player_send_chunk(player, cx, cz);
        }
    }
}

/* Обновить список видимых игроков */
void player_update_visible_entities(Player* player) {
    if (!player || !player->ready) return;
    
    pthread_rwlock_rdlock(&server_state.players_lock);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Player* other = &server_state.players[i];
        
        if (other->socket <= 0 || !other->ready || other->entity_id == player->entity_id) {
            continue;
        }
        
        /* Проверяем расстояние */
        double dx = other->x - player->x;
        double dz = other->z - player->z;
        double distance_sq = dx * dx + dz * dz;
        
        int render_range = RENDER_DISTANCE * 16;
        int render_range_sq = render_range * render_range;
        
        if (distance_sq < render_range_sq) {
            packet_send_entity_spawn(player, other);
        }
    }
    
    pthread_rwlock_unlock(&server_state.players_lock);
}

/* Получить здоровье игрока */
int32_t player_get_health(Player* player) {
    if (!player) return 0;
    return player->health;
}

/* Установить здоровье игрока */
void player_set_health(Player* player, int32_t health) {
    if (!player) return;
    
    if (health < 0) health = 0;
    if (health > 20) health = 20;
    
    player->health = health;
    
    /* TODO: отправить обновление здоровья клиенту */
    if (health <= 0) {
        /* Игрок мёртв */
        printf("[PLAYER] Игрок %s умер\n", player->username);
    }
}

/* Найти игрока по имени */
Player* player_find_by_name(const char* username) {
    pthread_rwlock_rdlock(&server_state.players_lock);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (server_state.players[i].socket > 0 && 
            strcmp(server_state.players[i].username, username) == 0) {
            
            Player* found = &server_state.players[i];
            pthread_rwlock_unlock(&server_state.players_lock);
            return found;
        }
    }
    
    pthread_rwlock_unlock(&server_state.players_lock);
    return NULL;
}

/* Найти игрока по сокету */
Player* player_find_by_socket(int socket) {
    pthread_rwlock_rdlock(&server_state.players_lock);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (server_state.players[i].socket == socket) {
            Player* found = &server_state.players[i];
            pthread_rwlock_unlock(&server_state.players_lock);
            return found;
        }
    }
    
    pthread_rwlock_unlock(&server_state.players_lock);
    return NULL;
}

/* Отправить сообщение игроку в чат */
void player_send_chat_message(Player* player, const char* message) {
    if (!player || !player->ready) return;
    
    printf("[CHAT] <%s> %s\n", player->username, message);
    
    /* TODO: реализовать отправку пакета чата */
}

/* Отправить сообщение всем игрокам */
void server_broadcast_chat(const char* message) {
    pthread_rwlock_rdlock(&server_state.players_lock);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (server_state.players[i].socket > 0 && server_state.players[i].ready) {
            player_send_chat_message(&server_state.players[i], message);
        }
    }
    
    pthread_rwlock_unlock(&server_state.players_lock);
}
