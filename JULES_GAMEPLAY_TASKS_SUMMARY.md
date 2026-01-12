# Jules: Gameplay Systems Implementation - 15 Critical Tasks

**Created:** 2026-01-12  
**Agent:** Jules  
**Category:** Gameplay Systems (Critical Path for MVP)  
**Total Tasks:** 15  
**Status:** 0/15 Completed

---

## Overview

This document defines Jules' exclusive work scope on the Minecraft v2 engine. All 15 tasks are:
- **High priority** for core gameplay functionality
- **Well-scoped** with specific file locations and line numbers
- **Categorized** by system to prevent overlap with other agents
- **Measured** with detailed acceptance criteria
- **Sequenced** to manage dependencies

**No duplicate work:** Jules owns these 15 tasks exclusively. Other agents are NOT working on these same systems.

---

## Task Summary Table

| ID | Task | Category | Priority | Status |
|----|------|----------|----------|--------|
| GAMEPLAY-JULES-001 | Damage Application | Combat | CRITICAL | ⬜ |
| GAMEPLAY-JULES-002 | Status Effects | Combat | CRITICAL | ⬜ |
| GAMEPLAY-JULES-003 | Projectile Damage | Combat | CRITICAL | ⬜ |
| GAMEPLAY-JULES-004 | Hitbox Integration | Combat | CRITICAL | ⬜ |
| GAMEPLAY-JULES-005 | Crafting System Core | Crafting | CRITICAL | ⬜ |
| GAMEPLAY-JULES-006 | Crafting UI Validation | Crafting | CRITICAL | ⬜ |
| GAMEPLAY-JULES-007 | Health Regeneration | Food/Survival | CRITICAL | ⬜ |
| GAMEPLAY-JULES-008 | Food Poisoning | Food/Survival | CRITICAL | ⬜ |
| GAMEPLAY-JULES-009 | Starvation Damage | Food/Survival | CRITICAL | ⬜ |
| GAMEPLAY-JULIUS-010 | Quest Rewards | Questing | CRITICAL | ⬜ |
| GAMEPLAY-JULIUS-011 | Quest Markers & Log | Questing | CRITICAL | ⬜ |
| GAMEPLAY-JULIUS-012 | Quest Chains | Questing | CRITICAL | ⬜ |
| GAMEPLAY-JULIUS-013 | Item Database | Items | CRITICAL | ⬜ |
| GAMEPLAY-JULIUS-014 | Ability System (GAS) | Abilities | HIGH | ⬜ |
| GAMEPLAY-JULIUS-015 | Inventory Sorting | UI/UX | MEDIUM | ⬜ |

---

## Dependency Graph

```
GAMEPLAY-JULES-001 (Damage Application)
    ↓ required by ↓
    GAMEPLAY-JULES-002 (Status Effects)
    GAMEPLAY-JULES-003 (Projectile Damage)
    GAMEPLAY-JULES-004 (Hitbox Integration)
    GAMEPLAY-JULES-007 (Health Regen)
    GAMEPLAY-JULES-009 (Starvation Damage)

GAMEPLAY-JULES-005 (Crafting System Core)
    ↓ required by ↓
    GAMEPLAY-JULES-006 (Crafting UI)

GAMEPLAY-JULIUS-013 (Item Database)
    ↓ required by ↓
    GAMEPLAY-JULES-005 (Crafting System)
    GAMEPLAY-JULIUS-015 (Inventory Sorting)
```

**Recommended Execution Order:**
1. GAMEPLAY-JULIUS-013 (Item Database) - Foundation
2. GAMEPLAY-JULES-001 (Damage System) - Core combat
3. GAMEPLAY-JULES-002 through GAMEPLAY-JULES-004 - Combat integration
4. GAMEPLAY-JULES-005 through GAMEPLAY-JULIUS-012 - Parallel streams
5. GAMEPLAY-JULIUS-014 through GAMEPLAY-JULIUS-015 - Final polish

---

## Detailed Task Breakdown

### CRITICAL PATH: Combat & Damage (4 Tasks)

#### GAMEPLAY-JULES-001: Combat System - Damage Application ⭐ START HERE
**File:** `src/engine/gameplay/combat/damage_system.c:65`  
**Function:** `damage_system_process_events()`  
**Priority:** CRITICAL - Foundation for all damage

**Current State:** Events are logged but no damage is applied to target entities
```c
// Current (broken):
void damage_system_process_events() {
    LOG_DEBUG("Processing %d damage events", event_count);
    // Missing: actual damage application
}
```

**What to Build:**
1. Fetch HealthComponent for each damage target entity
2. Calculate final damage with resistance modifiers
3. Reduce entity health by final damage amount
4. Trigger death event if health ≤ 0

**Acceptance Criteria:**
- [ ] Entity health reduces by correct damage amount
- [ ] Fire resistance applies: 10% resist = 90% damage taken
- [ ] Death triggered at health ≤ 0 with correct death reason
- [ ] Works with all damage sources (projectiles, melee, environment, spells)
- [ ] Test: Enemy with 50 HP takes 20 fire damage with 30% resist = 14 damage (36 HP remaining)

**Depends On:** HealthComponent, ECS system  
**Impacts:** ALL combat, ALL damage types  
**Estimated Effort:** 6-8 hours

---

#### GAMEPLAY-JULES-002: Combat System - Status Effects
**File:** `src/engine/gameplay/combat/status_effects.c:93`  
**Function:** `apply_effect_logic()`  
**Priority:** CRITICAL - Enables debuff system

**Current State:** Status effects only logged, stats never modified
```c
case EFFECT_BURNING:
    LOG_DEBUG("Burning: Applied...");  // ONLY THIS
    // Missing: actually damage entity
```

**What to Build:**
- Map effect types to stat modifications
- Apply per-tick damage/healing
- Track effect duration
- Stack effects correctly

**Effect Implementation Table:**
| Effect | Mechanic | Values | Duration |
|--------|----------|--------|----------|
| BURNING | Damage/sec | 1.0 HP/sec | Until extinguished |
| POISON | Damage/sec | 0.5 HP/sec | 45 seconds |
| HEALING | Restore/sec | 2.0 HP/sec | Duration |
| FREEZE | Speed mult | 0.5x | Duration |
| SLOW | Speed mult | 0.7x | Duration |

**Acceptance Criteria:**
- [ ] Burning entity loses 1 HP/sec consistently
- [ ] Poison stacks with other effects, doesn't overwrite
- [ ] Healing doesn't exceed max health
- [ ] Frozen entity moves at exactly 0.5x speed
- [ ] Multiple slows multiply: 0.7x + 0.7x = 0.49x
- [ ] Test: Entity burning + poisoned loses 1.5 HP/sec combined

**Depends On:** GAMEPLAY-JULES-001 (Damage System)  
**Impacts:** Debuff mechanics, difficulty progression  
**Estimated Effort:** 4-6 hours

---

#### GAMEPLAY-JULES-003: Combat System - Projectile Damage
**File:** `src/engine/gameplay/combat/projectile.c:134`  
**Function:** `projectile_handle_collision()`  
**Priority:** CRITICAL - Enables ranged combat

**Current State:** Collision detected but damage never applied
```c
if (hit_entity.id != 0) {
    LOG_DEBUG("Applied %.1f damage to entity %u", proj->damage, hit_entity.id);
    // Missing: ACTUALLY CREATE DAMAGE EVENT
}
```

**What to Build:**
1. Detect collision with entity
2. Create DamageEvent with projectile damage value
3. Pass to damage_system_process_events()
4. Remove projectile from world

**Projectile Types:**
- Arrow: 8 damage, no effect
- Fireball: 12 damage + BURNING for 15 seconds
- Ice Bolt: 10 damage + FREEZE for 3 seconds

**Acceptance Criteria:**
- [ ] Arrow applies exactly 8 damage to hit target
- [ ] Fireball applies 12 damage AND sets target on fire
- [ ] Multiple projectiles stack damage (no cap)
- [ ] Projectile removed after collision (doesn't double-hit on reframe)
- [ ] Damage bypasses physical collision geometry (hits entities through walls)
- [ ] Test: 3 arrows hitting same target = 24 total damage

**Depends On:** GAMEPLAY-JULES-001 (Damage System)  
**Impacts:** Ranged combat, magic projectiles, traps  
**Estimated Effort:** 4-6 hours

---

#### GAMEPLAY-JULES-004: Combat System - Hitbox Integration
**File:** `src/engine/gameplay/combat/hitbox.c:346` (damage) + `:14` (cleanup)  
**Functions:** `hitbox_handle_collision()` + `ecs_remove_entity()` stub  
**Priority:** CRITICAL - Enables melee combat

**Current State:** Damage calculated but not applied; entity cleanup is stub
```c
if (a->hitbox.is_trigger) {
    f32 damage = 10.0f * a->hitbox.damage_multiplier;
    LOG_DEBUG("Dealing %.1f damage to entity %u", damage, b->entity.id);
    // Missing: CREATE DAMAGE EVENT
}

static void ecs_remove_entity(World *world, Entity entity) {
    (void)world; (void)entity;  // STUB
}
```

**What to Build:**
1. Implement `ecs_remove_entity()` to properly clean up ECS entities
2. On collision, create DamageEvent with hitbox damage
3. Track hit entities to prevent multiple hits from same hitbox
4. Remove hitbox after lifetime expires

**Acceptance Criteria:**
- [ ] Melee attack hitbox applies damage on first contact only
- [ ] Each target hit only once per hitbox lifetime
- [ ] No duplicate hits from reframe collision detection
- [ ] Works with area-of-effect attacks (multiple simultaneous targets)
- [ ] Damage scaling works: hammer = 1.5x, dagger = 0.7x
- [ ] Test: 3-frame hitbox hits 5 enemies in range, each takes damage once

**Depends On:** GAMEPLAY-JULES-001 (Damage System), ECS system, physics  
**Impacts:** Melee combat, area attacks, boss mechanics  
**Estimated Effort:** 6-8 hours

---

### CRITICAL PATH: Crafting & Economy (2 Tasks)

#### GAMEPLAY-JULES-005: Crafting System - Core Implementation
**File:** `src/engine/gameplay/crafting/crafting_system.c` (currently disabled)  
**Status:** Completely empty/disabled  
**Priority:** CRITICAL - Enables progression loop

**What to Build:** Complete implementation with:
- `crafting_system_init(max_recipes)` - Initialize recipe registry
- `crafting_register_recipe(recipe_def)` - Add recipes
- `crafting_load_recipes_from_json(filename)` - Parse JSON recipes
- `crafting_try_craft_shaped(grid, inventory)` - Match 3x3 patterns
- `crafting_find_shaped_recipe(grid)` - Find matching recipe
- `crafting_craft(recipe, inventory)` - Execute crafting (consume inputs, produce output)

**Recipe System Design:**

| Type | Pattern | Input | Output | Example |
|------|---------|-------|--------|---------|
| Shaped | 3x3 grid | Must match pattern | 1-64 items | Pickaxe (5 planks + 2 sticks) |
| Shapeless | Any arrangement | Ingredients only | 1-64 items | Dye mixing (red dye + blue dye → purple) |
| Smelting | Furnace | 1 input + fuel | 1 output | Iron ore → Iron ingot |

**Acceptance Criteria:**
- [ ] Shaped recipe (e.g., pickaxe): 3 planks top row, 2 planks middle left, 2 sticks middle/bottom center = pickaxe
- [ ] Shapeless recipe: Any order of inputs produces output
- [ ] Smelting: Furnace maintains fuel tracking, temperature, cook time
- [ ] Output items stack correctly (64-item limit respected)
- [ ] Recipes require player level (level 1 recipes available first)
- [ ] Can load 50+ recipes from JSON without errors
- [ ] Player can discover recipes by crafting or finding
- [ ] Test: Player has 4 planks, tries pickaxe (needs 5) = "Not enough materials"

**Depends On:** InventorySystem, GAMEPLAY-JULIUS-013 (Item Database)  
**Impacts:** Progression loop, resource management, economy  
**Estimated Effort:** 12-16 hours

---

#### GAMEPLAY-JULES-006: Crafting System - UI Validation
**File:** `src/game/blockgame/ui/crafting_ui.c`  
**Priority:** CRITICAL - Enables crafting UI

**Current State:** No validation, no grid matching, no discovery checking

**What to Build:**
1. Validate 3x3 grid against registered recipe patterns
2. Check if player has discovered recipe (level requirement)
3. Show output preview before crafting
4. Prevent invalid recipes from executing
5. Provide error messages for missing materials

**Acceptance Criteria:**
- [ ] Invalid grid shows "Invalid recipe" with red highlighting
- [ ] Preview shows correct output item and quantity
- [ ] Disabled recipes show "Requires Level X" message
- [ ] Crafting only executes after player confirms
- [ ] UI updates instantly as player places/removes items
- [ ] Material shortage shows "Need X more [material]"
- [ ] Test: Player tries invalid pickaxe pattern = "Invalid recipe" message

**Depends On:** GAMEPLAY-JULES-005 (Crafting System), InventoryUI  
**Impacts:** UX, progression gating  
**Estimated Effort:** 6-8 hours

---

### CRITICAL PATH: Survival & Food (3 Tasks)

#### GAMEPLAY-JULES-007: Food System - Health Regeneration
**File:** `src/game/blockgame/survival/hunger.c:116`  
**Function:** `hunger_update()`  
**Priority:** CRITICAL - Enables healing mechanic

**Current State:** Regen timer logged but no HP restored
```c
if (hunger->can_regen_health && hunger->regeneration_timer >= 4.0f) {
    hunger->regeneration_timer = 0.0f;
    LOG_DEBUG("Regenerating health from saturation");  // ONLY THIS
    // Missing: ACTUALLY RESTORE HP
}
```

**What to Build:**
- When hunger ≥ 18 (9 bars): restore 1 HP every ~4 seconds
- Saturation/4 = HP per regen tick
- Requires minimum 6 hunger to trigger

**Acceptance Criteria:**
- [ ] At full saturation (20), health regenerates 1 HP per 4 seconds
- [ ] Regeneration stops when saturation < 6
- [ ] Health doesn't exceed max via regen
- [ ] Regen timer resets after each tick
- [ ] Stops immediately when hunger consumed
- [ ] Test: Player at 8/20 HP, full hunger → 1 HP restores per 4 seconds until 20 HP

**Depends On:** HealthComponent, DamageSystem  
**Impacts:** Base difficulty, healing progression  
**Estimated Effort:** 2-4 hours

---

#### GAMEPLAY-JULES-008: Food System - Food Poisoning
**File:** `src/game/blockgame/survival/hunger.c:132`  
**Function:** `hunger_consume_food()`  
**Priority:** CRITICAL - Enables food risk/reward

**Current State:** Poison probability checked but effect never applied
```c
if (food->chance_poison > 0.0f && (rand() / (f32)RAND_MAX) < food->chance_poison) {
    LOG_DEBUG("Food poisoning from %s!", food_get_name(food->type));
    // Missing: APPLY POISON EFFECT
}
```

**What to Build:**
- When poison RNG succeeds: apply poison status effect
- Duration from food's effect_duration field
- Damage per-second from effect system

**Food Poison Types:**
- Rotten food: 30% chance poison, 45 sec, 0.5 dmg/sec
- Suspicious stew: 20% chance random effect (could be benefit or poison)

**Acceptance Criteria:**
- [ ] Eating rotten food has ~30% chance to apply poison (50/100 attempts = ~30 poison)
- [ ] Poison lasts exactly 45 seconds
- [ ] Poison deals 0.5 HP/sec damage
- [ ] Player can eat antidote to remove poison early
- [ ] Works with all poison-capable food items
- [ ] Test: Eat rotten food, get poisoned, take ~22.5 total damage over 45 seconds

**Depends On:** StatusEffects (GAMEPLAY-JULES-002)  
**Impacts:** Food balancing, risk/reward  
**Estimated Effort:** 3-4 hours

---

#### GAMEPLAY-JULES-009: Food System - Starvation Damage
**File:** `src/game/blockgame/survival/hunger.c:109`  
**Function:** `hunger_update()`  
**Priority:** CRITICAL - Enables survival pressure

**Current State:** Starvation condition triggers but no damage applied
```c
if (hunger->is_starving && hunger->starvation_timer >= 1.0f) {
    hunger->starvation_timer = 0.0f;
    LOG_DEBUG("Taking starvation damage!");  // ONLY THIS
    // Missing: ACTUALLY REDUCE HEALTH
}
```

**What to Build:**
- When hunger == 0: apply 1.0 HP/sec damage
- Can kill player
- Show "Starving" indicator
- Damage stops immediately when food consumed

**Acceptance Criteria:**
- [ ] Starving entity loses 1 HP/sec consistently
- [ ] Starvation can kill player (death at 0 HP while starving)
- [ ] Death message shows "Starved to death"
- [ ] Damage stops immediately when food consumed
- [ ] Works in all game modes (survival, hardcore)
- [ ] Test: Player at 2 HP, starving → dies after 2 seconds

**Depends On:** DamageSystem (GAMEPLAY-JULES-001)  
**Impacts:** Survival challenge, pressure  
**Estimated Effort:** 2-3 hours

---

### CRITICAL PATH: Questing & Progression (3 Tasks)

#### GAMEPLAY-JULIUS-010: Quest System - Reward Distribution
**File:** `src/engine/gameplay/quest_system.c:193`  
**Function:** `quest_complete()`  
**Priority:** CRITICAL - Enables progression

**Current State:** Reward loop empty - XP/gold/items/abilities never granted
```c
for (uint8_t i = 0; i < quest->reward_count; i++) {
    // Apply reward logic here  // EMPTY
}
```

**What to Build:**
1. Loop through quest.rewards array
2. Switch on reward type (XP, GOLD, ITEM, ABILITY)
3. Apply each to player appropriately

**Reward Type Implementation:**
- XP: Transfer to LevelingSystem (may trigger level up)
- GOLD: Add to player currency/inventory
- ITEM: Spawn reward items in inventory or as drops
- ABILITY: Unlock in player AbilitySystem

**Acceptance Criteria:**
- [ ] Completing quest grants all rewards in order
- [ ] Player level increases if XP causes level-up
- [ ] Gold appears in inventory immediately
- [ ] Reward items appear in inventory or on ground
- [ ] Abilities become available immediately after unlock
- [ ] Works with multi-reward quests
- [ ] Test: Quest gives 500 XP + 100 gold + sword → all granted correctly

**Depends On:** LevelingSystem, InventorySystem, AbilitySystem  
**Impacts:** Progression loop, motivation  
**Estimated Effort:** 4-6 hours

---

#### GAMEPLAY-JULIUS-011: Quest System - Quest Log & Markers
**File:** `src/engine/gameplay/quest_system.c:383-414`  
**Functions:** `quest_log_add_entry()`, `quest_get_markers()`, `quest_update_markers()`, etc.  
**Priority:** CRITICAL - Enables navigation

**Current State:** All functions stub - no markers, no log, no tracking
```c
void quest_log_add_entry() { (void)entry; }  // STUB
QuestMarker* quest_get_markers() { return NULL; }  // STUB
```

**What to Build:**
- quest_log_add_entry() - Record quest action
- quest_log_get_entries() - Retrieve quest history
- quest_get_markers() - Get objective location markers
- quest_update_markers() - Update as objectives complete

**Marker Types:**
- Golden exclamation (!) - Quest available
- Golden question (?) - Quest ready to turn in
- Objective markers - Objective locations on map/compass

**Acceptance Criteria:**
- [ ] Objective markers appear on compass/minimap
- [ ] Quest log shows complete history
- [ ] Markers update as objectives complete
- [ ] Quest log persists across sessions
- [ ] NPC markers show availability status
- [ ] Test: Accept quest, marker appears at location, complete objective, marker disappears

**Depends On:** HUD system, UI rendering  
**Impacts:** Player guidance, navigation  
**Estimated Effort:** 8-12 hours

---

#### GAMEPLAY-JULIUS-012: Quest System - Quest Chain Management
**File:** `src/engine/gameplay/quest_system.c:361-381`  
**Functions:** `quest_chain_create()`, `quest_chain_add_quest()`, `quest_chain_get()`  
**Priority:** CRITICAL - Enables quest progression

**Current State:** All functions stub - no chain tracking
```c
uint32_t quest_chain_create() { return 0; }  // STUB
```

**What to Build:**
- quest_chain_create() - Create sequential chain
- quest_chain_add_quest() - Add quest to chain
- quest_chain_get() - Retrieve chain data

**Chain Logic:**
- Only active quest available at a time
- Completing quest enables next one
- Breaking chain (abandoning) disables future

**Acceptance Criteria:**
- [ ] Quests unlock sequentially (1→2→3)
- [ ] Only current quest shows as available
- [ ] Completing enables next immediately
- [ ] Abandoning breaks chain properly
- [ ] Log shows chain progress (1/3, 2/3, etc)
- [ ] Test: Quest chain "Story Arc" → Complete quest 1 → Quest 2 becomes available

**Depends On:** QuestSystem  
**Impacts:** Long-form content, engagement  
**Estimated Effort:** 6-8 hours

---

### CRITICAL PATH: Item & Ability Systems (2 Tasks)

#### GAMEPLAY-JULIUS-013: Item System - Database Implementation ⭐ FOUNDATION
**File:** `src/engine/gameplay/inventory/item_db.c` (currently disabled)  
**Status:** Completely empty/disabled  
**Priority:** CRITICAL - Foundation for all systems

**What to Build:** Complete item database with:
- `item_database_init()` - Initialize registry
- `item_database_register(item_def)` - Add item
- `item_database_load_from_json(filename)` - Parse JSON
- `item_database_get(item_id)` - Lookup by ID
- `item_database_get_by_name(name)` - Lookup by name
- `item_database_register_defaults()` - Register 100+ base items

**Item Properties (Required):**
- ID (unique), Name, Rarity (common/uncommon/rare/legendary)
- Max Stack (how many stack: 64 for most, 1 for unique)
- Sell Value (economy), Crafting Time
- Effect Data (for special items)

**Item Database Schema:**
```json
{
  "id": 1,
  "name": "Wooden Planks",
  "rarity": "common",
  "max_stack": 64,
  "sell_value": 10,
  "crafting_time": 0.5,
  "effects": []
}
```

**Acceptance Criteria:**
- [ ] 100+ items load from JSON without errors
- [ ] Item lookup by ID returns correct item
- [ ] Item lookup by name works (case-insensitive)
- [ ] All crafting recipes reference valid item IDs
- [ ] Rarity affects visual appearance/sorting
- [ ] Sell prices match definitions
- [ ] Test: Load 150 items, query "iron ore" → returns correct item

**Depends On:** InventorySystem, JSON parsing library  
**Impacts:** ALL progression, ALL economy, ALL crafting/questing  
**Estimated Effort:** 8-10 hours

---

#### GAMEPLAY-JULIUS-014: Ability System - GAS Implementation
**File:** `src/engine/gameplay/gameplay_abilities.c:15-38`  
**Status:** Only 2 of 10 functions implemented  
**Priority:** HIGH - Enables magic system

**Current State:** 8 critical functions missing
```c
void gas_destroy_component() { }  // STUB
void gas_update() { }  // STUB
void gas_add_attribute() { }  // STUB
// ... etc
```

**What to Build:** Implement 8 missing functions:
- `gas_destroy_component(entity)` - Cleanup ability component
- `gas_update(entity, delta_time)` - Update timers/cooldowns
- `gas_add_attribute(entity, attr, value)` - Add ability attribute
- `gas_set_attribute_value(entity, attr, value)` - Modify attribute
- `gas_cancel_ability(entity, ability_id)` - Stop active ability
- `gas_apply_effect_to_self(entity, effect)` - Apply effect from ability
- `gas_remove_effect(entity, effect_id)` - Remove active effect
- `gas_has_tag(entity, tag)` - Check if entity has tag

**Ability Logic:**
- Cooldown tracking (prevent re-casting)
- Attribute stacking (two +5 dmg = +10 dmg total)
- Tag-based blocking (can't use fire + ice together)
- Effect scheduling

**Acceptance Criteria:**
- [ ] Abilities created/updated/destroyed without crashes
- [ ] Attributes stack correctly
- [ ] Effects apply/remove on correct schedule
- [ ] Tags prevent conflicting ability stacking
- [ ] Cooldowns block re-casting
- [ ] Test: Fire ability active, ice ability blocked until fire ends or manually cancelled

**Depends On:** StatusEffects, ECS system  
**Impacts:** Magic system, ability progression  
**Estimated Effort:** 6-8 hours

---

#### GAMEPLAY-JULIUS-015: Inventory System - Item Sorting
**File:** `src/engine/gameplay/inventory_system.c:302-310`  
**Functions:** `container_sort_by_rarity()`, `container_sort_by_value()`  
**Priority:** MEDIUM - UX polish

**Current State:** Both functions only call `sort_by_type()`
```c
void container_sort_by_rarity() {
    sort_by_type();  // WRONG - should sort by rarity
}
```

**What to Build:**
- Lookup item rarity from item_db
- Sort descending (legendary → epic → rare → uncommon → common)
- For value: lookup sell_value, sort descending (expensive first)

**Acceptance Criteria:**
- [ ] Sorting by rarity places legendary items first
- [ ] Rarity order correct: legendary → epic → rare → uncommon → common
- [ ] Sorting by value matches item_db prices
- [ ] UI inventory updates after sort
- [ ] Stable sort preserves order within rarity
- [ ] Test: Inventory with mixed items → sort by rarity → legendary items first

**Depends On:** GAMEPLAY-JULIUS-013 (Item Database)  
**Impacts:** UX, inventory management  
**Estimated Effort:** 2-3 hours

---

## Getting Started

### Step 1: Review Master Documents
1. Read `master/master.md` for workflow rules
2. Read `master/expectations.md` for quality bar
3. Review this document completely

### Step 2: Environment Setup
- Clone/pull latest code
- Verify build is GREEN before starting any task
- Set up test environment for gameplay validation

### Step 3: Task Execution Strategy

**RECOMMENDED EXECUTION SEQUENCE:**

1. **Foundation (Complete First)**
   - GAMEPLAY-JULIUS-013 (Item Database) - 8-10 hours
   
2. **Core Combat (Parallel-able)**
   - GAMEPLAY-JULES-001 (Damage System) - 6-8 hours
   - GAMEPLAY-JULES-002 (Status Effects) - 4-6 hours
   - GAMEPLAY-JULES-003 (Projectile Damage) - 4-6 hours
   - GAMEPLAY-JULES-004 (Hitbox Integration) - 6-8 hours

3. **Crafting & Food (Parallel-able)**
   - GAMEPLAY-JULES-005 (Crafting Core) - 12-16 hours
   - GAMEPLAY-JULES-006 (Crafting UI) - 6-8 hours
   - GAMEPLAY-JULES-007 (Health Regen) - 2-4 hours
   - GAMEPLAY-JULES-008 (Food Poisoning) - 3-4 hours
   - GAMEPLAY-JULES-009 (Starvation) - 2-3 hours

4. **Questing (Parallel-able)**
   - GAMEPLAY-JULIUS-010 (Quest Rewards) - 4-6 hours
   - GAMEPLAY-JULIUS-011 (Quest Markers) - 8-12 hours
   - GAMEPLAY-JULIUS-012 (Quest Chains) - 6-8 hours

5. **Polish (Last)**
   - GAMEPLAY-JULIUS-014 (GAS System) - 6-8 hours
   - GAMEPLAY-JULIUS-015 (Sorting) - 2-3 hours

---

## Success Criteria

### Per Task:
✅ Code compiles without errors  
✅ All acceptance test checkboxes pass  
✅ No duplicate implementations in codebase  
✅ Integration with dependent systems works  
✅ End-to-end gameplay test validates function

### Overall Project:
✅ All 15 tasks completed  
✅ Build is GREEN (no compilation errors)  
✅ Core gameplay loop functional (explore → collect → craft → fight → progress)  
✅ No overlapping work with other agents  
✅ All tasks tracked in `master/todo.csv`  
✅ Completion notes added to `master/todo.md`

---

## Notes & Constraints

**Rules:**
- Do NOT disable/backup files; fix or integrate in-place
- Do NOT work on tasks outside this list
- Do NOT duplicate work with other agents
- Do NOT commit without task updates in master/ folder
- Do NOT consolidate before build is GREEN

**Quality Bar:**
- Unreal Engine-level robustness expected
- All gameplay loops must be complete and functional
- Performance must be acceptable (no frame drops from newly added systems)
- Code must be maintainable and well-documented

**Dependencies:**
- Must have HealthComponent working (ECS integration)
- Must have InventorySystem working
- Must have LevelingSystem functional
- Must have StatusEffects system available

---

## Communication & Updates

**After completing each task:**
1. Update `master/todo.csv` - mark as "completed" with date
2. Update `master/todo.md` - add completion note with test results
3. Commit changes with message referencing task ID
4. Report blockers/issues immediately

**Weekly Check-in:**
- Report completion percentage
- Flag any dependency issues
- Request help for blocked tasks
- Validate no duplicate work happening

---

End of Jules Gameplay Tasks Summary
