#ifndef GPU_COMPUTE_H
#define GPU_COMPUTE_H

#ifdef __cplusplus
extern "C" {
#endif

// Accessors for compute shader source code
const char* gpu_compute_get_navier_stokes_shader(void);
const char* gpu_compute_get_flip_p2g_shader(void);
const char* gpu_compute_get_sph_density_shader(void);
const char* gpu_compute_get_particle_update_shader(void);
const char* gpu_compute_get_shallow_water_shader(void);
const char* gpu_compute_get_broadphase_shader(void);

#ifdef __cplusplus
}
#endif

#endif // GPU_COMPUTE_H
