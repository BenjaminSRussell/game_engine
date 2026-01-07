#include "effects/vfx/destruction_system.h"
#include "include/core/logger.h"
#include "include/core/memory.h"
#include "include/math/math.h"
#include <string.h>
#include <include/math/math.h>
#include <stdlib.h>

/**
 * =================================================================================================
 *                                   DESTRUCTION & FRACTURE - IMPLEMENTATION
 * =================================================================================================
 */

// TASK_1900: Implement "Voronoi Diagram" generation for mesh slicing
static void destruction_generate_voronoi_sites(DestructionSystem* system, Vec3 center, f32 radius, u32 site_count) {
    if (!system || site_count == 0) return;
    
    system->voronoi_site_count = site_count;
    if (site_count > system->max_voronoi_sites) {
        site_count = system->max_voronoi_sites;
    }
    
    // Generate random Voronoi sites within the fracture region
    for (u32 i = 0; i < site_count; i++) {
        VoronoiSite* site = &system->voronoi_sites[i];
        
        // Random position within sphere
        f32 theta = 2.0f * PI_F * ((f32)rand() / RAND_MAX);
        f32 phi = PI_F * ((f32)rand() / RAND_MAX);
        f32 r = radius * cbrtf((f32)rand() / RAND_MAX);  // Cube root for uniform distribution
        
        site->position.x = center.x + r * sinf(phi) * cosf(theta);
        site->position.y = center.y + r * sinf(phi) * sinf(theta);
        site->position.z = center.z + r * cosf(phi);
        
        // Random normal direction (for fracture plane orientation)
        f32 normal_theta = 2.0f * PI_F * ((f32)rand() / RAND_MAX);
        f32 normal_phi = PI_F * ((f32)rand() / RAND_MAX);
        
        site->normal.x = sinf(normal_phi) * cosf(normal_theta);
        site->normal.y = sinf(normal_phi) * sinf(normal_theta);
        site->normal.z = cosf(normal_phi);
        site->normal = vec3_normalize(site->normal);
        
        // Weight for influence on fracture
        site->weight = 0.5f + 0.5f * ((f32)rand() / RAND_MAX);
        site->id = i;
    }
}

static f32 destruction_voronoi_distance_to_site(Vec3 point, VoronoiSite* site) {
    // Distance from point to Voronoi site with normal consideration
    Vec3 to_point = vec3_sub(point, site->position);
    f32 distance = vec3_length(to_point);
    
    // Add normal-based weighting for more interesting fracture patterns
    f32 normal_factor = fabsf(vec3_dot(vec3_normalize(to_point), site->normal));
    return distance * (1.0f + 0.3f * normal_factor);
}

static u32 destruction_find_closest_voronoi_site(DestructionSystem* system, Vec3 point) {
    if (!system) return 0;
    
    f32 min_distance = F32_MAX;
    u32 closest_site = 0;
    
    for (u32 i = 0; i < system->voronoi_site_count; i++) {
        f32 distance = destruction_voronoi_distance_to_site(point, &system->voronoi_sites[i]);
        if (distance < min_distance) {
            min_distance = distance;
            closest_site = i;
        }
    }
    
    return closest_site;
}

static void destruction_create_voronoi_piece(DestructionSystem* system, u32 site_id, Vec3 impact_point, f32 impact_force) {
    if (!system || site_id >= system->voronoi_site_count) return;
    
    if (system->piece_count >= system->max_pieces) {
        LOG_WARN("Cannot create more fracture pieces - maximum reached");
        return;
    }
    
    VoronoiSite* site = &system->voronoi_sites[site_id];
    FracturePiece* piece = &system->pieces[system->piece_count];
    
    memset(piece, 0, sizeof(FracturePiece));
    
    // Basic piece properties
    piece->piece_id = system->next_piece_id++;
    piece->position = site->position;
    piece->velocity = vec3_zero();
    piece->angular_velocity = vec3_zero();
    piece->mass = 1.0f;
    piece->state = PIECE_STATE_ACTIVE;
    piece->fracture_depth = 1;
    piece->material_type = 0;  // Default material
    
    // Calculate velocity based on impact
    Vec3 from_impact = vec3_sub(piece->position, impact_point);
    f32 distance = vec3_length(from_impact);
    if (distance > 0.0001f) {
        Vec3 direction = vec3_normalize(from_impact);
        f32 force_falloff = 1.0f - (distance / 10.0f);  // 10 unit falloff radius
        if (force_falloff > 0.0f) {
            piece->velocity = vec3_scale(direction, impact_force * force_falloff * 0.1f);
        }
    }
    
    // Structural integrity based on distance from impact
    piece->structural_integrity = 1.0f - (distance / 20.0f);  // Weaker further from impact
    piece->structural_integrity = fmaxf(0.1f, piece->structural_integrity);
    
    // Visual properties
    piece->color = vec3(0.8f, 0.6f, 0.4f);  // Brownish color for fractured material
    piece->roughness = 0.8f;
    piece->metalness = 0.0f;
    piece->has_inner_surface = true;
    
    // Timing
    piece->creation_time = time_get_current_ms();
    piece->last_update_time = piece->creation_time;
    
    // Create simple geometry for the piece (simplified cube)
    piece->vertex_count = 8;
    piece->index_count = 36;
    
    // Allocate memory for geometry
    piece->vertices = malloc(piece->vertex_count * sizeof(Vec3));
    piece->indices = malloc(piece->index_count * sizeof(u32));
    piece->normals = malloc(piece->vertex_count * sizeof(Vec3));
    piece->uvs = malloc(piece->vertex_count * sizeof(Vec2));
    
    if (piece->vertices && piece->indices && piece->normals && piece->uvs) {
        // Create a simple cube around the site position
        f32 size = 0.5f * site->weight;  // Size based on site weight
        
        // Cube vertices
        piece->vertices[0] = vec3(piece->position.x - size, piece->position.y - size, piece->position.z - size);
        piece->vertices[1] = vec3(piece->position.x + size, piece->position.y - size, piece->position.z - size);
        piece->vertices[2] = vec3(piece->position.x + size, piece->position.y + size, piece->position.z - size);
        piece->vertices[3] = vec3(piece->position.x - size, piece->position.y + size, piece->position.z - size);
        piece->vertices[4] = vec3(piece->position.x - size, piece->position.y - size, piece->position.z + size);
        piece->vertices[5] = vec3(piece->position.x + size, piece->position.y - size, piece->position.z + size);
        piece->vertices[6] = vec3(piece->position.x + size, piece->position.y + size, piece->position.z + size);
        piece->vertices[7] = vec3(piece->position.x - size, piece->position.y + size, piece->position.z + size);
        
        // Cube normals (simplified - all point outward)
        for (u32 i = 0; i < 8; i++) {
            piece->normals[i] = vec3_normalize(vec3_sub(piece->vertices[i], piece->position));
        }
        
        // Simple UV coordinates
        piece->uvs[0] = vec2(0.0f, 0.0f);
        piece->uvs[1] = vec2(1.0f, 0.0f);
        piece->uvs[2] = vec2(1.0f, 1.0f);
        piece->uvs[3] = vec2(0.0f, 1.0f);
        piece->uvs[4] = vec2(0.0f, 0.0f);
        piece->uvs[5] = vec2(1.0f, 0.0f);
        piece->uvs[6] = vec2(1.0f, 1.0f);
        piece->uvs[7] = vec2(0.0f, 1.0f);
        
        // Cube indices (12 triangles)
        u32 cube_indices[] = {
            0, 1, 2,  0, 2, 3,  // Front
            4, 7, 6,  4, 6, 5,  // Back
            0, 4, 5,  0, 5, 1,  // Bottom
            2, 6, 7,  2, 7, 3,  // Top
            0, 3, 7,  0, 7, 4,  // Left
            1, 5, 6,  1, 6, 2   // Right
        };
        memcpy(piece->indices, cube_indices, sizeof(cube_indices));
    }
    
    system->piece_count++;
    system->pieces_generated++;
}

void destruction_generate_voronoi_fracture(DestructionSystem* system, Vec3 impact_point, Vec3 impact_direction, f32 impact_force) {
    if (!system) return;
    
    u64 start_time = time_get_current_ms();
    
    // Generate Voronoi sites for fracture pattern
    u32 site_count = 8 + (u32)(impact_force * 2.0f);  // More sites for higher force
    site_count = (site_count > DESTRUCTION_MAX_VORONOI_SITES) ? DESTRUCTION_MAX_VORONOI_SITES : site_count;
    
    destruction_generate_voronoi_sites(system, impact_point, 5.0f + impact_force, site_count);
    
    // Create fracture pieces based on Voronoi sites
    for (u32 i = 0; i < system->voronoi_site_count; i++) {
        destruction_create_voronoi_piece(system, i, impact_point, impact_force);
    }
    
    // Create constraints between nearby pieces
    for (u32 i = 0; i < system->piece_count; i++) {
        for (u32 j = i + 1; j < system->piece_count; j++) {
            FracturePiece* piece_a = &system->pieces[i];
            FracturePiece* piece_b = &system->pieces[j];
            
            Vec3 diff = vec3_sub(piece_a->position, piece_b->position);
            f32 distance = vec3_length(diff);
            
            // Create constraint if pieces are close enough
            if (distance < 2.0f && system->constraint_count < system->max_constraints) {
                Vec3 connection_point = vec3_lerp(piece_a->position, piece_b->position, 0.5f);
                f32 break_threshold = impact_force * 0.5f * (1.0f - distance / 2.0f);
                
                destruction_create_breakable_constraint(system, piece_a->piece_id, piece_b->piece_id, 
                                                       connection_point, break_threshold);
            }
        }
    }
    
    u64 end_time = time_get_current_ms();
    system->fracture_time_ms += (end_time - start_time);
    system->total_fractures++;
    
    LOG_INFO("Generated Voronoi fracture: %u pieces, %u sites", system->piece_count, system->voronoi_site_count);
}

// TASK_1901: Implement "Recursive Fracture" (Break pieces into smaller pieces)
void destruction_generate_recursive_fracture(DestructionSystem* system, u32 parent_piece_id, Vec3 fracture_point, f32 fracture_energy) {
    if (!system) return;
    
    // Find parent piece
    FracturePiece* parent_piece = destruction_get_piece(system, parent_piece_id);
    if (!parent_piece || parent_piece->fracture_depth >= DESTRUCTION_MAX_FRACTURE_DEPTH) {
        return;
    }
    
    // Check if piece should fracture based on energy and structural integrity
    f32 fracture_threshold = parent_piece->structural_integrity * 10.0f;
    if (fracture_energy < fracture_threshold) {
        return;
    }
    
    // Remove parent piece
    destruction_remove_piece(system, parent_piece_id);
    
    // Generate child pieces
    u32 child_count = 2 + (u32)(fracture_energy / 5.0f);
    child_count = (child_count > 8) ? 8 : child_count;
    
    for (u32 i = 0; i < child_count && system->piece_count < system->max_pieces; i++) {
        FracturePiece* child = &system->pieces[system->piece_count];
        memset(child, 0, sizeof(FracturePiece));
        
        // Inherit properties from parent
        child->piece_id = system->next_piece_id++;
        child->parent_piece_id = parent_piece_id;
        child->fracture_depth = parent_piece->fracture_depth + 1;
        child->material_type = parent_piece->material_type;
        child->color = parent_piece->color;
        child->roughness = parent_piece->roughness;
        child->metalness = parent_piece->metalness;
        child->has_inner_surface = true;
        
        // Position with some randomness
        f32 offset = 0.2f;
        child->position.x = parent_piece->position.x + ((f32)rand() / RAND_MAX - 0.5f) * offset;
        child->position.y = parent_piece->position.y + ((f32)rand() / RAND_MAX - 0.5f) * offset;
        child->position.z = parent_piece->position.z + ((f32)rand() / RAND_MAX - 0.5f) * offset;
        
        // Velocity based on fracture energy
        Vec3 direction = vec3_sub(child->position, fracture_point);
        if (vec3_length_squared(direction) > 0.0001f) {
            direction = vec3_normalize(direction);
            child->velocity = vec3_scale(direction, fracture_energy * 0.05f);
        } else {
            child->velocity = vec3(((f32)rand() / RAND_MAX - 0.5f), 
                                 ((f32)rand() / RAND_MAX - 0.5f), 
                                 ((f32)rand() / RAND_MAX - 0.5f));
            child->velocity = vec3_scale(child->velocity, fracture_energy * 0.05f);
        }
        
        // Angular velocity
        child->angular_velocity = vec3(((f32)rand() / RAND_MAX - 0.5f) * 2.0f,
                                      ((f32)rand() / RAND_MAX - 0.5f) * 2.0f,
                                      ((f32)rand() / RAND_MAX - 0.5f) * 2.0f);
        
        // Mass and structural integrity
        child->mass = parent_piece->mass / (f32)child_count;
        child->structural_integrity = parent_piece->structural_integrity * 0.8f;
        
        // State
        child->state = PIECE_STATE_ACTIVE;
        
        // Timing
        child->creation_time = time_get_current_ms();
        child->last_update_time = child->creation_time;
        
        // Simple geometry (smaller cube)
        child->vertex_count = 8;
        child->index_count = 36;
        
        child->vertices = malloc(child->vertex_count * sizeof(Vec3));
        child->indices = malloc(child->index_count * sizeof(u32));
        child->normals = malloc(child->vertex_count * sizeof(Vec3));
        child->uvs = malloc(child->vertex_count * sizeof(Vec2));
        
        if (child->vertices && child->indices && child->normals && child->uvs) {
            f32 size = 0.3f / (f32)(child->fracture_depth);  // Smaller pieces at deeper levels
            
            // Create smaller cube
            child->vertices[0] = vec3(child->position.x - size, child->position.y - size, child->position.z - size);
            child->vertices[1] = vec3(child->position.x + size, child->position.y - size, child->position.z - size);
            child->vertices[2] = vec3(child->position.x + size, child->position.y + size, child->position.z - size);
            child->vertices[3] = vec3(child->position.x - size, child->position.y + size, child->position.z - size);
            child->vertices[4] = vec3(child->position.x - size, child->position.y - size, child->position.z + size);
            child->vertices[5] = vec3(child->position.x + size, child->position.y - size, child->position.z + size);
            child->vertices[6] = vec3(child->position.x + size, child->position.y + size, child->position.z + size);
            child->vertices[7] = vec3(child->position.x - size, child->position.y + size, child->position.z + size);
            
            for (u32 j = 0; j < 8; j++) {
                child->normals[j] = vec3_normalize(vec3_sub(child->vertices[j], child->position));
            }
            
            child->uvs[0] = vec2(0.0f, 0.0f);
            child->uvs[1] = vec2(1.0f, 0.0f);
            child->uvs[2] = vec2(1.0f, 1.0f);
            child->uvs[3] = vec2(0.0f, 1.0f);
            child->uvs[4] = vec2(0.0f, 0.0f);
            child->uvs[5] = vec2(1.0f, 0.0f);
            child->uvs[6] = vec2(1.0f, 1.0f);
            child->uvs[7] = vec2(0.0f, 1.0f);
            
            u32 cube_indices[] = {
                0, 1, 2,  0, 2, 3,  4, 7, 6,  4, 6, 5,
                0, 4, 5,  0, 5, 1,  2, 6, 7,  2, 7, 3,
                0, 3, 7,  0, 7, 4,  1, 5, 6,  1, 6, 2
            };
            memcpy(child->indices, cube_indices, sizeof(cube_indices));
        }
        
        system->piece_count++;
        system->pieces_generated++;
    }
    
// TASK_1910: Implement "Island Detection" for fractured pieces
static void destruction_flood_fill_island(DestructionSystem* system, u32 start_piece_id, 
                                         u32 island_index, bool* visited) {
    if (!system || start_piece_id >= system->max_pieces || visited[start_piece_id]) {
        return;
    }
    
    FracturePiece* start_piece = &system->pieces[start_piece_id];
    if (!start_piece || start_piece->state == PIECE_STATE_UNLOADED) {
        return;
    }
    
    // Mark as visited and add to current island
    visited[start_piece_id] = true;
    
    FractureIsland* island = &system->islands[island_index];
    island->piece_indices[island->piece_count] = start_piece_id;
    island->piece_count++;
    
    // Update island properties
    island->center_of_mass = vec3_add(island->center_of_mass, 
                                    vec3_scale(start_piece->position, start_piece->mass));
    island->total_mass += start_piece->mass;
    
    // Find connected pieces through constraints
    for (u32 i = 0; i < system->constraint_count; i++) {
        BreakableConstraint* constraint = &system->constraints[i];
        if (!constraint->is_broken) {
            u32 next_piece_id = 0;
            
            if (constraint->piece_a_id == start_piece_id) {
                next_piece_id = constraint->piece_b_id;
            } else if (constraint->piece_b_id == start_piece_id) {
                next_piece_id = constraint->piece_a_id;
            }
            
            if (next_piece_id != 0 && !visited[next_piece_id]) {
                destruction_flood_fill_island(system, next_piece_id, island_index, visited);
            }
        }
    }
    
    // Also check proximity-based connections (pieces close to each other)
    for (u32 i = 0; i < system->piece_count; i++) {
        if (i != start_piece_id && !visited[i]) {
            FracturePiece* other_piece = &system->pieces[i];
            if (other_piece && other_piece->state != PIECE_STATE_UNLOADED) {
                Vec3 diff = vec3_sub(start_piece->position, other_piece->position);
                f32 distance = vec3_length(diff);
                
                // Pieces within 1.0 unit are considered connected
                if (distance < 1.0f) {
                    destruction_flood_fill_island(system, i, island_index, visited);
                }
            }
        }
    }
}

void destruction_detect_islands(DestructionSystem* system) {
    if (!system) return;
    
    u64 start_time = time_get_current_ms();
    
    // Reset islands
    for (u32 i = 0; i < system->max_islands; i++) {
        system->islands[i].piece_count = 0;
        system->islands[i].total_mass = 0.0f;
        system->islands[i].center_of_mass = vec3_zero();
        system->islands[i].is_supported = false;
    }
    system->island_count = 0;
    
    // Track visited pieces
    bool* visited = calloc(system->max_pieces, sizeof(bool));
    if (!visited) {
        LOG_ERROR("Failed to allocate visited array for island detection");
        return;
    }
    
    // Find islands using flood fill
    for (u32 i = 0; i < system->piece_count; i++) {
        FracturePiece* piece = &system->pieces[i];
        if (piece && piece->state != PIECE_STATE_UNLOADED && !visited[i]) {
            if (system->island_count < system->max_islands) {
                destruction_flood_fill_island(system, i, system->island_count, visited);
                
                // Calculate final center of mass for this island
                FractureIsland* island = &system->islands[system->island_count];
                if (island->total_mass > 0.0001f) {
                    island->center_of_mass = vec3_scale(island->center_of_mass, 1.0f / island->total_mass);
                }
                
                system->island_count++;
            }
        }
    }
    
    free(visited);
    
    u64 end_time = time_get_current_ms();
    system->island_detection_time_ms += (end_time - start_time);
    
    LOG_TRACE("Island detection: %u islands found from %u pieces", 
              system->island_count, system->piece_count);
}

void destruction_update_island_support(DestructionSystem* system) {
    if (!system) return;
    
    for (u32 i = 0; i < system->island_count; i++) {
        FractureIsland* island = &system->islands[i];
        island->is_supported = false;
        
        // Check if any piece in island is supported (touching ground or stable object)
        for (u32 j = 0; j < island->piece_count; j++) {
            u32 piece_id = island->piece_indices[j];
            FracturePiece* piece = &system->pieces[piece_id];
            
            if (piece && piece->position.y <= 0.5f) {  // Ground level check
                island->is_supported = true;
                break;
            }
        }
    }
}

bool destruction_is_piece_supported(DestructionSystem* system, u32 piece_id) {
    if (!system) return false;
    
    // Find which island this piece belongs to
    for (u32 i = 0; i < system->island_count; i++) {
        FractureIsland* island = &system->islands[i];
        for (u32 j = 0; j < island->piece_count; j++) {
            if (island->piece_indices[j] == piece_id) {
                return island->is_supported;
            }
        }
    }
    
    return false;  // Piece not found in any island
}

// Core destruction system functions
void destruction_system_init(DestructionSystem* system) {
    if (!system) return;
    
    memset(system, 0, sizeof(DestructionSystem));
    
    // Set default parameters
    system->max_pieces = DESTRUCTION_MAX_PIECES;
    system->max_voronoi_sites = DESTRUCTION_MAX_VORONOI_SITES;
    system->max_islands = 100;  // Allocate space for islands
    system->max_constraints = 500;  // Allocate space for constraints
    
    // Allocate arrays
    system->pieces = malloc(system->max_pieces * sizeof(FracturePiece));
    system->voronoi_sites = malloc(system->max_voronoi_sites * sizeof(VoronoiSite));
    system->islands = malloc(system->max_islands * sizeof(FractureIsland));
    system->constraints = malloc(system->max_constraints * sizeof(BreakableConstraint));
    system->integrity_data = malloc(system->max_pieces * sizeof(StructuralIntegrity));
    
    if (!system->pieces || !system->voronoi_sites || !system->islands || 
        !system->constraints || !system->integrity_data) {
        LOG_ERROR("Failed to allocate destruction system arrays");
        destruction_system_shutdown(system);
        return;
    }
    
    // Initialize island piece index arrays
    for (u32 i = 0; i < system->max_islands; i++) {
        system->islands[i].piece_indices = malloc(system->max_pieces * sizeof(u32));
        if (!system->islands[i].piece_indices) {
            LOG_ERROR("Failed to allocate island piece indices");
            destruction_system_shutdown(system);
            return;
        }
    }
    
    // Initialize constraint arrays for islands
    for (u32 i = 0; i < system->max_islands; i++) {
        system->islands[i].piece_indices = malloc(system->max_pieces * sizeof(u32));
    }
    
    system->enable_fracture_lod = true;
    system->aggregation_threshold = 0.1f;
    
    LOG_INFO("Destruction system initialized");
    LOG_INFO("  Max pieces: %u", system->max_pieces);
    LOG_INFO("  Max Voronoi sites: %u", system->max_voronoi_sites);
    LOG_INFO("  Max islands: %u", system->max_islands);
}

void destruction_system_shutdown(DestructionSystem* system) {
    if (!system) return;
    
    // Clean up pieces
    if (system->pieces) {
        for (u32 i = 0; i < system->piece_count; i++) {
            FracturePiece* piece = &system->pieces[i];
            if (piece) {
                if (piece->vertices) free(piece->vertices);
                if (piece->indices) free(piece->indices);
                if (piece->normals) free(piece->normals);
                if (piece->uvs) free(piece->uvs);
            }
        }
        free(system->pieces);
        system->pieces = NULL;
    }
    
    // Clean up other arrays
    if (system->voronoi_sites) {
        free(system->voronoi_sites);
        system->voronoi_sites = NULL;
    }
    
    if (system->islands) {
        for (u32 i = 0; i < system->max_islands; i++) {
            if (system->islands[i].piece_indices) {
                free(system->islands[i].piece_indices);
            }
        }
        free(system->islands);
        system->islands = NULL;
    }
    
    if (system->constraints) {
        free(system->constraints);
        system->constraints = NULL;
    }
    
    if (system->integrity_data) {
        free(system->integrity_data);
        system->integrity_data = NULL;
    }
    
    memset(system, 0, sizeof(DestructionSystem));
    LOG_INFO("Destruction system shutdown");
}

void destruction_system_update(DestructionSystem* system, f32 delta_time) {
    if (!system) return;
    
    // Update piece physics
    for (u32 i = 0; i < system->piece_count; i++) {
        FracturePiece* piece = &system->pieces[i];
        if (!piece || piece->state != PIECE_STATE_ACTIVE) {
            continue;
        }
        
        // Apply gravity
        Vec3 gravity = vec3(0.0f, -9.81f, 0.0f);
        Vec3 acceleration = vec3_scale(gravity, 1.0f / piece->mass);
        
        // Update velocity and position
        piece->velocity = vec3_add(piece->velocity, vec3_scale(acceleration, delta_time));
        piece->position = vec3_add(piece->position, vec3_scale(piece->velocity, delta_time));
        
        // Update angular velocity and rotation (simplified)
        piece->angular_velocity = vec3_scale(piece->angular_velocity, 0.98f);  // Damping
        
        // Update timing
        piece->last_update_time = time_get_current_ms();
        
        // Check if piece should go to sleep
        f32 speed = vec3_length(piece->velocity);
        f32 angular_speed = vec3_length(piece->angular_velocity);
        
        if (speed < DESTRUCTION_SLEEP_THRESHOLD && angular_speed < DESTRUCTION_SLEEP_THRESHOLD) {
            piece->sleep_frames++;
            if (piece->sleep_frames > 60) {  // 1 second at 60 FPS
                destruction_set_piece_sleeping(system, piece->piece_id);
            }
        } else {
            piece->sleep_frames = 0;
        }
    }
    
    // Update constraints
    destruction_update_constraints(system);
    
    // Detect islands periodically
    static u32 island_update_counter = 0;
    island_update_counter++;
    if (island_update_counter >= 60) {  // Update every second
        destruction_detect_islands(system);
        destruction_update_island_support(system);
        island_update_counter = 0;
    }
}

FracturePiece* destruction_get_piece(DestructionSystem* system, u32 piece_id) {
    if (!system) return NULL;
    
    for (u32 i = 0; i < system->piece_count; i++) {
        FracturePiece* piece = &system->pieces[i];
        if (piece && piece->piece_id == piece_id) {
            return piece;
        }
    }
    
    return NULL;
}

void destruction_remove_piece(DestructionSystem* system, u32 piece_id) {
    if (!system) return;
    
    for (u32 i = 0; i < system->piece_count; i++) {
        FracturePiece* piece = &system->pieces[i];
        if (piece && piece->piece_id == piece_id) {
            // Clean up geometry
            if (piece->vertices) free(piece->vertices);
            if (piece->indices) free(piece->indices);
            if (piece->normals) free(piece->normals);
            if (piece->uvs) free(piece->uvs);
            
            // Move last piece to this position
            if (i < system->piece_count - 1) {
                system->pieces[i] = system->pieces[system->piece_count - 1];
            }
            system->piece_count--;
            return;
        }
    }
}

bool destruction_is_piece_active(DestructionSystem* system, u32 piece_id) {
    FracturePiece* piece = destruction_get_piece(system, piece_id);
    return piece && piece->state == PIECE_STATE_ACTIVE;
}

void destruction_set_piece_sleeping(DestructionSystem* system, u32 piece_id) {
    FracturePiece* piece = destruction_get_piece(system, piece_id);
    if (piece) {
        piece->state = PIECE_STATE_SLEEPING;
        piece->velocity = vec3_zero();
        piece->angular_velocity = vec3_zero();
    }
}

void destruction_set_piece_static(DestructionSystem* system, u32 piece_id) {
    FracturePiece* piece = destruction_get_piece(system, piece_id);
    if (piece) {
        piece->state = PIECE_STATIC;
        piece->velocity = vec3_zero();
        piece->angular_velocity = vec3_zero();
    }
}

void destruction_create_breakable_constraint(DestructionSystem* system, u32 piece_a_id, u32 piece_b_id, 
                                           Vec3 connection_point, f32 break_threshold) {
    if (!system || system->constraint_count >= system->max_constraints) {
        return;
    }
    
    BreakableConstraint* constraint = &system->constraints[system->constraint_count];
    constraint->piece_a_id = piece_a_id;
    constraint->piece_b_id = piece_b_id;
    constraint->connection_point = connection_point;
    constraint->break_threshold = break_threshold;
    constraint->current_stress = 0.0f;
    constraint->is_broken = false;
    
    system->constraint_count++;
}

void destruction_update_constraints(DestructionSystem* system) {
    if (!system) return;
    
    for (u32 i = 0; i < system->constraint_count; i++) {
        BreakableConstraint* constraint = &system->constraints[i];
        if (constraint->is_broken) {
            continue;
        }
        
        FracturePiece* piece_a = destruction_get_piece(system, constraint->piece_a_id);
        FracturePiece* piece_b = destruction_get_piece(system, constraint->piece_b_id);
        
        if (!piece_a || !piece_b) {
            constraint->is_broken = true;
            continue;
        }
        
        // Calculate stress based on relative velocity and distance
        Vec3 relative_velocity = vec3_sub(piece_a->velocity, piece_b->velocity);
        f32 speed_diff = vec3_length(relative_velocity);
        
        Vec3 current_a = vec3_add(piece_a->position, constraint->connection_point);
        Vec3 current_b = vec3_add(piece_b->position, constraint->connection_point);
        Vec3 diff = vec3_sub(current_a, current_b);
        f32 distance = vec3_length(diff);
        
        // Simple stress model based on velocity difference and distance
        constraint->current_stress = speed_diff * 0.5f + distance * 10.0f;
        
        // Break constraint if stress exceeds threshold
        if (constraint->current_stress > constraint->break_threshold) {
            constraint->is_broken = true;
            LOG_TRACE("Constraint broken between pieces %u and %u (stress: %.2f, threshold: %.2f)", 
                      constraint->piece_a_id, constraint->piece_b_id, 
                      constraint->current_stress, constraint->break_threshold);
        }
    }
}
