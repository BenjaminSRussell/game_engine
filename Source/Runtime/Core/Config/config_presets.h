#ifndef CONFIG_PRESETS_H
#define CONFIG_PRESETS_H

// Configuration preset types
typedef enum {
  CONFIG_PRESET_LOW,
  CONFIG_PRESET_MEDIUM,
  CONFIG_PRESET_HIGH,
  CONFIG_PRESET_ULTRA
} ConfigPreset;

// Apply configuration preset to global config
void config_apply_preset(ConfigPreset preset);

#endif
