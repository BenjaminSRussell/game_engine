// Shallow Water Equations Implementation
// Now uses the advanced fluid dynamics system

#include "fluid_dynamics.h"
#include "include/core/logger.h"

// Legacy compatibility functions
void shallow_water_init(void) {
  LOG_INFO("Shallow water system initialized using advanced fluid dynamics");
}

void shallow_water_update(f32 delta_time) {
  // This is now handled by the fluid_dynamics_update function
  // Keeping for backward compatibility
}

void shallow_water_render(void) {
  // Rendering is now handled by the fluid system
  // Keeping for backward compatibility
}
