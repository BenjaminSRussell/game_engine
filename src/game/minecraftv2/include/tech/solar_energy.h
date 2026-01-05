// include/tech/solar_energy.h
//
// Purpose: Defines the public API and data structures for a solar energy system.
// This header provides the means to simulate solar energy generation within the game,
// managing individual `SolarPanel` entities and a centralized `SolarEnergySystem`
// that tracks energy production and consumption based on the in-game time of day.
//
// Public APIs:
// - `SolarPanel`: Structure representing an individual solar panel, including its
//   `position`, `energy_stored`, `max_energy` capacity, `generation_rate`, active status,
//   and `last_update_time`.
// - `SolarEnergySystem`: The main structure managing all solar panels, tracking
//   `panel_count`, `panel_capacity`, `total_energy` available, `time_of_day`, and
//   `day_length` for realistic simulation.
// - `solar_energy_init`: Initializes the solar energy system with a specified maximum number of panels.
// - `solar_energy_free`: Frees all resources held by the solar energy system.
// - `solar_energy_add_panel`: Adds a new `SolarPanel` to the system, specifying its
//   position, capacity, and generation rate.
// - `solar_energy_remove_panel`: Removes a solar panel from the system by its ID.
// - `solar_energy_update`: Updates the energy generation for all panels based on the
//   current time of day and `delta_time`.
// - `solar_energy_consume`: Attempts to consume a specified amount of energy from the system.
// - `solar_energy_get_total`: Returns the total amount of energy currently available in the system.
// - `solar_energy_get_time_of_day`: Returns the current time of day as a normalized value (0.0 to 1.0).
// - `solar_energy_get_efficiency`: Returns the current solar energy generation efficiency (0.0 to 1.0)
//   based on the time of day.
//
// Ownership: The `SolarEnergySystem` owns the collection of `SolarPanel` instances it manages.
//
// Invariants:
// - A `SolarEnergySystem` must be initialized with `solar_energy_init` before use.
// - `solar_energy_update` should be called regularly (e.g., every game tick) with `delta_time`
//   for accurate energy simulation.
// - `time_of_day` is crucial for determining energy generation rates, typically peaking around 0.5 (noon).
// - `day_length` influences the speed of the day-night cycle.
//
#ifndef SOLAR_ENERGY_H
#define SOLAR_ENERGY_H


#include "../game_common.h"
#include <math/vec3.h>

// Forward declaration
typedef struct SolarPanel SolarPanel;

// Solar panel state
struct SolarPanel {
    Vec3 position;
    f32 energy_stored;      // Energy units stored
    f32 max_energy;         // Maximum energy capacity
    f32 generation_rate;    // Energy per second
    bool active;
    f32 last_update_time;
};

// Solar energy system
typedef struct SolarEnergySystem {
    SolarPanel *panels;
    u32 panel_count;
    u32 panel_capacity;
    f32 total_energy;
    f32 time_of_day;        // 0.0 to 1.0 (0 = midnight, 0.5 = noon)
    f32 day_length;        // Real-time seconds per day
} SolarEnergySystem;

// Initialize solar energy system
void solar_energy_init(SolarEnergySystem *system, u32 max_panels);

// Free solar energy system
void solar_energy_free(SolarEnergySystem *system);

// Add solar panel
u32 solar_energy_add_panel(SolarEnergySystem *system, Vec3 position, f32 max_energy, f32 generation_rate);

// Remove solar panel
void solar_energy_remove_panel(SolarEnergySystem *system, u32 panel_id);

// Update solar energy system (call every frame)
void solar_energy_update(SolarEnergySystem *system, f32 delta_time);

// Consume energy from system
bool solar_energy_consume(SolarEnergySystem *system, f32 amount);

// Get total energy available
f32 solar_energy_get_total(SolarEnergySystem *system);

// Get time of day (0.0 to 1.0)
f32 solar_energy_get_time_of_day(SolarEnergySystem *system);

// Get solar efficiency (0.0 to 1.0)
f32 solar_energy_get_efficiency(SolarEnergySystem *system);

#endif // SOLAR_ENERGY_H

