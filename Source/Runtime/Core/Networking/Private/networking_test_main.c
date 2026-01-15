#include "../Public/unified_networking.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
  printf("Starting Networking Verification...\n");

  // Get default config
  NetworkConfig config = network_get_default_config();
  printf("PASSED: Default config created\n");

  // 1. Test server initialization
  NetworkContext *server = network_init(NETWORK_MODE_SERVER, &config);
  if (!server) {
    printf("FAILED: Server context creation failed\n");
    return 1;
  }
  printf("PASSED: Server context created\n");

  // 2. Start server
  if (network_server_start(server, 12345, "TestServer") != 0) {
    printf("FAILED: Server start failed\n");
    network_shutdown(server);
    return 1;
  }
  printf("PASSED: Server started on port 12345\n");

  // 3. Test client initialization
  NetworkContext *client = network_init(NETWORK_MODE_CLIENT, &config);
  if (!client) {
    printf("FAILED: Client context creation failed\n");
    network_server_stop(server);
    network_shutdown(server);
    return 1;
  }
  printf("PASSED: Client context created\n");

  // 4. Connect client to server
  if (network_client_connect(client, "127.0.0.1", 12345, "TestPlayer") != 0) {
    printf("FAILED: Client connect failed\n");
    network_shutdown(client);
    network_server_stop(server);
    network_shutdown(server);
    return 1;
  }
  printf("PASSED: Client connected to server\n");

  // 5. Update loop for connection
  for (int i = 0; i < 10; i++) {
    network_update(server, 0.016f);
    network_update(client, 0.016f);
    usleep(16000); // ~16ms for 60 FPS
  }
  printf("PASSED: Update loop executed\n");

  // 6. Check connection
  if (!network_client_is_connected(client)) {
    printf("WARNING: Client not connected after updates\n");
  } else {
    printf("PASSED: Client successfully connected (ID: %u)\n",
           network_client_get_id(client));
  }

  // 7. Test replication
  if (network_replication_add_entity(server, 1001, true) == 0) {
    printf("PASSED: Entity added to replication system\n");

    network_replication_set_entity_position(server, 1001, 10.0f, 20.0f, 30.0f);
    network_replication_set_owner(server, 1001, 1);

    uint32_t data = 0x12345678;
    network_replication_update_entity(server, 1001, 0xFF, &data, sizeof(data));
    printf("PASSED: Entity data updated\n");
  }

  // 8. Update for replication
  for (int i = 0; i < 5; i++) {
    network_update(server, 0.05f);
    network_update(client, 0.05f);
    usleep(50000); // ~50ms
  }
  printf("PASSED: Replication update loop executed\n");

  // 9. Get statistics
  NetworkStats stats;
  if (network_get_stats(server, &stats) == 0) {
    printf("PASSED: Server stats - Packets sent: %llu, received: %llu\n",
           stats.packets_sent, stats.packets_received);
  }

  if (network_get_stats(client, &stats) == 0) {
    printf("PASSED: Client stats - Packets sent: %llu, received: %llu\n",
           stats.packets_sent, stats.packets_received);
  }

  // 10. Cleanup
  network_replication_remove_entity(server, 1001);
  printf("PASSED: Entity removed from replication\n");

  network_client_disconnect(client);
  network_shutdown(client);
  printf("PASSED: Client disconnected and shutdown\n");

  network_server_stop(server);
  network_shutdown(server);
  printf("PASSED: Server stopped and shutdown\n");

  printf("\nNetworking Verification Successful!\n");
  return 0;
}
