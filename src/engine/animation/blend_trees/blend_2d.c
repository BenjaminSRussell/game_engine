#include "animation/blend_trees/blend_2d.h"
#include "animation/animation_system.h"
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define INITIAL_CAPACITY 8
#define MAX_TRIANGLES 128
#define BLEND_EPSILON 0.0001f

typedef struct {
    AnimationClip *clip;
    float x, y;
} BlendSample2DParam;

typedef struct {
    int indices[3];
} BlendTriangle;

struct BlendSpace2D {
    BlendSample2DParam *samples;
    int sample_count;
    int sample_capacity;

    BlendTriangle *triangles;
    int triangle_count;
    bool triangulation_dirty;
};

BlendSpace2D* blend_2d_create(void) {
    BlendSpace2D *blend = (BlendSpace2D*)malloc(sizeof(BlendSpace2D));
    if (blend) {
        blend->sample_count = 0;
        blend->sample_capacity = INITIAL_CAPACITY;
        blend->samples = (BlendSample2DParam*)malloc(sizeof(BlendSample2DParam) * INITIAL_CAPACITY);
        blend->triangles = (BlendTriangle*)malloc(sizeof(BlendTriangle) * MAX_TRIANGLES);
        blend->triangle_count = 0;
        blend->triangulation_dirty = false;

        if (!blend->samples || !blend->triangles) {
            if (blend->samples) free(blend->samples);
            if (blend->triangles) free(blend->triangles);
            free(blend);
            return NULL;
        }
    }
    return blend;
}

void blend_2d_destroy(BlendSpace2D* blend) {
    if (blend) {
        if (blend->samples) free(blend->samples);
        if (blend->triangles) free(blend->triangles);
        free(blend);
    }
}

void blend_2d_add_clip(BlendSpace2D *blend, AnimationClip *clip, float x, float y) {
    if (!blend || !clip) return;

    if (blend->sample_count >= blend->sample_capacity) {
        int new_capacity = blend->sample_capacity * 2;
        BlendSample2DParam *new_samples = (BlendSample2DParam*)realloc(blend->samples, sizeof(BlendSample2DParam) * new_capacity);
        if (!new_samples) return;
        blend->samples = new_samples;
        blend->sample_capacity = new_capacity;
    }

    blend->samples[blend->sample_count].clip = clip;
    blend->samples[blend->sample_count].x = x;
    blend->samples[blend->sample_count].y = y;
    blend->sample_count++;
    blend->triangulation_dirty = true;
}

static void retriangulate(BlendSpace2D *blend) {
    blend->triangle_count = 0;
    int n = blend->sample_count;
    if (n < 3) {
        blend->triangulation_dirty = false;
        return;
    }

    for (int i = 0; i < n - 2; i++) {
        for (int j = i + 1; j < n - 1; j++) {
            for (int k = j + 1; k < n; k++) {
                float x1 = blend->samples[i].x; float y1 = blend->samples[i].y;
                float x2 = blend->samples[j].x; float y2 = blend->samples[j].y;
                float x3 = blend->samples[k].x; float y3 = blend->samples[k].y;

                // Check collinearity
                float area = 0.5f * fabs(x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2));
                if (area < BLEND_EPSILON) continue;

                // Circumcircle
                float D = 2 * (x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2));
                if (fabs(D) < BLEND_EPSILON) continue; // Should be caught by area check, but safe guard

                float Ux = ((x1*x1 + y1*y1)*(y2-y3) + (x2*x2 + y2*y2)*(y3-y1) + (x3*x3 + y3*y3)*(y1-y2)) / D;
                float Uy = ((x1*x1 + y1*y1)*(x3-x2) + (x2*x2 + y2*y2)*(x1-x3) + (x3*x3 + y3*y3)*(x2-x1)) / D;
                float R_sq = (x1-Ux)*(x1-Ux) + (y1-Uy)*(y1-Uy);

                bool is_delaunay = true;
                for (int m = 0; m < n; m++) {
                    if (m == i || m == j || m == k) continue;
                    float dist_sq = (blend->samples[m].x - Ux)*(blend->samples[m].x - Ux) +
                                    (blend->samples[m].y - Uy)*(blend->samples[m].y - Uy);
                    if (dist_sq < R_sq - BLEND_EPSILON) {
                        is_delaunay = false;
                        break;
                    }
                }

                if (is_delaunay) {
                    if (blend->triangle_count < MAX_TRIANGLES) {
                        blend->triangles[blend->triangle_count].indices[0] = i;
                        blend->triangles[blend->triangle_count].indices[1] = j;
                        blend->triangles[blend->triangle_count].indices[2] = k;
                        blend->triangle_count++;
                    }
                }
            }
        }
    }
    blend->triangulation_dirty = false;
}

static void get_barycentric(float px, float py,
                            float x1, float y1, float x2, float y2, float x3, float y3,
                            float *u, float *v, float *w) {
    float detT = (y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3);
    if (fabs(detT) < BLEND_EPSILON) {
        *u = *v = *w = 0.0f;
        return;
    }
    *u = ((y2 - y3) * (px - x3) + (x3 - x2) * (py - y3)) / detT;
    *v = ((y3 - y1) * (px - x3) + (x1 - x3) * (py - y3)) / detT;
    *w = 1.0f - *u - *v;
}

void blend_2d_evaluate(BlendSpace2D *blend, float time, float param_x, float param_y, Pose *output_pose) {
    if (!blend || !output_pose || blend->sample_count == 0) return;

    if (blend->triangulation_dirty) {
        retriangulate(blend);
    }

    // 1. Single sample case
    if (blend->sample_count == 1) {
        float duration = blend->samples[0].clip->duration;
        float phase = duration > 0 ? fmodf(time, duration) / duration : 0.0f;
        animation_sample_clip(blend->samples[0].clip, phase * duration, output_pose);
        return;
    }

    // 2. Two samples (interpolate) - not handled by triangles, treat as nearest or average?
    // Fallback logic covers this if triangle_count is 0.

    // 3. Find triangle
    int best_tri = -1;
    float best_u = 0, best_v = 0, best_w = 0;

    for (int i = 0; i < blend->triangle_count; i++) {
        int idx1 = blend->triangles[i].indices[0];
        int idx2 = blend->triangles[i].indices[1];
        int idx3 = blend->triangles[i].indices[2];

        float u, v, w;
        get_barycentric(param_x, param_y,
            blend->samples[idx1].x, blend->samples[idx1].y,
            blend->samples[idx2].x, blend->samples[idx2].y,
            blend->samples[idx3].x, blend->samples[idx3].y,
            &u, &v, &w);

        // Check if inside triangle (allowing small epsilon for float errors)
        if (u >= -BLEND_EPSILON && v >= -BLEND_EPSILON && w >= -BLEND_EPSILON) {
            best_tri = i;
            best_u = u; best_v = v; best_w = w;
            // Clamp negative small values to 0
            if (best_u < 0) best_u = 0;
            if (best_v < 0) best_v = 0;
            if (best_w < 0) best_w = 0;
            // Renormalize
            float sum = best_u + best_v + best_w;
            if (sum > BLEND_EPSILON) {
                best_u /= sum; best_v /= sum; best_w /= sum;
            }
            break;
        }
    }

    if (best_tri != -1) {
        // Found triangle, blend 3 samples
        int idx1 = blend->triangles[best_tri].indices[0];
        int idx2 = blend->triangles[best_tri].indices[1];
        int idx3 = blend->triangles[best_tri].indices[2];

        float d1 = blend->samples[idx1].clip->duration;
        float d2 = blend->samples[idx2].clip->duration;
        float d3 = blend->samples[idx3].clip->duration;
        if (d1 < BLEND_EPSILON) d1 = 1.0f;
        if (d2 < BLEND_EPSILON) d2 = 1.0f;
        if (d3 < BLEND_EPSILON) d3 = 1.0f;

        float blend_duration = d1*best_u + d2*best_v + d3*best_w;
        float phase = fmodf(time, blend_duration) / blend_duration;

        Pose p1, p2, p3;
        animation_sample_clip(blend->samples[idx1].clip, phase * d1, &p1);
        animation_sample_clip(blend->samples[idx2].clip, phase * d2, &p2);
        animation_sample_clip(blend->samples[idx3].clip, phase * d3, &p3);

        Pose temp;
        // Blend p1 and p2 -> temp. Weight for p2 relative to (p1+p2) is v / (u+v) ? No.
        // Final = p1*u + p2*v + p3*w.
        // animation_blend_poses(a, b, w, out) -> out = a*(1-w) + b*w.
        // We want out = p1*u + p2*v + p3*w.

        // Step 1: Blend p1 and p2.
        // Let's say res1 = p1 * (u/(u+v)) + p2 * (v/(u+v)).
        // Then Final = res1 * (u+v) + p3 * w.

        if (best_w > 0.999f) {
            *output_pose = p3;
        } else {
            float uv_sum = best_u + best_v;
            if (uv_sum < BLEND_EPSILON) {
                *output_pose = p3; // Should match w~1
            } else {
                float w2 = best_v / uv_sum;
                animation_blend_poses(&p1, &p2, w2, &temp);
                animation_blend_poses(&temp, &p3, best_w, output_pose);
            }
        }
    } else {
        // Fallback: Closest sample
        int best_idx = 0;
        float min_dist_sq = 1e9f;
        for (int i = 0; i < blend->sample_count; i++) {
            float dx = blend->samples[i].x - param_x;
            float dy = blend->samples[i].y - param_y;
            float dist_sq = dx*dx + dy*dy;
            if (dist_sq < min_dist_sq) {
                min_dist_sq = dist_sq;
                best_idx = i;
            }
        }

        float duration = blend->samples[best_idx].clip->duration;
        float phase = duration > 0 ? fmodf(time, duration) / duration : 0.0f;
        animation_sample_clip(blend->samples[best_idx].clip, phase * duration, output_pose);
    }
}
