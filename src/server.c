#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <dirent.h>
#include "server.h"
#include "protocol.h"

/* === ФУНКЦИИ СЕРВЕРА === */

void server_tick() {
    /* Вызывается каждый тик из основного цикла */
    /* Логика обновления уже в tick_thread */
}

void server_save_world() {
    pthread_rwlock_rdlock(&server_state.chunks_lock);
    
    printf("[SAVE] Сохранение мира... (чанков: %d)\n", server_state.loaded_chunks);
    
    for (int i = 0; i < server_state.loaded_chunks; i++) {
        chunk_save(&server_state.chunks[i]);
    }
    
    pthread_rwlock_unlock(&server_state.chunks_lock);
    
    printf("[SAVE] Мир сохранён\n");
}

/* === ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ === */

void server_print_stats() {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║         СТАТИСТИКА СЕРВЕРА            ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║ Тиков: %u\n", server_state.current_tick);
    printf("║ Активных игроков: %d / %d\n", server_state.active_players, MAX_PLAYERS);
    printf("║ Загруженных чанков: %d / %d\n", server_state.loaded_chunks, MAX_CHUNKS_LOADED);
    printf("║ Тиков с лагом: %d\n", server_state.ticks_with_lag);
    printf("╚════════════════════════════════════════╝\n");
}

void broadcast_chat_message(const char* username, const char* message) {
    char formatted[256];
    snprintf(formatted, sizeof(formatted), "<%s> %s", username, message);
    
    pthread_rwlock_rdlock(&server_state.players_lock);
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (server_state.players[i].socket > 0 && server_state.players[i].ready) {
            printf("[CHAT] %s\n", formatted);
        }
    }
    
    pthread_rwlock_unlock(&server_state.players_lock);
}

/* Получить список онлайн игроков */
void get_online_players(Player** out_players, int* out_count) {
    if (!out_count) return;
    *out_count = 0;
    
    pthread_rwlock_rdlock(&server_state.players_lock);
    
    for (int i = 0; i < MAX_PLAYERS && *out_count < MAX_PLAYERS; i++) {
        if (server_state.players[i].socket > 0 && server_state.players[i].ready) {
            if (out_players) {
                out_players[*out_count] = &server_state.players[i];
            }
            (*out_count)++;
        }
    }
    
    pthread_rwlock_unlock(&server_state.players_lock);
}

/* Удалить игрока из сервера */
void remove_player(Player* player) {
    if (!player) return;
    
    printf("[SERVER] Удаление игрока: %s\n", player->username);
    
    pthread_rwlock_wrlock(&server_state.players_lock);
    
    if (player->socket > 0) {
        close(player->socket);
        player->socket = 0;
    }
    
    server_state.active_players--;
    
    /* Уведомляем остальных игроков об удалении */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (server_state.players[i].socket > 0 && server_state.players[i].ready) {
            packet_send_entity_destroy(&server_state.players[i], player->entity_id);
        }
    }
    
    pthread_rwlock_unlock(&server_state.players_lock);
}

/* Телепортировать игрока */
void teleport_player(Player* player, double x, double y, double z) {
    if (!player) return;
    
    player_set_position(player, x, y, z, player->yaw, player->pitch);
    
    /* Отправляем новую позицию клиенту */
    packet_send_player_position_and_look(player);
    
    printf("[TELEPORT] %s -> (%.1f, %.1f, %.1f)\n", player->username, x, y, z);
}

/* Дать предмет игроку */
void give_item_to_player(Player* player, uint16_t item_id, uint8_t amount) {
    if (!player) return;
    
    /* Ищем свободный слот в инвентаре */
    for (int i = 0; i < MAX_INVENTORY_SIZE; i++) {
        if (player->inventory[i] == 0) {
            player->inventory[i] = item_id;
            printf("[INVENTORY] Дан предмет #%u игроку %s\n", item_id, player->username);
            return;
        }
    }
    
    printf("[INVENTORY] Инвентарь игрока %s полон!\n", player->username);
}

/* Установить здоровье игрока */
void set_player_health(Player* player, int32_t health) {
    player_set_health(player, health);
}

/* Получить информацию о игроке */
void get_player_info(Player* player, char* out_buffer, size_t buffer_size) {
    if (!player || !out_buffer) return;
    
    snprintf(out_buffer, buffer_size,
             "Игрок: %s\n"
             "  Position: (%.2f, %.2f, %.2f)\n"
             "  Health: %d/20\n"
             "  Experience: %.2f\n"
             "  Level: %d\n"
             "  Yaw: %.1f°, Pitch: %.1f°\n",
             player->username,
             player->x, player->y, player->z,
             player->health,
             player->experience,
             player->level,
             player->yaw, player->pitch);
}

/* === ИНИЦИАЛИЗАЦИЯ СОХРАНЁННОГО МИРА === */

void load_world_data() {
    printf("[WORLD] Загрузка сохранённых данных...\n");
    
    DIR* dir = opendir("world");
    if (!dir) {
        printf("[WORLD] Папка world не найдена, создаём новый мир\n");
        return;
    }
    
    struct dirent* entry;
    int chunk_count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "chunk_", 6) == 0) {
            chunk_count++;
        }
    }
    
    closedir(dir);
    
    printf("[WORLD] Найдено сохранённых чанков: %d\n", chunk_count);
}

/* === КОМАНДЫ === */

typedef void (*CommandFunc)(Player* player, const char* args);

struct {
    const char* name;
    CommandFunc func;
    const char* help;
} commands[] = {
    {"help", NULL, "Показать помощь"},
    {"ping", NULL, "Проверить пинг"},
    {"coords", NULL, "Показать координаты"},
    {NULL, NULL, NULL}
};

void execute_command(Player* player, const char* command) {
    if (!player || !command) return;
    
    printf("[COMMAND] %s выполняет: %s\n", player->username, command);
    
    /* Простой парсер команд */
    char cmd_name[64];
    const char* args = "";
    
    int space_pos = 0;
    for (int i = 0; command[i] && i < 63; i++) {
        if (command[i] == ' ') {
            space_pos = i;
            cmd_name[i] = '\0';
            args = &command[i + 1];
            break;
        }
        cmd_name[i] = command[i];
    }
    
    if (space_pos == 0) {
        strcpy(cmd_name, command);
    }
}
