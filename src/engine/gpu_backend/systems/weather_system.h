#ifndef WEATHER_SYSTEM_H
#define WEATHER_SYSTEM_H

#include <stdint.h>

typedef enum {
  WEATHER_CLEAR = 0,
  WEATHER_RAIN = 1,
  WEATHER_SNOW = 2,
  WEATHER_STORM = 3,
  WEATHER_FOG = 4
} WeatherType;

void weather_sys_init(void);
void weather_sys_shutdown(void);
void weather_sys_update(float delta_time);

void weather_sys_set_type(WeatherType type);
WeatherType weather_sys_get_type(void);

void weather_sys_set_rain_intensity(float intensity);
float weather_sys_get_rain_intensity(void);

void weather_sys_set_snow_intensity(float intensity);
float weather_sys_get_snow_intensity(void);

void weather_sys_set_wind_strength(float strength);
float weather_sys_get_wind_strength(void);

void weather_sys_set_fog_density(float density);
float weather_sys_get_fog_density(void);

void weather_sys_trigger_lightning(float x, float y, float z);

#endif // WEATHER_SYSTEM_H
