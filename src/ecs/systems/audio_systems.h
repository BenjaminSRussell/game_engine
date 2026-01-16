#ifndef AUDIO_SYSTEMS_H
#define AUDIO_SYSTEMS_H

#include "ecs/ecs.h"

/**
 * ECS Audio Systems
 *
 * - SpatialAudioSystem: Updates volume/pan based on distance to listener
 * - AudioSubmissionSystem: Generates commands for audio thread
 */

/**
 * Register all audio systems with ECS world
 */
void register_audio_systems(World *world);

/**
 * Spatial Audio System (Priority 80)
 * Calculates 3D audio parameters (distance attenuation, panning)
 */
void spatial_audio_system(SystemContext *ctx);

/**
 * Audio Submission System (Priority 90)
 * Submits play/stop/update commands to Audio Allocator/Thread
 */
void audio_submission_system(SystemContext *ctx);

#endif // AUDIO_SYSTEMS_H
