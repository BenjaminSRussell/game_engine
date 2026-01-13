// Weather audio system implementation
#include "engine/include/core/logger.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <weather/weather_audio.h>

// Default audio configuration
static const WeatherAudioConfig DEFAULT_CONFIG = {.master_volume = 1.0f,
                                                  .rain_volume = 0.7f,
                                                  .wind_volume = 0.6f,
                                                  .thunder_volume = 1.0f,
                                                  .hail_volume = 0.8f,
                                                  .snow_volume = 0.4f,
                                                  .fog_volume = 0.3f,
                                                  .use_3d_audio = true,
                                                  .max_audio_distance = 64.0f,
                                                  .rain_crossfade_time = 1.0f,
                                                  .wind_modulation_speed =
                                                      0.5f};

// Sound file mappings (would be loaded from resources)
static const char *WEATHER_SOUND_FILES[WEATHER_SOUND_COUNT] = {
    "rain_light.wav",  "rain_heavy.wav",      "wind_soft.wav",
    "wind_strong.wav", "thunder_distant.wav", "thunder_close.wav",
    "hail.wav",        "snow_ambient.wav",    "fog_ambience.wav"};

void weather_audio_init(WeatherAudioSystem *audio, AudioSystem *engine) {
  if (!audio)
    return;

  memset(audio, 0, sizeof(WeatherAudioSystem));
  audio->audio_engine = engine;
  audio->config = DEFAULT_CONFIG;
  audio->current_rain_intensity = 0.0f;
  audio->target_rain_intensity = 0.0f;
  audio->current_wind_intensity = 0.0f;
  audio->target_wind_intensity = 0.0f;
  audio->rain_playing = false;
  audio->wind_playing = false;
  audio->thunder.lightning_active = false;
  audio->thunder.time_since_lightning = 0.0f;
  audio->initialized = true;

  LOG_INFO("Weather audio system initialized");
}

void weather_audio_free(WeatherAudioSystem *audio) {
  if (!audio)
    return;

  weather_audio_stop_rain(audio);
  weather_audio_stop_wind(audio);
  weather_audio_stop_thunder(audio);

  memset(audio, 0, sizeof(WeatherAudioSystem));
}

f32 weather_audio_get_rain_intensity(const WeatherSystem *weather) {
  if (!weather)
    return 0.0f;

  WeatherType type = weather->current.type;
  f32 intensity = 0.0f;

  switch (type) {
  case WEATHER_RAIN_LIGHT:
    intensity = 0.3f;
    break;
  case WEATHER_RAIN_MODERATE:
    intensity = 0.6f;
    break;
  case WEATHER_RAIN_HEAVY:
    intensity = 1.0f;
    break;
  case WEATHER_STORM:
    intensity = 0.9f; // Storm has more wind than rain
    break;
  default:
    intensity = 0.0f;
  }

  // Apply transition smoothing
  if (weather->current.is_transitioning) {
    f32 next_intensity = 0.0f;
    if (weather->current.next_type == WEATHER_RAIN_LIGHT) {
      next_intensity = 0.3f;
    } else if (weather->current.next_type == WEATHER_RAIN_MODERATE) {
      next_intensity = 0.6f;
    } else if (weather->current.next_type == WEATHER_RAIN_HEAVY) {
      next_intensity = 1.0f;
    }

    f32 t = weather->current.transition_progress;
    intensity = intensity * (1.0f - t) + next_intensity * t;
  }

  return intensity;
}

f32 weather_audio_get_wind_intensity(const WeatherSystem *weather) {
  if (!weather)
    return 0.0f;

  // Wind intensity based on wind speed
  f32 base_speed = 5.0f;
  f32 max_speed = 25.0f;
  f32 speed = weather->current.wind_speed;

  f32 intensity = (speed - base_speed) / (max_speed - base_speed);
  if (intensity < 0.0f)
    intensity = 0.0f;
  if (intensity > 1.0f)
    intensity = 1.0f;

  return intensity;
}

bool weather_audio_should_have_thunder(const WeatherSystem *weather) {
  if (!weather)
    return false;
  return weather_type_has_lightning(weather->current.type);
}

f32 weather_audio_calculate_sound_delay(f32 distance) {
  // Sound travels at ~343 m/s
  // distance is in world units (assume 1 unit = 1 meter)
  if (distance < 0.1f)
    return 0.0f;
  return distance / 343.0f;
}

void weather_audio_play_rain(WeatherAudioSystem *audio, f32 intensity) {
  if (!audio || !audio->audio_engine || intensity < 0.01f)
    return;

  audio->target_rain_intensity = intensity;

  if (!audio->rain_playing && intensity > 0.1f) {
    // Start playing rain
    // audio->active_rain_channel = audio_play_looped(audio->audio_engine,
    //     WEATHER_SOUND_RAIN_HEAVY, intensity * audio->config.rain_volume);
    audio->rain_playing = true;
    LOG_TRACE("Rain audio started at intensity %.2f", intensity);
  }
}

void weather_audio_stop_rain(WeatherAudioSystem *audio) {
  if (!audio)
    return;

  audio->target_rain_intensity = 0.0f;
  audio->current_rain_intensity = 0.0f;

  if (audio->rain_playing) {
    // audio_stop_channel(audio->audio_engine, audio->active_rain_channel);
    audio->rain_playing = false;
    LOG_TRACE("Rain audio stopped");
  }
}

void weather_audio_play_wind(WeatherAudioSystem *audio, f32 speed,
                             const Vec3 *direction) {
  if (!audio || !audio->audio_engine || speed < 1.0f)
    return;

  // Map wind speed to intensity
  f32 max_wind_speed = 20.0f;
  f32 intensity = speed / max_wind_speed;
  if (intensity > 1.0f)
    intensity = 1.0f;

  audio->target_wind_intensity = intensity;

  if (!audio->wind_playing && intensity > 0.1f) {
    // Start playing wind
    // audio->active_wind_channel = audio_play_looped(audio->audio_engine,
    //     WEATHER_SOUND_WIND_STRONG, intensity * audio->config.wind_volume);
    audio->wind_playing = true;
    LOG_TRACE("Wind audio started at intensity %.2f", intensity);
  }
}

void weather_audio_stop_wind(WeatherAudioSystem *audio) {
  if (!audio)
    return;

  audio->target_wind_intensity = 0.0f;
  audio->current_wind_intensity = 0.0f;

  if (audio->wind_playing) {
    // audio_stop_channel(audio->audio_engine, audio->active_wind_channel);
    audio->wind_playing = false;
    LOG_TRACE("Wind audio stopped");
  }
}

void weather_audio_play_thunder(WeatherAudioSystem *audio, const Vec3 *position,
                                f32 distance) {
  if (!audio || !audio->audio_engine || !position)
    return;

  // Calculate sound delay based on distance
  f32 delay = weather_audio_calculate_sound_delay(distance);

  // Sound level depends on distance
  f32 volume = 1.0f;
  f32 max_distance = audio->config.max_audio_distance;

  if (distance > max_distance) {
    volume = 0.0f;
  } else if (distance > 0.0f) {
    volume = 1.0f - (distance / max_distance);
  }

  volume *= audio->config.thunder_volume;

  // Select thunder sound based on distance
  WeatherSoundID sound_id = (distance < 50.0f) ? WEATHER_SOUND_THUNDER_CLOSE
                                               : WEATHER_SOUND_THUNDER_DISTANT;

  // Play sound with 3D positioning if enabled
  if (audio->config.use_3d_audio && volume > 0.01f) {
    // audio_play_3d(audio->audio_engine, sound_id, position, volume, delay);
    LOG_TRACE("Thunder audio played at distance %.1f with delay %.2f", distance,
              delay);
  }
}

void weather_audio_stop_thunder(WeatherAudioSystem *audio) {
  if (!audio)
    return;

  audio->thunder.lightning_active = false;
  audio->thunder.played_sound = false;
}

void weather_audio_update(WeatherAudioSystem *audio,
                          const WeatherSystem *weather, f32 delta_time) {
  if (!audio || !audio->audio_engine || !audio->initialized)
    return;

  // Update rain intensity with smooth crossfade
  if (audio->current_rain_intensity != audio->target_rain_intensity) {
    f32 step = delta_time / audio->config.rain_crossfade_time;
    if (audio->current_rain_intensity < audio->target_rain_intensity) {
      audio->current_rain_intensity += step;
      if (audio->current_rain_intensity > audio->target_rain_intensity) {
        audio->current_rain_intensity = audio->target_rain_intensity;
      }
    } else {
      audio->current_rain_intensity -= step;
      if (audio->current_rain_intensity < audio->target_rain_intensity) {
        audio->current_rain_intensity = audio->target_rain_intensity;
      }
    }

    // Update rain volume if playing
    if (audio->rain_playing) {
      // audio_set_volume(audio->audio_engine, audio->active_rain_channel,
      //     audio->current_rain_intensity * audio->config.rain_volume);
    }
  }

  // Update wind intensity with smooth modulation
  if (audio->current_wind_intensity != audio->target_wind_intensity) {
    f32 step = delta_time * audio->config.wind_modulation_speed;
    if (audio->current_wind_intensity < audio->target_wind_intensity) {
      audio->current_wind_intensity += step;
      if (audio->current_wind_intensity > audio->target_wind_intensity) {
        audio->current_wind_intensity = audio->target_wind_intensity;
      }
    } else {
      audio->current_wind_intensity -= step;
      if (audio->current_wind_intensity < audio->target_wind_intensity) {
        audio->current_wind_intensity = audio->target_wind_intensity;
      }
    }

    // Update wind volume if playing
    if (audio->wind_playing) {
      // audio_set_volume(audio->audio_engine, audio->active_wind_channel,
      //     audio->current_wind_intensity * audio->config.wind_volume);
    }
  }

  // Get rain and wind intensity from weather
  f32 rain_intensity = weather_audio_get_rain_intensity(weather);
  f32 wind_intensity = weather_audio_get_wind_intensity(weather);

  // Update rain audio
  if (rain_intensity > 0.1f) {
    weather_audio_play_rain(audio, rain_intensity);
  } else {
    weather_audio_stop_rain(audio);
  }

  // Update wind audio
  if (wind_intensity > 0.1f) {
    weather_audio_play_wind(audio, weather->current.wind_speed,
                            &weather->current.wind_direction);
  } else {
    weather_audio_stop_wind(audio);
  }

  // Update thunder audio
  if (weather_audio_should_have_thunder(weather)) {
    audio->thunder.lightning_active = true;
    audio->thunder.time_since_lightning += delta_time;

    // Simulate lightning events periodically during storms
    if (audio->thunder.time_since_lightning > 5.0f) {
      audio->thunder.time_since_lightning = 0.0f;
      audio->thunder.played_sound = false;

      // Set a random delay for thunder (sound delay from visual lightning)
      audio->thunder.thunder_delay = (f32)rand() / (f32)RAND_MAX * 3.0f;
    }

    // Play thunder sound after delay
    if (!audio->thunder.played_sound &&
        audio->thunder.time_since_lightning >= audio->thunder.thunder_delay) {
      // Simulate lightning at random position
      Vec3 thunder_pos =
          vec3((f32)rand() / (f32)RAND_MAX * 100.0f - 50.0f, 100.0f,
               (f32)rand() / (f32)RAND_MAX * 100.0f - 50.0f);

      f32 distance = 50.0f + (f32)rand() / (f32)RAND_MAX * 50.0f;
      weather_audio_play_thunder(audio, &thunder_pos, distance);
      audio->thunder.played_sound = true;
    }
  } else {
    weather_audio_stop_thunder(audio);
  }
}

void weather_audio_set_config(WeatherAudioSystem *audio,
                              const WeatherAudioConfig *config) {
  if (!audio || !config)
    return;
  audio->config = *config;
}

WeatherAudioConfig weather_audio_get_default_config(void) {
  return DEFAULT_CONFIG;
}
