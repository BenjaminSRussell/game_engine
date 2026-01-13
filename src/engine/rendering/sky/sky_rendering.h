#ifndef SKY_RENDERING_H
#define SKY_RENDERING_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WeatherManager WeatherManager;

typedef enum {
    SKY_TYPE_CLEAR,
    SKY_TYPE_CLOUDY,
    SKY_TYPE_OVERCAST,
    SKY_TYPE_STORMY
} SkyType;

typedef enum {
    CLOUD_TYPE_NONE,
    CLOUD_TYPE_STRATUS,
    CLOUD_TYPE_CUMULUS,
    CLOUD_TYPE_CIRRUS,
    CLOUD_TYPE_VOLUMETRIC
} CloudType;

bool sky_system_init(bool enable_atmospheric_scattering, bool enable_volumetric_clouds);
void sky_system_shutdown(void);
void sky_system_update(float dt);
void sky_system_render(const float *view_matrix, const float *proj_matrix, const float *camera_pos);
void sky_system_set_weather_manager(WeatherManager *manager);
void sky_system_set_time_of_day(float hours);
void sky_system_set_weather(SkyType sky_type, CloudType cloud_type, float coverage, float density);
void sky_system_get_sun_direction(float *direction);
void sky_system_get_sun_color(float *color);
void sky_system_get_ambient_color(float *color);
void sky_system_enable_atmospheric_scattering(bool enabled);
void sky_system_get_stats(float *render_time, uint32_t *rendered_pixels);

#ifdef __cplusplus
}
#endif

#endif // SKY_RENDERING_H
