# API Examples - Примеры использования сервера

## Подключение игрока

```c
#include "server.h"
#include "protocol.h"

// Игрок автоматически создаётся при подключении
// В main.c функция network_thread_func обрабатывает входящие соединения
```

## Получение информации о игроке

```c
#include "server.h"

// Найти игрока по имени
Player* player = player_find_by_name("username");
if (player) {
    printf("Найден игрок: %s\n", player->username);
    printf("Позиция: (%.1f, %.1f, %.1f)\n", player->x, player->y, player->z);
    printf("Здоровье: %d/20\n", player->health);
}

// Найти игрока по сокету
Player* player = player_find_by_socket(client_socket);

// Получить список всех игроков
Player* online_players[MAX_PLAYERS];
int count;
get_online_players(online_players, &count);
printf("Онлайн: %d игроков\n", count);
```

## Работа с позициями

```c
#include "server.h"
#include "utils.h"

// Установить позицию игрока
player_set_position(player, 100.5, 64.0, 200.5, 0.0f, 0.0f);

// Телепортировать игрока
teleport_player(player, SPAWN_X, SPAWN_Y, SPAWN_Z);

// Вычислить расстояние между двумя игроками
double dist = distance_3d(player1->x, player1->y, player1->z,
                         player2->x, player2->y, player2->z);
printf("Расстояние: %.1f блоков\n", dist);
```

## Работа с блоками

```c
#include "server.h"
#include "limits.h"

// Получить блок по координатам
uint8_t block_type = block_get(100, 64, 200);

// Установить блок
block_set(100, 65, 200, BLOCK_STONE);
block_set(101, 65, 200, BLOCK_GRASS);
block_set(102, 65, 200, BLOCK_DIRT);

// Удалить блок (установить AIR)
block_set(100, 65, 200, BLOCK_AIR);

// Проверить тип блока
if (block_type == BLOCK_WATER) {
    printf("Это вода!\n");
}
```

## Работа с чанками

```c
#include "server.h"

// Получить или создать чанк
Chunk* chunk = chunk_get_or_create(10, 20);

// Сохранить чанк
chunk_save(chunk);

// Загрузить чанк
chunk_load(chunk);

// Отправить чанк игроку
player_send_chunk(player, 10, 20);

// Загрузить все чанки вокруг игрока
player_load_chunks_around(player);
```

## Здоровье и инвентарь

```c
#include "server.h"

// Установить здоровье
player_set_health(player, 10);  // 10 сердец

// Получить здоровье
int health = player_get_health(player);

// Дать предмет
give_item_to_player(player, ITEM_DIAMOND, 1);

// Очистить инвентарь
for (int i = 0; i < MAX_INVENTORY_SIZE; i++) {
    player->inventory[i] = 0;
}
```

## Сообщения в чат

```c
#include "server.h"

// Отправить сообщение одному игроку
player_send_chat_message(player, "Привет!");

// Отправить сообщение всем
server_broadcast_chat("§6[Сервер] Сервер переходит на обслуживание!");
```

## Вычисления расстояний

```c
#include "utils.h"

// 2D расстояние (только X и Z)
double dist_2d = distance_2d(x1, z1, x2, z2);

// 3D расстояние (с Y)
double dist_3d = distance_3d(x1, y1, z1, x2, y2, z2);

// Проверить видимость
int render_range = RENDER_DISTANCE * 16;
if (dist_3d < render_range) {
    printf("Игрок видим!\n");
}
```

## Работа со временем

```c
#include "utils.h"

// Получить текущее время в миллисекундах
uint64_t now_ms = get_millis();

// Получить текущее время в микросекундах
uint64_t now_us = get_micros();

// Профилирование
uint64_t start = get_millis();
// ... какая-то операция ...
uint64_t elapsed = get_millis() - start;
printf("Операция заняла %llu мс\n", elapsed);
```

## Случайные числа

```c
#include "utils.h"

// Случайное целое число от min до max
int random_num = random_int(1, 10);

// Случайное число от 0.0 до 1.0
float random_val = random_float();

// Применение - случайный спаун моба
int mob_x = random_int(spawn_x - 10, spawn_x + 10);
int mob_z = random_int(spawn_z - 10, spawn_z + 10);
```

## Работа со строками

```c
#include "utils.h"

// Дублировать строку
char* username_copy = string_duplicate("Player123");

// Обрезать пробелы
char str[] = "  hello world  ";
string_trim(str);  // "hello world"

// Проверить префикс
if (string_starts_with(message, "/")) {
    printf("Это команда!\n");
}

// Хеширование
uint32_t hash = hash_string("test");
```

## Статистика и отладка

```c
#include "server.h"
#include "utils.h"

// Показать статистику сервера
server_print_stats();

// Информация об игроке
char info[256];
get_player_info(player, info, sizeof(info));
printf("%s\n", info);

// Статистика памяти
print_memory_stats();

// Получить информацию о сервере
printf("Активные игроки: %d/%d\n", 
       server_state.active_players, MAX_PLAYERS);
printf("Загруженные чанки: %d/%d\n",
       server_state.loaded_chunks, MAX_CHUNKS_LOADED);
printf("Текущий тик: %u\n", server_state.current_tick);
```

## Расширение функциональности

### Добавить новую команду

```c
// В src/server.c добавьте:

void cmd_teleport(Player* player, const char* args) {
    double x, y, z;
    if (sscanf(args, "%lf %lf %lf", &x, &y, &z) == 3) {
        teleport_player(player, x, y, z);
        player_send_chat_message(player, "§aТелепортирован!");
    } else {
        player_send_chat_message(player, "§cИспользование: /tp <x> <y> <z>");
    }
}

// Регистрация команды в массиве commands
```

### Добавить новый тип блока

```c
// В include/limits.h добавьте:
#define BLOCK_CUSTOM_BLUE 100

// В src/chunk.c модифицируйте chunk_generate():
if (some_condition) {
    chunk->blocks[lx][y][lz] = BLOCK_CUSTOM_BLUE;
}
```

### Добавить обработчик пакетов

```c
// В src/protocol.c добавьте:

void protocol_custom_packet(Player* player, PacketBuffer* buf) {
    int32_t custom_id = buffer_read_varint(buf);
    char* message = buffer_read_string(buf, 256);
    
    printf("[CUSTOM] Игрок %s отправил: %s\n", 
           player->username, message);
    
    free(message);
}

// Вызывайте из главного цикла обработки пакетов
```

## Производительность и оптимизация

```c
#include "utils.h"

// Оптимизированное вычисление расстояния для группы игроков
for (int i = 0; i < server_state.active_players; i++) {
    Player* p = &server_state.players[i];
    
    // Быстрое 2D расстояние (избегаем квадратного корня)
    int dx = (int)(center->x - p->x);
    int dz = (int)(center->z - p->z);
    int dist_sq = dx*dx + dz*dz;
    
    if (LIKELY(dist_sq > render_range_sq)) {
        continue;  // Пропускаем дальних игроков
    }
    
    // Полное 3D расстояние только для близких
    double full_dist = distance_3d(center->x, center->y, center->z,
                                   p->x, p->y, p->z);
}
```

## Обработка ошибок

```c
#include "server.h"

// Проверка валидности указателя
if (!player) {
    fprintf(stderr, "[ERROR] Player pointer is NULL\n");
    return;
}

// Проверка сокета
if (player->socket <= 0) {
    printf("[WARNING] Player %s disconnected\n", player->username);
    return;
}

// Проверка границ
if (y < 0 || y >= 256) {
    printf("[ERROR] Y coordinate out of bounds: %d\n", y);
    return;
}

// ASSERT для дебага
ASSERT(player != NULL, "Player cannot be NULL");
ASSERT(player->health >= 0 && player->health <= 20, 
       "Health must be 0-20");
```

---

**Совет**: Для большинства операций рекомендуется использовать макросы `LIKELY`/`UNLIKELY` и `UNUSED` для оптимизации компилятором.
