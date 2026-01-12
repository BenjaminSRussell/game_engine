/**
 * =================================================================================================
 *                              RENDERING PIPELINE CORE
 *                              Agent: AGENT_RENDER_1
 * =================================================================================================
 */

#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    RENDER PASSES
 * =================================================================================================
 */

void pass_depth_prepass(void);
void pass_gbuffer(void);
void pass_shadow_cascade(void);
void pass_shadow_point(void);
void pass_shadow_spot(void);
void pass_ssao(void);
void pass_hbao(void);
void pass_gtao(void);
void pass_ssr(void);
void pass_ssgi(void);
void pass_deferred_lighting(void);
void pass_forward_transparent(void);
void pass_volumetric_fog(void);
void pass_volumetric_clouds(void);
void pass_atmosphere(void);
void pass_post_process(void);
void pass_ui(void);
void pass_debug(void);

/* =================================================================================================
 *                                    RENDER TARGETS
 * =================================================================================================
 */

void *render_target_create(uint32_t width, uint32_t height);
void render_target_destroy(void *rt);
void render_target_resize(void *rt, uint32_t width, uint32_t height);
void render_target_bind(void *rt);
void render_target_clear(void *rt, uint32_t flags, float *color);
void render_target_blit(void *src, void *dst);
void render_target_resolve(void *rt);
void render_target_read_pixels(void *rt, void *buffer);
void render_target_generate_mips(void *rt);

/* =================================================================================================
 *                                    MATERIAL SYSTEM
 * =================================================================================================
 */

void *material_create(const char *name);
void material_destroy(void *mat);
void material_set_shader(void *mat, uint32_t shader_id);
void material_set_texture(void *mat, const char *name, uint32_t texture_id);
void material_set_float(void *mat, const char *name, float value);
void material_set_vector(void *mat, const char *name, float *value);
void material_set_matrix(void *mat, const char *name, float *value);
void *material_instance_create(void *parent_mat);
void material_instance_override(void *inst, const char *name, float *value);
void material_bind(void *mat);
void material_serialize(void *mat, const char *path);
void material_deserialize(void *mat, const char *path);
void material_hot_reload(void *mat);

/* =================================================================================================
 *                                    MESH SYSTEM
 * =================================================================================================
 */

void *mesh_create(void);
void mesh_destroy(void *mesh);
void mesh_upload(void *mesh, float *vertices, uint32_t *indices);
void mesh_update_vertices(void *mesh, float *vertices, uint32_t offset,
                          uint32_t size);
void mesh_update_indices(void *mesh, uint32_t *indices, uint32_t offset,
                         uint32_t size);
void mesh_generate_tangents(void *mesh);
void mesh_generate_normals(void *mesh);
void mesh_optimize(void *mesh);
void mesh_simplify(void *mesh, float quality);
void mesh_generate_lod(void *mesh, int levels);
void mesh_calculate_bounds(void *mesh, float *min, float *max);
void *mesh_merge(void *mesh_a, void *mesh_b);
void mesh_split(void *mesh);
void *mesh_import_obj(const char *path);
void *mesh_import_fbx(const char *path);
void *mesh_import_gltf(const char *path);
void mesh_export(void *mesh, const char *path);

/* =================================================================================================
 *                                    TEXTURE SYSTEM
 * =================================================================================================
 */

void *texture_create_2d(uint32_t width, uint32_t height, uint32_t format);
void *texture_create_3d(uint32_t width, uint32_t height, uint32_t depth,
                        uint32_t format);
void *texture_create_cube(uint32_t size, uint32_t format);
void *texture_create_array(uint32_t width, uint32_t height, uint32_t layers,
                           uint32_t format);
void texture_destroy(void *tex);
void texture_upload(void *tex, void *data);
void texture_upload_compressed(void *tex, void *data, uint32_t size);
void texture_generate_mips(void *tex);
void texture_compress_bc1(void *tex);
void texture_compress_bc3(void *tex);
void texture_compress_bc5(void *tex);
void texture_compress_bc7(void *tex);
void *texture_import_png(const char *path);
void *texture_import_jpg(const char *path);
void *texture_import_hdr(const char *path);
void *texture_import_dds(const char *path);
void texture_streaming_load(void *tex);
void texture_streaming_unload(void *tex);

/* =================================================================================================
 *                                    SHADER SYSTEM
 * =================================================================================================
 */

uint32_t shader_compile_vertex(const char *source);
uint32_t shader_compile_fragment(const char *source);
uint32_t shader_compile_compute(const char *source);
uint32_t shader_link_program(uint32_t vs, uint32_t fs);
void shader_reflect(uint32_t program);
void shader_bind(uint32_t program);
void shader_set_uniforms(uint32_t program, void *buffer);
void shader_set_texture(uint32_t program, const char *name, void *tex);
void shader_set_buffer(uint32_t program, const char *name, void *buffer);
void shader_dispatch_compute(uint32_t program, uint32_t x, uint32_t y,
                             uint32_t z);
void shader_hot_reload(uint32_t program);
char *shader_preprocess(const char *source);
void shader_include_resolve(const char *path);
void shader_variant_generate(const char *source, const char **defines);
void shader_cache_load(const char *path);
void shader_cache_save(const char *path);

/* =================================================================================================
 *                                    LIGHTING
 * =================================================================================================
 */

void light_directional_add(float *direction, float *color);
void light_point_add(float *position, float *color, float radius);
void light_spot_add(float *position, float *direction, float *color,
                    float angle);
void light_area_add(float *position, float *color, float *size);
void light_probe_add(float *position, float radius);
void light_remove(uint32_t id);
void light_update(uint32_t id, void *data);
void light_culling(void);
void light_cluster_build(void);
void light_cluster_assign(void);
void light_shadow_setup(uint32_t light_id);
void light_bake_indirect(void);
void light_probe_capture(uint32_t probe_id);
void light_probe_blend(void);

/* =================================================================================================
 *                                    CULLING
 * =================================================================================================
 */

void culling_frustum(void);
void culling_occlusion_hi_z(void);
void culling_occlusion_gpu(void);
void culling_distance(float max_dist);
void culling_contribution(float threshold);
void culling_portal(void);
void culling_potentially_visible_set(void);
void culling_batch(void);

/* =================================================================================================
 *                                    RENDER GRAPH
 * =================================================================================================
 */

void *render_graph_create(void);
void render_graph_add_pass(void *rg, const char *name, void (*exec)(void));
void render_graph_add_resource(void *rg, const char *name);
void render_graph_compile(void *rg);
void render_graph_execute(void *rg);
void render_graph_validate(void *rg);
void render_graph_barrier_insert(void *rg);
void render_graph_resource_alias(void *rg, const char *name1,
                                 const char *name2);
void render_graph_visualize(void *rg);

#endif // RENDER_PIPELINE_H
