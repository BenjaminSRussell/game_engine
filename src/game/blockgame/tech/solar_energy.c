// Solar energy generation logic.
// Roadmap: docs/SOLAR_ENERGY_ROADMAP.md.
#include <math.h>
#include <math/vec3.h>
#include <stdlib.h>
#include <tech/solar_energy.h>
#include <time.h>

// Solar panel is now defined in header

// Solar energy system (defined in header)
// Struct defined in header
// struct SolarEnergySystem { ... };

// Calculate solar efficiency based on time of day
static f32 solar_get_efficiency(f32 time_of_day) {
  // Day cycle: 0.0 = midnight, 0.25 = dawn, 0.5 = noon, 0.75 = dusk, 1.0 =
  // midnight
  f32 sun_angle = (time_of_day - 0.25f) * 4.0f * 3.14159f; // -PI to PI

  if (sun_angle < -3.14159f / 2.0f || sun_angle > 3.14159f / 2.0f) {
    return 0.0f; // Night time
  }

  // Cosine curve for day cycle
  f32 efficiency = cosf(sun_angle);
  if (efficiency < 0.0f)
    efficiency = 0.0f;

  // Peak at noon (1.0), zero at night
  return efficiency;
}

// Initialize solar energy system
void solar_energy_init(SolarEnergySystem *system, u32 max_panels) {
  if (!system)
    return;

  system->panels = (SolarPanel *)calloc(max_panels, sizeof(SolarPanel));
  system->panel_capacity = max_panels;
  system->panel_count = 0;
  system->total_energy = 0.0f;
  system->time_of_day = 0.5f;   // Start at noon
  system->day_length = 1200.0f; // 20 minutes per day
}

// Free solar energy system
void solar_energy_free(SolarEnergySystem *system) {
  if (!system)
    return;
  if (system->panels) {
    free(system->panels);
    system->panels = NULL;
  }
  system->panel_count = 0;
  system->panel_capacity = 0;
}

// Add solar panel
u32 solar_energy_add_panel(SolarEnergySystem *system, Vec3 position,
                           f32 max_energy, f32 generation_rate) {
  if (!system || !system->panels)
    return UINT32_MAX;
  if (system->panel_count >= system->panel_capacity)
    return UINT32_MAX;

  SolarPanel *panel = &system->panels[system->panel_count];
  panel->position = position;
  panel->energy_stored = 0.0f;
  panel->max_energy = max_energy;
  panel->generation_rate = generation_rate;
  panel->active = true;
  panel->last_update_time = 0.0f;

  return system->panel_count++;
}

// Remove solar panel
void solar_energy_remove_panel(SolarEnergySystem *system, u32 panel_id) {
  if (!system || !system->panels)
    return;
  if (panel_id >= system->panel_count)
    return;

  // Shift panels down
  for (u32 i = panel_id; i < system->panel_count - 1; i++) {
    system->panels[i] = system->panels[i + 1];
  }
  system->panel_count--;
}

// Update solar energy system
void solar_energy_update(SolarEnergySystem *system, f32 delta_time) {
  if (!system || !system->panels)
    return;

  // Update time of day
  system->time_of_day += delta_time / system->day_length;
  if (system->time_of_day >= 1.0f) {
    system->time_of_day -= 1.0f;
  }

  // Calculate solar efficiency
  f32 efficiency = solar_get_efficiency(system->time_of_day);

  // Update each panel
  system->total_energy = 0.0f;
  for (u32 i = 0; i < system->panel_count; i++) {
    SolarPanel *panel = &system->panels[i];
    if (!panel->active)
      continue;

    // Generate energy based on time of day
    f32 energy_generated = panel->generation_rate * efficiency * delta_time;

    // Add to storage
    panel->energy_stored += energy_generated;
    if (panel->energy_stored > panel->max_energy) {
      panel->energy_stored = panel->max_energy;
    }

    system->total_energy += panel->energy_stored;
  }
}

// Consume energy from system
bool solar_energy_consume(SolarEnergySystem *system, f32 amount) {
  if (!system || !system->panels)
    return false;
  if (system->total_energy < amount)
    return false;

  // Consume from panels (distribute evenly)
  f32 remaining = amount;
  for (u32 i = 0; i < system->panel_count && remaining > 0.0f; i++) {
    SolarPanel *panel = &system->panels[i];
    if (!panel->active || panel->energy_stored <= 0.0f)
      continue;

    f32 consume_from_panel = remaining / (f32)(system->panel_count - i);
    if (consume_from_panel > panel->energy_stored) {
      consume_from_panel = panel->energy_stored;
    }

    panel->energy_stored -= consume_from_panel;
    remaining -= consume_from_panel;
  }

  system->total_energy -= amount;
  return true;
}

// Get total energy available
f32 solar_energy_get_total(SolarEnergySystem *system) {
  if (!system)
    return 0.0f;
  return system->total_energy;
}

// Get time of day (0.0 to 1.0)
f32 solar_energy_get_time_of_day(SolarEnergySystem *system) {
  if (!system)
    return 0.5f;
  return system->time_of_day;
}

// Get solar efficiency (0.0 to 1.0)
f32 solar_energy_get_efficiency(SolarEnergySystem *system) {
  if (!system)
    return 0.0f;
  return solar_get_efficiency(system->time_of_day);
}
