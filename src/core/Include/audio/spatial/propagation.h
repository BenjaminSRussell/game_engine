#ifndef SPATIAL_PROPAGATION_H
#define SPATIAL_PROPAGATION_H

#include <common.h>
#include "engine/include/math/math_all.h"

// -------------------------------------------------------------------------------------------------
// MATERIAL PROPERTIES FOR SOUND PROPAGATION
// -------------------------------------------------------------------------------------------------

typedef enum {
    MATERIAL_CONCRETE,
    MATERIAL_WOOD,
    MATERIAL_GLASS,
    MATERIAL_METAL,
    MATERIAL_FABRIC,
    MATERIAL_PLASTER,
    MATERIAL_BRICK,
    MATERIAL_AIR
} MaterialType;

typedef struct {
    MaterialType type;
    f32 absorption_low;     // Low frequency absorption (125Hz)
    f32 absorption_mid;     // Mid frequency absorption (1kHz)
    f32 absorption_high;    // High frequency absorption (4kHz)
    f32 transmission_loss;  // Transmission through material
} MaterialProperties;

// -------------------------------------------------------------------------------------------------
// PROPAGATION PATH TYPES
// -------------------------------------------------------------------------------------------------

typedef enum {
    PATH_DIRECT,        // Direct line-of-sight path
    PATH_REFLECTION,    // Reflected path off surfaces
    PATH_DIFFRACTION,   // Diffracted path around edges
    PATH_TRANSMISSION   // Transmitted path through materials
} PathType;

typedef struct {
    Vec3 position;
    f32 arrival_time;
    f32 energy;
    MaterialProperties material;
} PathPoint;

typedef struct {
    PathType type;
    Vec3 source;
    Vec3 listener;
    Vec3 points[16];        // Path waypoints
    u32 point_count;
    f32 distance;
    f32 energy;
    u32 reflection_count;
    u32 diffraction_count;
    bool obstructed;
    bool valid;
} PropagationPath;

// -------------------------------------------------------------------------------------------------
// PROPAGATION SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    // Configuration
    u32 max_paths;
    u32 max_reflections;
    f32 ray_step_size;
    f32 max_ray_distance;
    f32 diffraction_threshold;
    
    // Materials
    MaterialProperties* materials;
    u32 material_count;
    
    // Generated paths
    PropagationPath* paths;
    u32 path_count;
    
    bool initialized;
} PropagationSystem;

// -------------------------------------------------------------------------------------------------
// PROPAGATION SYSTEM FUNCTIONS
// -------------------------------------------------------------------------------------------------

bool Propagation_Init(PropagationSystem* system, u32 max_paths, u32 max_reflections);
void Propagation_Shutdown(PropagationSystem* system);

//  COMPLETED: Ray-based propagation path generation
u32 Propagation_GeneratePaths(PropagationSystem* system, const Vec3 source, const Vec3 listener, 
                             PropagationPath* paths, u32 max_paths);
PropagationPath Propagation_CreateDirectPath(const Vec3 source, const Vec3 listener);
u32 Propagation_GenerateReflectionPaths(PropagationSystem* system, const Vec3 source, const Vec3 listener,
                                       u32 reflection_order, PropagationPath* paths, u32 max_paths);
PropagationPath Propagation_CreateReflectionPath(const Vec3 source, const Vec3 listener, 
                                              const Vec3 reflection_point, u32 order);

//  COMPLETED: Diffraction path generation
u32 Propagation_GenerateDiffractionPaths(PropagationSystem* system, const Vec3 source, const Vec3 listener,
                                       PropagationPath* paths, u32 max_paths);
PropagationPath Propagation_CreateDiffractionPath(const Vec3 source, const Vec3 listener, const Vec3 edge_point);

// Utility functions
bool Propagation_RaycastObstruction(const Vec3 start, const Vec3 end, MaterialProperties* hit_material);
u32 Propagation_FindReflectionPoints(const Vec3 source, const Vec3 listener, Vec3* points, u32 max_points);
u32 Propagation_FindDiffractionEdges(const Vec3 source, const Vec3 listener, Vec3* edges, u32 max_edges);
bool Propagation_IsPointInsideObstacle(const Vec3 point);
MaterialProperties Propagation_GetMaterialAtPoint(const Vec3 point);
void Propagation_SortPathsByEnergy(PropagationPath* paths, u32 count);

// Main interface
void Propagation_CreateSystem(void);
PropagationSystem* Propagation_GetSystem(void);
u32 Propagation_ProcessSound(const Vec3 source, const Vec3 listener, PropagationPath* paths, u32 max_paths);

#endif // SPATIAL_PROPAGATION_H
