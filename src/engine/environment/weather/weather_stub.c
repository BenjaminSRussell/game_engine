/**
 * =================================================================================================
 *                          WEATHER SYSTEM STUB
 *                          For linkage in unit tests
 * =================================================================================================
 */

#include <stdbool.h>
#include <stdint.h>

// Mock types
typedef struct WeatherSystem WeatherSystem;
typedef enum {
  WEATHER_TYPE_CLEAR,
  WEATHER_TYPE_RAIN,
  WEATHER_TYPE_STORM
} WeatherType;

// Stubs
int weather_get_current_type(WeatherSystem *sys) { return 0; }
float weather_get_intensity(WeatherSystem *sys) { return 0.0f; }
