#include "include/gameplay/magic/spell_system.h"
#include "include/core/logger.h"
#include <string.h>
#include <stdlib.h>

#define MAX_REGISTERED_SPELLS 1024
#define MAX_COMBINATIONS 2048

typedef struct {
    SpellCombination combination;
    u32 spell_id;
} RegisteredCombination;

static struct {
    SpellDefinition spells[MAX_REGISTERED_SPELLS];
    u32 spell_count;

    RegisteredCombination combinations[MAX_COMBINATIONS];
    u32 combination_count;

    bool initialized;
} s_spell_system;

// Helper to sort elements for consistent matching
static int compare_elements(const void* a, const void* b) {
    return (*(const MagicElement*)a - *(const MagicElement*)b);
}

static void normalize_elements(MagicElement* elements, u32 count) {
    qsort(elements, count, sizeof(MagicElement), compare_elements);
}

bool spell_system_init(void) {
    if (s_spell_system.initialized) {
        LOG_WARN(LOG_CAT_GAME, "Spell system already initialized");
        return true;
    }

    memset(&s_spell_system, 0, sizeof(s_spell_system));
    s_spell_system.initialized = true;

    LOG_INFO(LOG_CAT_GAME, "Spell system initialized");
    return true;
}

void spell_system_shutdown(void) {
    s_spell_system.initialized = false;
    s_spell_system.spell_count = 0;
    s_spell_system.combination_count = 0;
    LOG_INFO(LOG_CAT_GAME, "Spell system shutdown");
}

bool spell_system_register_spell(SpellDefinition spell) {
    if (!s_spell_system.initialized) return false;

    if (s_spell_system.spell_count >= MAX_REGISTERED_SPELLS) {
        LOG_ERROR(LOG_CAT_GAME, "Cannot register spell: Max spells reached");
        return false;
    }

    // Check if ID already exists
    for (u32 i = 0; i < s_spell_system.spell_count; ++i) {
        if (s_spell_system.spells[i].id == spell.id) {
            LOG_ERROR(LOG_CAT_GAME, "Cannot register spell: ID %u already exists", spell.id);
            return false;
        }
    }

    s_spell_system.spells[s_spell_system.spell_count++] = spell;
    LOG_INFO(LOG_CAT_GAME, "Registered spell: %s (ID: %u)", spell.name, spell.id);
    return true;
}

bool spell_system_register_combination(const MagicElement* elements, u32 count, u32 spell_id) {
    if (!s_spell_system.initialized) return false;
    if (count == 0 || count > MAX_SPELL_ELEMENTS) {
        LOG_ERROR(LOG_CAT_GAME, "Invalid element count for combination: %u", count);
        return false;
    }

    if (s_spell_system.combination_count >= MAX_COMBINATIONS) {
        LOG_ERROR(LOG_CAT_GAME, "Cannot register combination: Max combinations reached");
        return false;
    }

    // Check if spell ID exists
    bool spell_found = false;
    for (u32 i = 0; i < s_spell_system.spell_count; ++i) {
        if (s_spell_system.spells[i].id == spell_id) {
            spell_found = true;
            break;
        }
    }

    if (!spell_found) {
        LOG_ERROR(LOG_CAT_GAME, "Cannot register combination for non-existent spell ID: %u", spell_id);
        return false;
    }

    RegisteredCombination* reg = &s_spell_system.combinations[s_spell_system.combination_count];

    // Copy and normalize elements
    memcpy(reg->combination.elements, elements, count * sizeof(MagicElement));
    reg->combination.element_count = count;
    normalize_elements(reg->combination.elements, count);

    reg->spell_id = spell_id;

    s_spell_system.combination_count++;
    return true;
}

u32 spell_system_combine(const MagicElement* elements, u32 count) {
    if (!s_spell_system.initialized || count == 0 || count > MAX_SPELL_ELEMENTS) {
        return 0;
    }

    // Create a local copy to normalize
    MagicElement sorted_elements[MAX_SPELL_ELEMENTS];
    memcpy(sorted_elements, elements, count * sizeof(MagicElement));
    normalize_elements(sorted_elements, count);

    for (u32 i = 0; i < s_spell_system.combination_count; ++i) {
        RegisteredCombination* reg = &s_spell_system.combinations[i];

        if (reg->combination.element_count == count) {
            bool match = true;
            for (u32 j = 0; j < count; ++j) {
                if (reg->combination.elements[j] != sorted_elements[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return reg->spell_id;
            }
        }
    }

    return 0;
}

const SpellDefinition* spell_system_get_spell(u32 spell_id) {
    if (!s_spell_system.initialized) return NULL;

    for (u32 i = 0; i < s_spell_system.spell_count; ++i) {
        if (s_spell_system.spells[i].id == spell_id) {
            return &s_spell_system.spells[i];
        }
    }
    return NULL;
}

const char* spell_system_get_element_name(MagicElement element) {
    switch (element) {
        case ELEMENT_NONE: return "None";
        case ELEMENT_FIRE: return "Fire";
        case ELEMENT_WATER: return "Water";
        case ELEMENT_EARTH: return "Earth";
        case ELEMENT_AIR: return "Air";
        case ELEMENT_LIGHTNING: return "Lightning";
        case ELEMENT_ICE: return "Ice";
        case ELEMENT_ARCANE: return "Arcane";
        case ELEMENT_LIFE: return "Life";
        case ELEMENT_DEATH: return "Death";
        default: return "Unknown";
    }
}
