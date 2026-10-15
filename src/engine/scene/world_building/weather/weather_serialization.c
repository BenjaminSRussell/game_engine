/**
 * =================================================================================================
 *                              WEATHER SYSTEM SERIALIZATION
 *                                  Agent: AGENT_WORLD_1
 * =================================================================================================
 *
 * Professional weather data persistence with JSON and binary formats,
 * version compatibility, and comprehensive error handling.
 *
 * =================================================================================================
 */

#include "weather_system.h"
#include "unified_memory_allocator.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Serialization format versions
#define WEATHER_SERIALIZATION_VERSION 1
#define WEATHER_BINARY_MAGIC 0x57454154 // 'WEAT'
#define WEATHER_JSON_MAGIC 0x4A534F4E // 'JSON'

// Maximum buffer sizes for safety
#define MAX_WEATHER_JSON_SIZE 65536
#define MAX_BINARY_BUFFER_SIZE 32768
#define MAX_STRING_LENGTH 1024

// Forward declarations for internal helpers
static bool serialize_weather_parameters_json(const WeatherParameters* params, char* buffer, size_t buffer_size);
static bool deserialize_weather_parameters_json(WeatherParameters* params, const char* json_data);

static bool serialize_time_of_day_json(const TimeOfDaySystem* system, char* buffer, size_t buffer_size);
static bool deserialize_time_of_day_json(TimeOfDaySystem* system, const char* json_data);

static bool serialize_volumetric_clouds_json(const VolumetricCloudSettings* clouds, char* buffer, size_t buffer_size);
static bool deserialize_volumetric_clouds_json(VolumetricCloudSettings* clouds, const char* json_data);

static bool serialize_precipitation_json(const PrecipitationSystem* precipitation, char* buffer, size_t buffer_size);
static bool deserialize_precipitation_json(PrecipitationSystem* precipitation, const char* json_data);

static bool serialize_lightning_json(const LightningSystem* lightning, char* buffer, size_t buffer_size);
static bool deserialize_lightning_json(LightningSystem* lightning, const char* json_data);

// Utility functions
static size_t json_write_string(char* buffer, size_t buffer_size, size_t offset, const char* key, const char* value);
static size_t json_write_float(char* buffer, size_t buffer_size, size_t offset, const char* key, float value);
static size_t json_write_int(char* buffer, size_t buffer_size, size_t offset, const char* key, int value);
static size_t json_write_bool(char* buffer, size_t buffer_size, size_t offset, const char* key, bool value);
static size_t json_write_float3(char* buffer, size_t buffer_size, size_t offset, const char* key, const float* values);
static size_t json_write_array_float(char* buffer, size_t buffer_size, size_t offset, const char* key, const float* values, size_t count);

// CRC32 for data integrity
static uint32_t calculate_crc32(const uint8_t* data, size_t size);

// Declared before use: json_write_uint is called by the writers above its definition.
static size_t json_write_uint(char* buffer, size_t buffer_size, size_t offset, const char* key, unsigned int value);

// JSON readers, emitter and binary payload helpers (defined later in this file)
static bool weather_type_is_valid(int value);
static size_t json_remove_trailing_comma(char* buffer, size_t buffer_size, size_t offset);
static size_t json_emit(char* buffer, size_t buffer_size, size_t offset, const char* format, ...);
static bool parse_json_int(const char* json, const char* key, int* out);
static bool parse_json_uint(const char* json, const char* key, uint32_t* out);
static bool parse_json_float(const char* json, const char* key, float* out);
static bool parse_json_bool(const char* json, const char* key, bool* out);
static bool parse_json_string(const char* json, const char* key, char* out, size_t out_size);
static bool parse_json_float3(const char* json, const char* key, float out[3]);
static bool parse_json_array_float(const char* json, const char* key, float* out, size_t count);
static bool extract_json_object(const char* json, char* out, size_t out_size);
static bool extract_json_array_element(const char* json, size_t index, char* out, size_t out_size);
static bool serialize_weather_manager_data(const WeatherManager* manager, uint8_t* buffer, size_t size);
static bool deserialize_weather_manager_data(WeatherManager* manager, const uint8_t* buffer, size_t size);

/**
 * Serialize weather manager to JSON format
 * Returns: true on success, false on failure
 */
bool weather_manager_serialize(const WeatherManager* manager, char* buffer, size_t buffer_size) {
    if (!manager || !buffer || buffer_size == 0) {
        return false;
    }
    
    memset(buffer, 0, buffer_size);
    size_t offset = 0;
    
    // Write header
    offset = json_emit(buffer, buffer_size, offset, 
        "{\n  \"version\": %d,\n", WEATHER_SERIALIZATION_VERSION);
    
    // Write current weather
    offset = json_write_int(buffer, buffer_size, offset, "current_weather", manager->current_weather);
    
    // Write weather presets
    offset = json_emit(buffer, buffer_size, offset, "  \"weather_presets\": [\n");
    for (int i = 0; i < 15; i++) {
        char preset_buffer[4096];
        if (serialize_weather_parameters_json(&manager->weather_presets[i], preset_buffer, sizeof(preset_buffer))) {
            offset = json_emit(buffer, buffer_size, offset, "    %s%s\n", 
                preset_buffer, (i < 14) ? "," : "");
        }
    }
    offset = json_emit(buffer, buffer_size, offset, "  ],\n");
    
    // Write transition state
    offset = json_emit(buffer, buffer_size, offset, "  \"transition\": {\n");
    offset = json_write_int(buffer, buffer_size, offset, "from_weather", manager->transition.from_weather);
    offset = json_write_int(buffer, buffer_size, offset, "to_weather", manager->transition.to_weather);
    offset = json_write_float(buffer, buffer_size, offset, "duration", manager->transition.duration);
    offset = json_write_float(buffer, buffer_size, offset, "progress", manager->transition.progress);
    offset = json_write_bool(buffer, buffer_size, offset, "is_active", manager->transition.is_active);
    offset = json_remove_trailing_comma(buffer, buffer_size, offset);
    offset = json_emit(buffer, buffer_size, offset, "  },\n");
    
    // Write time of day system
    char time_buffer[8192];
    if (serialize_time_of_day_json(&manager->time_of_day, time_buffer, sizeof(time_buffer))) {
        offset = json_emit(buffer, buffer_size, offset, "  \"time_of_day\": %s,\n", time_buffer);
    }
    
    // Write volumetric clouds
    char cloud_buffer[4096];
    if (serialize_volumetric_clouds_json(&manager->clouds, cloud_buffer, sizeof(cloud_buffer))) {
        offset = json_emit(buffer, buffer_size, offset, "  \"clouds\": %s,\n", cloud_buffer);
    }
    
    // Write precipitation system
    char precip_buffer[2048];
    if (serialize_precipitation_json(&manager->precipitation, precip_buffer, sizeof(precip_buffer))) {
        offset = json_emit(buffer, buffer_size, offset, "  \"precipitation\": %s,\n", precip_buffer);
    }
    
    // Write lightning system
    char lightning_buffer[2048];
    if (serialize_lightning_json(&manager->lightning, lightning_buffer, sizeof(lightning_buffer))) {
        offset = json_emit(buffer, buffer_size, offset, "  \"lightning\": %s,\n", lightning_buffer);
    }
    
    // Write weather zones
    offset = json_emit(buffer, buffer_size, offset, "  \"zones\": [\n");
    for (uint32_t i = 0; i < manager->zone_count; i++) {
        const WeatherZone* zone = &manager->zones[i];
        offset = json_emit(buffer, buffer_size, offset, "    {\n");
        offset = json_write_uint(buffer, buffer_size, offset, "id", zone->id);
        offset = json_write_string(buffer, buffer_size, offset, "name", zone->name);
        offset = json_write_float3(buffer, buffer_size, offset, "bounds_min", zone->bounds_min);
        offset = json_write_float3(buffer, buffer_size, offset, "bounds_max", zone->bounds_max);
        offset = json_write_float(buffer, buffer_size, offset, "blend_distance", zone->blend_distance);
        offset = json_write_int(buffer, buffer_size, offset, "forced_weather", zone->forced_weather);
        offset = json_write_bool(buffer, buffer_size, offset, "override_global", zone->override_global);
        offset = json_remove_trailing_comma(buffer, buffer_size, offset);
        offset = json_emit(buffer, buffer_size, offset, "    }%s\n", (i < manager->zone_count - 1) ? "," : "");
    }
    offset = json_emit(buffer, buffer_size, offset, "  ],\n");
    
    // Write auto weather settings
    offset = json_emit(buffer, buffer_size, offset, "  \"auto_weather\": {\n");
    offset = json_write_bool(buffer, buffer_size, offset, "enabled", manager->auto_weather_enabled);
    offset = json_write_float(buffer, buffer_size, offset, "change_interval", manager->weather_change_interval);
    offset = json_write_float(buffer, buffer_size, offset, "next_change", manager->next_weather_change);
    offset = json_write_array_float(buffer, buffer_size, offset, "probabilities", manager->weather_probabilities, 15);
    offset = json_remove_trailing_comma(buffer, buffer_size, offset);
    offset = json_emit(buffer, buffer_size, offset, "  },\n");
    
    // Write seasonal settings
    offset = json_emit(buffer, buffer_size, offset, "  \"seasonal\": {\n");
    offset = json_write_bool(buffer, buffer_size, offset, "enabled", manager->seasonal_weather);
    offset = json_write_uint(buffer, buffer_size, offset, "current_season", manager->current_season);
    offset = json_remove_trailing_comma(buffer, buffer_size, offset);
    offset = json_emit(buffer, buffer_size, offset, "  }\n");
    
    // Close JSON
    offset = json_emit(buffer, buffer_size, offset, "}\n");
    
    return offset < buffer_size;
}

/**
 * Deserialize weather manager from JSON format
 * Returns: true on success, false on failure
 */
bool weather_manager_deserialize(WeatherManager* manager, const char* json_data) {
    if (!manager || !json_data) {
        return false;
    }
    
    // Parse version
    int version = 0;
    if (sscanf(json_data, "{ \"version\": %d", &version) != 1) {
        return false;
    }
    
    if (version != WEATHER_SERIALIZATION_VERSION) {
        fprintf(stderr, "Weather serialization version mismatch: expected %d, got %d\n", 
                WEATHER_SERIALIZATION_VERSION, version);
        return false;
    }
    
    // Initialize manager
    weather_manager_init(manager);
    
    // Parse current weather
    int current_weather = 0;
    if (parse_json_int(json_data, "current_weather", &current_weather) && weather_type_is_valid(current_weather)) {
        manager->current_weather = (WeatherType)current_weather;
    }
    
    // Parse weather presets
    const char* presets_start = strstr(json_data, "\"weather_presets\":");
    if (presets_start) {
        presets_start = strchr(presets_start, '[');
        if (presets_start) {
            for (int i = 0; i < 15; i++) {
                char preset_json[4096];
                if (extract_json_array_element(presets_start, i, preset_json, sizeof(preset_json))) {
                    deserialize_weather_parameters_json(&manager->weather_presets[i], preset_json);
                }
            }
        }
    }
    
    // Parse transition state
    const char* transition_start = strstr(json_data, "\"transition\":");
    if (transition_start) {
        int from_weather = 0;
        if (parse_json_int(transition_start, "from_weather", &from_weather) && weather_type_is_valid(from_weather)) {
            manager->transition.from_weather = (WeatherType)from_weather;
        }
        int to_weather = 0;
        if (parse_json_int(transition_start, "to_weather", &to_weather) && weather_type_is_valid(to_weather)) {
            manager->transition.to_weather = (WeatherType)to_weather;
        }
        parse_json_float(transition_start, "duration", &manager->transition.duration);
        parse_json_float(transition_start, "progress", &manager->transition.progress);
        parse_json_bool(transition_start, "is_active", &manager->transition.is_active);
    }
    
    // Parse time of day
    const char* time_start = strstr(json_data, "\"time_of_day\":");
    if (time_start) {
        char time_json[8192];
        if (extract_json_object(time_start, time_json, sizeof(time_json))) {
            deserialize_time_of_day_json(&manager->time_of_day, time_json);
        }
    }
    
    // Parse volumetric clouds
    const char* cloud_start = strstr(json_data, "\"clouds\":");
    if (cloud_start) {
        char cloud_json[4096];
        if (extract_json_object(cloud_start, cloud_json, sizeof(cloud_json))) {
            deserialize_volumetric_clouds_json(&manager->clouds, cloud_json);
        }
    }
    
    // Parse precipitation
    const char* precip_start = strstr(json_data, "\"precipitation\":");
    if (precip_start) {
        char precip_json[2048];
        if (extract_json_object(precip_start, precip_json, sizeof(precip_json))) {
            deserialize_precipitation_json(&manager->precipitation, precip_json);
        }
    }
    
    // Parse lightning
    const char* lightning_start = strstr(json_data, "\"lightning\":");
    if (lightning_start) {
        char lightning_json[2048];
        if (extract_json_object(lightning_start, lightning_json, sizeof(lightning_json))) {
            deserialize_lightning_json(&manager->lightning, lightning_json);
        }
    }
    
    // Parse weather zones
    const char* zones_start = strstr(json_data, "\"zones\":");
    if (zones_start) {
        zones_start = strchr(zones_start, '[');
        if (zones_start) {
            // Count array elements. Counting '{' would also count the objects after the zone array.
            uint32_t zone_count = 0;
            char zone_probe[1024];
            while (extract_json_array_element(zones_start, zone_count, zone_probe, sizeof(zone_probe))) {
                zone_count++;
            }
            
            if (zone_count > 0) {
                manager->zones = calloc(zone_count, sizeof(WeatherZone));
                if (manager->zones) {
                    manager->zone_count = zone_count;
                    for (uint32_t i = 0; i < zone_count; i++) {
                        char zone_json[1024];
                        if (extract_json_array_element(zones_start, i, zone_json, sizeof(zone_json))) {
                            parse_json_uint(zone_json, "id", &manager->zones[i].id);
                            parse_json_string(zone_json, "name", manager->zones[i].name, sizeof(manager->zones[i].name));
                            parse_json_float3(zone_json, "bounds_min", manager->zones[i].bounds_min);
                            parse_json_float3(zone_json, "bounds_max", manager->zones[i].bounds_max);
                            parse_json_float(zone_json, "blend_distance", &manager->zones[i].blend_distance);
                            int forced_weather = 0;
                            if (parse_json_int(zone_json, "forced_weather", &forced_weather) && weather_type_is_valid(forced_weather)) {
                                manager->zones[i].forced_weather = (WeatherType)forced_weather;
                            }
                            parse_json_bool(zone_json, "override_global", &manager->zones[i].override_global);
                        }
                    }
                }
            }
        }
    }
    
    // Parse auto weather settings
    const char* auto_start = strstr(json_data, "\"auto_weather\":");
    if (auto_start) {
        parse_json_bool(auto_start, "enabled", &manager->auto_weather_enabled);
        parse_json_float(auto_start, "change_interval", &manager->weather_change_interval);
        parse_json_float(auto_start, "next_change", &manager->next_weather_change);
        parse_json_array_float(auto_start, "probabilities", manager->weather_probabilities, 15);
    }
    
    // Parse seasonal settings
    const char* seasonal_start = strstr(json_data, "\"seasonal\":");
    if (seasonal_start) {
        parse_json_bool(seasonal_start, "enabled", &manager->seasonal_weather);
        parse_json_uint(seasonal_start, "current_season", &manager->current_season);
    }
    
    return true;
}

/**
 * Serialize weather manager to binary format
 * Returns: true on success, false on failure
 */
bool weather_manager_serialize_binary(const WeatherManager* manager, uint8_t* buffer, size_t* size) {
    if (!manager || !buffer || !size || *size == 0) {
        return false;
    }
    
    size_t required_size = sizeof(uint32_t) + // Magic
                          sizeof(uint32_t) + // Version
                          sizeof(uint32_t) + // CRC32
                          sizeof(WeatherManager); // Main data
    
    if (*size < required_size) {
        *size = required_size;
        return false;
    }
    
    size_t offset = 0;
    
    // Write magic number
    *(uint32_t*)(buffer + offset) = WEATHER_BINARY_MAGIC;
    offset += sizeof(uint32_t);
    
    // Write version
    *(uint32_t*)(buffer + offset) = WEATHER_SERIALIZATION_VERSION;
    offset += sizeof(uint32_t);
    
    // Reserve space for CRC32 (will be filled later)
    uint32_t* crc_ptr = (uint32_t*)(buffer + offset);
    offset += sizeof(uint32_t);
    
    // Serialize main weather manager structure: exactly the payload the reader expects
    if (!serialize_weather_manager_data(manager, buffer + offset, required_size - offset)) {
        return false;
    }
    
    // Calculate and write CRC32 over the payload (not over the caller's buffer capacity)
    uint32_t crc = calculate_crc32(buffer + offset, required_size - offset);
    *crc_ptr = crc;
    
    *size = required_size;
    return true;
}

/**
 * Deserialize weather manager from binary format
 * Returns: true on success, false on failure
 */
bool weather_manager_deserialize_binary(WeatherManager* manager, const uint8_t* buffer, size_t size) {
    if (!manager || !buffer || size < sizeof(uint32_t) * 3) {
        return false;
    }
    
    size_t offset = 0;
    
    // Verify magic number
    uint32_t magic = *(uint32_t*)(buffer + offset);
    offset += sizeof(uint32_t);
    if (magic != WEATHER_BINARY_MAGIC) {
        fprintf(stderr, "Invalid binary weather data magic: 0x%08X\n", magic);
        return false;
    }
    
    // Verify version
    uint32_t version = *(uint32_t*)(buffer + offset);
    offset += sizeof(uint32_t);
    if (version != WEATHER_SERIALIZATION_VERSION) {
        fprintf(stderr, "Binary weather serialization version mismatch: expected %d, got %d\n", 
                WEATHER_SERIALIZATION_VERSION, version);
        return false;
    }
    
    // Verify CRC32
    uint32_t stored_crc = *(uint32_t*)(buffer + offset);
    offset += sizeof(uint32_t);
    
    // The payload is always sizeof(WeatherManager) bytes; bytes after it are ignored.
    const size_t payload_size = sizeof(WeatherManager);
    if (size - offset < payload_size) {
        return false;
    }
    
    uint32_t calculated_crc = calculate_crc32(buffer + offset, payload_size);
    if (stored_crc != calculated_crc) {
        fprintf(stderr, "Weather data CRC32 mismatch: expected 0x%08X, got 0x%08X\n", 
                stored_crc, calculated_crc);
        return false;
    }
    
    // Deserialize main data
    if (!deserialize_weather_manager_data(manager, buffer + offset, payload_size)) {
        return false;
    }
    
    return true;
}

// Internal serialization helpers
static bool serialize_weather_parameters_json(const WeatherParameters* params, char* buffer, size_t buffer_size) {
    if (!params || !buffer || buffer_size == 0) {
        return false;
    }
    
    size_t offset = 0;
    offset = json_emit(buffer, buffer_size, offset, "{\n");
    
    offset = json_write_int(buffer, buffer_size, offset, "type", params->type);
    offset = json_write_string(buffer, buffer_size, offset, "name", params->name);
    offset = json_write_float3(buffer, buffer_size, offset, "sky_color", params->sky_color);
    offset = json_write_float3(buffer, buffer_size, offset, "horizon_color", params->horizon_color);
    offset = json_write_float(buffer, buffer_size, offset, "cloud_coverage", params->cloud_coverage);
    offset = json_write_float(buffer, buffer_size, offset, "cloud_speed", params->cloud_speed);
    offset = json_write_float(buffer, buffer_size, offset, "cloud_height", params->cloud_height);
    offset = json_write_uint(buffer, buffer_size, offset, "cloud_texture", params->cloud_texture);
    
    offset = json_write_bool(buffer, buffer_size, offset, "has_precipitation", params->has_precipitation);
    offset = json_write_float(buffer, buffer_size, offset, "precipitation_intensity", params->precipitation_intensity);
    offset = json_write_uint(buffer, buffer_size, offset, "precipitation_texture", params->precipitation_texture);
    offset = json_write_float(buffer, buffer_size, offset, "precipitation_size", params->precipitation_size);
    offset = json_write_float(buffer, buffer_size, offset, "precipitation_speed", params->precipitation_speed);
    offset = json_write_bool(buffer, buffer_size, offset, "precipitation_affects_water", params->precipitation_affects_water);
    
    offset = json_write_float(buffer, buffer_size, offset, "fog_density", params->fog_density);
    offset = json_write_float(buffer, buffer_size, offset, "fog_height_falloff", params->fog_height_falloff);
    offset = json_write_float3(buffer, buffer_size, offset, "fog_color", params->fog_color);
    offset = json_write_float(buffer, buffer_size, offset, "fog_start_distance", params->fog_start_distance);
    offset = json_write_float(buffer, buffer_size, offset, "fog_end_distance", params->fog_end_distance);
    
    offset = json_write_float(buffer, buffer_size, offset, "sun_intensity_multiplier", params->sun_intensity_multiplier);
    offset = json_write_float(buffer, buffer_size, offset, "ambient_intensity_multiplier", params->ambient_intensity_multiplier);
    offset = json_write_float(buffer, buffer_size, offset, "shadow_intensity_multiplier", params->shadow_intensity_multiplier);
    
    offset = json_write_float(buffer, buffer_size, offset, "wind_speed", params->wind_speed);
    offset = json_write_float3(buffer, buffer_size, offset, "wind_direction", params->wind_direction);
    offset = json_write_float(buffer, buffer_size, offset, "wind_gustiness", params->wind_gustiness);
    
    offset = json_write_string(buffer, buffer_size, offset, "ambient_sound", params->ambient_sound);
    offset = json_write_float(buffer, buffer_size, offset, "ambient_volume", params->ambient_volume);
    
    offset = json_write_float(buffer, buffer_size, offset, "visibility_range", params->visibility_range);
    offset = json_write_float(buffer, buffer_size, offset, "movement_speed_modifier", params->movement_speed_modifier);
    offset = json_write_float(buffer, buffer_size, offset, "fire_extinguish_rate", params->fire_extinguish_rate);
    
    offset = json_emit(buffer, buffer_size, offset - 2, "\n    }");
    
    return offset < buffer_size;
}

// CRC32 implementation for data integrity
static uint32_t calculate_crc32(const uint8_t* data, size_t size) {
    static const uint32_t crc_table[256] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
        0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
        0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
        0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
        0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
        0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
        0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
        0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
        0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
        0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
        0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
        0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
        0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
        0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
        0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
        0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
        0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
        0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
        0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
        0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
        0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
        0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
        0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
        0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
        0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
        0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
        0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
        0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
        0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
        0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };
    
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        crc = (crc >> 8) ^ crc_table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

// Writes formatted text at `offset` and returns the new offset (absolute, like the json_write_*
// helpers). Nothing is written once offset reaches buffer_size, and the result may exceed
// buffer_size when the text is truncated, so callers test `offset < buffer_size` at the end.
static size_t json_vemit(char* buffer, size_t buffer_size, size_t offset, const char* format, va_list args) {
    if (offset >= buffer_size) {
        return offset;
    }
    int written = vsnprintf(buffer + offset, buffer_size - offset, format, args);
    if (written < 0) {
        return buffer_size;
    }
    return offset + (size_t)written;
}

static size_t json_emit(char* buffer, size_t buffer_size, size_t offset, const char* format, ...) {
    va_list args;
    va_start(args, format);
    size_t next = json_vemit(buffer, buffer_size, offset, format, args);
    va_end(args);
    return next;
}

// Utility functions for JSON writing
static size_t json_write_string(char* buffer, size_t buffer_size, size_t offset, const char* key, const char* value) {
    return json_emit(buffer, buffer_size, offset, "  \"%s\": \"%s\",\n", key, value);
}

static size_t json_write_float(char* buffer, size_t buffer_size, size_t offset, const char* key, float value) {
    return json_emit(buffer, buffer_size, offset, "  \"%s\": %.6f,\n", key, value);
}

static size_t json_write_int(char* buffer, size_t buffer_size, size_t offset, const char* key, int value) {
    return json_emit(buffer, buffer_size, offset, "  \"%s\": %d,\n", key, value);
}

static size_t json_write_uint(char* buffer, size_t buffer_size, size_t offset, const char* key, unsigned int value) {
    return json_emit(buffer, buffer_size, offset, "  \"%s\": %u,\n", key, value);
}

static size_t json_write_bool(char* buffer, size_t buffer_size, size_t offset, const char* key, bool value) {
    return json_emit(buffer, buffer_size, offset, "  \"%s\": %s,\n", key, value ? "true" : "false");
}

static size_t json_write_float3(char* buffer, size_t buffer_size, size_t offset, const char* key, const float* values) {
    return json_emit(buffer, buffer_size, offset, "  \"%s\": [%.6f, %.6f, %.6f],\n", key, values[0], values[1], values[2]);
}

static size_t json_write_array_float(char* buffer, size_t buffer_size, size_t offset, const char* key, const float* values, size_t count) {
    offset = json_emit(buffer, buffer_size, offset, "  \"%s\": [", key);
    for (size_t i = 0; i < count; i++) {
        offset = json_emit(buffer, buffer_size, offset, "%.6f%s", values[i], (i < count - 1) ? ", " : "");
    }
    return json_emit(buffer, buffer_size, offset, "],\n");
}

/* ==================================================================================================
 * JSON reading
 *
 * The writers above emit a fixed layout with unescaped strings, so the readers scan the text
 * directly. No JSON library is linked into the engine: src/engine/core/json.c is not in the build.
 * A key matches only a direct member of the object being searched, so a key that also appears in
 * a nested object cannot be mistaken for the outer one.
 * ================================================================================================== */

static bool weather_type_is_valid(int value) {
    return value >= 0 && value <= (int)WEATHER_SLEET;
}

static bool json_is_delimiter(char c) {
    return c == '\0' || c == ',' || c == '}' || c == ']' || c == ' ' || c == '\t' ||
           c == '\n' || c == '\r';
}

static const char* json_skip_ws(const char* p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }
    return p;
}

// p points at an opening quote. Returns the position after the closing quote, or NULL.
static const char* json_skip_string(const char* p) {
    if (*p != '"') {
        return NULL;
    }
    for (p++; *p != '"'; p++) {
        if (*p == '\0') {
            return NULL;
        }
        if (*p == '\\' && p[1] != '\0') {
            p++;
        }
    }
    return p + 1;
}

// p points at a JSON value. Returns the position after it, or NULL if it is malformed.
static const char* json_skip_value(const char* p) {
    p = json_skip_ws(p);
    if (*p == '"') {
        return json_skip_string(p);
    }
    if (*p == '{' || *p == '[') {
        int depth = 0;
        while (*p != '\0') {
            if (*p == '"') {
                p = json_skip_string(p);
                if (!p) {
                    return NULL;
                }
                continue;
            }
            if (*p == '{' || *p == '[') {
                depth++;
            } else if (*p == '}' || *p == ']') {
                if (--depth == 0) {
                    return p + 1;
                }
            }
            p++;
        }
        return NULL;
    }
    const char* start = p;
    while (!json_is_delimiter(*p)) {
        p++;
    }
    return (p == start) ? NULL : p;
}

// If json starts with a "name": pair, returns the position of the value after the colon.
// Otherwise returns the first non-space position of json.
static const char* json_value_start(const char* json) {
    const char* p = json_skip_ws(json);
    if (*p != '"') {
        return p;
    }
    p = json_skip_string(p);
    if (!p) {
        return NULL;
    }
    p = json_skip_ws(p);
    if (*p != ':') {
        return NULL;
    }
    return json_skip_ws(p + 1);
}

// Returns the value of `key` among the direct members of an object. json may point at the object,
// or at a "name": pair whose value is that object. Returns NULL if the key is absent.
static const char* json_find_member(const char* json, const char* key) {
    if (!json || !key) {
        return NULL;
    }
    const char* p = json_value_start(json);
    if (!p || *p != '{') {
        return NULL;
    }
    size_t key_length = strlen(key);
    p = json_skip_ws(p + 1);
    while (*p != '\0' && *p != '}') {
        const char* name_end = json_skip_string(p);
        if (!name_end) {
            return NULL;
        }
        bool match = (size_t)(name_end - p - 2) == key_length &&
                     strncmp(p + 1, key, key_length) == 0;
        p = json_skip_ws(name_end);
        if (*p != ':') {
            return NULL;
        }
        p = json_skip_ws(p + 1);
        if (match) {
            return p;
        }
        p = json_skip_value(p);
        if (!p) {
            return NULL;
        }
        p = json_skip_ws(p);
        if (*p == ',') {
            p = json_skip_ws(p + 1);
        }
    }
    return NULL;
}

static bool parse_json_int(const char* json, const char* key, int* out) {
    const char* value = json_find_member(json, key);
    if (!value || !out) {
        return false;
    }
    char* end = NULL;
    long parsed = strtol(value, &end, 10);
    if (end == value || !json_is_delimiter(*end) || parsed < INT_MIN || parsed > INT_MAX) {
        return false;
    }
    *out = (int)parsed;
    return true;
}

static bool parse_json_uint(const char* json, const char* key, uint32_t* out) {
    const char* value = json_find_member(json, key);
    if (!value || !out || *value == '-') {
        return false;
    }
    char* end = NULL;
    unsigned long long parsed = strtoull(value, &end, 10);
    if (end == value || !json_is_delimiter(*end) || parsed > UINT32_MAX) {
        return false;
    }
    *out = (uint32_t)parsed;
    return true;
}

static bool parse_json_float(const char* json, const char* key, float* out) {
    const char* value = json_find_member(json, key);
    if (!value || !out) {
        return false;
    }
    char* end = NULL;
    float parsed = strtof(value, &end);
    if (end == value || !json_is_delimiter(*end)) {
        return false;
    }
    *out = parsed;
    return true;
}

static bool parse_json_bool(const char* json, const char* key, bool* out) {
    const char* value = json_find_member(json, key);
    if (!value || !out) {
        return false;
    }
    if (strncmp(value, "true", 4) == 0 && json_is_delimiter(value[4])) {
        *out = true;
        return true;
    }
    if (strncmp(value, "false", 5) == 0 && json_is_delimiter(value[5])) {
        *out = false;
        return true;
    }
    return false;
}

// Copies a string value. Backslash escapes keep the escaped character. The output is written only
// if the whole string fits, so a rejected value leaves out unchanged.
static bool parse_json_string(const char* json, const char* key, char* out, size_t out_size) {
    const char* value = json_find_member(json, key);
    if (!value || !out || out_size == 0 || *value != '"') {
        return false;
    }
    size_t length = 0;
    for (const char* p = value + 1; *p != '"'; p++) {
        if (*p == '\0') {
            return false;
        }
        if (*p == '\\' && p[1] != '\0') {
            p++;
        }
        if (++length >= out_size) {
            return false;
        }
    }
    size_t n = 0;
    for (const char* p = value + 1; *p != '"'; p++) {
        if (*p == '\\' && p[1] != '\0') {
            p++;
        }
        out[n++] = *p;
    }
    out[n] = '\0';
    return true;
}

// Reads `count` comma-separated floats starting at p. Stores them in out if out is not NULL.
// Returns the position after the last value, or NULL if the text does not hold them.
static const char* json_read_floats(const char* p, float* out, size_t count) {
    p = json_skip_ws(p);
    for (size_t i = 0; i < count; i++) {
        char* end = NULL;
        float value = strtof(p, &end);
        if (end == p) {
            return NULL;
        }
        if (out) {
            out[i] = value;
        }
        p = json_skip_ws(end);
        if (i + 1 < count) {
            if (*p != ',') {
                return NULL;
            }
            p = json_skip_ws(p + 1);
        }
    }
    return p;
}

// Reads a JSON array of exactly `count` floats. Validates the whole array before writing to out.
static bool parse_json_array_float(const char* json, const char* key, float* out, size_t count) {
    const char* value = json_find_member(json, key);
    if (!value || !out || *value != '[') {
        return false;
    }
    const char* end = json_read_floats(value + 1, NULL, count);
    if (!end || *end != ']') {
        return false;
    }
    json_read_floats(value + 1, out, count);
    return true;
}

static bool parse_json_float3(const char* json, const char* key, float out[3]) {
    return parse_json_array_float(json, key, out, 3);
}

// Copies the object that json starts (or that a "name": pair before it holds) into out.
static bool extract_json_object(const char* json, char* out, size_t out_size) {
    if (!json || !out || out_size == 0) {
        return false;
    }
    const char* start = json_value_start(json);
    if (!start || *start != '{') {
        return false;
    }
    const char* end = json_skip_value(start);
    if (!end) {
        return false;
    }
    size_t length = (size_t)(end - start);
    if (length >= out_size) {
        return false;
    }
    memcpy(out, start, length);
    out[length] = '\0';
    return true;
}

// Copies element `index` of the array that json starts (or that a "name": pair before it holds).
// Returns false if there is no such element.
static bool extract_json_array_element(const char* json, size_t index, char* out, size_t out_size) {
    if (!json || !out || out_size == 0) {
        return false;
    }
    const char* p = json_value_start(json);
    if (!p || *p != '[') {
        return false;
    }
    p = json_skip_ws(p + 1);
    for (size_t i = 0; *p != ']'; i++) {
        if (*p == '\0') {
            return false;
        }
        const char* end = json_skip_value(p);
        if (!end) {
            return false;
        }
        if (i == index) {
            size_t length = (size_t)(end - p);
            if (length >= out_size) {
                return false;
            }
            memcpy(out, p, length);
            out[length] = '\0';
            return true;
        }
        p = json_skip_ws(end);
        if (*p == ',') {
            p = json_skip_ws(p + 1);
        } else if (*p != ']') {
            return false;
        }
    }
    return false;
}

// ==================================================================================================
// JSON writing for the subsystem objects
//
// Each member is written as `"key": value,`. An object is closed by json_close_object, which turns
// the final comma into the closing brace. Floats use %.9g, which round-trips a float exactly.
// ==================================================================================================

// Removes the comma that ends the last member, so a closing brace can follow. Returns the new offset.
static size_t json_remove_trailing_comma(char* buffer, size_t buffer_size, size_t offset) {
    if (offset >= 2 && offset <= buffer_size && buffer[offset - 2] == ',' && buffer[offset - 1] == '\n') {
        buffer[offset - 2] = '\n';
        return offset - 1;
    }
    return offset;
}

// Appends formatted text at *offset. Returns false, leaving *offset unchanged, if it does not fit.
static bool json_append(char* buffer, size_t buffer_size, size_t* offset, const char* format, ...) {
    va_list args;
    va_start(args, format);
    size_t next = json_vemit(buffer, buffer_size, *offset, format, args);
    va_end(args);
    if (next >= buffer_size) {
        return false;
    }
    *offset = next;
    return true;
}

static bool json_close_object(char* buffer, size_t buffer_size, size_t* offset) {
    if (*offset == 0 || *offset >= buffer_size) {
        return false;
    }
    if (buffer[*offset - 1] == ',') {
        buffer[*offset - 1] = '}';
        buffer[*offset] = '\0';
        return true;
    }
    return json_append(buffer, buffer_size, offset, "}");
}

static bool json_put_bool(char* buffer, size_t buffer_size, size_t* offset, const char* key, bool value) {
    return json_append(buffer, buffer_size, offset, "\"%s\": %s,", key, value ? "true" : "false");
}

static bool json_put_int(char* buffer, size_t buffer_size, size_t* offset, const char* key, int value) {
    return json_append(buffer, buffer_size, offset, "\"%s\": %d,", key, value);
}

static bool json_put_uint(char* buffer, size_t buffer_size, size_t* offset, const char* key, unsigned int value) {
    return json_append(buffer, buffer_size, offset, "\"%s\": %u,", key, value);
}

static bool json_put_float(char* buffer, size_t buffer_size, size_t* offset, const char* key, float value) {
    return json_append(buffer, buffer_size, offset, "\"%s\": %.9g,", key, (double)value);
}

static bool json_put_floats(char* buffer, size_t buffer_size, size_t* offset, const char* key,
                            const float* values, size_t count) {
    if (!json_append(buffer, buffer_size, offset, "\"%s\": [", key)) {
        return false;
    }
    for (size_t i = 0; i < count; i++) {
        if (i > 0 && !json_append(buffer, buffer_size, offset, ", ")) {
            return false;
        }
        if (!json_append(buffer, buffer_size, offset, "%.9g", (double)values[i])) {
            return false;
        }
    }
    return json_append(buffer, buffer_size, offset, "],");
}

// ==================================================================================================
// Subsystem serializers
//
// Persisted: every plain-data member. Not persisted: pointers and callbacks (process-local),
// time-of-day presets (pointer-owned; neither format writes them), and lightning bolts (transient
// render state; weather_manager_init owns their buffer). Zones are written by weather_manager_serialize.
// ==================================================================================================

static bool json_put_time_settings(char* buffer, size_t buffer_size, size_t* offset, const char* key,
                                   const TimeOfDaySettings* s) {
    return json_append(buffer, buffer_size, offset, "\"%s\": {", key) &&
           json_put_float(buffer, buffer_size, offset, "sun_azimuth", s->sun_azimuth) &&
           json_put_float(buffer, buffer_size, offset, "sun_elevation", s->sun_elevation) &&
           json_put_floats(buffer, buffer_size, offset, "sun_color", s->sun_color, 3) &&
           json_put_float(buffer, buffer_size, offset, "sun_intensity", s->sun_intensity) &&
           json_put_float(buffer, buffer_size, offset, "sun_disk_size", s->sun_disk_size) &&
           json_put_float(buffer, buffer_size, offset, "moon_azimuth", s->moon_azimuth) &&
           json_put_float(buffer, buffer_size, offset, "moon_elevation", s->moon_elevation) &&
           json_put_floats(buffer, buffer_size, offset, "moon_color", s->moon_color, 3) &&
           json_put_float(buffer, buffer_size, offset, "moon_intensity", s->moon_intensity) &&
           json_put_float(buffer, buffer_size, offset, "moon_phase", s->moon_phase) &&
           json_put_floats(buffer, buffer_size, offset, "zenith_color", s->zenith_color, 3) &&
           json_put_floats(buffer, buffer_size, offset, "horizon_color", s->horizon_color, 3) &&
           json_put_floats(buffer, buffer_size, offset, "ground_color", s->ground_color, 3) &&
           json_put_float(buffer, buffer_size, offset, "star_intensity", s->star_intensity) &&
           json_put_float(buffer, buffer_size, offset, "atmosphere_thickness", s->atmosphere_thickness) &&
           json_put_floats(buffer, buffer_size, offset, "ambient_color", s->ambient_color, 3) &&
           json_put_float(buffer, buffer_size, offset, "ambient_intensity", s->ambient_intensity) &&
           json_put_floats(buffer, buffer_size, offset, "fog_color", s->fog_color, 3) &&
           json_put_floats(buffer, buffer_size, offset, "shadow_color", s->shadow_color, 3) &&
           json_close_object(buffer, buffer_size, offset) &&
           json_append(buffer, buffer_size, offset, ",");
}

static void deserialize_time_settings_json(TimeOfDaySettings* s, const char* json) {
    parse_json_float(json, "sun_azimuth", &s->sun_azimuth);
    parse_json_float(json, "sun_elevation", &s->sun_elevation);
    parse_json_float3(json, "sun_color", s->sun_color);
    parse_json_float(json, "sun_intensity", &s->sun_intensity);
    parse_json_float(json, "sun_disk_size", &s->sun_disk_size);
    parse_json_float(json, "moon_azimuth", &s->moon_azimuth);
    parse_json_float(json, "moon_elevation", &s->moon_elevation);
    parse_json_float3(json, "moon_color", s->moon_color);
    parse_json_float(json, "moon_intensity", &s->moon_intensity);
    parse_json_float(json, "moon_phase", &s->moon_phase);
    parse_json_float3(json, "zenith_color", s->zenith_color);
    parse_json_float3(json, "horizon_color", s->horizon_color);
    parse_json_float3(json, "ground_color", s->ground_color);
    parse_json_float(json, "star_intensity", &s->star_intensity);
    parse_json_float(json, "atmosphere_thickness", &s->atmosphere_thickness);
    parse_json_float3(json, "ambient_color", s->ambient_color);
    parse_json_float(json, "ambient_intensity", &s->ambient_intensity);
    parse_json_float3(json, "fog_color", s->fog_color);
    parse_json_float3(json, "shadow_color", s->shadow_color);
}

static bool serialize_time_of_day_json(const TimeOfDaySystem* system, char* buffer, size_t buffer_size) {
    if (!system || !buffer || buffer_size == 0) {
        return false;
    }
    size_t offset = 0;
    bool ok = json_append(buffer, buffer_size, &offset, "{") &&
              json_put_float(buffer, buffer_size, &offset, "current_time", system->current_time) &&
              json_put_float(buffer, buffer_size, &offset, "time_scale", system->time_scale) &&
              json_put_bool(buffer, buffer_size, &offset, "is_paused", system->is_paused) &&
              json_put_uint(buffer, buffer_size, &offset, "day", system->day) &&
              json_put_uint(buffer, buffer_size, &offset, "month", system->month) &&
              json_put_uint(buffer, buffer_size, &offset, "year", system->year) &&
              json_put_float(buffer, buffer_size, &offset, "day_length_hours", system->day_length_hours) &&
              json_put_time_settings(buffer, buffer_size, &offset, "current_settings", &system->current_settings);
    return ok && json_close_object(buffer, buffer_size, &offset);
}

static bool deserialize_time_of_day_json(TimeOfDaySystem* system, const char* json_data) {
    if (!system || !json_data) {
        return false;
    }
    parse_json_float(json_data, "current_time", &system->current_time);
    parse_json_float(json_data, "time_scale", &system->time_scale);
    parse_json_bool(json_data, "is_paused", &system->is_paused);
    parse_json_uint(json_data, "day", &system->day);
    parse_json_uint(json_data, "month", &system->month);
    parse_json_uint(json_data, "year", &system->year);
    parse_json_float(json_data, "day_length_hours", &system->day_length_hours);
    const char* settings = json_find_member(json_data, "current_settings");
    if (settings) {
        deserialize_time_settings_json(&system->current_settings, settings);
    }
    return true;
}

static bool serialize_volumetric_clouds_json(const VolumetricCloudSettings* clouds, char* buffer, size_t buffer_size) {
    if (!clouds || !buffer || buffer_size == 0) {
        return false;
    }
    size_t offset = 0;
    bool ok = json_append(buffer, buffer_size, &offset, "{") &&
              json_put_bool(buffer, buffer_size, &offset, "enabled", clouds->enabled) &&
              json_put_float(buffer, buffer_size, &offset, "coverage", clouds->coverage) &&
              json_put_float(buffer, buffer_size, &offset, "cloud_type", clouds->cloud_type) &&
              json_put_float(buffer, buffer_size, &offset, "density", clouds->density) &&
              json_put_float(buffer, buffer_size, &offset, "altitude_min", clouds->altitude_min) &&
              json_put_float(buffer, buffer_size, &offset, "altitude_max", clouds->altitude_max) &&
              json_put_float(buffer, buffer_size, &offset, "detail_scale", clouds->detail_scale) &&
              json_put_float(buffer, buffer_size, &offset, "weather_scale", clouds->weather_scale) &&
              json_put_float(buffer, buffer_size, &offset, "wind_speed", clouds->wind_speed) &&
              json_put_floats(buffer, buffer_size, &offset, "wind_direction", clouds->wind_direction, 2) &&
              json_put_float(buffer, buffer_size, &offset, "light_absorption", clouds->light_absorption) &&
              json_put_float(buffer, buffer_size, &offset, "ambient_occlusion", clouds->ambient_occlusion) &&
              json_put_float(buffer, buffer_size, &offset, "shadow_strength", clouds->shadow_strength) &&
              json_put_uint(buffer, buffer_size, &offset, "shape_noise", clouds->shape_noise) &&
              json_put_uint(buffer, buffer_size, &offset, "detail_noise", clouds->detail_noise) &&
              json_put_uint(buffer, buffer_size, &offset, "weather_map", clouds->weather_map) &&
              json_put_int(buffer, buffer_size, &offset, "ray_march_steps", clouds->ray_march_steps) &&
              json_put_int(buffer, buffer_size, &offset, "light_march_steps", clouds->light_march_steps);
    return ok && json_close_object(buffer, buffer_size, &offset);
}

static bool deserialize_volumetric_clouds_json(VolumetricCloudSettings* clouds, const char* json_data) {
    if (!clouds || !json_data) {
        return false;
    }
    parse_json_bool(json_data, "enabled", &clouds->enabled);
    parse_json_float(json_data, "coverage", &clouds->coverage);
    parse_json_float(json_data, "cloud_type", &clouds->cloud_type);
    parse_json_float(json_data, "density", &clouds->density);
    parse_json_float(json_data, "altitude_min", &clouds->altitude_min);
    parse_json_float(json_data, "altitude_max", &clouds->altitude_max);
    parse_json_float(json_data, "detail_scale", &clouds->detail_scale);
    parse_json_float(json_data, "weather_scale", &clouds->weather_scale);
    parse_json_float(json_data, "wind_speed", &clouds->wind_speed);
    parse_json_array_float(json_data, "wind_direction", clouds->wind_direction, 2);
    parse_json_float(json_data, "light_absorption", &clouds->light_absorption);
    parse_json_float(json_data, "ambient_occlusion", &clouds->ambient_occlusion);
    parse_json_float(json_data, "shadow_strength", &clouds->shadow_strength);
    parse_json_uint(json_data, "shape_noise", &clouds->shape_noise);
    parse_json_uint(json_data, "detail_noise", &clouds->detail_noise);
    parse_json_uint(json_data, "weather_map", &clouds->weather_map);
    parse_json_int(json_data, "ray_march_steps", &clouds->ray_march_steps);
    parse_json_int(json_data, "light_march_steps", &clouds->light_march_steps);
    return true;
}

static bool serialize_precipitation_json(const PrecipitationSystem* precipitation, char* buffer, size_t buffer_size) {
    if (!precipitation || !buffer || buffer_size == 0) {
        return false;
    }
    size_t offset = 0;
    bool ok = json_append(buffer, buffer_size, &offset, "{") &&
              json_put_bool(buffer, buffer_size, &offset, "active", precipitation->active) &&
              json_put_int(buffer, buffer_size, &offset, "type", (int)precipitation->type) &&
              json_put_float(buffer, buffer_size, &offset, "intensity", precipitation->intensity) &&
              json_put_float(buffer, buffer_size, &offset, "precipitation_amount", precipitation->precipitation_amount) &&
              json_put_float(buffer, buffer_size, &offset, "precipitation_rate", precipitation->precipitation_rate) &&
              json_put_uint(buffer, buffer_size, &offset, "particle_buffer", precipitation->particle_buffer) &&
              json_put_uint(buffer, buffer_size, &offset, "max_particles", precipitation->max_particles) &&
              json_put_uint(buffer, buffer_size, &offset, "active_particles", precipitation->active_particles) &&
              json_put_uint(buffer, buffer_size, &offset, "particle_texture", precipitation->particle_texture) &&
              json_put_float(buffer, buffer_size, &offset, "particle_size", precipitation->particle_size) &&
              json_put_float(buffer, buffer_size, &offset, "streak_length", precipitation->streak_length) &&
              json_put_float(buffer, buffer_size, &offset, "opacity", precipitation->opacity) &&
              json_put_float(buffer, buffer_size, &offset, "fall_speed", precipitation->fall_speed) &&
              json_put_float(buffer, buffer_size, &offset, "wind_influence", precipitation->wind_influence) &&
              json_put_float(buffer, buffer_size, &offset, "collision_offset", precipitation->collision_offset) &&
              json_put_bool(buffer, buffer_size, &offset, "spawn_splashes", precipitation->spawn_splashes) &&
              json_put_uint(buffer, buffer_size, &offset, "splash_texture", precipitation->splash_texture) &&
              json_put_float(buffer, buffer_size, &offset, "splash_rate", precipitation->splash_rate) &&
              json_put_float(buffer, buffer_size, &offset, "splash_size", precipitation->splash_size);
    return ok && json_close_object(buffer, buffer_size, &offset);
}

static bool deserialize_precipitation_json(PrecipitationSystem* precipitation, const char* json_data) {
    if (!precipitation || !json_data) {
        return false;
    }
    int type = 0;
    if (parse_json_int(json_data, "type", &type) && weather_type_is_valid(type)) {
        precipitation->type = (WeatherType)type;
    }
    parse_json_bool(json_data, "active", &precipitation->active);
    parse_json_float(json_data, "intensity", &precipitation->intensity);
    parse_json_float(json_data, "precipitation_amount", &precipitation->precipitation_amount);
    parse_json_float(json_data, "precipitation_rate", &precipitation->precipitation_rate);
    parse_json_uint(json_data, "particle_buffer", &precipitation->particle_buffer);
    parse_json_uint(json_data, "max_particles", &precipitation->max_particles);
    parse_json_uint(json_data, "active_particles", &precipitation->active_particles);
    parse_json_uint(json_data, "particle_texture", &precipitation->particle_texture);
    parse_json_float(json_data, "particle_size", &precipitation->particle_size);
    parse_json_float(json_data, "streak_length", &precipitation->streak_length);
    parse_json_float(json_data, "opacity", &precipitation->opacity);
    parse_json_float(json_data, "fall_speed", &precipitation->fall_speed);
    parse_json_float(json_data, "wind_influence", &precipitation->wind_influence);
    parse_json_float(json_data, "collision_offset", &precipitation->collision_offset);
    parse_json_bool(json_data, "spawn_splashes", &precipitation->spawn_splashes);
    parse_json_uint(json_data, "splash_texture", &precipitation->splash_texture);
    parse_json_float(json_data, "splash_rate", &precipitation->splash_rate);
    parse_json_float(json_data, "splash_size", &precipitation->splash_size);
    return true;
}

static bool serialize_lightning_json(const LightningSystem* lightning, char* buffer, size_t buffer_size) {
    if (!lightning || !buffer || buffer_size == 0) {
        return false;
    }
    size_t offset = 0;
    bool ok = json_append(buffer, buffer_size, &offset, "{") &&
              json_put_float(buffer, buffer_size, &offset, "strike_frequency", lightning->strike_frequency) &&
              json_put_float(buffer, buffer_size, &offset, "next_strike_time", lightning->next_strike_time) &&
              json_put_float(buffer, buffer_size, &offset, "strike_interval_min", lightning->strike_interval_min) &&
              json_put_float(buffer, buffer_size, &offset, "strike_interval_max", lightning->strike_interval_max) &&
              json_put_float(buffer, buffer_size, &offset, "bolt_lifetime", lightning->bolt_lifetime) &&
              json_put_float(buffer, buffer_size, &offset, "flash_duration", lightning->flash_duration) &&
              json_put_bool(buffer, buffer_size, &offset, "trigger_thunder", lightning->trigger_thunder);
    return ok && json_close_object(buffer, buffer_size, &offset);
}

static bool deserialize_lightning_json(LightningSystem* lightning, const char* json_data) {
    if (!lightning || !json_data) {
        return false;
    }
    parse_json_float(json_data, "strike_frequency", &lightning->strike_frequency);
    parse_json_float(json_data, "next_strike_time", &lightning->next_strike_time);
    parse_json_float(json_data, "strike_interval_min", &lightning->strike_interval_min);
    parse_json_float(json_data, "strike_interval_max", &lightning->strike_interval_max);
    parse_json_float(json_data, "bolt_lifetime", &lightning->bolt_lifetime);
    parse_json_float(json_data, "flash_duration", &lightning->flash_duration);
    parse_json_bool(json_data, "trigger_thunder", &lightning->trigger_thunder);
    return true;
}

static bool deserialize_weather_parameters_json(WeatherParameters* params, const char* json_data) {
    if (!params || !json_data) {
        return false;
    }
    int type = 0;
    if (parse_json_int(json_data, "type", &type) && weather_type_is_valid(type)) {
        params->type = (WeatherType)type;
    }
    parse_json_string(json_data, "name", params->name, sizeof(params->name));
    parse_json_float3(json_data, "sky_color", params->sky_color);
    parse_json_float3(json_data, "horizon_color", params->horizon_color);
    parse_json_float(json_data, "cloud_coverage", &params->cloud_coverage);
    parse_json_float(json_data, "cloud_speed", &params->cloud_speed);
    parse_json_float(json_data, "cloud_height", &params->cloud_height);
    parse_json_uint(json_data, "cloud_texture", &params->cloud_texture);
    parse_json_bool(json_data, "has_precipitation", &params->has_precipitation);
    parse_json_float(json_data, "precipitation_intensity", &params->precipitation_intensity);
    parse_json_uint(json_data, "precipitation_texture", &params->precipitation_texture);
    parse_json_float(json_data, "precipitation_size", &params->precipitation_size);
    parse_json_float(json_data, "precipitation_speed", &params->precipitation_speed);
    parse_json_bool(json_data, "precipitation_affects_water", &params->precipitation_affects_water);
    parse_json_float(json_data, "fog_density", &params->fog_density);
    parse_json_float(json_data, "fog_height_falloff", &params->fog_height_falloff);
    parse_json_float3(json_data, "fog_color", params->fog_color);
    parse_json_float(json_data, "fog_start_distance", &params->fog_start_distance);
    parse_json_float(json_data, "fog_end_distance", &params->fog_end_distance);
    parse_json_float(json_data, "sun_intensity_multiplier", &params->sun_intensity_multiplier);
    parse_json_float(json_data, "ambient_intensity_multiplier", &params->ambient_intensity_multiplier);
    parse_json_float(json_data, "shadow_intensity_multiplier", &params->shadow_intensity_multiplier);
    parse_json_float(json_data, "wind_speed", &params->wind_speed);
    parse_json_float3(json_data, "wind_direction", params->wind_direction);
    parse_json_float(json_data, "wind_gustiness", &params->wind_gustiness);
    parse_json_string(json_data, "ambient_sound", params->ambient_sound, sizeof(params->ambient_sound));
    parse_json_float(json_data, "ambient_volume", &params->ambient_volume);
    parse_json_float(json_data, "visibility_range", &params->visibility_range);
    parse_json_float(json_data, "movement_speed_modifier", &params->movement_speed_modifier);
    parse_json_float(json_data, "fire_extinguish_rate", &params->fire_extinguish_rate);
    return true;
}

// ==================================================================================================
// Binary payload
//
// The payload is the WeatherManager struct as it lies in memory, so it has a fixed size. Pointers
// and callbacks are process-local: they are cleared when saving and never restored from the file.
// Zones (JSON only) and time-of-day presets (neither format) are therefore not in the payload.
// ==================================================================================================

static void clear_process_local_state(WeatherManager* manager) {
    manager->zones = NULL;
    manager->zone_count = 0;
    manager->time_of_day.presets = NULL;
    manager->time_of_day.preset_count = 0;
    manager->time_of_day.on_sunrise = NULL;
    manager->time_of_day.on_sunset = NULL;
    manager->time_of_day.on_midnight = NULL;
    manager->time_of_day.on_noon = NULL;
    manager->lightning.bolts = NULL;
    manager->lightning.bolt_count = 0;
    manager->lightning.max_bolts = 0;
}

static bool serialize_weather_manager_data(const WeatherManager* manager, uint8_t* buffer, size_t size) {
    if (!manager || !buffer || size < sizeof(WeatherManager)) {
        return false;
    }
    WeatherManager snapshot;
    memset(&snapshot, 0, sizeof(snapshot));
    memcpy(&snapshot, manager, sizeof(snapshot));
    clear_process_local_state(&snapshot);
    memcpy(buffer, &snapshot, sizeof(snapshot));
    return true;
}

static bool deserialize_weather_manager_data(WeatherManager* manager, const uint8_t* buffer, size_t size) {
    if (!manager || !buffer || size < sizeof(WeatherManager)) {
        return false;
    }
    WeatherManager snapshot;
    memcpy(&snapshot, buffer, sizeof(snapshot));
    if (!weather_type_is_valid((int)snapshot.current_weather) ||
        !weather_type_is_valid((int)snapshot.transition.from_weather) ||
        !weather_type_is_valid((int)snapshot.transition.to_weather)) {
        return false;
    }

    // Initialise first (this allocates the lightning bolt buffer), then keep that buffer.
    weather_manager_init(manager);
    LightningBolt* bolts = manager->lightning.bolts;
    uint32_t max_bolts = manager->lightning.max_bolts;
    *manager = snapshot;
    clear_process_local_state(manager);
    manager->lightning.bolts = bolts;
    manager->lightning.max_bolts = max_bolts;
    return true;
}
