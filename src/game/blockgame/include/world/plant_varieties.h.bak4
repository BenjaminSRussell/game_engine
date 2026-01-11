// include/world/plant_varieties.h
//
// Purpose: Extended plant generation system with 50+ plant species for
// diverse biome decoration. Includes carnivorous plants, aquatic plants,
// fungi, herbs, vines, and seasonal vegetation.
//
// Public APIs:
// - plant_varieties_init: Initialize the plant variety system
// - plant_varieties_generate_in_chunk: Generate diverse plants in a chunk
// - plant_variety_try_place: Attempt to place a specific plant variety
// - plant_variety_get_info: Get information about a plant variety
//
// Ownership: Plant variety system manages plant type definitions.
// Placed plants are owned by the chunk system as blocks.
//
// Invariants:
// - Plant varieties must be compatible with their biomes
// - Plants must have valid growth requirements (soil, light, water)
// - Seasonal plants only spawn in appropriate seasons
//
#ifndef PLANT_VARIETIES_H
#define PLANT_VARIETIES_H

#include "../game_common.h"
#include "../chunk/chunk.h"
#include "../world/generator.h"
#include "../weather/weather.h"

#ifdef __cplusplus
extern "C" {
#endif

// Extended plant variety types (50+ species)
typedef enum {
    // Grasses & Basic Plants (8)
    PLANT_VAR_GRASS_SHORT,
    PLANT_VAR_GRASS_TALL,
    PLANT_VAR_GRASS_WHEAT,
    PLANT_VAR_GRASS_REED,
    PLANT_VAR_DEAD_BUSH,
    PLANT_VAR_TUMBLEWEED,
    PLANT_VAR_FERN_SMALL,
    PLANT_VAR_FERN_LARGE,

    // Flowers (12)
    PLANT_VAR_FLOWER_ROSE,
    PLANT_VAR_FLOWER_TULIP_RED,
    PLANT_VAR_FLOWER_TULIP_YELLOW,
    PLANT_VAR_FLOWER_TULIP_WHITE,
    PLANT_VAR_FLOWER_DAISY,
    PLANT_VAR_FLOWER_SUNFLOWER,
    PLANT_VAR_FLOWER_DANDELION,
    PLANT_VAR_FLOWER_POPPY,
    PLANT_VAR_FLOWER_ORCHID,
    PLANT_VAR_FLOWER_LAVENDER,
    PLANT_VAR_FLOWER_BLUEBELL,
    PLANT_VAR_FLOWER_LOTUS,

    // Mushrooms & Fungi (8)
    PLANT_VAR_MUSHROOM_BROWN,
    PLANT_VAR_MUSHROOM_RED,
    PLANT_VAR_MUSHROOM_BLUE,
    PLANT_VAR_MUSHROOM_GLOW,
    PLANT_VAR_MUSHROOM_GIANT_BROWN,
    PLANT_VAR_MUSHROOM_GIANT_RED,
    PLANT_VAR_FUNGUS_SHELF,
    PLANT_VAR_FUNGUS_PUFFBALL,

    // Aquatic Plants (10)
    PLANT_VAR_LILY_PAD,
    PLANT_VAR_LILY_FLOWER,
    PLANT_VAR_SEAWEED,
    PLANT_VAR_SEAWEED_TALL,
    PLANT_VAR_KELP,
    PLANT_VAR_CORAL_BRAIN,
    PLANT_VAR_CORAL_TUBE,
    PLANT_VAR_CORAL_FIRE,
    PLANT_VAR_WATER_GRASS,
    PLANT_VAR_CATTAIL,

    // Carnivorous Plants (5)
    PLANT_VAR_VENUS_FLYTRAP,
    PLANT_VAR_PITCHER_PLANT,
    PLANT_VAR_SUNDEW,
    PLANT_VAR_BUTTERWORT,
    PLANT_VAR_BLADDERWORT,

    // Herbs & Medicinal (8)
    PLANT_VAR_HERB_BASIL,
    PLANT_VAR_HERB_MINT,
    PLANT_VAR_HERB_SAGE,
    PLANT_VAR_HERB_THYME,
    PLANT_VAR_HERB_CHAMOMILE,
    PLANT_VAR_HERB_GINSENG,
    PLANT_VAR_HERB_YARROW,
    PLANT_VAR_HERB_ALOE,

    // Vines & Climbers (5)
    PLANT_VAR_VINE_IVY,
    PLANT_VAR_VINE_KUDZU,
    PLANT_VAR_VINE_GRAPE,
    PLANT_VAR_VINE_WISTERIA,
    PLANT_VAR_VINE_HANGING,

    // Cacti & Succulents (6)
    PLANT_VAR_CACTUS_SAGUARO,
    PLANT_VAR_CACTUS_BARREL,
    PLANT_VAR_CACTUS_PRICKLY_PEAR,
    PLANT_VAR_SUCCULENT_ALOE,
    PLANT_VAR_SUCCULENT_AGAVE,
    PLANT_VAR_SUCCULENT_YUCCA,

    // Moss & Ground Cover (5)
    PLANT_VAR_MOSS_GREEN,
    PLANT_VAR_MOSS_ROCK,
    PLANT_VAR_LICHEN_YELLOW,
    PLANT_VAR_LICHEN_ORANGE,
    PLANT_VAR_CLOVER,

    // Berries & Shrubs (6)
    PLANT_VAR_BERRY_BUSH_RED,
    PLANT_VAR_BERRY_BUSH_BLUE,
    PLANT_VAR_BERRY_BUSH_BLACK,
    PLANT_VAR_SHRUB_SMALL,
    PLANT_VAR_SHRUB_FLOWERING,
    PLANT_VAR_BRAMBLE,

    PLANT_VARIETY_COUNT
} PlantVariety;

// Plant growth requirements
typedef struct {
    bool requires_water;        // Must be in/near water
    bool requires_sunlight;     // Needs direct sky access
    bool requires_shade;        // Grows better under trees
    bool requires_sand;         // Only grows on sand
    bool requires_soil;         // Only grows on dirt/grass
    bool can_grow_on_stone;     // Can grow on stone (moss, lichen)
    bool is_carnivorous;        // Carnivorous plant
    bool is_seasonal;           // Only spawns in certain seasons
    SeasonType preferred_season;// Best season for growth
    f32 min_temperature;        // Minimum temperature (Celsius)
    f32 max_temperature;        // Maximum temperature (Celsius)
    f32 min_humidity;           // Minimum humidity (0-1)
    f32 rarity;                 // 0.0 (common) to 1.0 (very rare)
} PlantRequirements;

// Plant visual/gameplay properties
typedef struct {
    const char *name;
    const char *description;
    PlantRequirements requirements;
    BiomeType preferred_biomes[4];  // Up to 4 preferred biomes
    u8 biome_count;
    f32 height_min;             // Visual height in blocks
    f32 height_max;
    bool has_particles;         // Spawns particle effects
    bool is_harvestable;        // Can be harvested for items
    bool glows_at_night;        // Emits light
    u8 light_level;             // 0-15
} PlantVarietyInfo;

// Plant particle effects
typedef enum {
    PLANT_PARTICLE_NONE,
    PLANT_PARTICLE_POLLEN,
    PLANT_PARTICLE_SPORES,
    PLANT_PARTICLE_SPARKLE,
    PLANT_PARTICLE_GLOW,
    PLANT_PARTICLE_DRIP
} PlantParticleType;

// Lifecycle
void plant_varieties_init(void);
void plant_varieties_free(void);

// Generation
void plant_varieties_generate_in_chunk(WorldGenerator *gen, Chunk *chunk,
                                      const WeatherSystem *weather);
bool plant_variety_try_place(Chunk *chunk, i32 x, i32 y, i32 z,
                             PlantVariety variety, u32 seed);

// Queries
const PlantVarietyInfo *plant_variety_get_info(PlantVariety variety);
bool plant_variety_can_grow_at(PlantVariety variety, Chunk *chunk,
                               i32 x, i32 y, i32 z, BiomeType biome,
                               const WeatherSystem *weather);
PlantVariety plant_variety_get_random_for_biome(BiomeType biome, SeasonType season,
                                                u32 *seed);

// Particle effects
void plant_variety_spawn_particles(PlantVariety variety, Vec3 position);
PlantParticleType plant_variety_get_particle_type(PlantVariety variety);

// Seasonal updates
void plant_variety_update_seasonal(Chunk *chunk, SeasonType season);
bool plant_variety_is_seasonal(PlantVariety variety);

#ifdef __cplusplus
}
#endif

#endif // PLANT_VARIETIES_H
