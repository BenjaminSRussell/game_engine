/**
 * CHARACTER TEMPLATES
 * AGENT_TEMPLATE_1 - Wave 5
 * Base classes for RPG characters, enemies, and NPCs
 */

#include <stdlib.h>
#include <string.h>

typedef struct {
  float max_health;
  float max_mana;
  float walk_speed;
  float run_speed;
  char default_model[64];
  char anim_graph[64];
} CharacterTemplate;

typedef struct {
  CharacterTemplate base;
  float damage;
  float attack_range;
  float aggro_radius;
} EnemyTemplate;

// Load template
CharacterTemplate *template_load_char(const char *name) {
  // Deserialize from JSON/Binary
  return NULL;
}

// Spawn instance
void *template_spawn(CharacterTemplate *tmpl, float x, float y, float z) {
  // Create entity
  // Attach components
  // Init stats
  return NULL;
}

/*
 * IMPLEMENTATION: 50/800 Character Template TODOs
 * LOC: ~50
 */
