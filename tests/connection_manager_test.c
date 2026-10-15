/*
 * connection_manager_test.c
 * Connection lifecycle: create, connect, info, capacity, timeout, server bind.
 * Compiles connection_manager.c directly and does not link Engine.
 */

#include "network/connection_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int transitions;
    ConnectionState from[8];
    ConnectionState to[8];
    ConnectionID ids[8];
    int disconnects;
    ConnectionID disconnect_id;
    char reason[64];
    void *user_seen;
} Probe;

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void on_state(ConnectionID id, ConnectionState old_state,
                     ConnectionState new_state, void *user_data) {
    Probe *probe = (Probe *)user_data;

    if (probe->transitions < 8) {
        probe->ids[probe->transitions] = id;
        probe->from[probe->transitions] = old_state;
        probe->to[probe->transitions] = new_state;
    }
    probe->transitions++;
    probe->user_seen = user_data;
}

static void on_disconnect(ConnectionID id, const char *reason, void *user_data) {
    Probe *probe = (Probe *)user_data;

    probe->disconnects++;
    probe->disconnect_id = id;
    probe->reason[0] = '\0';
    if (reason) {
        snprintf(probe->reason, sizeof(probe->reason), "%s", reason);
    }
    probe->user_seen = user_data;
}

static void bind_probe(ConnectionManager *manager, Probe *probe) {
    memset(probe, 0, sizeof(*probe));
    connection_manager_set_state_change_callback(manager, on_state, probe);
    connection_manager_set_disconnection_callback(manager, on_disconnect, probe);
    connection_manager_set_packet_received_callback(manager, NULL, probe);
}

static void test_null_and_defaults(void) {
    ConnectionManager *manager;
    ConnectionStats stats;

    printf("Testing create, defaults, and null guards...\n");
    manager = connection_manager_create(0);
    if (!manager) {
        fail("create default");
    }
    if (connection_manager_get_statistics(manager, &stats) != 0) {
        fail("statistics");
    }
    if (stats.total_connections != 32 || stats.active_connections != 0 ||
        stats.total_bytes_sent != 0 || stats.total_bytes_received != 0 ||
        stats.total_packets_sent != 0 || stats.total_packets_received != 0) {
        fail("default statistics");
    }
    if (connection_manager_get_statistics(NULL, &stats) != -1 ||
        connection_manager_get_statistics(manager, NULL) != -1) {
        fail("null statistics");
    }
    if (connection_manager_update(NULL, 0.016f) != -1) {
        fail("null update");
    }
    if (connection_manager_update(manager, 0.016f) != 0) {
        fail("empty update");
    }
    if (connection_manager_is_server(NULL) || connection_manager_is_server(manager)) {
        fail("server flag before bind");
    }
    if (connection_manager_connect(NULL, "127.0.0.1", 9, 1000) != 0 ||
        connection_manager_connect(manager, NULL, 9, 1000) != 0) {
        fail("null connect");
    }
    if (connection_manager_disconnect(NULL, 1) != -1) {
        fail("null disconnect");
    }
    if (connection_manager_get_state(NULL, 1) != CONNECTION_DISCONNECTED ||
        connection_manager_get_state(manager, 0) != CONNECTION_DISCONNECTED) {
        fail("missing connection state");
    }
    connection_manager_destroy(NULL);
    connection_manager_destroy(manager);
}

static void test_connect_info(void) {
    ConnectionManager *manager;
    ConnectionID id;
    ConnectionID other;
    ConnectionInfo info;
    ConnectionStats stats;
    Probe probe;

    printf("Testing connect info and remote address...\n");
    manager = connection_manager_create(4);
    if (!manager) {
        fail("create");
    }
    bind_probe(manager, &probe);

    if (connection_manager_connect(manager, "not-an-ip", 9, 1000) != 0 ||
        connection_manager_connect(manager, "999.1.1.1", 9, 1000) != 0) {
        fail("invalid address consumed a slot");
    }

    id = connection_manager_connect(manager, "127.0.0.1", 9, 5000);
    if (id == 0) {
        fail("connect");
    }
    if (probe.transitions != 1 || probe.ids[0] != id ||
        probe.from[0] != CONNECTION_DISCONNECTED ||
        probe.to[0] != CONNECTION_CONNECTING || probe.user_seen != &probe) {
        fail("connecting callback");
    }
    if (connection_manager_get_state(manager, id) != CONNECTION_CONNECTING) {
        fail("connecting state");
    }

    memset(&info, 0, sizeof(info));
    if (connection_manager_get_info(NULL, id, &info) != -1 ||
        connection_manager_get_info(manager, id, NULL) != -1 ||
        connection_manager_get_info(manager, 0, &info) != -2) {
        fail("get_info guards");
    }
    memset(&info, 0, sizeof(info));
    if (connection_manager_get_info(manager, id, &info) != 0) {
        fail("get_info");
    }
    if (info.id != id || info.remote_port != 9 ||
        info.state != CONNECTION_CONNECTING || info.packets_sent != 0 ||
        info.packets_received != 0 || info.bytes_sent != 0 ||
        info.bytes_received != 0) {
        fail("connection info fields");
    }
    if (!info.remote_address || strcmp(info.remote_address, "127.0.0.1") != 0) {
        fail("remote address");
    }

    other = connection_manager_connect(manager, "192.168.1.50", 24444, 5000);
    if (other == 0 || other == id) {
        fail("second connect");
    }
    memset(&info, 0, sizeof(info));
    if (connection_manager_get_info(manager, other, &info) != 0 ||
        info.remote_port != 24444 || !info.remote_address ||
        strcmp(info.remote_address, "192.168.1.50") != 0) {
        fail("second remote address");
    }

    if (connection_manager_get_statistics(manager, &stats) != 0 ||
        stats.total_connections != 4 || stats.active_connections != 0) {
        fail("connecting connections are not active");
    }
    if (connection_manager_update(manager, 0.016f) != 0 ||
        connection_manager_get_state(manager, id) != CONNECTION_CONNECTING) {
        fail("short update timed out a long timeout");
    }

    connection_manager_destroy(manager);
}

static void test_capacity_and_disconnect(void) {
    ConnectionManager *manager;
    ConnectionID first;
    ConnectionID second;
    ConnectionID third;
    ConnectionID reused;
    Probe probe;

    printf("Testing capacity and manual disconnect...\n");
    manager = connection_manager_create(2);
    if (!manager) {
        fail("create");
    }
    bind_probe(manager, &probe);

    first = connection_manager_connect(manager, "127.0.0.1", 9, 5000);
    second = connection_manager_connect(manager, "127.0.0.1", 10, 5000);
    third = connection_manager_connect(manager, "127.0.0.1", 11, 5000);
    if (first == 0 || second == 0 || first == second || third != 0) {
        fail("capacity");
    }
    if (connection_manager_disconnect(manager, 99999) != -2) {
        fail("unknown disconnect");
    }
    if (connection_manager_disconnect(manager, first) != 0) {
        fail("disconnect");
    }
    if (probe.disconnects != 1 || probe.disconnect_id != first ||
        strcmp(probe.reason, "Manual disconnect") != 0) {
        fail("disconnect callback");
    }
    if (connection_manager_get_state(manager, first) != CONNECTION_DISCONNECTED) {
        fail("disconnected state");
    }
    if (connection_manager_disconnect(manager, first) != -2) {
        fail("double disconnect");
    }

    reused = connection_manager_connect(manager, "127.0.0.1", 12, 5000);
    if (reused == 0 || reused == second) {
        fail("slot reuse");
    }
    if (connection_manager_get_state(manager, reused) != CONNECTION_CONNECTING ||
        connection_manager_get_state(manager, second) != CONNECTION_CONNECTING) {
        fail("states after reuse");
    }

    connection_manager_destroy(manager);
}

static void test_send_rejected_until_connected(void) {
    ConnectionManager *manager;
    ConnectionID id;
    TransportPacket packet;
    TransportPacket lone;
    TransportPacket *received;
    TransportPacket *many;
    uint8_t payload[4] = {1, 2, 3, 4};
    uint32_t count;

    printf("Testing send and receive before the peer answers...\n");
    manager = connection_manager_create(1);
    if (!manager) {
        fail("create");
    }
    id = connection_manager_connect(manager, "127.0.0.1", 9, 5000);
    if (id == 0) {
        fail("connect");
    }

    memset(&packet, 0, sizeof(packet));
    packet.data = payload;
    packet.size = sizeof(payload);
    packet.type = PACKET_TYPE_HEARTBEAT;
    packet.delivery_type = PACKET_DELIVERY_UNRELIABLE;

    if (connection_manager_send_packet(NULL, id, &packet) != -1 ||
        connection_manager_send_packet(manager, id, NULL) != -1) {
        fail("null send");
    }
    if (connection_manager_send_packet(manager, id, &packet) != -2) {
        fail("send while connecting");
    }
    connection_manager_set_bandwidth_limit(NULL, id, 8);
    connection_manager_set_bandwidth_limit(manager, id, 8);
    if (connection_manager_send_packet(manager, id, &packet) != -2) {
        fail("bandwidth checked before connected");
    }

    received = (TransportPacket *)0x1;
    count = 99;
    if (connection_manager_receive_packets(NULL, id, &received, &count) != -1 ||
        connection_manager_receive_packets(manager, id, NULL, &count) != -1 ||
        connection_manager_receive_packets(manager, id, &received, NULL) != -1) {
        fail("null receive");
    }
    received = (TransportPacket *)0x1;
    count = 99;
    if (connection_manager_receive_packets(manager, id, &received, &count) != 0 ||
        received != NULL || count != 0) {
        fail("empty receive");
    }
    if (connection_manager_receive_packets(manager, 99999, &received, &count) != -2) {
        fail("unknown receive");
    }

    connection_manager_free_packet(NULL);
    connection_manager_free_packets(NULL, 3);
    memset(&lone, 0, sizeof(lone));
    lone.data = malloc(4);
    if (!lone.data) {
        fail("malloc");
    }
    memcpy(lone.data, "ping", 4);
    lone.size = 4;
    connection_manager_free_packet(&lone);
    if (lone.data != NULL) {
        fail("free_packet");
    }

    many = calloc(2, sizeof(*many));
    if (!many) {
        fail("calloc");
    }
    many[0].data = malloc(1);
    many[1].data = malloc(1);
    if (!many[0].data || !many[1].data) {
        fail("packet malloc");
    }
    many[0].data[0] = 7;
    many[1].data[0] = 8;
    connection_manager_free_packets(many, 2);

    connection_manager_destroy(manager);
}

static void test_server_lifecycle(void) {
    ConnectionManager *manager;

    printf("Testing server bind and shutdown...\n");
    manager = connection_manager_create(1);
    if (!manager) {
        fail("create");
    }
    if (connection_manager_create_server(NULL, "127.0.0.1", 0) != -1 ||
        connection_manager_create_server(manager, NULL, 0) != -1) {
        fail("null server");
    }
    if (connection_manager_create_server(manager, "not-an-ip", 1) != -3) {
        fail("invalid bind address");
    }
    if (connection_manager_is_server(manager)) {
        fail("invalid bind marked server");
    }
    if (connection_manager_create_server(manager, "127.0.0.1", 0) != 0) {
        fail("bind");
    }
    if (!connection_manager_is_server(manager)) {
        fail("is_server");
    }
    if (connection_manager_shutdown_server(manager) != 0 ||
        connection_manager_is_server(manager)) {
        fail("shutdown");
    }
    if (connection_manager_shutdown_server(NULL) != -1) {
        fail("null shutdown");
    }
    if (connection_manager_shutdown_server(manager) != 0) {
        fail("second shutdown");
    }
    if (connection_manager_create_server(manager, "127.0.0.1", 0) != 0 ||
        !connection_manager_is_server(manager)) {
        fail("rebind");
    }

    connection_manager_destroy(manager);
}

static void test_timeout(void) {
    ConnectionManager *manager;
    ConnectionID id;
    Probe probe;

    printf("Testing connection timeout...\n");
    manager = connection_manager_create(1);
    if (!manager) {
        fail("create");
    }
    bind_probe(manager, &probe);
    id = connection_manager_connect(manager, "127.0.0.1", 9, 20);
    if (id == 0) {
        fail("connect");
    }

    usleep(200000);
    if (connection_manager_update(manager, 0.2f) != 0) {
        fail("update");
    }
    if (probe.transitions < 2 || probe.to[0] != CONNECTION_CONNECTING ||
        probe.from[1] != CONNECTION_CONNECTING ||
        probe.to[1] != CONNECTION_DISCONNECTED) {
        fail("timeout transition");
    }
    if (probe.disconnects != 1 || probe.disconnect_id != id ||
        strcmp(probe.reason, "Timeout") != 0) {
        fail("timeout callback");
    }
    if (connection_manager_get_state(manager, id) != CONNECTION_DISCONNECTED) {
        fail("timed out state");
    }

    connection_manager_destroy(manager);
}

int main(void) {
    test_null_and_defaults();
    test_connect_info();
    test_capacity_and_disconnect();
    test_send_rejected_until_connected();
    test_server_lifecycle();
    test_timeout();
    printf("ok\n");
    return 0;
}
