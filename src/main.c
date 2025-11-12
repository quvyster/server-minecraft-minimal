#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "globals.h"
#include "server.h"
#include "protocol.h"

/* Глобальное состояние */
ServerState server_state = {0};
static volatile bool should_exit = false;

/* Обработчик сигналов */
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n[SERVER] Получен сигнал завершения...\n");
        should_exit = true;
        server_state.running = false;
    }
}

/* Функция сетевого потока */
void* network_thread_func(void* arg) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket;
    Player* player;
    
    printf("[NETWORK] Сетевой поток запущен\n");
    
    while (server_state.running && !should_exit) {
        /* Принимаем подключение с таймаутом */
        client_socket = accept(server_state.server_socket, 
                              (struct sockaddr*)&client_addr, 
                              &client_addr_len);
        
        if (client_socket < 0) {
            if (!server_state.running) break;
            usleep(10000);  /* 10мс */
            continue;
        }
        
        /* Проверяем лимит игроков */
        pthread_rwlock_rdlock(&server_state.players_lock);
        if (server_state.active_players >= MAX_PLAYERS) {
            pthread_rwlock_unlock(&server_state.players_lock);
            
            const char* kick_msg = "§cСервер переполнен";
            close(client_socket);
            continue;
        }
        pthread_rwlock_unlock(&server_state.players_lock);
        
        /* Находим свободное место для игрока */
        pthread_rwlock_wrlock(&server_state.players_lock);
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (server_state.players[i].socket == 0) {
                player = &server_state.players[i];
                player->socket = client_socket;
                player->entity_id = i;
                player->protocol_state = 0;  /* Handshake */
                strncpy(player->ip, inet_ntoa(client_addr.sin_addr), 15);
                player->port = ntohs(client_addr.sin_port);
                player->health = 20;
                player->join_time = time(NULL);
                server_state.active_players++;
                
                printf("[NETWORK] Новый клиент подключился: %s:%d (ID=%d, всего=%d)\n",
                       player->ip, player->port, player->entity_id, 
                       server_state.active_players);
                break;
            }
        }
        pthread_rwlock_unlock(&server_state.players_lock);
    }
    
    printf("[NETWORK] Сетевой поток завершился\n");
    return NULL;
}

/* Функция игрового цикла */
void* tick_thread_func(void* arg) {
    uint64_t last_tick = 0;
    uint64_t tick_start, tick_end, tick_delta;
    int lag_frames = 0;
    
    printf("[TICK] Игровой цикл запущен (TPS=%d)\n", TICK_RATE);
    
    while (server_state.running && !should_exit) {
        tick_start = (uint64_t)time(NULL) * 1000 + clock() / (CLOCKS_PER_SEC / 1000);
        
        /* === ОСНОВНОЙ ИГРОВОЙ ТИК === */
        server_state.current_tick++;
        
        /* Обновляем мобов (реже для экономии) */
        if (server_state.current_tick % MOB_AI_TICKS == 0) {
            /* Обновление AI мобов */
        }
        
        /* Обновляем редстоун (реже) */
        if (server_state.current_tick % REDSTONE_UPDATE_INTERVAL == 0) {
            /* Обновление редстоуна */
        }
        
        /* Обновляем жидкости (реже) */
        if (server_state.current_tick % FLUID_UPDATE_TICKS == 0) {
            /* Обновление текучести */
        }
        
        /* Обновляем сущности */
        if (server_state.current_tick % ENTITY_UPDATE_RATE == 0) {
            pthread_rwlock_rdlock(&server_state.players_lock);
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (server_state.players[i].socket > 0 && server_state.players[i].ready) {
                    /* Обновляем позицию игрока для остальных */
                    player_broadcast_position(&server_state.players[i]);
                }
            }
            pthread_rwlock_unlock(&server_state.players_lock);
        }
        
        /* Keep-alive для игроков */
        if (server_state.current_tick % 600 == 0) {  /* каждые 30 сек */
            pthread_rwlock_rdlock(&server_state.players_lock);
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (server_state.players[i].socket > 0 && server_state.players[i].ready) {
                    packet_send_keep_alive(&server_state.players[i], server_state.current_tick);
                }
            }
            pthread_rwlock_unlock(&server_state.players_lock);
        }
        
        /* Периодическое сохранение мира */
        if (server_state.current_tick % (SAVE_INTERVAL / TIME_BETWEEN_TICKS) == 0) {
            server_save_world();
        }
        
        /* === СИНХРОНИЗАЦИЯ ТИКОВ === */
        tick_end = (uint64_t)time(NULL) * 1000 + clock() / (CLOCKS_PER_SEC / 1000);
        tick_delta = tick_end - tick_start;
        
        if (tick_delta < TIME_BETWEEN_TICKS) {
            usleep((TIME_BETWEEN_TICKS - tick_delta) * 1000);
        } else {
            lag_frames++;
            if (lag_frames > 5) {
                server_state.ticks_with_lag++;
            }
        }
        
        /* Логирование информации каждые 20 сек */
        if (server_state.current_tick % 400 == 0) {
            printf("[TICK] Тик #%u | Игроков: %d | Лаг-фреймов: %d\n",
                   server_state.current_tick, server_state.active_players, 
                   server_state.ticks_with_lag);
        }
    }
    
    printf("[TICK] Игровой цикл завершился\n");
    return NULL;
}

/* Инициализация сервера */
bool server_init() {
    struct sockaddr_in server_addr;
    int opt = 1;
    
    printf("[SERVER] Инициализация сервера...\n");
    printf("[SERVER] Максимум игроков: %d\n", MAX_PLAYERS);
    printf("[SERVER] Частота тиков: %d TPS\n", TICK_RATE);
    printf("[SERVER] Дистанция рендера: %d блоков\n", RENDER_DISTANCE);
    
    /* Инициализируем структуру состояния */
    memset(&server_state, 0, sizeof(ServerState));
    server_state.running = true;
    server_state.current_tick = 0;
    pthread_rwlock_init(&server_state.players_lock, NULL);
    pthread_rwlock_init(&server_state.chunks_lock, NULL);
    
    /* Создаём сокет */
    server_state.server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_state.server_socket < 0) {
        perror("[ERROR] Не удалось создать сокет");
        return false;
    }
    
    /* Устанавливаем опции сокета */
    if (setsockopt(server_state.server_socket, SOL_SOCKET, SO_REUSEADDR, 
                   &opt, sizeof(opt)) < 0) {
        perror("[ERROR] setsockopt failed");
        return false;
    }
    
    /* Привязываем сокет */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (bind(server_state.server_socket, (struct sockaddr*)&server_addr, 
             sizeof(server_addr)) < 0) {
        perror("[ERROR] bind failed");
        return false;
    }
    
    /* Начинаем слушать */
    if (listen(server_state.server_socket, 128) < 0) {
        perror("[ERROR] listen failed");
        return false;
    }
    
    printf("[SERVER] Сервер слушает на порту %d\n", SERVER_PORT);
    
    /* Инициализируем игроков */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        server_state.players[i].socket = 0;
        server_state.players[i].entity_id = i;
    }
    
    /* Инициализируем чанки */
    server_state.chunks = malloc(sizeof(Chunk) * MAX_CHUNKS_LOADED);
    if (!server_state.chunks) {
        printf("[ERROR] Не удалось выделить память для чанков\n");
        return false;
    }
    server_state.loaded_chunks = 0;
    
    /* Создаём потоки */
    if (pthread_create(&server_state.tick_thread, NULL, tick_thread_func, NULL) != 0) {
        perror("[ERROR] Не удалось создать tick thread");
        return false;
    }
    
    if (pthread_create(&server_state.network_thread, NULL, network_thread_func, NULL) != 0) {
        perror("[ERROR] Не удалось создать network thread");
        return false;
    }
    
    printf("[SERVER] Сервер успешно инициализирован\n");
    return true;
}

/* Завершение сервера */
void server_shutdown() {
    printf("[SERVER] Завершение работы сервера...\n");
    
    server_state.running = false;
    
    /* Закрываем все соединения игроков */
    pthread_rwlock_wrlock(&server_state.players_lock);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (server_state.players[i].socket > 0) {
            close(server_state.players[i].socket);
        }
    }
    pthread_rwlock_unlock(&server_state.players_lock);
    
    /* Сохраняем мир */
    server_save_world();
    
    /* Закрываем сокет сервера */
    if (server_state.server_socket > 0) {
        close(server_state.server_socket);
    }
    
    /* Ждём потоков */
    pthread_join(server_state.tick_thread, NULL);
    pthread_join(server_state.network_thread, NULL);
    
    /* Освобождаем память */
    if (server_state.chunks) {
        free(server_state.chunks);
    }
    
    pthread_rwlock_destroy(&server_state.players_lock);
    pthread_rwlock_destroy(&server_state.chunks_lock);
    
    printf("[SERVER] Сервер остановлен\n");
}

/* === ОСНОВНАЯ ПРОГРАММА === */
int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║  Optimized Minecraft Server v1.0       ║\n");
    printf("║  Protoc ол 772 (1.21.8)                ║\n");
    printf("║  Для слабого железа (2GB RAM)          ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    if (!server_init()) {
        printf("[FATAL] Не удалось инициализировать сервер\n");
        return 1;
    }
    
    /* Главный цикл */
    while (!should_exit) {
        sleep(1);
    }
    
    server_shutdown();
    printf("[DONE] Выход\n");
    return 0;
}
