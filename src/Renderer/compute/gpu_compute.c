#include "include/rendering/gpu_compute.h"
#include "physics_compute_shaders.h"

const char* gpu_compute_get_navier_stokes_shader(void) {
    return NAVIER_STOKES_COMPUTE_SHADER;
}

const char* gpu_compute_get_flip_p2g_shader(void) {
    return FLIP_P2G_SHADER;
}

const char* gpu_compute_get_sph_density_shader(void) {
    return SPH_DENSITY_SHADER;
}

const char* gpu_compute_get_particle_update_shader(void) {
    return PARTICLE_UPDATE_SHADER;
}

const char* gpu_compute_get_shallow_water_shader(void) {
    return SHALLOW_WATER_SHADER;
}

const char* gpu_compute_get_broadphase_shader(void) {
    return GPU_BROADPHASE_SHADER;
}
