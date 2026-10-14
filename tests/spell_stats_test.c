/*
 * spell_stats_test.c
 * Activation count and total active time on AbilitySpec.
 * Compiles gameplay_abilities.c directly and does not link Engine.
 */

#include "include/gameplay/gameplay_abilities.h"

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

static int g_activations;

static bool always_ok(void *owner, const AbilitySpec *spec) {
    (void)owner;
    (void)spec;
    return true;
}

static bool never_ok(void *owner, const AbilitySpec *spec) {
    (void)owner;
    (void)spec;
    return false;
}

static void on_activate(void *owner, AbilitySpec *spec) {
    (void)owner;
    (void)spec;
    g_activations++;
}

static void on_end(void *owner, AbilitySpec *spec) {
    (void)owner;
    (void)spec;
}

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static GameplayAbility make_ability(const char *name, bool (*can_activate)(void *, const AbilitySpec *)) {
    GameplayAbility ability;
    memset(&ability, 0, sizeof(ability));
    strncpy(ability.name, name, sizeof(ability.name) - 1);
    ability.can_activate = can_activate;
    ability.on_activate = on_activate;
    ability.on_end = on_end;
    return ability;
}

int main(void) {
    AbilitySystemComponent *comp;
    GameplayAbility freeze;
    GameplayAbility blocked;
    AbilityStats stats;
    int owner = 1;

    comp = gas_create_component(&owner);
    if (!comp) {
        fail("create");
    }

    freeze = make_ability("Freeze", always_ok);
    blocked = make_ability("Blocked", never_ok);
    gas_give_ability(comp, &freeze);
    gas_give_ability(comp, &blocked);

    if (gas_get_ability_stats(NULL, "Freeze", &stats)) {
        fail("null comp");
    }
    if (gas_get_ability_stats(comp, NULL, &stats)) {
        fail("null name");
    }
    if (gas_get_ability_stats(comp, "Freeze", NULL)) {
        fail("null out");
    }
    if (gas_get_ability_stats(comp, "Missing", &stats)) {
        fail("missing ability");
    }

    if (!gas_get_ability_stats(comp, "Freeze", &stats)) {
        fail("stats before activate");
    }
    if (stats.activation_count != 0 || stats.total_active_time != 0.0f) {
        fail("initial stats");
    }

    if (!gas_try_activate_ability(comp, "Freeze")) {
        fail("activate");
    }
    if (g_activations != 1) {
        fail("activate callback");
    }
    if (!gas_get_ability_stats(comp, "Freeze", &stats) || stats.activation_count != 1) {
        fail("count after one activation");
    }

    if (gas_try_activate_ability(comp, "Blocked")) {
        fail("blocked ability activated");
    }
    if (!gas_get_ability_stats(comp, "Blocked", &stats) ||
        stats.activation_count != 0 || stats.total_active_time != 0.0f) {
        fail("blocked ability gained stats");
    }

    gas_update(comp, 0.5f);
    gas_update(comp, 0.25f);
    if (!gas_get_ability_stats(comp, "Freeze", &stats) || stats.total_active_time != 0.75f) {
        fprintf(stderr, "FAIL: active time %.4f\n", stats.total_active_time);
        exit(1);
    }
    if (!gas_get_ability_stats(comp, "Blocked", &stats) || stats.total_active_time != 0.0f) {
        fail("inactive ability accumulated time");
    }

    if (!gas_try_activate_ability(comp, "Freeze")) {
        fail("second activate");
    }
    if (!gas_get_ability_stats(comp, "Freeze", &stats) || stats.activation_count != 2) {
        fail("count after two activations");
    }

    gas_cancel_ability(comp, "Freeze");
    gas_update(comp, 1.0f);
    if (!gas_get_ability_stats(comp, "Freeze", &stats)) {
        fail("stats after cancel");
    }
    if (stats.total_active_time != 0.75f) {
        fail("time accumulated while inactive");
    }
    if (stats.activation_count != 2) {
        fail("count changed on cancel");
    }

    if (!gas_activate_ability(comp, "Freeze")) {
        fail("activate via gas_activate_ability");
    }
    if (!gas_get_ability_stats(comp, "Freeze", &stats) || stats.activation_count != 3) {
        fail("count after gas_activate_ability");
    }

    gas_destroy_component(comp);
    printf("ok\n");
    return 0;
}
