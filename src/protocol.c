#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "protocol.h"
#include "server.h"

/* === БУФЕР ПАКЕТОВ === */

PacketBuffer* buffer_create(size_t initial_size) {
    PacketBuffer* buf = malloc(sizeof(PacketBuffer));
    if (!buf) return NULL;
    
    buf->data = malloc(initial_size);
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    
    buf->size = initial_size;
    buf->position = 0;
    
    return buf;
}

void buffer_free(PacketBuffer* buf) {
    if (!buf) return;
    if (buf->data) free(buf->data);
    free(buf);
}

/* VarInt кодирование */
static void buffer_write_varint(PacketBuffer* buf, int32_t value) {
    while ((value & 0xFFFFFF80) != 0) {
        if (buf->position >= buf->size - 1) {
            buf->size *= 2;
            buf->data = realloc(buf->data, buf->size);
        }
        buf->data[buf->position++] = (uint8_t)((value & 0x7F) | 0x80);
        value >>= 7;
    }
    if (buf->position >= buf->size) {
        buf->size *= 2;
        buf->data = realloc(buf->data, buf->size);
    }
    buf->data[buf->position++] = (uint8_t)(value & 0x7F);
}

static int32_t buffer_read_varint(PacketBuffer* buf) {
    int32_t result = 0;
    int shift = 0;
    
    while (buf->position < buf->size) {
        uint8_t byte = buf->data[buf->position++];
        result |= (int32_t)(byte & 0x7F) << shift;
        
        if ((byte & 0x80) == 0) {
            return result;
        }
        shift += 7;
    }
    
    return 0;
}

void buffer_write_byte(PacketBuffer* buf, uint8_t value) {
    if (buf->position >= buf->size) {
        buf->size *= 2;
        buf->data = realloc(buf->data, buf->size);
    }
    buf->data[buf->position++] = value;
}

void buffer_write_short(PacketBuffer* buf, int16_t value) {
    if (buf->position + 2 > buf->size) {
        buf->size *= 2;
        buf->data = realloc(buf->data, buf->size);
    }
    *(int16_t*)&buf->data[buf->position] = htons(value);
    buf->position += 2;
}

void buffer_write_int(PacketBuffer* buf, int32_t value) {
    if (buf->position + 4 > buf->size) {
        buf->size *= 2;
        buf->data = realloc(buf->data, buf->size);
    }
    *(int32_t*)&buf->data[buf->position] = htonl(value);
    buf->position += 4;
}

void buffer_write_long(PacketBuffer* buf, int64_t value) {
    if (buf->position + 8 > buf->size) {
        buf->size *= 2;
        buf->data = realloc(buf->data, buf->size);
    }
    uint32_t high = htonl((uint32_t)(value >> 32));
    uint32_t low = htonl((uint32_t)(value & 0xFFFFFFFF));
    *(uint32_t*)&buf->data[buf->position] = high;
    *(uint32_t*)&buf->data[buf->position + 4] = low;
    buf->position += 8;
}

void buffer_write_float(PacketBuffer* buf, float value) {
    if (buf->position + 4 > buf->size) {
        buf->size *= 2;
        buf->data = realloc(buf->data, buf->size);
    }
    uint32_t bits;
    memcpy(&bits, &value, 4);
    *(uint32_t*)&buf->data[buf->position] = htonl(bits);
    buf->position += 4;
}

void buffer_write_double(PacketBuffer* buf, double value) {
    if (buf->position + 8 > buf->size) {
        buf->size *= 2;
        buf->data = realloc(buf->data, buf->size);
    }
    uint64_t bits;
    memcpy(&bits, &value, 8);
    uint32_t high = htonl((uint32_t)(bits >> 32));
    uint32_t low = htonl((uint32_t)(bits & 0xFFFFFFFF));
    *(uint32_t*)&buf->data[buf->position] = high;
    *(uint32_t*)&buf->data[buf->position + 4] = low;
    buf->position += 8;
}

void buffer_write_string(PacketBuffer* buf, const char* str) {
    if (!str) str = "";
    size_t len = strlen(str);
    
    buffer_write_varint(buf, (int32_t)len);
    
    if (buf->position + len > buf->size) {
        buf->size = buf->position + len + 256;
        buf->data = realloc(buf->data, buf->size);
    }
    
    memcpy(&buf->data[buf->position], str, len);
    buf->position += len;
}

void buffer_write_uuid(PacketBuffer* buf, const uint8_t* uuid) {
    if (buf->position + 16 > buf->size) {
        buf->size *= 2;
        buf->data = realloc(buf->data, buf->size);
    }
    memcpy(&buf->data[buf->position], uuid, 16);
    buf->position += 16;
}

void buffer_write_position(PacketBuffer* buf, int32_t x, int32_t y, int32_t z) {
    int64_t value = ((int64_t)x & 0x3FFFFFF) << 38;
    value |= ((int64_t)z & 0x3FFFFFF) << 12;
    value |= ((int64_t)y & 0xFFF);
    buffer_write_long(buf, value);
}

/* Чтение */
uint8_t buffer_read_byte(PacketBuffer* buf) {
    if (buf->position >= buf->size) return 0;
    return buf->data[buf->position++];
}

int16_t buffer_read_short(PacketBuffer* buf) {
    if (buf->position + 2 > buf->size) return 0;
    int16_t value = ntohs(*(int16_t*)&buf->data[buf->position]);
    buf->position += 2;
    return value;
}

int32_t buffer_read_int(PacketBuffer* buf) {
    if (buf->position + 4 > buf->size) return 0;
    int32_t value = ntohl(*(int32_t*)&buf->data[buf->position]);
    buf->position += 4;
    return value;
}

int64_t buffer_read_long(PacketBuffer* buf) {
    if (buf->position + 8 > buf->size) return 0;
    uint32_t high = ntohl(*(uint32_t*)&buf->data[buf->position]);
    uint32_t low = ntohl(*(uint32_t*)&buf->data[buf->position + 4]);
    buf->position += 8;
    return ((int64_t)high << 32) | low;
}

float buffer_read_float(PacketBuffer* buf) {
    if (buf->position + 4 > buf->size) return 0.0f;
    uint32_t bits = ntohl(*(uint32_t*)&buf->data[buf->position]);
    buf->position += 4;
    float value;
    memcpy(&value, &bits, 4);
    return value;
}

double buffer_read_double(PacketBuffer* buf) {
    if (buf->position + 8 > buf->size) return 0.0;
    uint32_t high = ntohl(*(uint32_t*)&buf->data[buf->position]);
    uint32_t low = ntohl(*(uint32_t*)&buf->data[buf->position + 4]);
    buf->position += 8;
    uint64_t bits = ((uint64_t)high << 32) | low;
    double value;
    memcpy(&value, &bits, 8);
    return value;
}

char* buffer_read_string(PacketBuffer* buf, size_t max_len) {
    int32_t len = buffer_read_varint(buf);
    if (len < 0 || len > (int32_t)max_len) {
        return NULL;
    }
    
    char* str = malloc(len + 1);
    if (!str) return NULL;
    
    memcpy(str, &buf->data[buf->position], len);
    buf->position += len;
    str[len] = '\0';
    
    return str;
}

void buffer_read_uuid(PacketBuffer* buf, uint8_t* uuid) {
    if (buf->position + 16 > buf->size) return;
    memcpy(uuid, &buf->data[buf->position], 16);
    buf->position += 16;
}

/* === ОТПРАВКА ПАКЕТОВ === */

static void send_packet(Player* player, int32_t packet_id, PacketBuffer* payload) {
    if (!player || player->socket <= 0 || !payload) return;
    
    PacketBuffer* packet = buffer_create(payload->position + 16);
    
    /* Пишем ID пакета */
    buffer_write_varint(packet, packet_id);
    
    /* Копируем payload */
    memcpy(&packet->data[packet->position], payload->data, payload->position);
    packet->position += payload->position;
    
    /* Пишем длину пакета в начало */
    PacketBuffer* final_packet = buffer_create(packet->position + 8);
    buffer_write_varint(final_packet, (int32_t)packet->position);
    memcpy(&final_packet->data[final_packet->position], packet->data, packet->position);
    final_packet->position += packet->position;
    
    /* Отправляем */
    ssize_t sent = send(player->socket, final_packet->data, final_packet->position, 0);
    if (sent < 0) {
        printf("[WARNING] Не удалось отправить пакет игроку %s\n", player->username);
    }
    
    buffer_free(packet);
    buffer_free(final_packet);
}

void packet_send_login_success(Player* player) {
    if (!player) return;
    
    PacketBuffer* payload = buffer_create(256);
    
    /* UUID */
    buffer_write_uuid(payload, player->uuid);
    
    /* Username */
    buffer_write_string(payload, player->username);
    
    /* Properties (0 свойств) */
    buffer_write_varint(payload, 0);
    
    send_packet(player, 0x02, payload);  /* Login Success packet */
    buffer_free(payload);
    
    printf("[PROTOCOL] Игрок %s успешно залогинился\n", player->username);
}

void packet_send_spawn_position(Player* player) {
    if (!player) return;
    
    PacketBuffer* payload = buffer_create(32);
    
    /* Позиция спауна */
    buffer_write_position(payload, SPAWN_X, SPAWN_Y, SPAWN_Z);
    
    /* Angle (не используется) */
    buffer_write_float(payload, 0.0f);
    
    send_packet(player, 0x4C, payload);  /* Spawn Position пакет */
    buffer_free(payload);
}

void packet_send_player_position_and_look(Player* player) {
    if (!player) return;
    
    PacketBuffer* payload = buffer_create(64);
    
    /* X, Y, Z */
    buffer_write_double(payload, player->x);
    buffer_write_double(payload, player->y);
    buffer_write_double(payload, player->z);
    
    /* Yaw, Pitch */
    buffer_write_float(payload, player->yaw);
    buffer_write_float(payload, player->pitch);
    
    /* Flags (0 = absolute positioning) */
    buffer_write_byte(payload, 0x00);
    
    /* Teleport ID */
    buffer_write_varint(payload, 0);
    
    /* Dismount vehicle */
    buffer_write_byte(payload, 0);
    
    send_packet(player, 0x3A, payload);  /* Player Position and Rotation */
    buffer_free(payload);
}

void packet_send_chunk_data(Player* player, Chunk* chunk) {
    if (!player || !chunk) return;
    
    /* Упрощённо: отправляем только основную информацию о чанке */
    PacketBuffer* payload = buffer_create(8192);
    
    /* Координаты чанка */
    buffer_write_int(payload, chunk->x);
    buffer_write_int(payload, chunk->z);
    
    /* Data structure (упрощённо - отправляем сырые данные) */
    buffer_write_varint(payload, 1);  /* primary bit mask */
    buffer_write_varint(payload, 0);  /* heightmap count */
    buffer_write_varint(payload, 0);  /* biome data length */
    buffer_write_varint(payload, sizeof(chunk->blocks));  /* data length */
    
    /* Отправляем данные блоков */
    memcpy(&payload->data[payload->position], chunk->blocks, sizeof(chunk->blocks));
    payload->position += sizeof(chunk->blocks);
    
    buffer_write_varint(payload, 0);  /* block entities count */
    
    send_packet(player, 0x21, payload);  /* Chunk Data */
    buffer_free(payload);
}

void packet_send_block_change(Player* player, int32_t x, int32_t y, int32_t z, uint8_t block_id) {
    if (!player) return;
    
    PacketBuffer* payload = buffer_create(32);
    
    buffer_write_position(payload, x, y, z);
    buffer_write_varint(payload, block_id);
    
    send_packet(player, 0x09, payload);  /* Block Change */
    buffer_free(payload);
}

void packet_send_entity_spawn(Player* player, Player* entity) {
    if (!player || !entity) return;
    
    PacketBuffer* payload = buffer_create(64);
    
    /* Entity ID */
    buffer_write_varint(payload, entity->entity_id);
    
    /* UUID */
    buffer_write_uuid(payload, entity->uuid);
    
    /* Type (PLAYER = 0x0F) */
    buffer_write_varint(payload, 0x0F);
    
    /* X, Y, Z (в 1/4096 блока) */
    buffer_write_double(payload, entity->x);
    buffer_write_double(payload, entity->y);
    buffer_write_double(payload, entity->z);
    
    /* Pitch, Yaw */
    buffer_write_byte(payload, (uint8_t)(entity->pitch * 256 / 360));
    buffer_write_byte(payload, (uint8_t)(entity->yaw * 256 / 360));
    
    /* Head Yaw */
    buffer_write_byte(payload, (uint8_t)(entity->yaw * 256 / 360));
    
    /* Velocity */
    buffer_write_short(payload, 0);
    buffer_write_short(payload, 0);
    buffer_write_short(payload, 0);
    
    send_packet(player, 0x00, payload);  /* Spawn Player */
    buffer_free(payload);
}

void packet_send_entity_destroy(Player* player, int32_t entity_id) {
    if (!player) return;
    
    PacketBuffer* payload = buffer_create(16);
    
    /* Count */
    buffer_write_varint(payload, 1);
    
    /* Entity ID */
    buffer_write_varint(payload, entity_id);
    
    send_packet(player, 0x3A, payload);  /* Destroy Entities */
    buffer_free(payload);
}

void packet_send_entity_move_relative(Player* player, Player* entity) {
    if (!player || !entity) return;
    
    PacketBuffer* payload = buffer_create(32);
    
    /* Entity ID */
    buffer_write_varint(payload, entity->entity_id);
    
    /* Delta X, Y, Z (в 1/4096 блока, заполнитель) */
    buffer_write_short(payload, (int16_t)(entity->x * 4096));
    buffer_write_short(payload, (int16_t)(entity->y * 4096));
    buffer_write_short(payload, (int16_t)(entity->z * 4096));
    
    /* On Ground */
    buffer_write_byte(payload, entity->on_ground ? 1 : 0);
    
    send_packet(player, 0x2A, payload);  /* Entity Position */
    buffer_free(payload);
}

void packet_send_keep_alive(Player* player, int64_t keep_alive_id) {
    if (!player) return;
    
    PacketBuffer* payload = buffer_create(16);
    buffer_write_long(payload, keep_alive_id);
    
    send_packet(player, 0x21, payload);  /* Keep Alive */
    buffer_free(payload);
}

void packet_send_disconnect(Player* player, const char* reason) {
    if (!player) return;
    
    PacketBuffer* payload = buffer_create(256);
    
    /* JSON chat */
    char json[512];
    snprintf(json, sizeof(json), "{\"text\":\"%s\"}", reason);
    buffer_write_string(payload, json);
    
    send_packet(player, 0x17, payload);  /* Disconnect */
    buffer_free(payload);
}

/* === ОБРАБОТКА ПАКЕТОВ === */

void protocol_handshake(Player* player, PacketBuffer* buf) {
    if (!player || !buf) return;
    
    int32_t protocol_version = buffer_read_varint(buf);
    char* server_address = buffer_read_string(buf, 256);
    int16_t server_port = buffer_read_short(buf);
    int32_t next_state = buffer_read_varint(buf);
    
    printf("[PROTOCOL] Handshake: version=%d, state=%d\n", protocol_version, next_state);
    
    player->protocol_state = next_state;
    
    if (server_address) free(server_address);
}

void protocol_login_start(Player* player, PacketBuffer* buf) {
    if (!player || !buf) return;
    
    char* username = buffer_read_string(buf, 16);
    if (!username) return;
    
    strncpy(player->username, username, sizeof(player->username) - 1);
    player->protocol_state = 3;  /* Play state */
    player->ready = true;
    
    printf("[PROTOCOL] Login Start: %s\n", username);
    
    /* Отправляем успех логина */
    packet_send_login_success(player);
    
    /* Отправляем спаун позицию и позицию игрока */
    packet_send_spawn_position(player);
    packet_send_player_position_and_look(player);
    
    /* Загружаем чанки вокруг игрока */
    player_load_chunks_around(player);
    player_update_visible_entities(player);
    
    free(username);
}

void protocol_play_position_and_rotation(Player* player, PacketBuffer* buf) {
    if (!player || !buf) return;
    
    double x = buffer_read_double(buf);
    double y = buffer_read_double(buf);
    double z = buffer_read_double(buf);
    float yaw = buffer_read_float(buf);
    float pitch = buffer_read_float(buf);
    uint8_t on_ground = buffer_read_byte(buf);
    
    player_set_position(player, x, y, z, yaw, pitch);
    player->on_ground = on_ground != 0;
}

void protocol_play_block_place(Player* player, PacketBuffer* buf) {
    if (!player || !buf) return;
    
    /* Упрощённо - не реализуем полностью */
    printf("[PROTOCOL] Block Place от %s\n", player->username);
}

void protocol_play_block_dig(Player* player, PacketBuffer* buf) {
    if (!player || !buf) return;
    
    uint8_t status = buffer_read_byte(buf);
    int32_t x = buffer_read_int(buf);
    uint8_t y = buffer_read_byte(buf);
    int32_t z = buffer_read_int(buf);
    
    if (status == 2) {  /* Destroy block */
        block_set(x, y, z, 0);  /* Удаляем блок */
        printf("[PROTOCOL] Block Dig: %d,%d,%d удалён\n", x, y, z);
    }
}
