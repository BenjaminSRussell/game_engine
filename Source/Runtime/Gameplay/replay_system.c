// replay_system.c - Implementation
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define REPLAY_VERSION 1
#define REPLAY_MAGIC "VFGRPLY"
#define MAX_REPLAY_FRAMES 100000
#define MAX_REPLAY_EVENTS 50000
#define REPLAY_BUFFER_SIZE (16 * 1024 * 1024) // 16MB

typedef enum {
  REPLAY_EVENT_INPUT = 0,
  REPLAY_EVENT_ENTITY_SPAWN,
  REPLAY_EVENT_ENTITY_DESTROY,
  REPLAY_EVENT_POSITION_UPDATE,
  REPLAY_EVENT_ROTATION_UPDATE,
  REPLAY_EVENT_STATE_CHANGE,
  REPLAY_EVENT_CUSTOM
} ReplayEventType;

typedef struct {
  u32 frame_number;
  ReplayEventType type;
  u32 entity_id;
  u32 data_size;
  u8 data[256];
} ReplayEvent;

typedef struct {
  u32 frame_number;
  f32 timestamp;
  u32 event_count;
  u32 event_start_index;
} ReplayFrame;

typedef struct {
  char magic[8];
  u32 version;
  u32 total_frames;
  u32 total_events;
  u32 recording_duration_ms;
  u32 checksum;
  char map_name[64];
  u32 player_count;
  u32 game_mode;
} ReplayHeader;

typedef struct {
  ReplayHeader header;
  ReplayFrame frames[MAX_REPLAY_FRAMES];
  ReplayEvent events[MAX_REPLAY_EVENTS];
  u32 frame_count;
  u32 event_count;
  bool is_recording;
  bool is_playing;
  u32 current_frame;
  f32 playback_time;
  FILE *record_file;
} ReplaySession;

static ReplaySession g_current_replay = {0};

bool replay_init(void) {
  memset(&g_current_replay, 0, sizeof(g_current_replay));
  
  LOG_INFO("Replay system initialized");
  return true;
}

void replay_shutdown(void) {
  if (g_current_replay.is_recording) {
    replay_stop_recording();
  }
  if (g_current_replay.is_playing) {
    replay_stop_playback();
  }
  
  memset(&g_current_replay, 0, sizeof(g_current_replay));
  LOG_INFO("Replay system shutdown");
}

bool replay_start_recording(const char *map_name, u32 game_mode) {
  if (g_current_replay.is_recording || g_current_replay.is_playing) {
    LOG_ERROR("Replay system already active");
    return false;
  }
  
  memset(&g_current_replay, 0, sizeof(g_current_replay));
  
  // Setup header
  memcpy(g_current_replay.header.magic, REPLAY_MAGIC, 8);
  g_current_replay.header.version = REPLAY_VERSION;
  g_current_replay.header.total_frames = 0;
  g_current_replay.header.total_events = 0;
  g_current_replay.header.recording_duration_ms = 0;
  g_current_replay.header.game_mode = game_mode;
  
  if (map_name) {
    strncpy(g_current_replay.header.map_name, map_name, 63);
    g_current_replay.header.map_name[63] = '\0';
  }
  
  g_current_replay.is_recording = true;
  g_current_replay.current_frame = 0;
  g_current_replay.playback_time = 0.0f;
  
  LOG_INFO("Started replay recording: %s", map_name ? map_name : "unknown");
  return true;
}

void replay_stop_recording(void) {
  if (!g_current_replay.is_recording) {
    return;
  }
  
  // Calculate checksum
  g_current_replay.header.checksum = 0;
  for (u32 i = 0; i < g_current_replay.event_count; i++) {
    u8 *data = (u8*)&g_current_replay.events[i];
    for (size_t j = 0; j < sizeof(ReplayEvent); j++) {
      g_current_replay.header.checksum = ((g_current_replay.header.checksum << 1) | 
                                         (g_current_replay.header.checksum >> 31)) ^ data[j];
    }
  }
  
  g_current_replay.header.total_frames = g_current_replay.frame_count;
  g_current_replay.header.total_events = g_current_replay.event_count;
  g_current_replay.is_recording = false;
  
  LOG_INFO("Stopped replay recording: %d frames, %d events", 
           g_current_replay.frame_count, g_current_replay.event_count);
}

bool replay_start_playback(void) {
  if (g_current_replay.is_recording || g_current_replay.is_playing) {
    LOG_ERROR("Replay system already active");
    return false;
  }
  
  if (g_current_replay.frame_count == 0) {
    LOG_ERROR("No replay data to play");
    return false;
  }
  
  g_current_replay.is_playing = true;
  g_current_replay.current_frame = 0;
  g_current_replay.playback_time = 0.0f;
  
  LOG_INFO("Started replay playback: %d frames", g_current_replay.frame_count);
  return true;
}

void replay_stop_playback(void) {
  if (!g_current_replay.is_playing) {
    return;
  }
  
  g_current_replay.is_playing = false;
  g_current_replay.current_frame = 0;
  g_current_replay.playback_time = 0.0f;
  
  LOG_INFO("Stopped replay playback");
}

bool replay_is_recording(void) {
  return g_current_replay.is_recording;
}

bool replay_is_playing(void) {
  return g_current_replay.is_playing;
}

void replay_add_frame(f32 timestamp) {
  if (!g_current_replay.is_recording || g_current_replay.frame_count >= MAX_REPLAY_FRAMES) {
    return;
  }
  
  ReplayFrame *frame = &g_current_replay.frames[g_current_replay.frame_count];
  frame->frame_number = g_current_replay.frame_count;
  frame->timestamp = timestamp;
  frame->event_count = 0;
  frame->event_start_index = g_current_replay.event_count;
  
  g_current_replay.frame_count++;
  g_current_replay.current_frame = g_current_replay.frame_count;
}

void replay_add_event(ReplayEventType type, u32 entity_id, const void *data, u32 data_size) {
  if (!g_current_replay.is_recording || g_current_replay.event_count >= MAX_REPLAY_EVENTS) {
    return;
  }
  
  if (data_size > sizeof(((ReplayEvent*)0)->data)) {
    LOG_ERROR("Replay event data too large: %d bytes", data_size);
    return;
  }
  
  ReplayEvent *event = &g_current_replay.events[g_current_replay.event_count];
  event->frame_number = g_current_replay.frame_count;
  event->type = type;
  event->entity_id = entity_id;
  event->data_size = data_size;
  
  if (data && data_size > 0) {
    memcpy(event->data, data, data_size);
  }
  
  g_current_replay.event_count++;
  
  // Update current frame's event count
  if (g_current_replay.frame_count > 0) {
    ReplayFrame *frame = &g_current_replay.frames[g_current_replay.frame_count - 1];
    frame->event_count++;
  }
}

void replay_add_input_event(u32 entity_id, u32 input_type, u32 input_value) {
  struct {
    u32 type;
    u32 value;
  } input_data = { input_type, input_value };
  
  replay_add_event(REPLAY_EVENT_INPUT, entity_id, &input_data, sizeof(input_data));
}

void replay_add_position_event(u32 entity_id, f32 x, f32 y, f32 z) {
  struct {
    f32 x, y, z;
  } pos_data = { x, y, z };
  
  replay_add_event(REPLAY_EVENT_POSITION_UPDATE, entity_id, &pos_data, sizeof(pos_data));
}

void replay_add_rotation_event(u32 entity_id, f32 pitch, f32 yaw, f32 roll) {
  struct {
    f32 pitch, yaw, roll;
  } rot_data = { pitch, yaw, roll };
  
  replay_add_event(REPLAY_EVENT_ROTATION_UPDATE, entity_id, &rot_data, sizeof(rot_data));
}

void replay_update(f32 delta_time) {
  if (!g_current_replay.is_playing) {
    return;
  }
  
  g_current_replay.playback_time += delta_time;
  
  // Process frames up to current time
  while (g_current_replay.current_frame < g_current_replay.frame_count) {
    ReplayFrame *frame = &g_current_replay.frames[g_current_replay.current_frame];
    
    if (frame->timestamp > g_current_replay.playback_time) {
      break;
    }
    
    // Process events for this frame
    for (u32 i = 0; i < frame->event_count; i++) {
      u32 event_index = frame->event_start_index + i;
      if (event_index >= g_current_replay.event_count) break;
      
      ReplayEvent *event = &g_current_replay.events[event_index];
      replay_process_event(event);
    }
    
    g_current_replay.current_frame++;
  }
  
  // Check if playback is complete
  if (g_current_replay.current_frame >= g_current_replay.frame_count) {
    replay_stop_playback();
  }
}

void replay_process_event(ReplayEvent *event) {
  if (!event) return;
  
  switch (event->type) {
    case REPLAY_EVENT_INPUT: {
      struct {
        u32 type;
        u32 value;
      } *input_data = (void*)event->data;
      
      // Apply input to entity
      LOG_DEBUG("Replay input: entity %d, type %d, value %d", 
                event->entity_id, input_data->type, input_data->value);
      break;
    }
    
    case REPLAY_EVENT_POSITION_UPDATE: {
      struct {
        f32 x, y, z;
      } *pos_data = (void*)event->data;
      
      // Update entity position
      LOG_DEBUG("Replay position: entity %d, (%.2f, %.2f, %.2f)", 
                event->entity_id, pos_data->x, pos_data->y, pos_data->z);
      break;
    }
    
    case REPLAY_EVENT_ROTATION_UPDATE: {
      struct {
        f32 pitch, yaw, roll;
      } *rot_data = (void*)event->data;
      
      // Update entity rotation
      LOG_DEBUG("Replay rotation: entity %d, (%.2f, %.2f, %.2f)", 
                event->entity_id, rot_data->pitch, rot_data->yaw, rot_data->roll);
      break;
    }
    
    case REPLAY_EVENT_ENTITY_SPAWN:
      LOG_DEBUG("Replay entity spawn: %d", event->entity_id);
      break;
      
    case REPLAY_EVENT_ENTITY_DESTROY:
      LOG_DEBUG("Replay entity destroy: %d", event->entity_id);
      break;
      
    case REPLAY_EVENT_STATE_CHANGE:
      LOG_DEBUG("Replay state change: entity %d", event->entity_id);
      break;
      
    case REPLAY_EVENT_CUSTOM:
      LOG_DEBUG("Replay custom event: entity %d", event->entity_id);
      break;
      
    default:
      LOG_WARN("Unknown replay event type: %d", event->type);
      break;
  }
}

bool replay_save(const char *filename) {
  if (!filename || g_current_replay.is_recording) {
    LOG_ERROR("Cannot save replay while recording");
    return false;
  }
  
  FILE *file = fopen(filename, "wb");
  if (!file) {
    LOG_ERROR("Failed to open replay file: %s", filename);
    return false;
  }
  
  bool success = false;
  
  // Write header
  if (fwrite(&g_current_replay.header, sizeof(ReplayHeader), 1, file) == 1) {
    // Write frames
    if (fwrite(g_current_replay.frames, sizeof(ReplayFrame), 
               g_current_replay.frame_count, file) == g_current_replay.frame_count) {
      // Write events
      if (fwrite(g_current_replay.events, sizeof(ReplayEvent), 
                 g_current_replay.event_count, file) == g_current_replay.event_count) {
        success = true;
        LOG_INFO("Replay saved to: %s (%d frames, %d events)", 
                 filename, g_current_replay.frame_count, g_current_replay.event_count);
      } else {
        LOG_ERROR("Failed to write replay events");
      }
    } else {
      LOG_ERROR("Failed to write replay frames");
    }
  } else {
    LOG_ERROR("Failed to write replay header");
  }
  
  fclose(file);
  return success;
}

bool replay_load(const char *filename) {
  if (!filename) {
    return false;
  }
  
  if (g_current_replay.is_recording || g_current_replay.is_playing) {
    LOG_ERROR("Cannot load replay while system is active");
    return false;
  }
  
  FILE *file = fopen(filename, "rb");
  if (!file) {
    LOG_ERROR("Failed to open replay file: %s", filename);
    return false;
  }
  
  bool success = false;
  ReplayHeader header;
  
  // Read header
  if (fread(&header, sizeof(ReplayHeader), 1, file) == 1) {
    // Validate header
    if (memcmp(header.magic, REPLAY_MAGIC, 8) == 0 && 
        header.version == REPLAY_VERSION &&
        header.total_frames <= MAX_REPLAY_FRAMES &&
        header.total_events <= MAX_REPLAY_EVENTS) {
      
      memset(&g_current_replay, 0, sizeof(g_current_replay));
      g_current_replay.header = header;
      
      // Read frames
      if (fread(g_current_replay.frames, sizeof(ReplayFrame), 
                 header.total_frames, file) == header.total_frames) {
        // Read events
        if (fread(g_current_replay.events, sizeof(ReplayEvent), 
                   header.total_events, file) == header.total_events) {
          g_current_replay.frame_count = header.total_frames;
          g_current_replay.event_count = header.total_events;
          success = true;
          
          LOG_INFO("Replay loaded from: %s (%d frames, %d events)", 
                   filename, header.total_frames, header.total_events);
        } else {
          LOG_ERROR("Failed to read replay events");
        }
      } else {
        LOG_ERROR("Failed to read replay frames");
      }
    } else {
      LOG_ERROR("Invalid replay file format");
    }
  } else {
    LOG_ERROR("Failed to read replay header");
  }
  
  fclose(file);
  return success;
}

void replay_get_info(char *out_info, u32 max_length) {
  if (!out_info || max_length == 0) return;
  
  if (g_current_replay.frame_count == 0) {
    snprintf(out_info, max_length, "No replay data");
    return;
  }
  
  f32 duration = 0.0f;
  if (g_current_replay.frame_count > 0) {
    ReplayFrame *last_frame = &g_current_replay.frames[g_current_replay.frame_count - 1];
    duration = last_frame->timestamp;
  }
  
  snprintf(out_info, max_length, 
           "Replay: %s, %d frames, %d events, %.2fs duration, Mode: %d",
           g_current_replay.header.map_name,
           g_current_replay.frame_count,
           g_current_replay.event_count,
           duration,
           g_current_replay.header.game_mode);
}

u32 replay_get_frame_count(void) {
  return g_current_replay.frame_count;
}

u32 replay_get_event_count(void) {
  return g_current_replay.event_count;
}

f32 replay_get_duration(void) {
  if (g_current_replay.frame_count == 0) return 0.0f;
  
  ReplayFrame *last_frame = &g_current_replay.frames[g_current_replay.frame_count - 1];
  return last_frame->timestamp;
}

void replay_seek_to_frame(u32 frame_number) {
  if (!g_current_replay.is_playing || frame_number >= g_current_replay.frame_count) {
    return;
  }
  
  g_current_replay.current_frame = frame_number;
  
  // Update playback time
  if (frame_number < g_current_replay.frame_count) {
    g_current_replay.playback_time = g_current_replay.frames[frame_number].timestamp;
  }
}

void replay_seek_to_time(f32 timestamp) {
  if (!g_current_replay.is_playing) return;
  
  for (u32 i = 0; i < g_current_replay.frame_count; i++) {
    if (g_current_replay.frames[i].timestamp >= timestamp) {
      replay_seek_to_frame(i);
      break;
    }
  }
}
