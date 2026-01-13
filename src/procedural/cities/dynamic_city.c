/*
 * dynamic_city.c
 * Dynamic city generation algorithms
 *
 * Part of the Procedural Generation subsystem
 * Advanced 3D Rendering Engine
 */

#include "dynamic_city.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static float random_float(uint32_t* seed) {
    *seed = (*seed * 1103515245 + 12345) & 0x7fffffff;
    return (float)*seed / (float)0x7fffffff;
}

static Vec3 random_point_in_circle(uint32_t* seed, float radius) {
    float angle = random_float(seed) * 2.0f * M_PI;
    float r = sqrtf(random_float(seed)) * radius;
    
    Vec3 point;
    point.x = cosf(angle) * r;
    point.y = 0.0f;
    point.z = sinf(angle) * r;
    return point;
}

static float distance(const Vec3* a, const Vec3* b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

CityGenerator* city_create_generator(const Vec3* center, float radius, uint32_t seed) {
    if (!center || radius <= 0) return NULL;
    
    CityGenerator* generator = malloc(sizeof(CityGenerator));
    if (!generator) return NULL;
    
    generator->city = malloc(sizeof(CityData));
    if (!generator->city) {
        free(generator);
        return NULL;
    }
    
    generator->city->city_center = *center;
    generator->city->city_radius = radius;
    generator->city->seed = seed;
    generator->city->buildings = NULL;
    generator->city->building_count = 0;
    generator->city->roads = NULL;
    generator->city->road_count = 0;
    generator->city->zones = NULL;
    generator->city->zone_count = 0;
    
    generator->params = malloc(sizeof(CityParameters));
    if (!generator->params) {
        free(generator->city);
        free(generator);
        return NULL;
    }
    
    // Default parameters
    generator->params->min_building_height = 10.0f;
    generator->params->max_building_height = 100.0f;
    generator->params->building_density = 0.7f;
    generator->params->road_spacing = 50.0f;
    generator->params->block_size = 100.0f;
    generator->params->building_styles = 5;
    generator->params->generate_parks = true;
    generator->params->generate_water_features = true;
    
    generator->is_generated = false;
    
    return generator;
}

bool city_generate_road_network(CityGenerator* generator) {
    if (!generator) return false;
    
    uint32_t seed = generator->city->seed;
    float radius = generator->city->city_radius;
    float spacing = generator->params->road_spacing;
    
    // Calculate number of roads needed
    uint32_t num_radial = (uint32_t)(radius / spacing) + 1;
    uint32_t num_circular = (uint32_t)(radius / spacing) + 1;
    uint32_t total_roads = num_radial + num_circular + 10; // Extra for connections
    
    generator->city->roads = malloc(total_roads * sizeof(Road));
    if (!generator->city->roads) return false;
    
    generator->city->road_count = 0;
    
    // Generate radial roads
    for (uint32_t i = 0; i < num_radial; i++) {
        float angle = (float)i / (float)num_radial * 2.0f * M_PI;
        
        Road road;
        road.start = generator->city->city_center;
        road.end.x = road.start.x + cosf(angle) * radius;
        road.end.y = road.start.y;
        road.end.z = road.start.z + sinf(angle) * radius;
        road.width = 8.0f;
        road.road_type = 0; // Main road
        road.one_way = false;
        
        generator->city->roads[generator->city->road_count++] = road;
    }
    
    // Generate circular roads
    for (uint32_t i = 1; i <= num_circular; i++) {
        float r = (float)i / (float)num_circular * radius;
        uint32_t segments = 32;
        
        for (uint32_t j = 0; j < segments; j++) {
            float angle1 = (float)j / (float)segments * 2.0f * M_PI;
            float angle2 = (float)(j + 1) / (float)segments * 2.0f * M_PI;
            
            Road road;
            road.start.x = generator->city->city_center.x + cosf(angle1) * r;
            road.start.y = generator->city->city_center.y;
            road.start.z = generator->city->city_center.z + sinf(angle1) * r;
            road.end.x = generator->city->city_center.x + cosf(angle2) * r;
            road.end.y = generator->city->city_center.y;
            road.end.z = generator->city->city_center.z + sinf(angle2) * r;
            road.width = 6.0f;
            road.road_type = 1; // Secondary road
            road.one_way = false;
            
            if (generator->city->road_count < total_roads) {
                generator->city->roads[generator->city->road_count++] = road;
            }
        }
    }
    
    return true;
}

bool city_generate_zones(CityGenerator* generator) {
    if (!generator) return false;
    
    uint32_t seed = generator->city->seed;
    float radius = generator->city->city_radius;
    
    // Create zones: residential, commercial, industrial, parks
    uint32_t num_zones = 8 + (random_float(&seed) > 0.5f ? 4 : 0); // 8-12 zones
    
    generator->city->zones = malloc(num_zones * sizeof(CityZone));
    if (!generator->city->zones) return false;
    
    generator->city->zone_count = 0;
    
    // Central business district
    CityZone cbd;
    cbd.center = generator->city->city_center;
    cbd.radius = radius * 0.2f;
    cbd.zone_type = 2; // Commercial
    cbd.density = 0.9f;
    generator->city->zones[generator->city->zone_count++] = cbd;
    
    // Surrounding zones
    for (uint32_t i = 0; i < num_zones - 1; i++) {
        CityZone zone;
        zone.center = random_point_in_circle(&seed, radius * 0.8f);
        zone.radius = radius * (0.1f + random_float(&seed) * 0.2f);
        zone.zone_type = (uint32_t)(random_float(&seed) * 3.0f); // 0=residential, 1=industrial, 2=commercial
        zone.density = 0.5f + random_float(&seed) * 0.4f;
        
        generator->city->zones[generator->city->zone_count++] = zone;
    }
    
    return true;
}

bool city_generate_buildings(CityGenerator* generator) {
    if (!generator) return false;
    
    uint32_t seed = generator->city->seed;
    float radius = generator->city->city_radius;
    float density = generator->params->building_density;
    
    // Estimate number of buildings
    float area = M_PI * radius * radius;
    uint32_t max_buildings = (uint32_t)(area * density / 100.0f); // Rough estimate
    
    generator->city->buildings = malloc(max_buildings * sizeof(Building));
    if (!generator->city->buildings) return false;
    
    generator->city->building_count = 0;
    
    // Generate buildings in each zone
    for (uint32_t z = 0; z < generator->city->zone_count; z++) {
        CityZone* zone = &generator->city->zones[z];
        uint32_t buildings_in_zone = (uint32_t)(zone->density * max_buildings / generator->city->zone_count);
        
        for (uint32_t i = 0; i < buildings_in_zone && generator->city->building_count < max_buildings; i++) {
            Building building;
            
            // Random position within zone
            Vec3 offset = random_point_in_circle(&seed, zone->radius * 0.8f);
            building.position.x = zone->center.x + offset.x;
            building.position.y = zone->center.y;
            building.position.z = zone->center.z + offset.z;
            
            // Building size based on zone type
            float size_factor = 1.0f;
            if (zone->zone_type == 2) { // Commercial
                size_factor = 1.5f;
            } else if (zone->zone_type == 1) { // Industrial
                size_factor = 2.0f;
            }
            
            building.size.x = (5.0f + random_float(&seed) * 15.0f) * size_factor;
            building.size.y = 0.0f; // Will be set by height
            building.size.z = (5.0f + random_float(&seed) * 15.0f) * size_factor;
            
            // Building height
            float height_factor = 1.0f;
            if (zone->zone_type == 2) { // Commercial - taller
                height_factor = 2.0f;
            }
            
            building.height = generator->params->min_building_height + 
                            random_float(&seed) * (generator->params->max_building_height - generator->params->min_building_height) * height_factor;
            building.floors = (uint32_t)(building.height / 3.0f);
            building.building_type = zone->zone_type;
            building.style = (uint8_t)(random_float(&seed) * generator->params->building_styles);
            
            generator->city->buildings[generator->city->building_count++] = building;
        }
    }
    
    return true;
}

bool city_generate_parks(CityGenerator* generator) {
    if (!generator || !generator->params->generate_parks) return false;
    
    uint32_t seed = generator->city->seed + 1000;
    float radius = generator->city->city_radius;
    
    // Add 3-5 parks
    uint32_t num_parks = 3 + (uint32_t)(random_float(&seed) * 3.0f);
    
    for (uint32_t i = 0; i < num_parks; i++) {
        CityZone park;
        park.center = random_point_in_circle(&seed, radius * 0.7f);
        park.radius = 20.0f + random_float(&seed) * 50.0f;
        park.zone_type = 3; // Park
        park.density = 0.0f;
        
        // Reallocate zones array to include parks
        CityZone* new_zones = realloc(generator->city->zones, 
                                     (generator->city->zone_count + 1) * sizeof(CityZone));
        if (new_zones) {
            generator->city->zones = new_zones;
            generator->city->zones[generator->city->zone_count++] = park;
        }
    }
    
    return true;
}

bool city_generate_water_features(CityGenerator* generator) {
    if (!generator || !generator->params->generate_water_features) return false;
    
    uint32_t seed = generator->city->seed + 2000;
    float radius = generator->city->city_radius;
    
    // Add 1-2 water features
    uint32_t num_water = 1 + (uint32_t)(random_float(&seed) * 2.0f);
    
    for (uint32_t i = 0; i < num_water; i++) {
        CityZone water;
        water.center = random_point_in_circle(&seed, radius * 0.8f);
        water.radius = 30.0f + random_float(&seed) * 70.0f;
        water.zone_type = 4; // Water
        water.density = 0.0f;
        
        // Reallocate zones array to include water features
        CityZone* new_zones = realloc(generator->city->zones, 
                                     (generator->city->zone_count + 1) * sizeof(CityZone));
        if (new_zones) {
            generator->city->zones = new_zones;
            generator->city->zones[generator->city->zone_count++] = water;
        }
    }
    
    return true;
}

bool city_simulate_growth(CityGenerator* generator, float time_step) {
    if (!generator) return false;
    
    // Simple growth simulation - add new buildings over time
    if (random_float(&generator->city->seed) < 0.01f * time_step) {
        // Add a new building
        uint32_t new_count = generator->city->building_count + 1;
        Building* new_buildings = realloc(generator->city->buildings, new_count * sizeof(Building));
        
        if (new_buildings) {
            generator->city->buildings = new_buildings;
            
            // Generate new building at edge of city
            Vec3 edge_pos = random_point_in_circle(&generator->city->seed, generator->city->city_radius * 0.9f);
            
            Building new_building;
            new_building.position = edge_pos;
            new_building.size.x = 5.0f + random_float(&generator->city->seed) * 15.0f;
            new_building.size.y = 0.0f;
            new_building.size.z = 5.0f + random_float(&generator->city->seed) * 15.0f;
            new_building.height = generator->params->min_building_height + 
                                 random_float(&generator->city->seed) * 30.0f;
            new_building.floors = (uint32_t)(new_building.height / 3.0f);
            new_building.building_type = (uint32_t)(random_float(&generator->city->seed) * 3.0f);
            new_building.style = (uint8_t)(random_float(&generator->city->seed) * generator->params->building_styles);
            
            generator->city->buildings[generator->city->building_count++] = new_building;
        }
    }
    
    return true;
}

TrafficSystem* city_create_traffic_system(CityGenerator* generator) {
    if (!generator) return NULL;
    
    TrafficSystem* traffic = malloc(sizeof(TrafficSystem));
    if (!traffic) return NULL;
    
    traffic->traffic_density = 0.5f;
    traffic->simulate_traffic = true;
    
    // Create vehicles based on city size
    uint32_t num_vehicles = (uint32_t)(generator->city->building_count * traffic->traffic_density * 0.1f);
    traffic->vehicles = malloc(num_vehicles * sizeof(Vehicle));
    traffic->vehicle_count = num_vehicles;
    
    for (uint32_t i = 0; i < num_vehicles; i++) {
        Vehicle* vehicle = &traffic->vehicles[i];
        
        // Random starting position on a road
        if (generator->city->road_count > 0) {
            uint32_t road_idx = (uint32_t)(random_float(&generator->city->seed) * generator->city->road_count);
            Road* road = &generator->city->roads[road_idx];
            
            float t = random_float(&generator->city->seed);
            vehicle->position.x = road->start.x + t * (road->end.x - road->start.x);
            vehicle->position.y = road->start.y + t * (road->end.y - road->start.y);
            vehicle->position.z = road->start.z + t * (road->end.z - road->start.z);
            
            vehicle->velocity.x = 0.0f;
            vehicle->velocity.y = 0.0f;
            vehicle->velocity.z = 0.0f;
            
            vehicle->current_road = road_idx;
            vehicle->progress = t;
            vehicle->destination = (uint32_t)(random_float(&generator->city->seed) * generator->city->road_count);
        }
    }
    
    return traffic;
}

bool city_update_traffic(TrafficSystem* traffic, float delta_time) {
    if (!traffic || !traffic->simulate_traffic) return false;
    
    for (uint32_t i = 0; i < traffic->vehicle_count; i++) {
        Vehicle* vehicle = &traffic->vehicles[i];
        
        // Simple movement along roads
        vehicle->progress += delta_time * 0.1f; // Speed factor
        
        if (vehicle->progress >= 1.0f) {
            // Reached end of road, pick new road
            vehicle->progress = 0.0f;
            vehicle->current_road = vehicle->destination;
            vehicle->destination = (vehicle->destination + 1) % 10; // Simple cycling
        }
    }
    
    return true;
}

Building* city_get_nearest_building(const CityData* city, const Vec3* position) {
    if (!city || !position || city->building_count == 0) return NULL;
    
    Building* nearest = &city->buildings[0];
    float min_dist = distance(position, &nearest->position);
    
    for (uint32_t i = 1; i < city->building_count; i++) {
        float dist = distance(position, &city->buildings[i].position);
        if (dist < min_dist) {
            min_dist = dist;
            nearest = &city->buildings[i];
        }
    }
    
    return nearest;
}

CityZone* city_get_zone_at(const CityData* city, const Vec3* position) {
    if (!city || !position) return NULL;
    
    for (uint32_t i = 0; i < city->zone_count; i++) {
        float dist = distance(position, &city->zones[i].center);
        if (dist <= city->zones[i].radius) {
            return &city->zones[i];
        }
    }
    
    return NULL;
}

void city_destroy_generator(CityGenerator* generator) {
    if (!generator) return;
    
    city_destroy_data(generator->city);
    free(generator->params);
    free(generator);
}

void city_destroy_data(CityData* city) {
    if (!city) return;
    
    free(city->buildings);
    free(city->roads);
    free(city->zones);
    free(city);
}

void city_destroy_traffic_system(TrafficSystem* traffic) {
    if (!traffic) return;
    
    free(traffic->vehicles);
    free(traffic);
}
