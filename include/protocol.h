#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "server.h"

/* Minecraft Protocol 772 (1.21.8) */

typedef struct {
    uint8_t* data;
    size_t size;
    size_t position;
} PacketBuffer;

/* === Функции буфера пакетов === */
PacketBuffer* buffer_create(size_t initial_size);
void buffer_free(PacketBuffer* buf);
void buffer_write_byte(PacketBuffer* buf, uint8_t value);
void buffer_write_short(PacketBuffer* buf, int16_t value);
void buffer_write_int(PacketBuffer* buf, int32_t value);
void buffer_write_long(PacketBuffer* buf, int64_t value);
void buffer_write_float(PacketBuffer* buf, float value);
void buffer_write_double(PacketBuffer* buf, double value);
void buffer_write_string(PacketBuffer* buf, const char* str);
void buffer_write_uuid(PacketBuffer* buf, const uint8_t* uuid);
void buffer_write_position(PacketBuffer* buf, int32_t x, int32_t y, int32_t z);

uint8_t buffer_read_byte(PacketBuffer* buf);
int16_t buffer_read_short(PacketBuffer* buf);
int32_t buffer_read_int(PacketBuffer* buf);
int64_t buffer_read_long(PacketBuffer* buf);
float buffer_read_float(PacketBuffer* buf);
double buffer_read_double(PacketBuffer* buf);
char* buffer_read_string(PacketBuffer* buf, size_t max_len);
void buffer_read_uuid(PacketBuffer* buf, uint8_t* uuid);

/* === Обработка пакетов === */
void protocol_handshake(Player* player, PacketBuffer* buf);
void protocol_login_start(Player* player, PacketBuffer* buf);
void protocol_play_position_and_rotation(Player* player, PacketBuffer* buf);
void protocol_play_block_place(Player* player, PacketBuffer* buf);
void protocol_play_block_dig(Player* player, PacketBuffer* buf);

/* === Отправка пакетов === */
void packet_send_login_success(Player* player);
void packet_send_spawn_position(Player* player);
void packet_send_player_position_and_look(Player* player);
void packet_send_chunk_data(Player* player, Chunk* chunk);
void packet_send_block_change(Player* player, int32_t x, int32_t y, int32_t z, uint8_t block_id);
void packet_send_player_info(Player* player, Player* target);
void packet_send_entity_spawn(Player* player, Player* entity);
void packet_send_entity_destroy(Player* player, int32_t entity_id);
void packet_send_entity_move_relative(Player* player, Player* entity);
void packet_send_keep_alive(Player* player, int64_t keep_alive_id);
void packet_send_disconnect(Player* player, const char* reason);

#endif /* PROTOCOL_H */
