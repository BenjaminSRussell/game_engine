#include <player/spell_combination.h>
#include <core/logger.h>
#include <string.h>

#define MAX_COMBINATIONS 16
#define MAX_RECENT_SPELLS 10

static SpellCombination combinations[MAX_COMBINATIONS];
static u32 combination_count = 0;

static SpellType recent_spells[MAX_RECENT_SPELLS];
static u32 recent_spell_count = 0;
static f32 combination_window = 2.0f; // 2 seconds to complete a combination
static f32 combination_timer = 0.0f;

void spell_combination_system_init(void) {
    combination_count = 0;
    recent_spell_count = 0;
    combination_timer = 0.0f;

    // Define some combinations
    SpellCombination fireball_lightning = {
        .spells = {SPELL_FIREBALL, SPELL_LIGHTNING},
        .spell_count = 2,
        .resulting_spell = SPELL_FIREBALL // Placeholder for a real combo spell
    };
    combinations[combination_count++] = fireball_lightning;
}

void spell_combination_system_add_spell(PlayerMagicComponent* magic, SpellType spell) {
    if (recent_spell_count < MAX_RECENT_SPELLS) {
        recent_spells[recent_spell_count++] = spell;
        combination_timer = combination_window;
    }
}

void spell_combination_system_update(PlayerMagicComponent* magic, f32 delta_time) {
    if (combination_timer > 0) {
        combination_timer -= delta_time;
        if (combination_timer <= 0) {
            recent_spell_count = 0;
        }
    }

    if (recent_spell_count > 1) {
        for (u32 i = 0; i < combination_count; ++i) {
            SpellCombination* combo = &combinations[i];
            if (recent_spell_count == combo->spell_count) {
                bool match = true;
                for (u32 j = 0; j < combo->spell_count; ++j) {
                    if (recent_spells[j] != combo->spells[j]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    LOG_INFO("Spell combination triggered: %d", combo->resulting_spell);
                    // Reset recent spells
                    recent_spell_count = 0;
                    combination_timer = 0.0f;
                    
                    // TODO: Cast the resulting spell
                    // player_cast_spell(magic->player_system, combo->resulting_spell, target);
                    break;
                }
            }
        }
    }
}