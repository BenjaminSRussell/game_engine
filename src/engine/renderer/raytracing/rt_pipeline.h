#ifndef RT_PIPELINE_H
#define RT_PIPELINE_H

typedef struct {
  float origin[3];
  float direction[3];
  float t_min;
  float t_max;
} Ray;

typedef struct {
  unsigned int shader_binding_table_offset;
  unsigned int miss_shader_index;
  unsigned int hit_group_index;
  float max_ray_length;
  unsigned int max_recursion_depth;
} RTPipelineConfig;

int rt_pipeline_init(unsigned int width, unsigned int height);
void rt_create_raygen_shader(const char *shader_code);
void rt_create_closest_hit_shader(const char *shader_code);
void rt_create_any_hit_shader(const char *shader_code);
void rt_create_miss_shader(const char *shader_code);
void rt_dispatch_rays(unsigned int width, unsigned int height);
int rt_trace_ray_cpu(Ray *ray, float *hit_distance, float hit_normal[3]);
void rt_build_sbt(void *raygen, void *miss, void *hit_group);
void rt_update_acceleration_structure(void *geometry_data,
                                      unsigned int geometry_count);
void rt_set_pipeline_state(RTPipelineConfig *config);
void rt_get_stats(unsigned int *rays_traced, float *avg_bounces);
void rt_hybrid_render(unsigned int width, unsigned int height);
void rt_denoise_output(void *noisy_buffer, void *output_buffer,
                       unsigned int width, unsigned int height,
                       void *normal_buffer, void *albedo_buffer);
void rt_pipeline_shutdown();

#endif // RT_PIPELINE_H
