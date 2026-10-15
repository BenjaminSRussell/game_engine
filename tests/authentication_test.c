/*
 * authentication_test.c
 * Anti-cheat movement, combat, kick, and ban behavior.
 * Compiles authentication.c directly and does not link Engine.
 */

#include "network/security/authentication.h"

#include <stdio.h>
#include <stdlib.h>

static uint64_t g_now = 1000;

u64 get_current_time_ms(void) { return g_now; }

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static Vec3 v3(float x, float y, float z) {
  Vec3 v;
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}

static void expect_stats(uint32_t bans, uint32_t kicks, uint32_t speed,
                         uint32_t fly, uint32_t combat) {
  uint32_t got_bans = 99, got_kicks = 99, got_speed = 99, got_fly = 99,
           got_combat = 99;
  anticheat_get_statistics(&got_bans, &got_kicks, &got_speed, &got_fly,
                           &got_combat);
  if (got_bans != bans || got_kicks != kicks || got_speed != speed ||
      got_fly != fly || got_combat != combat) {
    fprintf(stderr,
            "FAIL: stats bans=%u kicks=%u speed=%u fly=%u combat=%u\n",
            got_bans, got_kicks, got_speed, got_fly, got_combat);
    exit(1);
  }
}

int main(void) {
  Vec3 still;
  Vec3 vel;

  g_now = 10000;
  if (!anticheat_init(8)) {
    fail("init");
  }

  still = v3(0.0f, 0.0f, 0.0f);
  vel = v3(0.0f, 0.0f, 0.0f);

  /* Same timestamp as profile creation is ignored. */
  if (!anticheat_validate_movement(1, "ada", still, vel, 10000)) {
    fail("same timestamp");
  }

  /* 20 units in 1s is under the 25 u/s speed-hack line. */
  if (!anticheat_validate_movement(1, "ada", v3(20.0f, 0.0f, 0.0f), vel,
                                   11000)) {
    fail("legal move");
  }
  expect_stats(0, 0, 0, 0, 0);
  if (anticheat_is_player_banned(1)) {
    fail("legal player banned");
  }

  /* 30 units in 1s is a speed hack, but one hit is under the ban threshold. */
  if (!anticheat_validate_movement(1, "ada", v3(50.0f, 0.0f, 0.0f), vel,
                                   12000)) {
    fail("speed hack should not ban yet");
  }
  expect_stats(0, 0, 1, 0, 0);

  /* High vertical velocity over a short interval is a fly hack. */
  vel = v3(0.0f, 6.0f, 0.0f);
  if (!anticheat_validate_movement(1, "ada", v3(50.0f, 0.0f, 0.0f), vel,
                                   12100)) {
    fail("fly hack should not ban yet");
  }
  expect_stats(0, 0, 1, 1, 0);
  vel = v3(0.0f, 0.0f, 0.0f);

  /* A miss does not create a profile or count as an attack. */
  if (!anticheat_validate_combat(3, "cy", 30000, false)) {
    fail("miss");
  }
  anticheat_kick_player(3, "no profile");
  expect_stats(0, 0, 1, 1, 0);

  /* Sixth hit inside the one-second window is a combat hack. */
  if (!anticheat_validate_combat(3, "cy", 30000, true) ||
      !anticheat_validate_combat(3, "cy", 30100, true) ||
      !anticheat_validate_combat(3, "cy", 30200, true) ||
      !anticheat_validate_combat(3, "cy", 30300, true) ||
      !anticheat_validate_combat(3, "cy", 30400, true) ||
      !anticheat_validate_combat(3, "cy", 30500, true)) {
    fail("combat hack should not ban yet");
  }
  expect_stats(0, 0, 1, 1, 1);
  if (anticheat_is_player_banned(3)) {
    fail("combat player banned early");
  }

  if (!anticheat_validate_client(3, "cy", 30600, 0xA11CEu)) {
    fail("client hash");
  }

  anticheat_kick_player(3, "test kick");
  expect_stats(0, 1, 1, 1, 1);
  if (anticheat_is_player_banned(3) || anticheat_is_player_banned(99)) {
    fail("kick is not a ban");
  }

  /* One more speed hack bans once the threshold drops to 1. */
  anticheat_update_settings(10.0f, 5.0f, 5, 1);
  g_now = 40000;
  if (!anticheat_validate_movement(4, "dee", still, vel, 40000)) {
    fail("seed dee");
  }
  if (anticheat_validate_movement(4, "dee", v3(30.0f, 0.0f, 0.0f), vel,
                                  41000)) {
    fail("speed ban");
  }
  if (!anticheat_is_player_banned(4)) {
    fail("dee not banned");
  }
  if (anticheat_validate_movement(4, "dee", v3(31.0f, 0.0f, 0.0f), vel,
                                  42000) ||
      anticheat_validate_combat(4, "dee", 42100, true) ||
      anticheat_validate_client(4, "dee", 42200, 1)) {
    fail("banned player still accepted");
  }
  expect_stats(1, 1, 2, 1, 1);

  /* Large jump with speed and fly limits raised is a teleport ban. */
  anticheat_update_settings(10000.0f, 10000.0f, 5, 1);
  g_now = 50000;
  if (!anticheat_validate_movement(5, "eve", still, vel, 50000)) {
    fail("seed eve");
  }
  if (anticheat_validate_movement(5, "eve", v3(60.0f, 0.0f, 0.0f), vel,
                                  50500)) {
    fail("teleport ban");
  }
  if (!anticheat_is_player_banned(5)) {
    fail("eve not banned");
  }
  expect_stats(2, 1, 2, 1, 1);

  /* Three hits with a cap of two attacks per second bans. */
  anticheat_update_settings(10000.0f, 10000.0f, 2, 1);
  if (!anticheat_validate_combat(6, "fay", 60000, true) ||
      !anticheat_validate_combat(6, "fay", 60100, true)) {
    fail("legal attack rate");
  }
  if (anticheat_validate_combat(6, "fay", 60200, true)) {
    fail("combat ban");
  }
  if (!anticheat_is_player_banned(6)) {
    fail("fay not banned");
  }
  expect_stats(3, 1, 2, 1, 2);

  /* NULL statistic slots are skipped. */
  anticheat_get_statistics(NULL, NULL, NULL, NULL, NULL);

  anticheat_shutdown();
  anticheat_shutdown();
  if (anticheat_is_player_banned(4)) {
    fail("ban survived shutdown");
  }

  g_now = 70000;
  if (!anticheat_init(4)) {
    fail("reinit");
  }
  if (!anticheat_validate_movement(1, "ada", still, vel, 70000) ||
      !anticheat_validate_movement(1, "ada", v3(1.0f, 0.0f, 0.0f), vel,
                                   71000)) {
    fail("move after reinit");
  }
  if (anticheat_is_player_banned(1)) {
    fail("reinit player banned");
  }
  expect_stats(0, 0, 0, 0, 0);
  anticheat_shutdown();

  printf("authentication tests passed\n");
  return 0;
}
