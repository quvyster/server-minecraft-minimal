#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
   ОПТИМИЗАЦИЯ ДЛЯ СЛАБОГО ЖЕЛЕЗА (2GB RAM, 1000 игроков)
   ============================================================ */

/* === СЕТЕВЫЕ ПАРАМЕТРЫ === */
#define MAX_PLAYERS 1000
#define SERVER_PORT 25565
#define TICK_RATE 20  /* тики в секунду */
#define TIME_BETWEEN_TICKS (1000 / TICK_RATE)  /* мс между тиками */

/* === ОПТИМИЗАЦИЯ ПАМЯТИ === */
#define CHUNK_SIZE 16
#define RENDER_DISTANCE 6  /* блоков от игрока */
#define MAX_CHUNKS_LOADED 512  /* максимум загруженных чанков */
#define CHUNK_UNLOAD_TIMEOUT 300000  /* мс до выгрузки неиспользуемого чанка */

/* === ОПТИМИЗАЦИЯ МОБОВ === */
#define ENABLE_MOBS 1  /* 1 = вкл, 0 = выкл */
#define MOB_LIMIT 100  /* максимум мобов на сервере */
#define MOB_SPAWN_RATE 0.3f  /* меньше = реже спауны */
#define MOB_AI_TICKS 3  /* обновлять AI каждые N тиков */

/* === ОПТИМИЗАЦИЯ РЕДСТОУНА === */
#define ENABLE_REDSTONE 1
#define REDSTONE_UPDATE_LIMIT 1000  /* макс обновлений в тик */
#define REDSTONE_UPDATE_INTERVAL 2  /* обновлять каждые N тиков */

/* === ОПТИМИЗАЦИЯ ЖИДКОСТЕЙ === */
#define ENABLE_FLUIDS 1
#define FLUID_UPDATE_TICKS 5  /* обновлять текучесть каждые N тиков */
#define FLUID_FLOW_LIMIT 500  /* макс потоков в тик */

/* === ОПТИМИЗАЦИЯ ФИЗИКИ === */
#define ENABLE_PHYSICS 1
#define ENTITY_UPDATE_RATE 2  /* обновлять сущности каждые N тиков */
#define COLLISION_SIMPLE 1  /* упрощённая коллизия */

/* === ХРАНИЛИЩЕ === */
#define ENABLE_CHESTS 1
#define CHEST_UPDATE_INTERVAL 4
#define MAX_INVENTORY_SIZE 36  /* элементов в инвентаре */

/* === СОХРАНЕНИЕ МИРА === */
#define SAVE_INTERVAL 12000  /* мс между сохранениями (60 сек) */
#define COMPRESSION_LEVEL 1  /* 1 = минимум, 9 = максимум */
#define ASYNC_SAVE 1  /* сохранять в отдельном потоке */

/* === ОПТИМИЗАЦИЯ ГЕНЕРАЦИИ === */
#define CHUNK_GEN_THREADS 2  /* потоков на генерацию чанков */
#define CHUNK_GEN_QUEUE_SIZE 64

/* === ТРАНСЛЯЦИЯ ДВИЖЕНИЙ === */
#define BROADCAST_ALL_MOVEMENT 1  /* транслировать ВСЕ движения */
#define MOVEMENT_BROADCAST_INTERVAL 1  /* каждый тик */
#define POSITION_DELTA_THRESHOLD 0.125f  /* блоков на обновление */

/* === ПАРАМЕТРЫ ИГРОКОВ === */
#define PLAYER_DESPAWN_RADIUS 256  /* блоков */
#define PLAYER_SPAWN_RADIUS 100
#define PLAYER_KEEP_ALIVE_INTERVAL 30000  /* мс */
#define PLAYER_TIMEOUT 60000  /* мс */

/* === ПРОЧЕЕ === */
#define MOTD "§6Optimized Server§r\n§7Ultra-lightweight for weak hardware"
#define SPAWN_X 0
#define SPAWN_Y 64
#define SPAWN_Z 0
#define DIFFICULTY 1  /* 0=peaceful, 1=easy, 2=normal, 3=hard */
#define PVP_ENABLED 1
#define FIRE_TICK 30  /* огонь горит дольше */

/* === DEBUG === */
#define DEBUG_LOG 0
#define DEBUG_MEMORY 0

#endif /* GLOBALS_H */
