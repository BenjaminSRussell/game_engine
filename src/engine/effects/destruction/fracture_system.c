// fracture_system.c - Advanced Destruction System Implementation
#include <include/effects/vfx/destruction_system.h>
#include <include/core/logger.h>
#include <include/math/vec3.h>
#include <include/math/mat4.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static FracturePiece *g_fracture_pieces = NULL;
static u32 g_piece_count = 0;
static u32 g_piece_capacity = 0;
static bool g_destruction_initialized = false;

bool destruction_system_init(u32 max_pieces) {
  if (g_destruction_initialized) {
    LOG_WARN("Destruction system already initialized");
    return true;
  }
  
  g_piece_capacity = max_pieces > 0 ? max_pieces : DESTRUCTION_MAX_PIECES;
  g_fracture_pieces = (FracturePiece*)calloc(g_piece_capacity, sizeof(FracturePiece));
  
  if (!g_fracture_pieces) {
    LOG_ERROR("Failed to allocate fracture pieces array");
    return false;
  }
  
  g_piece_count = 0;
  g_destruction_initialized = true;
  
  LOG_INFO("Destruction system initialized with %u max pieces", g_piece_capacity);
  return true;
}

void destruction_system_shutdown(void) {
  if (!g_destruction_initialized) return;
  
  // Free all piece data
  for (u32 i = 0; i < g_piece_count; i++) {
    FracturePiece *piece = &g_fracture_pieces[i];
    
    if (piece->vertices) free(piece->vertices);
    if (piece->indices) free(piece->indices);
    if (piece->normals) free(piece->normals);
    if (piece->uvs) free(piece->uvs);
  }
  
  free(g_fracture_pieces);
  g_fracture_pieces = NULL;
  g_piece_count = 0;
  g_piece_capacity = 0;
  g_destruction_initialized = false;
  
  LOG_INFO("Destruction system shutdown");
}

u32 destruction_fracture_mesh(const Vec3 *vertices, u32 vertex_count,
                            const u32 *indices, u32 index_count,
                            const Vec3 *impact_point, f32 impact_force,
                            FractureType fracture_type, u32 max_pieces) {
  if (!g_destruction_initialized || !vertices || !indices || !impact_point) {
    return 0;
  }
  
  // Generate Voronoi sites for fracture
  VoronoiSite sites[DESTRUCTION_MAX_VORONOI_SITES];
  u32 site_count = destruction_generate_voronoi_sites(impact_point, impact_force, sites);
  
  // Create fracture pieces
  u32 pieces_created = destruction_create_fracture_pieces(
    vertices, vertex_count, indices, index_count,
    sites, site_count, fracture_type, max_pieces);
  
  LOG_INFO("Created %u fracture pieces from mesh with %u vertices", 
           pieces_created, vertex_count);
  
  return pieces_created;
}

u32 destruction_generate_voronoi_sites(const Vec3 *impact_point, f32 impact_force,
                                       VoronoiSite *out_sites) {
  if (!impact_point || !out_sites) return 0;
  
  u32 site_count = 0;
  f32 radius = impact_force * 0.5f; // Fracture radius based on impact force
  
  // Generate sites around impact point
  for (u32 i = 0; i < DESTRUCTION_MAX_VORONOI_SITES && site_count < DESTRUCTION_MAX_VORONOI_SITES; i++) {
    // Random position within fracture radius
    f32 theta = ((f32)rand() / RAND_MAX) * 2.0f * M_PI;
    f32 phi = ((f32)rand() / RAND_MAX) * M_PI;
    f32 r = ((f32)rand() / RAND_MAX) * radius;
    
    Vec3 position = {
      impact_point->x + r * sinf(phi) * cosf(theta),
      impact_point->y + r * cosf(phi),
      impact_point->z + r * sinf(phi) * sinf(theta)
    };
    
    // Random normal direction
    Vec3 normal = {
      ((f32)rand() / RAND_MAX) * 2.0f - 1.0f,
      ((f32)rand() / RAND_MAX) * 2.0f - 1.0f,
      ((f32)rand() / RAND_MAX) * 2.0f - 1.0f
    };
    normal = vec3_normalize(&normal);
    
    // Weight based on distance from impact point
    f32 distance = vec3_distance(&position, impact_point);
    f32 weight = 1.0f - (distance / radius);
    
    out_sites[site_count++] = (VoronoiSite){
      .position = position,
      .normal = normal,
      .weight = weight,
      .id = i
    };
  }
  
  return site_count;
}

u32 destruction_create_fracture_pieces(const Vec3 *vertices, u32 vertex_count,
                                       const u32 *indices, u32 index_count,
                                       const VoronoiSite *sites, u32 site_count,
                                       FractureType fracture_type, u32 max_pieces) {
  if (!vertices || !indices || !sites || site_count == 0) return 0;
  
  u32 pieces_created = 0;
  
  switch (fracture_type) {
    case FRACTURE_VORONOI:
      pieces_created = destruction_create_voronoi_fractures(
        vertices, vertex_count, indices, index_count, sites, site_count, max_pieces);
      break;
      
    case FRACTURE_RECURSIVE:
      pieces_created = destruction_create_recursive_fractures(
        vertices, vertex_count, indices, index_count, sites, site_count, max_pieces);
      break;
      
    case FRACTURE_SLICE:
      pieces_created = destruction_create_slice_fractures(
        vertices, vertex_count, indices, index_count, sites, site_count, max_pieces);
      break;
      
    default:
      LOG_WARN("Unsupported fracture type: %d", fracture_type);
      return 0;
  }
  
  return pieces_created;
}

u32 destruction_create_voronoi_fractures(const Vec3 *vertices, u32 vertex_count,
                                        const u32 *indices, u32 index_count,
                                        const VoronoiSite *sites, u32 site_count,
                                        u32 max_pieces) {
  if (g_piece_count + site_count > g_piece_capacity) {
    LOG_WARN("Not enough capacity for %u fracture pieces", site_count);
    return 0;
  }
  
  u32 pieces_created = 0;
  
  // Create a piece for each Voronoi site
  for (u32 i = 0; i < site_count && pieces_created < max_pieces; i++) {
    FracturePiece *piece = &g_fracture_pieces[g_piece_count + pieces_created];
    memset(piece, 0, sizeof(FracturePiece));
    
    // Generate piece geometry around Voronoi site
    destruction_generate_piece_geometry(piece, vertices, vertex_count, indices, index_count, &sites[i]);
    
    // Set piece properties
    piece->piece_id = g_piece_count + pieces_created;
    piece->parent_piece_id = UINT32_MAX; // No parent
    piece->fracture_depth = 1;
    piece->state = PIECE_STATE_ACTIVE;
    piece->creation_time = get_current_time();
    piece->last_update_time = piece->creation_time;
    
    // Calculate physics properties
    destruction_calculate_piece_physics(piece);
    
    // Set material properties
    piece->structural_integrity = 1.0f;
    piece->stress_factor = 0.0f;
    piece->material_type = 0;
    piece->color = (Vec3){0.8f, 0.8f, 0.8f};
    piece->roughness = 0.5f;
    piece->metalness = 0.0f;
    piece->has_inner_surface = true;
    
    pieces_created++;
  }
  
  g_piece_count += pieces_created;
  return pieces_created;
}

void destruction_generate_piece_geometry(FracturePiece *piece, const Vec3 *vertices, u32 vertex_count,
                                        const u32 *indices, u32 index_count, const VoronoiSite *site) {
  if (!piece || !vertices || !indices || !site) return;
  
  // Simple geometry generation - create a box around the Voronoi site
  f32 box_size = 0.5f; // Default piece size
  
  // Allocate vertices for a box (8 vertices)
  piece->vertex_count = 8;
  piece->vertices = (Vec3*)malloc(piece->vertex_count * sizeof(Vec3));
  piece->normals = (Vec3*)malloc(piece->vertex_count * sizeof(Vec3));
  piece->uvs = (Vec2*)malloc(piece->vertex_count * sizeof(Vec2));
  
  // Generate box vertices around site
  Vec3 offsets[8] = {
    {-box_size, -box_size, -box_size}, {box_size, -box_size, -box_size},
    {box_size, box_size, -box_size}, {-box_size, box_size, -box_size},
    {-box_size, -box_size, box_size}, {box_size, -box_size, box_size},
    {box_size, box_size, box_size}, {-box_size, box_size, box_size}
  };
  
  for (u32 i = 0; i < 8; i++) {
    piece->vertices[i] = vec3_add(site->position, offsets[i]);
    piece->normals[i] = vec3_normalize(&offsets[i]);
    piece->uvs[i] = (Vec2){(offsets[i].x + box_size) / (2.0f * box_size),
                           (offsets[i].y + box_size) / (2.0f * box_size)};
  }
  
  // Generate indices for box (12 triangles = 36 indices)
  piece->index_count = 36;
  piece->indices = (u32*)malloc(piece->index_count * sizeof(u32));
  
  u32 box_indices[36] = {
    // Front face
    0, 1, 2, 0, 2, 3,
    // Back face
    4, 6, 5, 4, 7, 6,
    // Top face
    3, 2, 6, 3, 6, 7,
    // Bottom face
    0, 4, 5, 0, 5, 1,
    // Right face
    1, 5, 6, 1, 6, 2,
    // Left face
    0, 3, 7, 0, 7, 4
  };
  
  memcpy(piece->indices, box_indices, sizeof(box_indices));
  
  // Set initial position
  piece->position = site->position;
  piece->transform = mat4_identity();
  piece->transform.m[3][0] = site->position.x;
  piece->transform.m[3][1] = site->position.y;
  piece->transform.m[3][2] = site->position.z;
}

void destruction_calculate_piece_physics(FracturePiece *piece) {
  if (!piece) return;
  
  // Calculate center of mass (simplified - assume uniform density)
  Vec3 center = {0.0f, 0.0f, 0.0f};
  for (u32 i = 0; i < piece->vertex_count; i++) {
    center = vec3_add(center, piece->vertices[i]);
  }
  center = vec3_scale(center, 1.0f / piece->vertex_count);
  
  // Calculate volume (simplified - assume box)
  Vec3 min = piece->vertices[0];
  Vec3 max = piece->vertices[0];
  
  for (u32 i = 1; i < piece->vertex_count; i++) {
    min.x = fminf(min.x, piece->vertices[i].x);
    min.y = fminf(min.y, piece->vertices[i].y);
    min.z = fminf(min.z, piece->vertices[i].z);
    
    max.x = fmaxf(max.x, piece->vertices[i].x);
    max.y = fmaxf(max.y, piece->vertices[i].y);
    max.z = fmaxf(max.z, piece->vertices[i].z);
  }
  
  Vec3 size = vec3_sub(max, min);
  f32 volume = size.x * size.y * size.z;
  
  // Set mass (density = 1000 kg/m³ for concrete-like material)
  piece->mass = fmaxf(DESTRUCTION_MIN_PIECE_VOLUME, volume * 1000.0f);
  
  // Calculate inertia tensor (simplified for box)
  f32 ix = piece->mass * (size.y * size.y + size.z * size.z) / 12.0f;
  f32 iy = piece->mass * (size.x * size.x + size.z * size.z) / 12.0f;
  f32 iz = piece->mass * (size.x * size.x + size.y * size.y) / 12.0f;
  
  piece->inertia_tensor = (Mat4){{
    {{ix, 0.0f, 0.0f, 0.0f},
     {0.0f, iy, 0.0f, 0.0f},
     {0.0f, 0.0f, iz, 0.0f},
     {0.0f, 0.0f, 0.0f, 1.0f}}
  }};
  
  // Set initial velocity (explosion effect)
  f32 explosion_force = 5.0f;
  piece->velocity = vec3_scale(vec3_normalize(&piece->position), explosion_force);
  piece->angular_velocity = (Vec3){
    ((f32)rand() / RAND_MAX - 0.5f) * 2.0f,
    ((f32)rand() / RAND_MAX - 0.5f) * 2.0f,
    ((f32)rand() / RAND_MAX - 0.5f) * 2.0f
  };
}

void destruction_update(f32 delta_time) {
  if (!g_destruction_initialized) return;
  
  f64 current_time = get_current_time();
  
  for (u32 i = 0; i < g_piece_count; i++) {
    FracturePiece *piece = &g_fracture_pieces[i];
    
    if (piece->state == PIECE_STATE_STATIC) continue;
    
    // Update physics for active pieces
    if (piece->state == PIECE_STATE_ACTIVE) {
      destruction_update_piece_physics(piece, delta_time);
      
      // Check if piece should go to sleep
      if (vec3_length_sq(&piece->velocity) < DESTRUCTION_SLEEP_THRESHOLD * DESTRUCTION_SLEEP_THRESHOLD &&
          vec3_length_sq(&piece->angular_velocity) < DESTRUCTION_SLEEP_THRESHOLD * DESTRUCTION_SLEEP_THRESHOLD) {
        piece->sleep_frames++;
        
        if (piece->sleep_frames > 60) { // 1 second at 60 FPS
          piece->state = PIECE_STATE_SLEEPING;
          piece->sleep_frames = 0;
        }
      } else {
        piece->sleep_frames = 0;
      }
    }
    
    piece->last_update_time = current_time;
  }
  
  // Remove pieces that are too far away or too old
  destruction_cleanup_pieces();
}

void destruction_update_piece_physics(FracturePiece *piece, f32 delta_time) {
  if (!piece) return;
  
  // Apply gravity
  Vec3 gravity = {0.0f, -9.81f, 0.0f};
  Vec3 acceleration = vec3_scale(gravity, 1.0f / piece->mass);
  
  // Update velocity
  piece->velocity = vec3_add(piece->velocity, vec3_scale(acceleration, delta_time));
  
  // Apply damping
  piece->velocity = vec3_scale(piece->velocity, 0.99f);
  piece->angular_velocity = vec3_scale(piece->angular_velocity, 0.98f);
  
  // Update position
  piece->position = vec3_add(piece->position, vec3_scale(piece->velocity, delta_time));
  
  // Update transform
  piece->transform.m[3][0] = piece->position.x;
  piece->transform.m[3][1] = piece->position.y;
  piece->transform.m[3][2] = piece->position.z;
  
  // Update stress factor based on velocity
  f32 speed = vec3_length(&piece->velocity);
  piece->stress_factor = fminf(1.0f, speed / 20.0f);
  
  // Check for secondary fractures
  if (piece->stress_factor > 0.8f && piece->fracture_depth < DESTRUCTION_MAX_FRACTURE_DEPTH) {
    destruction_fracture_piece(piece);
  }
}

void destruction_fracture_piece(FracturePiece *piece) {
  if (!piece || piece->fracture_depth >= DESTRUCTION_MAX_FRACTURE_DEPTH) return;
  
  // Create smaller pieces from this piece
  u32 new_pieces = destruction_recursive_fracture(piece);
  
  if (new_pieces > 0) {
    LOG_DEBUG("Fractured piece %u into %u smaller pieces", piece->piece_id, new_pieces);
    
    // Mark original piece as destroyed
    piece->state = PIECE_STATE_STATIC;
  }
}

u32 destruction_recursive_fracture(const FracturePiece *parent_piece) {
  if (!parent_piece || g_piece_count + 4 > g_piece_capacity) return 0;
  
  u32 pieces_created = 0;
  f32 child_size = 0.25f; // Smaller pieces
  
  // Create 4 smaller pieces
  for (u32 i = 0; i < 4 && pieces_created < 4; i++) {
    FracturePiece *child = &g_fracture_pieces[g_piece_count + pieces_created];
    memset(child, 0, sizeof(FracturePiece));
    
    // Set child properties
    child->piece_id = g_piece_count + pieces_created;
    child->parent_piece_id = parent_piece->piece_id;
    child->fracture_depth = parent_piece->fracture_depth + 1;
    child->state = PIECE_STATE_ACTIVE;
    child->creation_time = get_current_time();
    child->last_update_time = child->creation_time;
    
    // Position child pieces around parent
    f32 angle = (f32)i * M_PI / 2.0f;
    Vec3 offset = {cosf(angle) * child_size, 0.0f, sinf(angle) * child_size};
    child->position = vec3_add(parent_piece->position, offset);
    
    // Copy and scale geometry
    child->vertex_count = parent_piece->vertex_count;
    child->vertices = (Vec3*)malloc(child->vertex_count * sizeof(Vec3));
    child->normals = (Vec3*)malloc(child->vertex_count * sizeof(Vec3));
    child->uvs = (Vec2*)malloc(child->vertex_count * sizeof(Vec2));
    
    for (u32 j = 0; j < child->vertex_count; j++) {
      Vec3 relative_pos = vec3_sub(parent_piece->vertices[j], parent_piece->position);
      relative_pos = vec3_scale(relative_pos, 0.5f); // Scale down
      child->vertices[j] = vec3_add(child->position, relative_pos);
      child->normals[j] = parent_piece->normals[j];
      child->uvs[j] = parent_piece->uvs[j];
    }
    
    child->index_count = parent_piece->index_count;
    child->indices = (u32*)malloc(child->index_count * sizeof(u32));
    memcpy(child->indices, parent_piece->indices, child->index_count * sizeof(u32));
    
    // Calculate physics for child
    destruction_calculate_piece_physics(child);
    
    // Set material properties
    child->structural_integrity = parent_piece->structural_integrity * 0.8f;
    child->stress_factor = 0.0f;
    child->material_type = parent_piece->material_type;
    child->color = parent_piece->color;
    child->roughness = parent_piece->roughness;
    child->metalness = parent_piece->metalness;
    child->has_inner_surface = true;
    
    pieces_created++;
  }
  
  g_piece_count += pieces_created;
  return pieces_created;
}

void destruction_cleanup_pieces(void) {
  if (!g_destruction_initialized) return;
  
  f64 current_time = get_current_time();
  f64 max_age = 30.0f; // Remove pieces older than 30 seconds
  f32 max_distance = 100.0f; // Remove pieces farther than 100 units from origin
  
  u32 write_index = 0;
  
  for (u32 i = 0; i < g_piece_count; i++) {
    FracturePiece *piece = &g_fracture_pieces[i];
    
    f32 distance = vec3_length(&piece->position);
    f64 age = current_time - piece->creation_time;
    
    // Keep piece if it's not too old, not too far, and not destroyed
    if (age < max_age && distance < max_distance && piece->state != PIECE_STATE_STATIC) {
      // Move piece to new position if needed
      if (write_index != i) {
        g_fracture_pieces[write_index] = g_fracture_pieces[i];
      }
      write_index++;
    } else {
      // Free piece data
      if (piece->vertices) free(piece->vertices);
      if (piece->indices) free(piece->indices);
      if (piece->normals) free(piece->normals);
      if (piece->uvs) free(piece->uvs);
    }
  }
  
  if (write_index != g_piece_count) {
    LOG_DEBUG("Cleaned up %u destruction pieces", g_piece_count - write_index);
    g_piece_count = write_index;
  }
}

u32 destruction_get_piece_count(void) {
  return g_destruction_initialized ? g_piece_count : 0;
}

FracturePiece* destruction_get_piece(u32 piece_id) {
  if (!g_destruction_initialized || piece_id >= g_piece_count) return NULL;
  
  return &g_fracture_pieces[piece_id];
}

void destruction_clear_all(void) {
  if (!g_destruction_initialized) return;
  
  // Free all piece data
  for (u32 i = 0; i < g_piece_count; i++) {
    FracturePiece *piece = &g_fracture_pieces[i];
    
    if (piece->vertices) free(piece->vertices);
    if (piece->indices) free(piece->indices);
    if (piece->normals) free(piece->normals);
    if (piece->uvs) free(piece->uvs);
  }
  
  g_piece_count = 0;
  LOG_INFO("Cleared all destruction pieces");
}

void destruction_apply_explosion(const Vec3 *center, f32 radius, f32 force) {
  if (!g_destruction_initialized || !center) return;
  
  for (u32 i = 0; i < g_piece_count; i++) {
    FracturePiece *piece = &g_fracture_pieces[i];
    
    if (piece->state == PIECE_STATE_STATIC) continue;
    
    // Calculate distance from explosion center
    Vec3 to_piece = vec3_sub(piece->position, *center);
    f32 distance = vec3_length(&to_piece);
    
    if (distance < radius) {
      // Apply explosion force
      f32 force_factor = 1.0f - (distance / radius);
      Vec3 explosion_direction = vec3_normalize(&to_piece);
      Vec3 explosion_force_vec = vec3_scale(explosion_direction, force * force_factor);
      
      piece->velocity = vec3_add(piece->velocity, explosion_force_vec);
      piece->angular_velocity = vec3_add(piece->angular_velocity, 
        vec3_scale((Vec3){((f32)rand() / RAND_MAX - 0.5f), ((f32)rand() / RAND_MAX - 0.5f), ((f32)rand() / RAND_MAX - 0.5f)}, 
        force_factor * 2.0f));
      
      // Increase stress
      piece->stress_factor = fminf(1.0f, piece->stress_factor + force_factor);
      
      // Wake up sleeping pieces
      if (piece->state == PIECE_STATE_SLEEPING) {
        piece->state = PIECE_STATE_ACTIVE;
        piece->sleep_frames = 0;
      }
    }
  }
  
  LOG_DEBUG("Applied explosion at (%.2f, %.2f, %.2f) with radius %.2f and force %.2f",
           center->x, center->y, center->z, radius, force);
}

// Internal helper functions
static f64 get_current_time(void) {
  static f64 time_counter = 0.0f;
  time_counter += 0.016f; // Assume 60 FPS
  return time_counter;
}

static Vec3 vec3_normalize(const Vec3 *v) {
  f32 length = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
  if (length < 1e-6f) return (Vec3){0.0f, 0.0f, 0.0f};
  
  return (Vec3){v->x / length, v->y / length, v->z / length};
}

static f32 vec3_length(const Vec3 *v) {
  return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

static f32 vec3_length_sq(const Vec3 *v) {
  return v->x * v->x + v->y * v->y + v->z * v->z;
}

static Vec3 vec3_add(const Vec3 *a, const Vec3 *b) {
  return (Vec3){a->x + b->x, a->y + b->y, a->z + b->z};
}

static Vec3 vec3_sub(const Vec3 *a, const Vec3 *b) {
  return (Vec3){a->x - b->x, a->y - b->y, a->z - b->z};
}

static Vec3 vec3_scale(const Vec3 *v, f32 s) {
  return (Vec3){v->x * s, v->y * s, v->z * s};
}

static f32 vec3_dot(const Vec3 *a, const Vec3 *b) {
  return a->x * b->x + a->y * b->y + a->z * b->z;
}
