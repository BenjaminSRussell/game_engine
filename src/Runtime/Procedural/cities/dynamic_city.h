#ifndef DYNAMIC_CITY_H
#define DYNAMIC_CITY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    Vec3 position;
    Vec3 size;
    uint32_t building_type;
    float height;
    uint32_t floors;
    uint8_t style;
} Building;

typedef struct {
    Vec3 start;
    Vec3 end;
    float width;
    uint32_t road_type;
    bool one_way;
} Road;

typedef struct {
    Vec3 center;
    float radius;
    uint32_t zone_type;
    float density;
} CityZone;

typedef struct {
    Building* buildings;
    uint32_t building_count;
    Road* roads;
    uint32_t road_count;
    CityZone* zones;
    uint32_t zone_count;
    Vec3 city_center;
    float city_radius;
    uint32_t seed;
} CityData;

typedef struct {
    float min_building_height;
    float max_building_height;
    float building_density;
    float road_spacing;
    float block_size;
    uint32_t building_styles;
    bool generate_parks;
    bool generate_water_features;
} CityParameters;

typedef struct {
    CityData* city;
    CityParameters* params;
    bool is_generated;
} CityGenerator;

// City generation functions
CityGenerator* city_create_generator(const Vec3* center, float radius, uint32_t seed);
bool city_generate_road_network(CityGenerator* generator);
bool city_generate_zones(CityGenerator* generator);
bool city_generate_buildings(CityGenerator* generator);
bool city_generate_parks(CityGenerator* generator);
bool city_generate_water_features(CityGenerator* generator);

// Road generation algorithms
bool city_generate_grid_roads(CityGenerator* generator);
bool city_generate_radial_roads(CityGenerator* generator);
bool city_generate_organic_roads(CityGenerator* generator);
bool city_connect_roads(CityGenerator* generator);

// Building generation algorithms
bool city_generate_residential_buildings(CityGenerator* generator, const CityZone* zone);
bool city_generate_commercial_buildings(CityGenerator* generator, const CityZone* zone);
bool city_generate_industrial_buildings(CityGenerator* generator, const CityZone* zone);
bool city_generate_special_buildings(CityGenerator* generator);

// Zone management
bool city_create_zone(CityGenerator* generator, const Vec3* center, float radius, uint32_t zone_type);
bool city_zone_buildings(CityGenerator* generator);
bool city_optimize_zones(CityGenerator* generator);

// Building styles and architecture
typedef struct {
    uint32_t style_id;
    char name[64];
    float min_height;
    float max_height;
    float height_variation;
    uint32_t color_palette[8];
    bool has_roof;
    bool has_balconies;
    bool has_details;
} BuildingStyle;

bool city_register_building_style(CityGenerator* generator, const BuildingStyle* style);
bool city_apply_building_style(CityGenerator* generator, uint32_t style_id);

// City growth and evolution
bool city_simulate_growth(CityGenerator* generator, float time_step);
bool city_add_buildings(CityGenerator* generator, uint32_t count);
bool city_remove_buildings(CityGenerator* generator, uint32_t count);
bool city_update_road_network(CityGenerator* generator);

// Traffic simulation
typedef struct {
    Vec3 position;
    Vec3 velocity;
    uint32_t current_road;
    float progress;
    uint32_t destination;
} Vehicle;

typedef struct {
    Vehicle* vehicles;
    uint32_t vehicle_count;
    float traffic_density;
    bool simulate_traffic;
} TrafficSystem;

TrafficSystem* city_create_traffic_system(CityGenerator* generator);
bool city_update_traffic(TrafficSystem* traffic, float delta_time);
void city_destroy_traffic_system(TrafficSystem* traffic);

// City services and utilities
bool city_place_power_lines(CityGenerator* generator);
bool city_place_water_systems(CityGenerator* generator);
bool city_place_public_transport(CityGenerator* generator);
bool city_place_emergency_services(CityGenerator* generator);

// Export and import
bool city_export_to_obj(const CityData* city, const char* filename);
bool city_export_to_json(const CityData* city, const char* filename);
bool city_import_city_data(CityData* city, const char* filename);

// Utility functions
Building* city_get_nearest_building(const CityData* city, const Vec3* position);
Road* city_get_nearest_road(const CityData* city, const Vec3* position);
CityZone* city_get_zone_at(const CityData* city, const Vec3* position);
bool city_is_position_in_city(const CityData* city, const Vec3* position);

// Cleanup
void city_destroy_generator(CityGenerator* generator);
void city_destroy_data(CityData* city);

#ifdef __cplusplus
}
#endif

#endif // DYNAMIC_CITY_H
