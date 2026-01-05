/*
 * descriptor_writer.h
 * Batched descriptor updates
 */

#ifndef CORE_DESCRIPTOR_WRITER_H
#define CORE_DESCRIPTOR_WRITER_H

#include "descriptor_pool.h"
#include "../../resource_management/resource_handle.h"
#include "descriptor_set_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t id;
} descriptor_writer_handle_t;

/* Initialization */
void descriptor_writer_init_system(void);
void descriptor_writer_shutdown_system(void);

/* Lifecycle */
descriptor_writer_handle_t descriptor_writer_create(void);
void descriptor_writer_destroy(descriptor_writer_handle_t writer);

/* Write Operations */
void descriptor_writer_write_buffer(descriptor_writer_handle_t writer, 
                                    uint32_t binding, 
                                    buffer_handle_t buffer, 
                                    size_t offset, 
                                    size_t range, 
                                    descriptor_type_t type);

void descriptor_writer_write_image(descriptor_writer_handle_t writer, 
                                   uint32_t binding, 
                                   texture_handle_t texture, 
                                   sampler_handle_t sampler, 
                                   descriptor_type_t type); // type: usually COMBINED_IMAGE_SAMPLER

/* Flush */
// Applies all pending writes to the target set
// Returns true on success
bool descriptor_writer_build(descriptor_writer_handle_t writer, 
                             descriptor_set_handle_t target_set);

// Clears pending writes without applying them
void descriptor_writer_clear(descriptor_writer_handle_t writer);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DESCRIPTOR_WRITER_H */
