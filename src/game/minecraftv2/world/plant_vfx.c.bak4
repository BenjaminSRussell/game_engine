// Plant visual effects implementation
#include <core/logger.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <weather/weather.h>
#include <world/plant_vfx.h>

// Default VFX configuration
static const PlantVFXConfig DEFAULT_CONFIG = {.pollen_emission_rate = 5.0f,
                                              .spore_emission_rate = 3.0f,
                                              .seed_emission_rate = 2.0f,
                                              .nectar_glow_intensity = 0.8f,
                                              .particle_lifetime_min = 1.0f,
                                              .particle_lifetime_max = 8.0f,
                                              .wind_influence = 0.5f,
                                              .gravity_scale = 0.3f,
                                              .enable_particle_physics = true,
                                              .enable_glow_effects = true,
                                              .max_particles_per_plant = 32,
                                              .max_total_particles = 4096};

// Emission properties for each plant variety
static const PlantEmissionProperties PLANT_EMISSIONS[PLANT_VARIETY_COUNT] = {
    // Flowers
    [PLANT_VAR_FLOWER_ROSE] = {.pollen_rate = 2.0f,
                               .spore_rate = 0.0f,
                               .seed_rate = 1.0f,
                               .seed_speed = 3.0f,
                               .petal_rate = 0.5f,
                               .emits_nectar = true,
                               .glows_at_night = false,
                               .glow_intensity = 0.0f},
    [PLANT_VAR_FLOWER_TULIP_RED] = {.pollen_rate = 1.5f,
                                    .spore_rate = 0.0f,
                                    .seed_rate = 1.0f,
                                    .seed_speed = 2.5f,
                                    .petal_rate = 0.8f,
                                    .emits_nectar = true,
                                    .glows_at_night = false,
                                    .glow_intensity = 0.0f},
    [PLANT_VAR_FLOWER_SUNFLOWER] = {.pollen_rate = 3.0f,
                                    .spore_rate = 0.0f,
                                    .seed_rate = 2.0f,
                                    .seed_speed = 4.0f,
                                    .petal_rate = 1.0f,
                                    .emits_nectar = true,
                                    .glows_at_night = false,
                                    .glow_intensity = 0.0f},
    [PLANT_VAR_FLOWER_DANDELION] = {.pollen_rate = 0.5f,
                                    .spore_rate = 0.0f,
                                    .seed_rate = 3.0f,
                                    .seed_speed = 5.0f,
                                    .petal_rate = 0.2f,
                                    .emits_nectar = false,
                                    .glows_at_night = false,
                                    .glow_intensity = 0.0f},
    [PLANT_VAR_FLOWER_LAVENDER] = {.pollen_rate = 2.5f,
                                   .spore_rate = 0.0f,
                                   .seed_rate = 1.5f,
                                   .seed_speed = 2.0f,
                                   .petal_rate = 1.5f,
                                   .emits_nectar = true,
                                   .glows_at_night = false,
                                   .glow_intensity = 0.0f},

    // Shrubs
    [PLANT_VAR_BERRY_BUSH_RED] = {.pollen_rate = 0.8f,
                                  .spore_rate = 0.0f,
                                  .seed_rate = 2.0f,
                                  .seed_speed = 3.5f,
                                  .petal_rate = 0.3f,
                                  .emits_nectar = true,
                                  .glows_at_night = false,
                                  .glow_intensity = 0.0f},
    [PLANT_VAR_SHRUB_SMALL] = {.pollen_rate = 1.0f,
                               .spore_rate = 0.0f,
                               .seed_rate = 2.5f,
                               .seed_speed = 4.0f,
                               .petal_rate = 0.0f,
                               .emits_nectar = false,
                               .glows_at_night = false,
                               .glow_intensity = 0.0f},
    [PLANT_VAR_SHRUB_FLOWERING] = {.pollen_rate = 0.5f,
                                   .spore_rate = 0.0f,
                                   .seed_rate = 1.0f,
                                   .seed_speed = 2.5f,
                                   .petal_rate = 0.1f,
                                   .emits_nectar = false,
                                   .glows_at_night = false,
                                   .glow_intensity = 0.0f},

    // Grass and Ground Cover
    [PLANT_VAR_GRASS_TALL] = {.pollen_rate = 3.0f,
                              .spore_rate = 0.0f,
                              .seed_rate = 4.0f,
                              .seed_speed = 6.0f,
                              .petal_rate = 0.0f,
                              .emits_nectar = false,
                              .glows_at_night = false,
                              .glow_intensity = 0.0f},
    [PLANT_VAR_MOSS_GREEN] = {.pollen_rate = 0.2f,
                              .spore_rate = 2.0f,
                              .seed_rate = 0.5f,
                              .seed_speed = 1.5f,
                              .petal_rate = 0.0f,
                              .emits_nectar = false,
                              .glows_at_night = false,
                              .glow_intensity = 0.0f},
    [PLANT_VAR_CLOVER] = {.pollen_rate = 0.8f,
                          .spore_rate = 0.0f,
                          .seed_rate = 1.5f,
                          .seed_speed = 2.5f,
                          .petal_rate = 0.2f,
                          .emits_nectar = true,
                          .glows_at_night = false,
                          .glow_intensity = 0.0f},
    [PLANT_VAR_TUMBLEWEED] = {.pollen_rate = 1.2f,
                              .spore_rate = 0.0f,
                              .seed_rate = 2.0f,
                              .seed_speed = 5.5f,
                              .petal_rate = 0.1f,
                              .emits_nectar = false,
                              .glows_at_night = false,
                              .glow_intensity = 0.0f},
    [PLANT_VAR_FLOWER_BLUEBELL] = {.pollen_rate = 1.5f,
                                   .spore_rate = 0.0f,
                                   .seed_rate = 1.8f,
                                   .seed_speed = 3.0f,
                                   .petal_rate = 0.5f,
                                   .emits_nectar = true,
                                   .glows_at_night = false,
                                   .glow_intensity = 0.0f},

    // Mushrooms
    [PLANT_VAR_MUSHROOM_RED] = {.pollen_rate = 0.0f,
                                .spore_rate = 3.0f,
                                .seed_rate = 0.0f,
                                .seed_speed = 0.0f,
                                .petal_rate = 0.0f,
                                .emits_nectar = false,
                                .glows_at_night = true,
                                .glow_intensity = 0.6f},
    [PLANT_VAR_MUSHROOM_GIANT_BROWN] = {.pollen_rate = 0.0f,
                                        .spore_rate = 4.0f,
                                        .seed_rate = 0.0f,
                                        .seed_speed = 0.0f,
                                        .petal_rate = 0.0f,
                                        .emits_nectar = false,
                                        .glows_at_night = true,
                                        .glow_intensity = 0.8f},
    [PLANT_VAR_MUSHROOM_BROWN] = {.pollen_rate = 0.0f,
                                  .spore_rate = 2.0f,
                                  .seed_rate = 0.0f,
                                  .seed_speed = 0.0f,
                                  .petal_rate = 0.0f,
                                  .emits_nectar = false,
                                  .glows_at_night = false,
                                  .glow_intensity = 0.0f},
    [PLANT_VAR_FUNGUS_SHELF] = {.pollen_rate = 0.0f,
                                .spore_rate = 2.5f,
                                .seed_rate = 0.0f,
                                .seed_speed = 0.0f,
                                .petal_rate = 0.0f,
                                .emits_nectar = false,
                                .glows_at_night = false,
                                .glow_intensity = 0.0f},
    [PLANT_VAR_MUSHROOM_GLOW] = {.pollen_rate = 0.0f,
                                 .spore_rate = 3.5f,
                                 .seed_rate = 0.0f,
                                 .seed_speed = 0.0f,
                                 .petal_rate = 0.0f,
                                 .emits_nectar = false,
                                 .glows_at_night = true,
                                 .glow_intensity = 1.0f},

    // Cacti and Succulents
    [PLANT_VAR_CACTUS_BARREL] = {.pollen_rate = 0.5f,
                                 .spore_rate = 0.0f,
                                 .seed_rate = 1.0f,
                                 .seed_speed = 2.0f,
                                 .petal_rate = 0.0f,
                                 .emits_nectar = false,
                                 .glows_at_night = false,
                                 .glow_intensity = 0.0f},
    [PLANT_VAR_CACTUS_PRICKLY_PEAR] = {.pollen_rate = 0.4f,
                                       .spore_rate = 0.0f,
                                       .seed_rate = 1.5f,
                                       .seed_speed = 3.0f,
                                       .petal_rate = 0.0f,
                                       .emits_nectar = false,
                                       .glows_at_night = false,
                                       .glow_intensity = 0.0f},
    [PLANT_VAR_SUCCULENT_ALOE] = {.pollen_rate = 0.0f,
                                  .spore_rate = 0.0f,
                                  .seed_rate = 0.5f,
                                  .seed_speed = 1.5f,
                                  .petal_rate = 0.0f,
                                  .emits_nectar = false,
                                  .glows_at_night = false,
                                  .glow_intensity = 0.0f},
    [PLANT_VAR_SUCCULENT_AGAVE] = {.pollen_rate = 0.3f,
                                   .spore_rate = 0.0f,
                                   .seed_rate = 0.8f,
                                   .seed_speed = 2.0f,
                                   .petal_rate = 0.2f,
                                   .emits_nectar = false,
                                   .glows_at_night = false,
                                   .glow_intensity = 0.0f},

    // Aquatic Plants
    [PLANT_VAR_LILY_PAD] = {.pollen_rate = 1.0f,
                            .spore_rate = 0.0f,
                            .seed_rate = 0.5f,
                            .seed_speed = 0.5f,
                            .petal_rate = 0.3f,
                            .emits_nectar = true,
                            .glows_at_night = false,
                            .glow_intensity = 0.0f},
    [PLANT_VAR_FLOWER_LOTUS] = {.pollen_rate = 1.5f,
                                .spore_rate = 0.0f,
                                .seed_rate = 1.0f,
                                .seed_speed = 0.3f,
                                .petal_rate = 0.5f,
                                .emits_nectar = true,
                                .glows_at_night = false,
                                .glow_intensity = 0.0f},
    [PLANT_VAR_CATTAIL] = {.pollen_rate = 4.0f,
                           .spore_rate = 0.0f,
                           .seed_rate = 3.0f,
                           .seed_speed = 4.0f,
                           .petal_rate = 0.0f,
                           .emits_nectar = false,
                           .glows_at_night = false,
                           .glow_intensity = 0.0f},
    [PLANT_VAR_GRASS_REED] = {.pollen_rate = 2.0f,
                              .spore_rate = 0.0f,
                              .seed_rate = 2.0f,
                              .seed_speed = 3.0f,
                              .petal_rate = 0.0f,
                              .emits_nectar = false,
                              .glows_at_night = false,
                              .glow_intensity = 0.0f},
    [PLANT_VAR_KELP] = {.pollen_rate = 0.0f,
                        .spore_rate = 1.5f,
                        .seed_rate = 0.0f,
                        .seed_speed = 0.0f,
                        .petal_rate = 0.0f,
                        .emits_nectar = false,
                        .glows_at_night = false,
                        .glow_intensity = 0.0f},

    // Exotic Plants
    [PLANT_VAR_PITCHER_PLANT] = {.pollen_rate = 1.0f,
                                 .spore_rate = 0.0f,
                                 .seed_rate = 0.8f,
                                 .seed_speed = 2.0f,
                                 .petal_rate = 0.2f,
                                 .emits_nectar = true,
                                 .glows_at_night = false,
                                 .glow_intensity = 0.0f},
    [PLANT_VAR_VENUS_FLYTRAP] = {.pollen_rate = 0.5f,
                                 .spore_rate = 0.0f,
                                 .seed_rate = 0.8f,
                                 .seed_speed = 2.5f,
                                 .petal_rate = 0.1f,
                                 .emits_nectar = false,
                                 .glows_at_night = false,
                                 .glow_intensity = 0.0f},
    [PLANT_VAR_FLOWER_ORCHID] = {.pollen_rate = 1.0f,
                                 .spore_rate = 0.0f,
                                 .seed_rate = 0.3f,
                                 .seed_speed = 1.0f,
                                 .petal_rate = 0.4f,
                                 .emits_nectar = true,
                                 .glows_at_night = false,
                                 .glow_intensity = 0.0f},
    [PLANT_VAR_VINE_WISTERIA] = {.pollen_rate = 2.5f,
                                 .spore_rate = 0.0f,
                                 .seed_rate = 1.5f,
                                 .seed_speed = 3.5f,
                                 .petal_rate = 1.8f,
                                 .emits_nectar = true,
                                 .glows_at_night = false,
                                 .glow_intensity = 0.0f},

    // Vines
    [PLANT_VAR_VINE_IVY] = {.pollen_rate = 0.3f,
                            .spore_rate = 0.0f,
                            .seed_rate = 1.0f,
                            .seed_speed = 2.0f,
                            .petal_rate = 0.1f,
                            .emits_nectar = false,
                            .glows_at_night = false,
                            .glow_intensity = 0.0f},
    [PLANT_VAR_VINE_GRAPE] = {.pollen_rate = 0.8f,
                              .spore_rate = 0.0f,
                              .seed_rate = 1.5f,
                              .seed_speed = 2.0f,
                              .petal_rate = 0.2f,
                              .emits_nectar = true,
                              .glows_at_night = false,
                              .glow_intensity = 0.0f},

    // Wetland
    [PLANT_VAR_WATER_GRASS] = {.pollen_rate = 1.5f,
                               .spore_rate = 0.0f,
                               .seed_rate = 1.0f,
                               .seed_speed = 2.5f,
                               .petal_rate = 0.3f,
                               .emits_nectar = true,
                               .glows_at_night = false,
                               .glow_intensity = 0.0f},
    [PLANT_VAR_FLOWER_TULIP_WHITE] = {.pollen_rate = 1.8f,
                                      .spore_rate = 0.0f,
                                      .seed_rate = 1.5f,
                                      .seed_speed = 2.5f,
                                      .petal_rate = 0.5f,
                                      .emits_nectar = true,
                                      .glows_at_night = false,
                                      .glow_intensity = 0.0f},

    // Alpine
    [PLANT_VAR_FLOWER_POPPY] = {.pollen_rate = 1.0f,
                                .spore_rate = 0.0f,
                                .seed_rate = 1.0f,
                                .seed_speed = 3.0f,
                                .petal_rate = 0.4f,
                                .emits_nectar = true,
                                .glows_at_night = false,
                                .glow_intensity = 0.0f},
    [PLANT_VAR_LILY_FLOWER] = {.pollen_rate = 0.9f,
                               .spore_rate = 0.0f,
                               .seed_rate = 0.9f,
                               .seed_speed = 2.0f,
                               .petal_rate = 0.3f,
                               .emits_nectar = true,
                               .glows_at_night = false,
                               .glow_intensity = 0.0f},
    [PLANT_VAR_FLOWER_DAISY] = {.pollen_rate = 0.7f,
                                .spore_rate = 0.0f,
                                .seed_rate = 0.7f,
                                .seed_speed = 2.0f,
                                .petal_rate = 0.2f,
                                .emits_nectar = false,
                                .glows_at_night = false,
                                .glow_intensity = 0.0f},
    [PLANT_VAR_MOSS_ROCK] = {.pollen_rate = 0.1f,
                             .spore_rate = 1.5f,
                             .seed_rate = 0.3f,
                             .seed_speed = 1.0f,
                             .petal_rate = 0.0f,
                             .emits_nectar = false,
                             .glows_at_night = false,
                             .glow_intensity = 0.0f},

    // Desert
    [PLANT_VAR_CACTUS_SAGUARO] = {.pollen_rate = 0.7f,
                                  .spore_rate = 0.0f,
                                  .seed_rate = 1.0f,
                                  .seed_speed = 2.5f,
                                  .petal_rate = 0.0f,
                                  .emits_nectar = true,
                                  .glows_at_night = true,
                                  .glow_intensity = 0.4f},

    // Bioluminescent
    [PLANT_VAR_VINE_HANGING] = {.pollen_rate = 0.2f,
                                .spore_rate = 0.0f,
                                .seed_rate = 0.8f,
                                .seed_speed = 2.0f,
                                .petal_rate = 0.5f,
                                .emits_nectar = false,
                                .glows_at_night = true,
                                .glow_intensity = 0.9f},
    [PLANT_VAR_FERN_SMALL] = {.pollen_rate = 0.5f,
                              .spore_rate = 1.5f,
                              .seed_rate = 0.5f,
                              .seed_speed = 1.5f,
                              .petal_rate = 0.0f,
                              .emits_nectar = false,
                              .glows_at_night = true,
                              .glow_intensity = 0.5f},
};

void plant_vfx_init(PlantVFXSystem *vfx, u32 max_particles) {
  if (!vfx || max_particles == 0)
    return;

  memset(vfx, 0, sizeof(PlantVFXSystem));
  vfx->max_particles = max_particles;
  vfx->particles =
      (PlantParticle *)malloc(max_particles * sizeof(PlantParticle));

  if (!vfx->particles) {
    LOG_ERROR("Failed to allocate plant VFX particles");
    return;
  }

  for (u32 i = 0; i < max_particles; i++) {
    vfx->particles[i].active = false;
  }

  vfx->config = DEFAULT_CONFIG;
  vfx->active_count = 0;
  vfx->initialized = true;

  LOG_INFO("Plant VFX system initialized with %u max particles", max_particles);
}

void plant_vfx_free(PlantVFXSystem *vfx) {
  if (!vfx)
    return;

  if (vfx->particles) {
    free(vfx->particles);
    vfx->particles = NULL;
  }

  memset(vfx, 0, sizeof(PlantVFXSystem));
}

void plant_vfx_set_weather(PlantVFXSystem *vfx, const WeatherSystem *weather) {
  if (!vfx)
    return;
  vfx->weather = weather;
}

void plant_vfx_update(PlantVFXSystem *vfx, f32 delta_time) {
  if (!vfx || !vfx->initialized)
    return;

  Vec3 wind_direction = vec3(1.0f, 0.0f, 0.0f);
  f32 wind_speed = 0.0f;
  if (vfx->weather) {
    wind_direction = weather_get_wind_direction(vfx->weather);
    wind_speed = weather_get_wind_speed(vfx->weather);
  }

  u32 new_active_count = 0;

  for (u32 i = 0; i < vfx->active_count; i++) {
    PlantParticle *particle = &vfx->particles[i];

    if (!particle->active)
      continue;

    // Update lifetime
    particle->lifetime -= delta_time;

    if (particle->lifetime <= 0.0f) {
      particle->active = false;
      continue;
    }

    // Apply physics if enabled
    if (vfx->config.enable_particle_physics) {
      // Apply gravity
      particle->acceleration.y -= vfx->config.gravity_scale * 9.81f;

      // Apply wind influence
      particle->acceleration = vec3_add(
          particle->acceleration,
          vec3_mul(wind_direction, wind_speed * vfx->config.wind_influence));

      // Update velocity
      particle->velocity = vec3_add(
          particle->velocity, vec3_mul(particle->acceleration, delta_time));

      // Apply drag
      particle->velocity = vec3_mul(particle->velocity, 0.99f);

      // Update position
      particle->position = vec3_add(particle->position,
                                    vec3_mul(particle->velocity, delta_time));

      // Reset acceleration for next frame
      particle->acceleration = vec3(0.0f, 0.0f, 0.0f);
    }

    // Update rotation
    particle->rotation += particle->rotation_speed * delta_time;

    // Update brightness (fade out over lifetime)
    f32 age_ratio = 1.0f - (particle->lifetime / particle->max_lifetime);
    particle->brightness = 1.0f - (age_ratio * 0.5f); // Fade to 50% brightness

    new_active_count++;
  }

  vfx->active_count = new_active_count;
}

void plant_vfx_emit(PlantVFXSystem *vfx, PlantVFXParticleType type,
                    PlantVariety variety, Vec3 position, Vec3 direction,
                    u32 count) {
  if (!vfx || !vfx->initialized || count == 0)
    return;

  u32 emitted = 0;

  for (u32 i = vfx->active_count; i < vfx->max_particles && emitted < count;
       i++) {
    PlantParticle *particle = &vfx->particles[i];

    if (particle->active)
      continue;

    // Initialize particle
    particle->position = position;
    particle->velocity =
        vec3_mul(direction, 2.0f + (f32)rand() / (f32)RAND_MAX);
    particle->acceleration = vec3(0.0f, 0.0f, 0.0f);
    particle->lifetime = vfx->config.particle_lifetime_min +
                         (f32)rand() / (f32)RAND_MAX *
                             (vfx->config.particle_lifetime_max -
                              vfx->config.particle_lifetime_min);
    particle->max_lifetime = particle->lifetime;
    particle->size = 0.1f + (f32)rand() / (f32)RAND_MAX * 0.2f;
    particle->rotation = (f32)rand() / (f32)RAND_MAX * 6.28318f;
    particle->rotation_speed = -3.0f + (f32)rand() / (f32)RAND_MAX * 6.0f;
    particle->type = type;
    particle->plant_variety = variety;
    particle->brightness = 1.0f;
    particle->active = true;

    emitted++;
    vfx->active_count++;
  }
}

void plant_vfx_emit_pollen(PlantVFXSystem *vfx, PlantVariety variety,
                           Vec3 position, f32 intensity) {
  if (!vfx)
    return;

  u32 count = (u32)(5 * intensity);
  if (count == 0)
    count = 1;

  Vec3 up_direction = vec3(0.0f, 1.0f, 0.0f);
  plant_vfx_emit(vfx, PLANT_VFX_PARTICLE_POLLEN, variety, position,
                 up_direction, count);
}

void plant_vfx_emit_spores(PlantVFXSystem *vfx, PlantVariety variety,
                           Vec3 position, f32 intensity) {
  if (!vfx)
    return;

  u32 count = (u32)(3 * intensity);
  if (count == 0)
    count = 1;

  Vec3 up_direction = vec3(0.0f, 0.5f, 0.0f);
  plant_vfx_emit(vfx, PLANT_VFX_PARTICLE_SPORE, variety, position, up_direction,
                 count);
}

void plant_vfx_emit_seeds(PlantVFXSystem *vfx, PlantVariety variety,
                          Vec3 position, Vec3 wind_direction, f32 wind_speed) {
  if (!vfx)
    return;

  u32 count = 3;

  Vec3 seed_direction = vec3_add(vec3(0.0f, 0.5f, 0.0f),
                                 vec3_mul(wind_direction, wind_speed * 0.1f));

  plant_vfx_emit(vfx, PLANT_VFX_PARTICLE_SEED, variety, position,
                 seed_direction, count);
}

void plant_vfx_emit_nectar_glow(PlantVFXSystem *vfx, PlantVariety variety,
                                Vec3 position) {
  if (!vfx)
    return;

  Vec3 scatter_direction = vec3((f32)rand() / (f32)RAND_MAX - 0.5f,
                                (f32)rand() / (f32)RAND_MAX * 0.5f + 0.5f,
                                (f32)rand() / (f32)RAND_MAX - 0.5f);

  plant_vfx_emit(vfx, PLANT_VFX_PARTICLE_NECTAR, variety, position,
                 scatter_direction, 2);
}

void plant_vfx_emit_bloom(PlantVFXSystem *vfx, PlantVariety variety,
                          Vec3 position) {
  if (!vfx)
    return;

  // Bloom particles expand outward
  for (u32 i = 0; i < 8; i++) {
    f32 angle = (f32)i / 8.0f * 6.28318f;
    Vec3 direction = vec3(cosf(angle) * 0.3f, 1.0f, sinf(angle) * 0.3f);

    plant_vfx_emit(vfx, PLANT_VFX_PARTICLE_BLOOM, variety, position, direction,
                   1);
  }
}

void plant_vfx_emit_petals(PlantVFXSystem *vfx, PlantVariety variety,
                           Vec3 position, Vec3 wind_direction) {
  if (!vfx)
    return;

  u32 count = 4;

  Vec3 petal_direction =
      vec3_add(vec3(0.0f, 0.5f, 0.0f), vec3_mul(wind_direction, 0.3f));

  plant_vfx_emit(vfx, PLANT_VFX_PARTICLE_PETAL, variety, position,
                 petal_direction, count);
}

u32 plant_vfx_get_active_count(const PlantVFXSystem *vfx) {
  return vfx ? vfx->active_count : 0;
}

const PlantParticle *plant_vfx_get_particles(const PlantVFXSystem *vfx) {
  return vfx ? vfx->particles : NULL;
}

void plant_vfx_set_config(PlantVFXSystem *vfx, const PlantVFXConfig *config) {
  if (!vfx || !config)
    return;
  vfx->config = *config;
}

PlantVFXConfig plant_vfx_get_default_config(void) { return DEFAULT_CONFIG; }

PlantEmissionProperties
plant_vfx_get_emission_properties(PlantVariety variety) {
  if (variety >= PLANT_VARIETY_COUNT) {
    return (PlantEmissionProperties){0};
  }
  return PLANT_EMISSIONS[variety];
}
