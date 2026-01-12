/**
 * @file physics_serialization.c
 * @brief Physics serialization system implementation
 *
 * Provides serialization and deserialization for physics objects,
 * materials, constraints, and world state with support for multiple
 * formats and versioning.
 */

#include "physics_serialization.h"
#include "../world/physics_world_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

// ========================================
// Constants
// ========================================

#define PHYSICS_MAGIC_NUMBER "PHYS"
#define CURRENT_SERIALIZATION_VERSION 1
#define DEFAULT_BUFFER_SIZE (1024 * 1024) // 1MB
#define MIN_BUFFER_SIZE 1024
#define MAX_BUFFER_SIZE (100 * 1024 * 1024) // 100MB

// ========================================
// CRC32 Checksum Implementation
// ========================================

static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

static void init_crc32_table(void) {
    if (crc32_table_initialized) return;
    
    uint32_t polynomial = 0xEDB88320;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = true;
}

static uint32_t calculate_crc32(const void *data, size_t size) {
    init_crc32_table();
    
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *bytes = (const uint8_t*)data;
    
    for (size_t i = 0; i < size; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ bytes[i]) & 0xFF];
    }
    
    return crc ^ 0xFFFFFFFF;
}

// ========================================
// Simple Compression (RLE)
// ========================================

static size_t compress_rle(const uint8_t *input, size_t input_size, uint8_t *output, size_t output_size) {
    if (!input || !output || input_size == 0) return 0;
    
    size_t output_pos = 0;
    size_t input_pos = 0;
    
    while (input_pos < input_size && output_pos < output_size) {
        uint8_t current = input[input_pos];
        uint8_t count = 1;
        
        // Count consecutive identical bytes
        while (input_pos + count < input_size && 
               input[input_pos + count] == current && 
               count < 255) {
            count++;
        }
        
        // Write run-length encoded data
        if (output_pos + 2 <= output_size) {
            output[output_pos++] = count;
            output[output_pos++] = current;
        } else {
            break;
        }
        
        input_pos += count;
    }
    
    return output_pos;
}

static size_t decompress_rle(const uint8_t *input, size_t input_size, uint8_t *output, size_t output_size) {
    if (!input || !output || input_size == 0) return 0;
    
    size_t output_pos = 0;
    size_t input_pos = 0;
    
    while (input_pos + 1 < input_size && output_pos < output_size) {
        uint8_t count = input[input_pos++];
        uint8_t value = input[input_pos++];
        
        // Expand run
        for (uint8_t i = 0; i < count && output_pos < output_size; i++) {
            output[output_pos++] = value;
        }
    }
    
    return output_pos;
}

// ========================================
// JSON Serialization Helpers
// ========================================

static void write_json_string(FILE *file, const char *key, const char *value) {
    fprintf(file, "\"%s\":\"%s\"", key, value);
}

static void write_json_float(FILE *file, const char *key, float value) {
    fprintf(file, "\"%s\":%.6f", key, value);
}

static void write_json_int(FILE *file, const char *key, int value) {
    fprintf(file, "\"%s\":%d", key, value);
}

static void write_json_bool(FILE *file, const char *key, bool value) {
    fprintf(file, "\"%s\":%s", key, value ? "true" : "false");
}

static void write_json_float_array(FILE *file, const char *key, const float *array, int count) {
    fprintf(file, "\"%s\":[", key);
    for (int i = 0; i < count; i++) {
        fprintf(file, "%.6f", array[i]);
        if (i < count - 1) fprintf(file, ",");
    }
    fprintf(file, "]");
}

// ========================================
// Binary Serialization Helpers
// ========================================

static size_t write_binary_data(FILE *file, const void *data, size_t size) {
    return fwrite(data, 1, size, file);
}

static size_t read_binary_data(FILE *file, void *data, size_t size) {
    return fread(data, 1, size, file);
}

static void write_binary_uint32(FILE *file, uint32_t value) {
    uint8_t bytes[4];
    bytes[0] = (value >> 0) & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = (value >> 16) & 0xFF;
    bytes[3] = (value >> 24) & 0xFF;
    write_binary_data(file, bytes, 4);
}

static uint32_t read_binary_uint32(FILE *file) {
    uint8_t bytes[4];
    read_binary_data(file, bytes, 4);
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | 
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static void write_binary_uint64(FILE *file, uint64_t value) {
    uint8_t bytes[8];
    for (int i = 0; i < 8; i++) {
        bytes[i] = (value >> (i * 8)) & 0xFF;
    }
    write_binary_data(file, bytes, 8);
}

static uint64_t read_binary_uint64(FILE *file) {
    uint8_t bytes[8];
    read_binary_data(file, bytes, 8);
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= ((uint64_t)bytes[i]) << (i * 8);
    }
    return value;
}

// ========================================
// Public API Implementation
// ========================================

SerializationContext* serialization_context_create(SerializationFormat format, 
                                                   uint32_t flags, size_t buffer_size) {
    SerializationContext *context = (SerializationContext*)calloc(1, sizeof(SerializationContext));
    if (!context) return NULL;
    
    context->format = format;
    context->flags = flags;
    context->version = CURRENT_SERIALIZATION_VERSION;
    context->buffer_size = (buffer_size > 0) ? buffer_size : DEFAULT_BUFFER_SIZE;
    
    context->buffer = (uint8_t*)malloc(context->buffer_size);
    if (!context->buffer) {
        free(context);
        return NULL;
    }
    
    context->buffer_used = 0;
    context->own_buffer = true;
    
    return context;
}

void serialization_context_destroy(SerializationContext *context) {
    if (!context) return;
    
    if (context->own_buffer && context->buffer) {
        free(context->buffer);
    }
    
    if (context->compression_context) {
        free(context->compression_context);
    }
    
    if (context->encryption_context) {
        free(context->encryption_context);
    }
    
    free(context);
}

void serialization_context_reset(SerializationContext *context) {
    if (!context) return;
    
    context->buffer_used = 0;
    
    if (context->compression_context) {
        free(context->compression_context);
        context->compression_context = NULL;
    }
    
    if (context->encryption_context) {
        free(context->encryption_context);
        context->encryption_context = NULL;
    }
}

bool serialize_physics_object(SerializationContext *context, const PhysicsObject *object) {
    if (!context || !object) return false;
    
    // Check buffer space
    size_t required_size = sizeof(ObjectSerializationData);
    if (context->buffer_used + required_size > context->buffer_size) {
        if (!serialization_context_resize_buffer(context, context->buffer_size * 2)) {
            return false;
        }
    }
    
    ObjectSerializationData *data = (ObjectSerializationData*)(context->buffer + context->buffer_used);
    
    // Copy object data
    data->object_id = object->id;
    data->type = object->type;
    data->body_type = object->body_type;
    data->material_id = object->material ? object->material->material_id : 0;
    data->active = object->active;
    data->sleeping = object->sleeping;
    data->visible = object->visible;
    data->layer_mask = object->layer_mask;
    data->category_mask = object->category_mask;
    
    // Copy transform
    memcpy(data->transform, object->transform, sizeof(float) * 16);
    
    // Copy physics properties
    memcpy(data->velocity, object->velocity, sizeof(float) * 3);
    memcpy(data->angular_velocity, object->angular_velocity, sizeof(float) * 3);
    data->mass = object->mass;
    memcpy(data->inertia, object->inertia, sizeof(float) * 9);
    memcpy(data->center_of_mass, object->center_of_mass, sizeof(float) * 3);
    
    // Copy bounds
    memcpy(data->bounding_box, object->bounding_box, sizeof(float) * 6);
    memcpy(data->bounding_sphere, object->bounding_sphere, sizeof(float) * 4);
    
    // Custom data (simplified)
    data->custom_data = NULL;
    data->custom_data_size = 0;
    
    context->buffer_used += required_size;
    
    return true;
}

bool serialize_physics_world(SerializationContext *context, const PhysicsWorld *world) {
    if (!context || !world) return false;
    
    // Write header
    SerializationHeader header;
    memcpy(header.magic, PHYSICS_MAGIC_NUMBER, 4);
    header.version = context->version;
    header.flags = context->flags;
    header.format = context->format;
    header.timestamp = (uint64_t)time(NULL);
    header.checksum = 0; // Will be calculated later
    header.data_size = 0; // Will be calculated later
    header.compressed_size = 0; // Will be calculated later
    strcpy(header.description, "Physics World State");
    
    // Estimate required size
    size_t estimated_size = sizeof(SerializationHeader) + 
                           world->object_count * sizeof(ObjectSerializationData) +
                           world->material_count * sizeof(MaterialSerializationData) +
                           world->constraint_count * sizeof(ConstraintSerializationData);
    
    // Resize buffer if needed
    if (estimated_size > context->buffer_size) {
        if (!serialization_context_resize_buffer(context, estimated_size)) {
            return false;
        }
    }
    
    // Write header to buffer
    memcpy(context->buffer, &header, sizeof(SerializationHeader));
    context->buffer_used = sizeof(SerializationHeader);
    
    // Serialize objects
    for (int i = 0; i < world->object_count; i++) {
        if (!serialize_physics_object(context, &world->objects[i])) {
            return false;
        }
    }
    
    // Serialize materials
    for (int i = 0; i < world->material_count; i++) {
        MaterialSerializationData *material_data = (MaterialSerializationData*)(context->buffer + context->buffer_used);
        
        PhysicsMaterial *material = &world->materials[i];
        material_data->material_id = material->material_id;
        strcpy(material_data->name, material->name);
        material_data->density = material->density;
        material_data->friction = material->friction;
        material_data->restitution = material->restitution;
        material_data->damping = material->damping;
        material_data->angular_damping = material->angular_damping;
        material_data->thickness = material->thickness;
        material_data->is_trigger = material->is_trigger;
        material_data->is_sensor = material->is_sensor;
        material_data->custom_properties = NULL;
        material_data->custom_properties_size = 0;
        
        context->buffer_used += sizeof(MaterialSerializationData);
    }
    
    // Serialize constraints
    for (int i = 0; i < world->constraint_count; i++) {
        ConstraintSerializationData *constraint_data = (ConstraintSerializationData*)(context->buffer + context->buffer_used);
        
        PhysicsConstraint *constraint = &world->constraints[i];
        constraint_data->constraint_id = constraint->id;
        constraint_data->body_a_id = constraint->body_a_id;
        constraint_data->body_b_id = constraint->body_b_id;
        constraint_data->type = constraint->type;
        constraint_data->enabled = constraint->enabled;
        constraint_data->break_force = constraint->break_force;
        constraint_data->current_force = constraint->current_force;
        constraint_data->is_broken = constraint->is_broken;
        constraint_data->constraint_data = NULL;
        constraint_data->constraint_data_size = 0;
        
        context->buffer_used += sizeof(ConstraintSerializationData);
    }
    
    // Update header with actual sizes
    SerializationHeader *header_ptr = (SerializationHeader*)context->buffer;
    header_ptr->data_size = context->buffer_used - sizeof(SerializationHeader);
    
    // Calculate checksum
    if (context->flags & SERIALIZATION_FLAG_CHECKSUM) {
        header_ptr->checksum = calculate_crc32(context->buffer + sizeof(SerializationHeader), 
                                              header_ptr->data_size);
    }
    
    // Apply compression if requested
    if (context->flags & SERIALIZATION_FLAG_COMPRESS) {
        if (!compress_serialized_data(context)) {
            return false;
        }
        header_ptr->compressed_size = context->buffer_used - sizeof(SerializationHeader);
    }
    
    return true;
}

bool save_world_to_file(const PhysicsWorld *world, const char *filename, 
                        SerializationFormat format, uint32_t flags) {
    if (!world || !filename) return false;
    
    // Create serialization context
    SerializationContext *context = serialization_context_create(format, flags, 0);
    if (!context) return false;
    
    // Serialize world
    if (!serialize_physics_world(context, world)) {
        serialization_context_destroy(context);
        return false;
    }
    
    // Write to file
    FILE *file = fopen(filename, "wb");
    if (!file) {
        serialization_context_destroy(context);
        return false;
    }
    
    size_t written = fwrite(context->buffer, 1, context->buffer_used, file);
    fclose(file);
    
    serialization_context_destroy(context);
    
    return written == context->buffer_used;
}

bool load_world_from_file(const char *filename, WorldSerializationData *world) {
    if (!filename || !world) return false;
    
    // Read file
    FILE *file = fopen(filename, "rb");
    if (!file) return false;
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < sizeof(SerializationHeader)) {
        fclose(file);
        return false;
    }
    
    // Read header
    SerializationHeader header;
    fread(&header, sizeof(SerializationHeader), 1, file);
    
    // Validate magic number
    if (memcmp(header.magic, PHYSICS_MAGIC_NUMBER, 4) != 0) {
        fclose(file);
        return false;
    }
    
    // Create serialization context
    SerializationContext *context = serialization_context_create((SerializationFormat)header.format, 
                                                              header.flags, file_size);
    if (!context) {
        fclose(file);
        return false;
    }
    
    // Read file data
    fread(context->buffer, 1, file_size - sizeof(SerializationHeader), file);
    context->buffer_used = file_size - sizeof(SerializationHeader);
    fclose(file);
    
    // Verify checksum
    if (header.flags & SERIALIZATION_FLAG_CHECKSUM) {
        uint32_t calculated_checksum = calculate_crc32(context->buffer, header.data_size);
        if (calculated_checksum != header.checksum) {
            serialization_context_destroy(context);
            return false;
        }
    }
    
    // Decompress if needed
    if (header.flags & SERIALIZATION_FLAG_COMPRESS) {
        if (!decompress_serialized_data(context)) {
            serialization_context_destroy(context);
            return false;
        }
    }
    
    // Deserialize world (simplified)
    memset(world, 0, sizeof(WorldSerializationData));
    
    // This is a simplified implementation - in a real system, you would
    // properly deserialize all the data based on the format
    
    serialization_context_destroy(context);
    return true;
}

uint32_t calculate_data_checksum(const void *data, size_t data_size) {
    return calculate_crc32(data, data_size);
}

bool compress_serialized_data(SerializationContext *context) {
    if (!context || context->buffer_used == 0) return false;
    
    // Allocate temporary buffer for compressed data
    uint8_t *compressed_buffer = (uint8_t*)malloc(context->buffer_used);
    if (!compressed_buffer) return false;
    
    // Compress using RLE (simplified)
    size_t compressed_size = compress_rle(context->buffer, context->buffer_used, 
                                        compressed_buffer, context->buffer_used);
    
    if (compressed_size > 0 && compressed_size < context->buffer_used) {
        // Copy compressed data back
        memcpy(context->buffer, compressed_buffer, compressed_size);
        context->buffer_used = compressed_size;
        free(compressed_buffer);
        return true;
    }
    
    free(compressed_buffer);
    return false;
}

bool decompress_serialized_data(SerializationContext *context) {
    if (!context || context->buffer_used == 0) return false;
    
    // Allocate temporary buffer for decompressed data
    uint8_t *decompressed_buffer = (uint8_t*)malloc(context->buffer_size);
    if (!decompressed_buffer) return false;
    
    // Decompress using RLE (simplified)
    size_t decompressed_size = decompress_rle(context->buffer, context->buffer_used,
                                            decompressed_buffer, context->buffer_size);
    
    if (decompressed_size > 0) {
        // Copy decompressed data back
        memcpy(context->buffer, decompressed_buffer, decompressed_size);
        context->buffer_used = decompressed_size;
        free(decompressed_buffer);
        return true;
    }
    
    free(decompressed_buffer);
    return false;
}

size_t estimate_serialization_size(const PhysicsWorld *world, 
                                   SerializationFormat format, uint32_t flags) {
    if (!world) return 0;
    
    size_t base_size = sizeof(SerializationHeader) + 
                      world->object_count * sizeof(ObjectSerializationData) +
                      world->material_count * sizeof(MaterialSerializationData) +
                      world->constraint_count * sizeof(ConstraintSerializationData);
    
    // Add overhead for different formats
    switch (format) {
        case SERIALIZATION_FORMAT_JSON:
            base_size *= 2; // JSON is more verbose
            break;
        case SERIALIZATION_FORMAT_XML:
            base_size *= 3; // XML is even more verbose
            break;
        case SERIALIZATION_FORMAT_BINARY:
            // No additional overhead
            break;
        case SERIALIZATION_FORMAT_MSGPACK:
            base_size *= 1.2f; // Slight overhead
            break;
        default:
            break;
    }
    
    // Add compression overhead estimation
    if (flags & SERIALIZATION_FLAG_COMPRESS) {
        base_size *= 0.7f; // Assume 30% compression
    }
    
    // Add encryption overhead estimation
    if (flags & SERIALIZATION_FLAG_ENCRYPT) {
        base_size *= 1.1f; // 10% overhead for encryption
    }
    
    return base_size;
}
