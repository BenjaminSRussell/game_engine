// include/tech/energy_system.h
//
// Purpose: Energy system inspired by GregTech (EU - Energy Units)
// Supports multiple energy types, voltage tiers, and power networks
// Essential for tech mods
//
#ifndef ENERGY_SYSTEM_H
#define ENERGY_SYSTEM_H

#include "../game_common.h"
#include <core/big_int.h>

// Energy unit types
typedef enum {
  ENERGY_TYPE_EU, // GregTech Energy Units (default)
  ENERGY_TYPE_RF, // Redstone Flux (compatibility)
  ENERGY_TYPE_FE, // Forge Energy
  ENERGY_TYPE_AE, // Applied Energistics
  ENERGY_TYPE_COUNT
} EnergyType;

// Voltage tier (GregTech-style)
typedef enum {
  VOLTAGE_TIER_ULV = 0, // Ultra Low Voltage (8V)
  VOLTAGE_TIER_LV = 1,  // Low Voltage (32V)
  VOLTAGE_TIER_MV = 2,  // Medium Voltage (128V)
  VOLTAGE_TIER_HV = 3,  // High Voltage (512V)
  VOLTAGE_TIER_EV = 4,  // Extreme Voltage (2048V)
  VOLTAGE_TIER_IV = 5,  // Insane Voltage (8192V)
  VOLTAGE_TIER_LuV = 6, // Ludicrous Voltage (32768V)
  VOLTAGE_TIER_ZPM = 7, // ZPM (131072V)
  VOLTAGE_TIER_UV = 8,  // Ultimate Voltage (524288V)
  VOLTAGE_TIER_MAX = 9, // Maximum Voltage
  VOLTAGE_TIER_COUNT = 10
} VoltageTier;

// Energy amount (using big integers for very large values)
typedef struct {
  EnergyType type;
  BigInt amount; // Amount of energy (can be very large)
} Energy;

// Energy network node
typedef struct {
  EntityID entity_id;
  bool is_producer; // Can produce energy
  bool is_consumer; // Can consume energy
  bool is_storage;  // Can store energy
  EnergyType energy_type;
  VoltageTier voltage_tier;
  BigInt production_rate;  // EU/t production
  BigInt consumption_rate; // EU/t consumption
  BigInt capacity;         // Storage capacity
  BigInt stored;           // Currently stored energy
  f32 efficiency;          // Efficiency (0.0-1.0)
} EnergyNode;

// Energy network (connects producers, consumers, and storage)
typedef struct {
  EnergyNode *nodes;
  u32 node_count;
  u32 node_capacity;
  EnergyType energy_type;
  VoltageTier max_voltage;
  BigInt total_production;
  BigInt total_consumption;
  BigInt total_capacity;
  BigInt total_stored;
} EnergyNetwork;

// Initialize energy system
void energy_system_init(void);
void energy_system_free(void);

// Energy operations
Energy energy_create(EnergyType type, u64 amount);
Energy energy_create_big(EnergyType type, const BigInt *amount);
Energy energy_add(const Energy *a, const Energy *b);
Energy energy_sub(const Energy *a, const Energy *b);
Energy energy_mul(const Energy *energy, f32 multiplier);
bool energy_can_convert(const Energy *from, EnergyType to_type);
Energy energy_convert(const Energy *from, EnergyType to_type);

// Voltage tier operations
u64 voltage_tier_to_voltage(VoltageTier tier);
VoltageTier voltage_from_voltage(u64 voltage);
f32 voltage_tier_to_efficiency(
    VoltageTier tier); // Higher tiers = better efficiency
const char *voltage_tier_name(VoltageTier tier);

// Energy node management
EnergyNode *energy_node_create(EntityID entity_id, EnergyType type,
                               VoltageTier tier);
void energy_node_free(EnergyNode *node);
bool energy_node_can_accept(EnergyNode *node, const Energy *energy);
bool energy_node_store(EnergyNode *node, const Energy *energy);
Energy energy_node_extract(EnergyNode *node, Energy amount);
Energy energy_node_produce(EnergyNode *node, f32 delta_time);

// Energy network management
EnergyNetwork *energy_network_create(EnergyType type, VoltageTier max_voltage);
void energy_network_free(EnergyNetwork *network);
void energy_network_add_node(EnergyNetwork *network, EnergyNode *node);
void energy_network_remove_node(EnergyNetwork *network, EntityID entity_id);
void energy_network_update(EnergyNetwork *network, f32 delta_time);
bool energy_network_has_capacity(EnergyNetwork *network, const Energy *energy);

// Energy transfer between nodes
bool energy_transfer(EnergyNode *from, EnergyNode *to, const Energy *amount);

// Energy constants (GregTech values)
#define EU_PER_RF 4 // 1 RF = 4 EU
#define EU_PER_FE 4 // 1 FE = 4 EU
#define EU_PER_AE 8 // 1 AE = 8 EU

// Voltage values (GregTech-style)
#define VOLTAGE_ULV 8ULL
#define VOLTAGE_LV 32ULL
#define VOLTAGE_MV 128ULL
#define VOLTAGE_HV 512ULL
#define VOLTAGE_EV 2048ULL
#define VOLTAGE_IV 8192ULL
#define VOLTAGE_LUV 32768ULL
#define VOLTAGE_ZPM 131072ULL
#define VOLTAGE_UV 524288ULL

#endif // ENERGY_SYSTEM_H
