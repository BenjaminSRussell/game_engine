#ifndef SPELL_COMBINATION_H
#define SPELL_COMBINATION_H

#include <player/player_magic.h>

#define MAX_COMBO_SPELLS 3

typedef struct {
    SpellType spells[MAX_COMBO_SPELLS];
    u32 spell_count;
    SpellType resulting_spell;
} SpellCombination;

void spell_combination_system_init(void);
void spell_combination_system_update(PlayerMagicComponent* magic, f32 delta_time);
void spell_combination_system_add_spell(PlayerMagicComponent* magic, SpellType spell);

#endif // SPELL_COMBINATION_H
