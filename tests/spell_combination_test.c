/*
 * spell_combination_test.c
 * Fire+Fire and Fire+Water resolve to registered spells in either order.
 * An unknown pair does not invent a spell.
 * Compiles spell_system.c directly and does not link Engine.
 */

#include "include/gameplay/magic/spell_system.h"
#include "core/logging/unified_logger.h"

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

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static SpellDefinition make_spell(u32 id, const char *name, MagicElement element, SpellType type) {
    SpellDefinition spell;
    memset(&spell, 0, sizeof(spell));
    spell.id = id;
    strncpy(spell.name, name, sizeof(spell.name) - 1);
    spell.primary_element = element;
    spell.type = type;
    return spell;
}

static void expect_combo(const MagicElement *elements, u32 count, u32 expected_id,
                         const char *expected_name, const char *label) {
    u32 id = spell_system_combine(elements, count);
    const SpellDefinition *spell = spell_system_get_spell(id);
    const char *name = spell ? spell->name : "(none)";
    printf("%s -> %s (%u)\n", label, name, id);
    if (id != expected_id || !spell || strcmp(spell->name, expected_name) != 0) {
        fprintf(stderr, "FAIL: %s (got %u %s, expected %u %s)\n", label, id, name,
                expected_id, expected_name);
        exit(1);
    }
}

int main(void) {
    SpellDefinition fireball;
    SpellDefinition steam;
    MagicElement fire_fire[2];
    MagicElement fire_water[2];
    MagicElement water_fire[2];
    MagicElement earth_air[2];
    u32 unknown;

    if (!spell_system_init()) {
        fail("init");
    }

    fireball = make_spell(1, "Fireball", ELEMENT_FIRE, SPELL_TYPE_PROJECTILE);
    steam = make_spell(2, "Steam", ELEMENT_WATER, SPELL_TYPE_AREA_OF_EFFECT);
    if (!spell_system_register_spell(fireball) || !spell_system_register_spell(steam)) {
        fail("register spells");
    }

    fire_fire[0] = ELEMENT_FIRE;
    fire_fire[1] = ELEMENT_FIRE;
    fire_water[0] = ELEMENT_FIRE;
    fire_water[1] = ELEMENT_WATER;
    if (!spell_system_register_combination(fire_fire, 2, 1) ||
        !spell_system_register_combination(fire_water, 2, 2)) {
        fail("register combinations");
    }

    expect_combo(fire_fire, 2, 1, "Fireball", "Fire+Fire");

    water_fire[0] = ELEMENT_WATER;
    water_fire[1] = ELEMENT_FIRE;
    expect_combo(fire_water, 2, 2, "Steam", "Fire+Water");
    expect_combo(water_fire, 2, 2, "Steam", "Water+Fire");

    earth_air[0] = ELEMENT_EARTH;
    earth_air[1] = ELEMENT_AIR;
    unknown = spell_system_combine(earth_air, 2);
    printf("Earth+Air -> %u\n", unknown);
    if (unknown != 0 || spell_system_get_spell(unknown) != NULL) {
        fail("unknown pair invented a spell");
    }

    earth_air[0] = ELEMENT_AIR;
    earth_air[1] = ELEMENT_EARTH;
    unknown = spell_system_combine(earth_air, 2);
    printf("Air+Earth -> %u\n", unknown);
    if (unknown != 0) {
        fail("reversed unknown pair invented a spell");
    }

    spell_system_shutdown();
    printf("ok\n");
    return 0;
}
