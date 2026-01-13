# Compilation Errors - Detailed Reference Guide

## Overview
This document provides a complete reference of all 23 compilation errors found in the Minecraft v2 codebase, with specific fixes for each.

**Date**: January 12, 2026
**Build Configuration**: CMake 4.1.2, macOS/Metal backend
**Error Count**: 23 critical errors across 4 files

---

## ERROR SUMMARY TABLE

| Error ID | Type | File | Line | Severity | Fix Effort |
|----------|------|------|------|----------|-----------|
| E1-E8 | Logging API | player_food.c | 98-299 | CRITICAL | 2-3 hrs |
| E9-E19 | ECS API | npc.c | 262-504 | CRITICAL | 4-6 hrs |
| E20 | Missing Header | pathfinding_cache_advanced.c | 6 | HIGH | 30 min |
| E21 | Type Definition | player_vehicle.h | 82 | HIGH | 30 min |
| E22 | Macro Conflict | game_common.h/math/types.h | 109/452 | MEDIUM | 30 min |
| E23 | Vector Constructor | npc.c | 398-459 | HIGH | 1 hr |

---

## DETAILED ERROR BREAKDOWN

### ERROR GROUP 1: LOGGING API MISALIGNMENT (E1-E8)

**Affected File**: `src/game/blockgame/player/player_food.c`

**Root Cause**: Logging system refactored to require `LogCategory` parameter, but old code uses old format.

#### Current Logger API (NEW - Correct)
```c
// Location: src/engine/include/core/logging/unified_logger.h:218-222
#define LOG_DEBUG(category, format, ...) \
    logger_log(LOG_LEVEL_DEBUG, category, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_INFO(category, format, ...) \
    logger_log(LOG_LEVEL_INFO, category, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

void logger_log(LogLevel level, LogCategory category, const char* file, int line,
                const char* function, const char* format, ...);
```

#### Old Code (WRONG)
```c
// player_food.c:98
LOG_DEBUG("Player started eating %s (duration: %.2fs)", item->base.name, duration);
```

#### New Code (CORRECT)
```c
// player_food.c:98
LOG_DEBUG(LOG_CATEGORY_GAMEPLAY, "Player started eating %s (duration: %.2fs)", item->base.name, duration);
```

#### Errors in Detail

| Error # | Line | Old Code | Fixed Code | Category |
|---------|------|----------|-----------|----------|
| E1 | 98 | `LOG_DEBUG("Player started eating...")` | `LOG_DEBUG(LOG_CATEGORY_GAMEPLAY, "Player started...")` | String format mismatch |
| E2 | 131 | `LOG_INFO("Food particle effect...")` | `LOG_INFO(LOG_CATEGORY_GAMEPLAY, "Food particle...")` | String format mismatch |
| E3 | 143 | `LOG_DEBUG("Eating cancelled")` | `LOG_DEBUG(LOG_CATEGORY_GAMEPLAY, "Eating cancelled")` | Missing category |
| E4 | 197 | `LOG_INFO("Player ate %s...")` | `LOG_INFO(LOG_CATEGORY_GAMEPLAY, "Player ate...")` | String format mismatch |
| E5 | 216 | `LOG_INFO("Food effect applied")` | `LOG_INFO(LOG_CATEGORY_GAMEPLAY, "Food effect...")` | Missing category |
| E6 | 220 | `LOG_INFO("Food effect applied: Regeneration")` | `LOG_INFO(LOG_CATEGORY_GAMEPLAY, "Food effect...")` | Missing category |
| E7 | 239 | `LOG_INFO("Player drank Milk...")` | `LOG_INFO(LOG_CATEGORY_GAMEPLAY, "Player drank...")` | Missing category |
| E8 | 299 | `LOG_DEBUG("Item in slot %d has spoiled")` | `LOG_DEBUG(LOG_CATEGORY_GAMEPLAY, "Item in slot...")` | Missing category |

#### Solution

**Step 1**: Define LogCategory enum (add to `src/engine/include/core/logging/unified_logger.h`)
```c
typedef enum {
    LOG_CATEGORY_CORE,
    LOG_CATEGORY_RENDERING,
    LOG_CATEGORY_PHYSICS,
    LOG_CATEGORY_GAMEPLAY,
    LOG_CATEGORY_AI,
    LOG_CATEGORY_AUDIO,
    LOG_CATEGORY_NETWORK,
    LOG_CATEGORY_ASSET,
    LOG_CATEGORY_UI,
    LOG_CATEGORY_EDITOR,
    LOG_CATEGORY_SCRIPTING,
    LOG_CATEGORY_ANIMATION,
    LOG_CATEGORY_PLATFORM,
    LOG_CATEGORY_COUNT
} LogCategory;
```

**Step 2**: Update `player_food.c` - all 8 locations
```bash
# Find all occurrences
grep -n "LOG_DEBUG\|LOG_INFO" src/game/blockgame/player/player_food.c

# Replace pattern (verify each one)
# OLD: LOG_DEBUG("text")
# NEW: LOG_DEBUG(LOG_CATEGORY_GAMEPLAY, "text")

# OLD: LOG_INFO("text")
# NEW: LOG_INFO(LOG_CATEGORY_GAMEPLAY, "text")
```

**Step 3**: Check for similar issues in other files
```bash
# Find all LOG_* calls that might have wrong format
grep -r "LOG_DEBUG\|LOG_INFO\|LOG_ERROR\|LOG_WARN" src/game/ --include="*.c" | \
  grep -v "LOG_CATEGORY" | \
  head -20
```

---

### ERROR GROUP 2: ECS API MIGRATION (E9-E19)

**Affected File**: `src/engine/ai/legacy/npc.c`

**Root Cause**: ECS system was refactored with new API, but legacy NPC code still uses old API.

#### Current ECS APIs (Investigation Needed)

First, check actual API in:
- `src/engine/include/ecs/` directory
- Look for: `ecs_get_component()`, `ecs_query()`, etc.

**Assumed Modern API** (based on error patterns):
```c
// NEW API (assumed):
void *ecs_get_component(ECS *ecs, EntityID entity, ComponentTypeID component_type);
ECSQuery *ecs_query_create(ECS *ecs, ...component_types...);
void ecs_query_free(ECSQuery *query);

// OLD API (what npc.c uses):
void *world_get_component(ECS *ecs, EntityID entity, u32 component_id);
void world_query_free(Query *query);
```

#### Errors in Detail

| Error # | Line | Issue | Component |
|---------|------|-------|-----------|
| E9 | 262-263 | `world_get_component()` undefined | NPC lookup |
| E10 | 269 | `npc_ai_update_single()` has wrong signature | Function arity |
| E11 | 272 | `world_query_free()` undefined | Query cleanup |
| E12 | 281-282 | `world_get_component()` undefined | Transform lookup |
| E13 | 382-383 | `world_get_component()` undefined | RigidBody lookup |
| E14 | 395-396 | `world_get_component()` undefined | RigidBody lookup |
| E15 | 398 | `vec3()` constructor not recognized | Vector math |
| E16 | 411 | `vec3()` constructor not recognized | Vector math |
| E17 | 459 | `vec3()` constructor not recognized | Vector math |
| E18 | 470 | Type mismatch: `Entity` vs `EntityID` | Entity type |
| E19 | 496-504 | Forward declared struct incomplete | PlayerSystem |

#### Solution Option A: Compatibility Layer (Recommended for Quick Fix)

Create `src/engine/ecs/compat_layer.h`:
```c
#ifndef ECS_COMPAT_LAYER_H
#define ECS_COMPAT_LAYER_H

#include "ecs/world.h"
#include "ecs/components.h"

// Compatibility shims for legacy code
static inline void *world_get_component(ECS *ecs, EntityID entity, u32 component_id) {
    // Map old component IDs to new types
    // This needs to be filled based on actual ID→Type mapping
    ComponentTypeID type = (ComponentTypeID)component_id;
    return ecs_get_component(ecs, entity, type);
}

static inline void world_query_free(void *query) {
    if (query) {
        ecs_query_free((ECSQuery *)query);
    }
}

// Vector math functions (if not already available)
#include "math/vectors.h"
// vec3() should already work if math/vectors.h is included

#endif
```

Add to `npc.c`:
```c
#include "ecs/compat_layer.h"  // Add near top of includes
```

#### Solution Option B: Full Migration (Long-term Better Solution)

Migrate `npc.c` to use modern ECS directly:

```c
// OLD (legacy):
NPCComponent *npc = (NPCComponent *)world_get_component(
    system->ecs, entity, NPC_COMPONENT_ID);

// NEW (modern):
NPCComponent *npc = ecs_get_component(
    &system->ecs, entity, COMPONENT_TYPE_NPC);
```

Steps:
1. Document actual ECS API (run `grep -r "ecs_get_component\|ecs_query" src/engine --include="*.h" | head -10`)
2. Create migration guide mapping old IDs to new types
3. Systematically update all function calls
4. Test NPC behavior

#### Fix for vec3() Constructor Error

This is likely just a missing include. Add to `npc.c`:
```c
#include "math/vectors.h"  // or wherever vec3 is defined
```

Check current includes in `npc.c` to find correct header.

#### Fix for PlayerSystem Forward Declaration

The issue is that `PlayerSystem` is forward declared but not fully defined where needed.

**Option 1**: Include full header
```c
// Instead of:
extern struct PlayerSystem *g_player_system;

// Use:
#include "player/player_system.h"
extern PlayerSystem *g_player_system;
```

**Option 2**: Use opaque pointer pattern (if circular includes prevent Option 1)
```c
// In npc.c where PlayerSystem is used:
if (g_player_system && g_player_system->player) {
    // This requires full definition
    // Either include the header or restructure code
}
```

---

### ERROR E20: MISSING HEADER FILE

**File**: `src/engine/ai/pathfinding/pathfinding_cache_advanced.c:6`
**Error**: `#include "pathfinding/pathfinding_cache_advanced.h"` file not found

#### Solution

1. **Check if file exists**:
```bash
find /Users/benjaminrussell/Desktop/"Minecraft v2" -name "pathfinding_cache_advanced.h" 2>/dev/null
ls -la src/engine/ai/pathfinding/ | grep cache_advanced
```

2. **If file exists but not found**:
   - Add to CMakeLists.txt include paths:
   ```cmake
   include_directories(src/engine/ai/pathfinding)
   ```

3. **If file doesn't exist**:
   - Create it: `touch src/engine/ai/pathfinding/pathfinding_cache_advanced.h`
   - Add header guard and forward declarations based on .c file usage

4. **If file is intentionally disabled**:
   - Check if it's in a `#if 0` block
   - Add conditional compilation in CMakeLists.txt

---

### ERROR E21: MISSING TYPE DEFINITION

**File**: `src/game/blockgame/include/player/player_vehicle.h:82`
**Error**: `VehicleDamageType` is not defined

#### Current Code (WRONG)
```c
// player_vehicle.h:82
void player_vehicle_damage_typed(PlayerSystem *system, f32 damage, VehicleDamageType damage_type);
                                                                 ^ NOT DEFINED
```

#### Solution

Add to `src/game/blockgame/include/player/player_vehicle.h`:

```c
#ifndef VEHICLE_DAMAGE_TYPE_H
#define VEHICLE_DAMAGE_TYPE_H

#include "game_common.h"

typedef enum {
    VEHICLE_DAMAGE_IMPACT = 0,      // Vehicle hit something
    VEHICLE_DAMAGE_COLLISION,        // Collision with object
    VEHICLE_DAMAGE_LAVA,             // Exposed to lava
    VEHICLE_DAMAGE_WATER_DAMAGE,     // Special water damage
    VEHICLE_DAMAGE_FALL,             // Fall damage
    VEHICLE_DAMAGE_BURNING,          // Fire damage
    VEHICLE_DAMAGE_EXPLOSION,        // Explosion damage
    VEHICLE_DAMAGE_CUSTOM,           // Custom damage type
    VEHICLE_DAMAGE_TYPE_COUNT
} VehicleDamageType;

#endif
```

Place this enum before the function declaration that uses it.

---

### ERROR E22: MACRO REDEFINITION CONFLICT

**Files**:
- `src/game/blockgame/include/game_common.h:109`
- `src/engine/include/core/math/types.h:452`

**Conflict**:
```c
// game_common.h:109
#define PI_2 (3.14159265358979323846f * 2.0f)  // = 2π ≈ 6.283

// math/types.h:452
#define PI_2 1.57079632679489661923f  // = π/2 ≈ 1.571
```

These are DIFFERENT VALUES! This will cause incorrect math calculations.

#### Solution

1. **Rename for clarity**:
```c
// In math/types.h (or new file: math/constants.h)
#define PI             3.14159265358979323846f
#define PI_DOUBLE      (PI * 2.0f)      // 2π
#define PI_HALF        (PI * 0.5f)      // π/2
#define PI_QUARTER     (PI * 0.25f)     // π/4
#define TAU            (PI * 2.0f)      // Alternative name for 2π

// Keep old names as deprecated fallbacks:
#define PI_2 PI_DOUBLE  // DEPRECATED - use PI_DOUBLE
```

2. **Update game_common.h**:
```c
// Remove the old definition
// #define PI_2 (3.14159265358979323846f * 2.0f)

// Import from central location:
#include "core/math/constants.h"
```

3. **Find and update all usages**:
```bash
grep -r "PI_2" src/game src/engine --include="*.c" --include="*.h" | wc -l
grep -r "PI_2" src/game src/engine --include="*.c" --include="*.h" | head -20
```

---

### ERROR E23: VECTOR CONSTRUCTOR ISSUES

**File**: `src/engine/ai/legacy/npc.c` (multiple lines: 398, 411, 459)

**Error**: `vec3()` not recognized as valid expression/type

#### Current Code (WRONG)
```c
// Line 398
rigid_body_set_velocity(rb_comp->body, vec3(0, 0, 0));
                                       ^ Not a type or function

// Line 411
Vec3 player_pos = vec3(0, 0, 0);
                  ^ Not found

// Line 459
...vec3((rand() % 20) - 10, 0, (rand() % 20) - 10)...
     ^ Not found
```

#### Solution

**Check what's included**:
```c
// Look at current includes in npc.c
head -50 src/engine/ai/legacy/npc.c | grep "#include"
```

**Add missing include**:
```c
// If vec3 is in math/vectors.h:
#include "math/vectors.h"

// Or if it's defined elsewhere:
#include "core/math/vec3.h"
// or
#include "math/vec_types.h"
// etc.
```

**Check vec3 definition** (find correct spelling):
```bash
grep -r "vec3\|Vec3" src/engine/include --include="*.h" | grep -i "define\|typedef" | head -10
```

**Possible solutions**:

1. If `vec3` is a macro:
```c
#define vec3(x, y, z) ((Vec3){(x), (y), (z)})
// Make sure it's included
```

2. If it's a function:
```c
Vec3 vec3(f32 x, f32 y, f32 z);
// Declaration should be in header, include that header
```

3. If it's a constructor (less likely in C):
```c
// Use struct initializer instead:
Vec3 pos = {0.0f, 0.0f, 0.0f};
// Instead of vec3(0, 0, 0)
```

---

## MACRO REDEFINITION DETAIL

**Warning Found**:
```
/Users/benjaminrussell/Desktop/Minecraft v2/src/game/blockgame/include/inventory/inventory.h:58
In file included from ...game_common.h:109
#define PI_2 (3.14159265358979323846f * 2.0f)
                ^

From /Users/benjaminrussell/Desktop/Minecraft v2/src/engine/include/core/logging/../math/types.h:452
#define PI_2 1.57079632679489661923f
                ^

note: previous definition is here
```

This is CRITICAL because:
- If you use `PI_2` in math calculations, you might get wrong answer
- Depends on which include happens last
- Search and replace `PI_2` references to verify they're using the right value

---

## VERIFICATION CHECKLIST

After applying all fixes, verify with:

```bash
cd "/Users/benjaminrussell/Desktop/Minecraft v2"
rm -rf build
mkdir build
cd build
cmake ..
make -j4 2>&1 | tee build.log

# Check results:
grep -i "error:" build.log | wc -l    # Should be 0
grep -i "warning:" build.log | wc -l  # Should be <20
tail -20 build.log  # Should show success
```

---

## FILE-BY-FILE SUMMARY

### src/game/blockgame/player/player_food.c
**Errors**: E1-E8 (Logging macros)
**Fix Time**: 15 minutes
**Fix Type**: Add category parameter to all LOG_* calls
**Impact**: Affects gameplay logging throughout game systems

### src/engine/ai/legacy/npc.c
**Errors**: E9-E19 (ECS API + vectors + forward declarations)
**Fix Time**: 1-2 hours
**Fix Type**: API migration or compatibility layer + add includes
**Impact**: NPC system will be functional

### src/engine/ai/pathfinding/pathfinding_cache_advanced.c
**Errors**: E20 (Missing header)
**Fix Time**: 10 minutes
**Fix Type**: Locate or create header file
**Impact**: Pathfinding cache system will compile

### src/game/blockgame/include/player/player_vehicle.h
**Errors**: E21 (Missing enum)
**Fix Time**: 5 minutes
**Fix Type**: Add VehicleDamageType enum definition
**Impact**: Vehicle damage system will compile

### src/game/blockgame/include/game_common.h + math/types.h
**Errors**: E22 (Macro conflict)
**Fix Time**: 15 minutes
**Fix Type**: Consolidate PI constants
**Impact**: Math calculations will be correct

**Total Fix Time**: 2-3 hours
**Expected Result**: Full compilation success

---

## NEXT STEPS

1. **Review this document** - Understand each error
2. **Apply fixes in order** - Start with most impactful errors
3. **Rebuild after each fix** - Verify progress
4. **Document any additional errors** - There may be cascading errors
5. **Run full test suite** - Ensure no regressions

---

**Document Version**: 1.0
**Last Updated**: 2026-01-12
**Status**: Ready for implementation
