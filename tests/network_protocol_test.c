/*
 * network_protocol_test.c
 * Wire header encode/decode, checksum, and packet validation.
 * Compiles network_protocol.c directly and does not link Engine.
 */

#include "network/protocol/network_protocol.h"
#include "include/core/logger.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
    (void)level;
    (void)category;
    (void)file;
    (void)line;
    (void)function;
    (void)format;
}

static int g_chat_hits;

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static bool on_chat(const NetworkPacket *packet, void *user_data) {
    int *hits = user_data;
    if (!packet || packet->header.message_type != MSG_CHAT_MESSAGE) {
        fail("chat handler packet");
    }
    if (packet->header.data_size != 4 || memcmp(packet->data, "ping", 4) != 0) {
        fail("chat payload");
    }
    (*hits)++;
    g_chat_hits++;
    return true;
}

static void seal_packet(NetworkPacket *packet) {
    uint16_t sum;

    packet->header.checksum = 0;
    sum = calculate_checksum(&packet->header, offsetof(PacketHeader, checksum));
    sum = (uint16_t)(calculate_checksum(packet->data, packet->header.data_size) ^ sum);
    packet->header.checksum = sum;
}

static void test_checksum_words(void) {
    const uint8_t even_bytes[] = {0x01, 0x02, 0x03, 0x04};
    const uint8_t odd_bytes[] = {0x01, 0x02, 0x03};
    uint16_t even_sum;
    uint16_t odd_sum;

    printf("Testing checksum...\n");
    if (calculate_checksum(NULL, 0) != 0 || calculate_checksum(even_bytes, 0) != 0) {
        fail("empty checksum");
    }

    /* words 0x0201 + 0x0403 = 0x0604, one's complement 0xF9FB */
    even_sum = calculate_checksum(even_bytes, sizeof(even_bytes));
    if (even_sum != (uint16_t)~0x0604u) {
        fail("even checksum");
    }

    /* 0x0201 + odd byte 0x03 = 0x0204, one's complement 0xFDFB */
    odd_sum = calculate_checksum(odd_bytes, sizeof(odd_bytes));
    if (odd_sum != (uint16_t)~0x0204u) {
        fail("odd checksum");
    }
}

static void test_header_codec(void) {
    PacketHeader header;
    PacketHeader decoded;
    uint8_t wire[PACKET_HEADER_WIRE_SIZE];
    const uint8_t expect[PACKET_HEADER_WIRE_SIZE] = {
        NETWORK_PROTOCOL_VERSION,
        MSG_CHAT_MESSAGE,
        0x34, 0x12,
        0xAB, 0x00,
        0x04, 0x03, 0x02, 0x01,
        PACKET_FLAG_RELIABLE,
        0x05, 0x00,
        0xEF, 0xBE
    };

    printf("Testing header serialize/deserialize...\n");
    if (PACKET_HEADER_WIRE_SIZE != 15) {
        fail("wire size");
    }

    memset(&header, 0, sizeof(header));
    header.protocol_version = NETWORK_PROTOCOL_VERSION;
    header.message_type = MSG_CHAT_MESSAGE;
    header.sequence_number = 0x1234;
    header.ack_number = 0x00AB;
    header.timestamp = 0x01020304;
    header.flags = PACKET_FLAG_RELIABLE;
    header.data_size = 5;
    header.checksum = 0xBEEF;

    memset(wire, 0xA5, sizeof(wire));
    serialize_packet_header(&header, wire);
    if (memcmp(wire, expect, sizeof(expect)) != 0) {
        fail("wire bytes");
    }

    memset(&decoded, 0x5A, sizeof(decoded));
    if (!deserialize_packet_header(wire, &decoded)) {
        fail("deserialize");
    }
    if (decoded.protocol_version != header.protocol_version ||
        decoded.message_type != header.message_type ||
        decoded.sequence_number != header.sequence_number ||
        decoded.ack_number != header.ack_number ||
        decoded.timestamp != header.timestamp ||
        decoded.flags != header.flags ||
        decoded.data_size != header.data_size ||
        decoded.checksum != header.checksum) {
        fail("roundtrip fields");
    }

    if (deserialize_packet_header(NULL, &decoded) ||
        deserialize_packet_header(wire, NULL)) {
        fail("deserialize null");
    }
    serialize_packet_header(NULL, wire);
    serialize_packet_header(&header, NULL);
}

static void test_validate_and_addresses(void) {
    NetworkPacket packet;
    NetworkAddress parsed;
    NetworkAddress same;
    NetworkAddress other;
    const char payload[] = "ping";

    printf("Testing validate_packet and addresses...\n");
    memset(&packet, 0, sizeof(packet));
    packet.header.protocol_version = NETWORK_PROTOCOL_VERSION;
    packet.header.message_type = MSG_CHAT_MESSAGE;
    packet.header.sequence_number = 7;
    packet.header.ack_number = 0;
    packet.header.timestamp = 1000;
    packet.header.flags = PACKET_FLAG_RELIABLE;
    packet.header.data_size = 4;
    memcpy(packet.data, payload, 4);
    if (validate_packet(&packet)) {
        fail("zero checksum accepted");
    }
    seal_packet(&packet);
    if (!validate_packet(&packet)) {
        fail("sealed packet rejected");
    }

    packet.header.checksum ^= 0x00FFu;
    if (validate_packet(&packet)) {
        fail("bad checksum accepted");
    }
    packet.header.checksum ^= 0x00FFu;

    packet.header.protocol_version = 9;
    if (validate_packet(&packet)) {
        fail("bad version accepted");
    }
    packet.header.protocol_version = NETWORK_PROTOCOL_VERSION;

    packet.header.data_size = (uint16_t)(MAX_PACKET_SIZE - sizeof(PacketHeader) + 1);
    if (validate_packet(&packet)) {
        fail("oversize payload accepted");
    }
    if (validate_packet(NULL)) {
        fail("null packet accepted");
    }

    parsed = parse_address("10.1.2.3:27015");
    if (parsed.address != ((10u << 24) | (1u << 16) | (2u << 8) | 3u) ||
        parsed.port != 27015) {
        fail("parse address");
    }
    same = parsed;
    other = parse_address("10.1.2.3:27016");
    if (!address_equals(&parsed, &same) || address_equals(&parsed, &other) ||
        address_equals(NULL, &parsed) || address_equals(&parsed, NULL)) {
        fail("address equals");
    }
    if (address_hash(&parsed) != (parsed.address ^ ((uint32_t)parsed.port << 16)) ||
        address_hash(NULL) != 0) {
        fail("address hash");
    }
    parsed = parse_address(NULL);
    if (parsed.address != 0 || parsed.port != 0) {
        fail("null address");
    }
}

static void test_session(void) {
    ProtocolConfig config;
    NetworkProtocol *client;
    NetworkProtocol *server;
    NetworkAddress peer;
    NetworkAddress stranger;
    NetworkPacket packet;
    NetworkConnection *conn;
    ProtocolStats stats;
    uint32_t id;
    int hits = 0;
    uint16_t too_big;

    printf("Testing protocol session...\n");
    client = protocol_create(NULL, false);
    if (!client || client->is_server || client->config.port != 0 ||
        client->config.max_connections != 1) {
        fail("default client config");
    }
    protocol_destroy(client);

    memset(&config, 0, sizeof(config));
    config.port = 7777;
    config.max_connections = 4;
    config.heartbeat_interval = 30.0f;
    config.connection_timeout = 60.0f;
    config.rtt_smoothing_factor = 0.1f;
    server = protocol_create(&config, true);
    if (!server || !protocol_initialize(server) || !server->is_initialized) {
        fail("server create");
    }

    peer = parse_address("10.0.0.8:27015");
    id = protocol_add_connection(server, &peer);
    if (id == UINT32_MAX || !protocol_get_connection(server, id)) {
        fail("add connection");
    }
    protocol_register_handler(server, MSG_CHAT_MESSAGE, on_chat, &hits);

    memset(&packet, 0, sizeof(packet));
    packet.header.protocol_version = NETWORK_PROTOCOL_VERSION;
    packet.header.message_type = MSG_CHAT_MESSAGE;
    packet.header.sequence_number = 4;
    packet.header.data_size = 4;
    memcpy(packet.data, "ping", 4);
    seal_packet(&packet);
    packet.from_address = peer;
    packet.receive_time = 1.0f;
    protocol_process_packet(server, &packet);
    if (hits != 1 || g_chat_hits != 1) {
        fail("handler not called");
    }

    packet.header.checksum ^= 0x00FFu;
    protocol_process_packet(server, &packet);
    if (hits != 1) {
        fail("bad packet dispatched");
    }

    stranger = parse_address("10.0.0.9:27016");
    memset(&packet, 0, sizeof(packet));
    packet.header.protocol_version = NETWORK_PROTOCOL_VERSION;
    packet.header.message_type = MSG_CONNECT;
    seal_packet(&packet);
    packet.from_address = stranger;
    protocol_process_packet(server, &packet);
    conn = protocol_find_connection_by_address(server, &stranger);
    if (!conn || conn->state != CONN_CONNECTED) {
        fail("connect packet");
    }

    if (!protocol_send_reliable(server, id, MSG_CHAT_MESSAGE, "hi", 2) ||
        !protocol_send_unreliable(server, id, MSG_HEARTBEAT, NULL, 0)) {
        fail("send");
    }
    too_big = (uint16_t)(MAX_PACKET_SIZE - sizeof(PacketHeader) + 1);
    if (protocol_send_packet(server, id, MSG_CHAT_MESSAGE, "x", too_big, 0) ||
        protocol_send_packet(NULL, 0, MSG_NONE, NULL, 0, 0) ||
        protocol_receive_packet(server, &packet)) {
        fail("send rejects");
    }

    protocol_get_stats(server, &stats);
    if (stats.packets_received < 1 || stats.packets_sent < 1) {
        fail("stats");
    }
    protocol_reset_stats(server);
    protocol_get_stats(server, &stats);
    if (stats.packets_sent != 0 || stats.packets_received != 0) {
        fail("reset stats");
    }

    protocol_unregister_handler(server, MSG_CHAT_MESSAGE);
    protocol_shutdown(server);
    if (server->is_initialized || protocol_get_connection(server, id) != NULL) {
        fail("shutdown");
    }
    protocol_destroy(server);
}

int main(void) {
    test_checksum_words();
    test_header_codec();
    test_validate_and_addresses();
    test_session();
    printf("network_protocol tests passed\n");
    return 0;
}
