#ifndef PARALLEL_AUDIO_H
#define PARALLEL_AUDIO_H

#include "core/types.h"
#include "engine/include/math/math_all.h"

/**
 * Parallel Audio Processing
 *
 * Uses worker threads for:
 * - Ray-cast occlusion calculations
 * - Audio decoding/streaming
 * - Heavy DSP effects
 */

/**
 * Initialize parallel audio
 */
void parallel_audio_init(u32 thread_count);

/**
 * Parallel Occlusion Calculation
 * Traces rays from listener to sources in parallel
 *
 * @param listener_pos Listener position
 * @param sources      Array of active source positions
 * @param count        Number of sources
 * @param results      Output occlusion factors (0.0 - 1.0)
 */
void parallel_audio_occlusion(Vec3 listener_pos, Vec3 *sources, u32 count,
                              f32 *results);

/**
 * Parallel Audio Mixing (Hook)
 * Mixes voices into output buffer in parallel chunks
 */
void parallel_audio_mix(void *buffer, u32 frames);

#endif // PARALLEL_AUDIO_H
