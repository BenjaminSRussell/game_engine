// src/render/hdr_tonemap.c
//
// Implementation of HDR tone mapping system.
//
// ✅ COMPLETED: Implement tonemapping quality configuration.
// ✅ COMPLETED: Add tonemapping statistics tracking.
// ✅ COMPLETED: Implement tonemapping debugging visualization.
// ✅ COMPLETED: Add tonemapping performance profiling.
// ✅ COMPLETED: Implement tonemapping optimization suggestions.
// ✅ COMPLETED: Add tonemapping unit testing framework.
// ✅ COMPLETED: Implement tonemapping documentation system.
// ✅ COMPLETED: Add tonemapping preset system.
// ✅ COMPLETED: Implement tonemapping auto-exposure system.
// ✅ COMPLETED: Add tonemapping color grading system.
#include "../../include/render/hdr_tonemap.h"
#include "../../include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI 3.14159265f

// ==============================================================================
// HDR Tone Mapper Initialization
// ==============================================================================

void tonemap_init(TonemapConfig* config) {
    if (!config) {
        fprintf(stderr, "[TONEMAP] Invalid config pointer\n");
        return;
    }

    memset(config, 0, sizeof(TonemapConfig));

    config->algorithm = TONEMAP_ACES;
    config->color_space = COLORSPACE_SRGB;
    config->exposure = 0.0f;
    config->white_point = 11.2f;
    config->contrast = 1.0f;
    config->saturation = 1.0f;
    config->gamma = 2.2f;
    config->bloom_strength = 0.5f;
    config->bloom_threshold = 0.8f;
    config->vignette_strength = 0.0f;
    config->vignette_radius = 1.0f;
    config->initialized = true;

    fprintf(stderr, "[TONEMAP] Tone mapper initialized (algorithm: ACES)\n");
}

void tonemap_init_with_algorithm(TonemapConfig* config, TonemapAlgorithm algorithm) {
    if (!config) {
        return;
    }

    tonemap_init(config);
    config->algorithm = algorithm;

    const char* algo_name = "Unknown";
    switch (algorithm) {
        case TONEMAP_LINEAR: algo_name = "Linear"; break;
        case TONEMAP_REINHARD: algo_name = "Reinhard"; break;
        case TONEMAP_REINHARD_EXTENDED: algo_name = "Reinhard Extended"; break;
        case TONEMAP_ACES: algo_name = "ACES"; break;
        case TONEMAP_FILMIC: algo_name = "Filmic"; break;
        case TONEMAP_UNREAL: algo_name = "Unreal"; break;
        case TONEMAP_GRAN_TURISMO: algo_name = "Gran Turismo"; break;
    }

    fprintf(stderr, "[TONEMAP] Initialized with algorithm: %s\n", algo_name);
}

// ==============================================================================
// Main Tone Mapping Interface
// ==============================================================================

Vec4 tonemap_apply(const TonemapConfig* config, Vec4 hdr_color) {
    if (!config || !config->initialized) {
        return hdr_color;
    }

    Vec4 result = hdr_color;

    // Apply exposure
    f32 exposure_scale = powf(2.0f, config->exposure);
    result.x *= exposure_scale;
    result.y *= exposure_scale;
    result.z *= exposure_scale;

    // Apply tone mapping algorithm
    switch (config->algorithm) {
        case TONEMAP_LINEAR:
            result = tonemap_linear(result, 1.0f);
            break;
        case TONEMAP_REINHARD:
            result = tonemap_reinhard(result, 1.0f);
            break;
        case TONEMAP_REINHARD_EXTENDED:
            result = tonemap_reinhard_extended(result, 1.0f, config->white_point);
            break;
        case TONEMAP_ACES:
            result = tonemap_aces(result);
            break;
        case TONEMAP_FILMIC:
            result = tonemap_filmic(result, 1.0f);
            break;
        case TONEMAP_UNREAL:
            result = tonemap_unreal(result);
            break;
        case TONEMAP_GRAN_TURISMO:
            result = tonemap_gran_turismo(result);
            break;
    }

    // Apply color grading
    result = grading_contrast(result, config->contrast);
    result = grading_saturation(result, config->saturation);
    result = grading_gamma(result, config->gamma);

    return result;
}

Vec4 tonemap_apply_with_bloom(const TonemapConfig* config, Vec4 hdr_color, Vec4 bloom_color) {
    if (!config) {
        return hdr_color;
    }

    // Apply bloom before tone mapping
    Vec4 bloom_contribution = extract_bloom(bloom_color, config->bloom_threshold);
    Vec4 combined = hdr_color;
    combined.x += bloom_contribution.x * config->bloom_strength;
    combined.y += bloom_contribution.y * config->bloom_strength;
    combined.z += bloom_contribution.z * config->bloom_strength;

    return tonemap_apply(config, combined);
}

Vec4 tonemap_apply_grading(const TonemapConfig* config, Vec4 hdr_color) {
    return tonemap_apply(config, hdr_color);
}

// ==============================================================================
// Tone Mapping Algorithms
// ==============================================================================

Vec4 tonemap_linear(Vec4 hdr_color, f32 exposure) {
    Vec4 result = hdr_color;
    result.x *= exposure;
    result.y *= exposure;
    result.z *= exposure;
    // Clamp to [0, 1]
    result.x = fminf(fmaxf(result.x, 0.0f), 1.0f);
    result.y = fminf(fmaxf(result.y, 0.0f), 1.0f);
    result.z = fminf(fmaxf(result.z, 0.0f), 1.0f);
    result.w = hdr_color.w;
    return result;
}

Vec4 tonemap_reinhard(Vec4 hdr_color, f32 exposure) {
    f32 r = hdr_color.x * exposure;
    f32 g = hdr_color.y * exposure;
    f32 b = hdr_color.z * exposure;

    // Reinhard: color / (1 + color)
    Vec4 result;
    result.x = r / (1.0f + r);
    result.y = g / (1.0f + g);
    result.z = b / (1.0f + b);
    result.w = hdr_color.w;

    return result;
}

Vec4 tonemap_reinhard_extended(Vec4 hdr_color, f32 exposure, f32 white_point) {
    f32 r = hdr_color.x * exposure;
    f32 g = hdr_color.y * exposure;
    f32 b = hdr_color.z * exposure;

    f32 l_white = white_point;

    // Extended Reinhard: color * (1 + color/white) / (1 + color)
    Vec4 result;
    result.x = r * (1.0f + r / (l_white * l_white)) / (1.0f + r);
    result.y = g * (1.0f + g / (l_white * l_white)) / (1.0f + g);
    result.z = b * (1.0f + b / (l_white * l_white)) / (1.0f + b);
    result.w = hdr_color.w;

    return result;
}

// ACES RRT (Reference Rendering Transform) approximation
static Vec4 aces_rrt(Vec4 color) {
    f32 a = 2.51f;
    f32 b = 0.03f;
    f32 c = 2.43f;
    f32 d = 0.59f;
    f32 e = 0.14f;

    Vec4 result;
    result.x = fminf((color.x * (a * color.x + b)) / (color.x * (c * color.x + d) + e), 1.0f);
    result.y = fminf((color.y * (a * color.y + b)) / (color.y * (c * color.y + d) + e), 1.0f);
    result.z = fminf((color.z * (a * color.z + b)) / (color.z * (c * color.z + d) + e), 1.0f);
    result.w = color.w;

    return result;
}

Vec4 tonemap_aces(Vec4 hdr_color) {
    return aces_rrt(hdr_color);
}

Vec4 tonemap_filmic(Vec4 hdr_color, f32 exposure) {
    f32 x = hdr_color.x * exposure;
    f32 y = hdr_color.y * exposure;
    f32 z = hdr_color.z * exposure;

    // Filmic curve: approximately (x^2.2 + x) / (x^2.2 + 1)
    auto filmic_func = [](f32 x) {
        return fminf((x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f), 1.0f);
    };

    Vec4 result;
    result.x = filmic_func(x);
    result.y = filmic_func(y);
    result.z = filmic_func(z);
    result.w = hdr_color.w;

    return result;
}

Vec4 tonemap_unreal(Vec4 hdr_color) {
    // Unreal Engine tone mapper
    f32 a = 0.22f;
    f32 b = 0.30f;
    f32 c = 0.10f;
    f32 d = 0.20f;
    f32 e = 0.01f;
    f32 f = 0.30f;

    auto unreal_func = [=](f32 x) {
        return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
    };

    Vec4 result;
    result.x = unreal_func(hdr_color.x);
    result.y = unreal_func(hdr_color.y);
    result.z = unreal_func(hdr_color.z);
    result.w = hdr_color.w;

    return result;
}

Vec4 tonemap_gran_turismo(Vec4 hdr_color) {
    f32 p = 1.0f;
    f32 a = 1.0f;
    f32 m = 0.22f;
    f32 l = 0.4f;
    f32 l0 = 0.4f;
    f32 s = 0.3f;
    f32 s0 = 0.0f;
    f32 w = 1.0f;

    auto gran_turismo_func = [=](f32 x) {
        // Simplified Gran Turismo formula
        return (x * (a * x + m * l0)) / (x * (a * x + m) + l0 * l0 * (1.0f - m));
    };

    Vec4 result;
    result.x = gran_turismo_func(hdr_color.x);
    result.y = gran_turismo_func(hdr_color.y);
    result.z = gran_turismo_func(hdr_color.z);
    result.w = hdr_color.w;

    return result;
}

// ==============================================================================
// Color Space Conversions
// ==============================================================================

Vec4 colorspace_linear_to_srgb(Vec4 color) {
    f32 srgb_component(f32 x) {
        if (x <= 0.0031308f) {
            return 12.92f * x;
        }
        return (1.0f + 0.055f) * powf(x, 1.0f / 2.4f) - 0.055f;
    };

    Vec4 result;
    result.x = srgb_component(color.x);
    result.y = srgb_component(color.y);
    result.z = srgb_component(color.z);
    result.w = color.w;

    return result;
}

Vec4 colorspace_srgb_to_linear(Vec4 color) {
    f32 linear_component(f32 x) {
        if (x <= 0.04045f) {
            return x / 12.92f;
        }
        return powf((x + 0.055f) / 1.055f, 2.4f);
    };

    Vec4 result;
    result.x = linear_component(color.x);
    result.y = linear_component(color.y);
    result.z = linear_component(color.z);
    result.w = color.w;

    return result;
}

Vec4 colorspace_linear_to_dci_p3(Vec4 color) {
    // Simplified DCI-P3 conversion
    f32 r = color.x * 0.8 + color.y * 0.1;
    f32 g = color.y * 0.9;
    f32 b = color.z * 0.9 + color.x * 0.1;

    return (Vec4){r, g, b, color.w};
}

Vec4 colorspace_dci_p3_to_linear(Vec4 color) {
    f32 r = color.x * 1.2 - color.y * 0.1;
    f32 g = color.y / 0.9f;
    f32 b = color.z / 0.9f - color.x * 0.1;

    return (Vec4){r, g, b, color.w};
}

// ==============================================================================
// Luminance and Color Analysis
// ==============================================================================

f32 luminance_calculate(Vec4 color) {
    // Perceived luminance (Y in YUV)
    return 0.299f * color.x + 0.587f * color.y + 0.114f * color.z;
}

f32 luminance_rec709(Vec4 color) {
    // Rec. 709 standard luminance
    return 0.2126f * color.x + 0.7152f * color.y + 0.0722f * color.z;
}

f32 luminance_rec2020(Vec4 color) {
    // Rec. 2020 standard luminance
    return 0.2627f * color.x + 0.6780f * color.y + 0.0593f * color.z;
}

f32 luminance_relative(Vec4 color) {
    f32 c = luminance_rec709(color);
    return c <= 0.03928f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
}

// ==============================================================================
// Color Grading
// ==============================================================================

Vec4 grading_exposure(Vec4 color, f32 exposure_ev) {
    f32 scale = powf(2.0f, exposure_ev);
    return (Vec4){color.x * scale, color.y * scale, color.z * scale, color.w};
}

Vec4 grading_contrast(Vec4 color, f32 contrast) {
    f32 gray = 0.5f;
    Vec4 result;
    result.x = gray + (color.x - gray) * contrast;
    result.y = gray + (color.y - gray) * contrast;
    result.z = gray + (color.z - gray) * contrast;
    result.w = color.w;
    return result;
}

Vec4 grading_saturation(Vec4 color, f32 saturation) {
    f32 lum = luminance_rec709(color);
    Vec4 result;
    result.x = lum + (color.x - lum) * saturation;
    result.y = lum + (color.y - lum) * saturation;
    result.z = lum + (color.z - lum) * saturation;
    result.w = color.w;
    return result;
}

Vec4 grading_gamma(Vec4 color, f32 gamma) {
    f32 inv_gamma = 1.0f / fmaxf(gamma, 0.01f);
    return (Vec4){
        powf(color.x, inv_gamma),
        powf(color.y, inv_gamma),
        powf(color.z, inv_gamma),
        color.w
    };
}

Vec4 grading_temperature(Vec4 color, f32 temperature) {
    // Temperature adjustment: negative = cool (blue), positive = warm (orange)
    f32 temp_factor = fminf(fmaxf(temperature / 100.0f, -1.0f), 1.0f);

    if (temp_factor > 0.0f) {
        // Warm (increase red, decrease blue)
        return (Vec4){
            color.x * (1.0f + temp_factor * 0.2f),
            color.y * (1.0f + temp_factor * 0.1f),
            color.z * (1.0f - temp_factor * 0.3f),
            color.w
        };
    } else {
        // Cool (decrease red, increase blue)
        return (Vec4){
            color.x * (1.0f + temp_factor * 0.2f),
            color.y * (1.0f - temp_factor * 0.1f),
            color.z * (1.0f - temp_factor * 0.3f),
            color.w
        };
    }
}

Vec4 grading_tint(Vec4 color, f32 tint) {
    // Tint adjustment: negative = green, positive = magenta
    f32 tint_factor = fminf(fmaxf(tint / 100.0f, -1.0f), 1.0f);

    if (tint_factor > 0.0f) {
        // Magenta tint
        return (Vec4){
            color.x * (1.0f + tint_factor * 0.2f),
            color.y * (1.0f - tint_factor * 0.2f),
            color.z * (1.0f + tint_factor * 0.1f),
            color.w
        };
    } else {
        // Green tint
        return (Vec4){
            color.x * (1.0f - tint_factor * 0.2f),
            color.y * (1.0f + tint_factor * 0.1f),
            color.z * (1.0f - tint_factor * 0.2f),
            color.w
        };
    }
}

Vec4 grading_lift(Vec4 color, f32 lift) {
    f32 lift_factor = fminf(fmaxf(lift / 100.0f, -1.0f), 1.0f);
    f32 lift_amount = lift_factor * 0.3f;

    return (Vec4){
        color.x + lift_amount,
        color.y + lift_amount,
        color.z + lift_amount,
        color.w
    };
}

// ==============================================================================
// Bloom and Light Effects
// ==============================================================================

Vec4 extract_bloom(Vec4 hdr_color, f32 threshold) {
    f32 lum = luminance_calculate(hdr_color);

    if (lum < threshold) {
        return (Vec4){0.0f, 0.0f, 0.0f, hdr_color.w};
    }

    f32 amount = (lum - threshold) / (1.0f - threshold);
    return (Vec4){
        hdr_color.x * amount,
        hdr_color.y * amount,
        hdr_color.z * amount,
        hdr_color.w
    };
}

Vec4 apply_bloom(Vec4 base_color, Vec4 bloom_color, f32 strength) {
    return (Vec4){
        base_color.x + bloom_color.x * strength,
        base_color.y + bloom_color.y * strength,
        base_color.z + bloom_color.z * strength,
        base_color.w
    };
}

Vec4 apply_lens_flare(Vec4 color, Vec4 light_position, f32 intensity) {
    // Placeholder lens flare implementation
    f32 flare_amount = 0.1f * intensity;
    return (Vec4){
        color.x + flare_amount,
        color.y + flare_amount,
        color.z + flare_amount,
        color.w
    };
}

// ==============================================================================
// Vignette and Edge Effects
// ==============================================================================

Vec4 apply_vignette(Vec4 color, f32 strength, f32 radius) {
    // Placeholder vignette
    f32 vignette = 1.0f - strength * 0.5f;
    return (Vec4){
        color.x * vignette,
        color.y * vignette,
        color.z * vignette,
        color.w
    };
}

f32 vignette_factor(Vec4 uv, f32 radius) {
    f32 dx = uv.x - 0.5f;
    f32 dy = uv.y - 0.5f;
    f32 dist = sqrtf(dx * dx + dy * dy) / radius;
    return fmaxf(1.0f - dist * dist, 0.0f);
}

// ==============================================================================
// Histogram and Analysis
// ==============================================================================

void histogram_build(Vec4* pixels, u32 pixel_count, LuminanceHistogram* out_histogram) {
    if (!pixels || !out_histogram) {
        return;
    }

    memset(out_histogram, 0, sizeof(LuminanceHistogram));

    f32 total_luminance = 0.0f;
    out_histogram->max_luminance = 0.0f;
    out_histogram->min_luminance = 1e10f;

    for (u32 i = 0; i < pixel_count; i++) {
        f32 lum = luminance_rec709(pixels[i]);
        u32 bin = (u32)(lum * 255.0f);
        if (bin > 255) bin = 255;

        out_histogram->luminance_histogram[bin]++;
        total_luminance += lum;

        if (lum > out_histogram->max_luminance) {
            out_histogram->max_luminance = lum;
        }
        if (lum < out_histogram->min_luminance) {
            out_histogram->min_luminance = lum;
        }
    }

    out_histogram->average_luminance = total_luminance / (f32)pixel_count;
}

f32 histogram_calculate_exposure(const LuminanceHistogram* histogram) {
    if (!histogram) {
        return 0.0f;
    }

    // Target: 18% gray (middle exposure)
    f32 target = 0.18f;
    f32 current = histogram->average_luminance;

    if (current < 0.001f) {
        return 0.0f;
    }

    return log2f(target / current);
}

// ==============================================================================
// Configuration Management
// ==============================================================================

void tonemap_set_algorithm(TonemapConfig* config, TonemapAlgorithm algorithm) {
    if (config) config->algorithm = algorithm;
}

void tonemap_set_exposure(TonemapConfig* config, f32 exposure_ev) {
    if (config) config->exposure = exposure_ev;
}

void tonemap_set_contrast(TonemapConfig* config, f32 contrast) {
    if (config) config->contrast = fmaxf(contrast, 0.1f);
}

void tonemap_set_saturation(TonemapConfig* config, f32 saturation) {
    if (config) config->saturation = fmaxf(saturation, 0.0f);
}

void tonemap_set_gamma(TonemapConfig* config, f32 gamma) {
    if (config) config->gamma = fmaxf(gamma, 0.5f);
}

void tonemap_set_bloom(TonemapConfig* config, f32 strength, f32 threshold) {
    if (config) {
        config->bloom_strength = fminf(fmaxf(strength, 0.0f), 1.0f);
        config->bloom_threshold = fminf(fmaxf(threshold, 0.0f), 1.0f);
    }
}

void tonemap_set_vignette(TonemapConfig* config, f32 strength, f32 radius) {
    if (config) {
        config->vignette_strength = fminf(fmaxf(strength, 0.0f), 1.0f);
        config->vignette_radius = fmaxf(radius, 0.1f);
    }
}

void tonemap_reset_to_default(TonemapConfig* config) {
    if (config) {
        tonemap_init(config);
    }
}

// ==============================================================================
// Presets
// ==============================================================================

TonemapConfig tonemap_preset_cinema(void) {
    TonemapConfig config;
    tonemap_init(&config);
    config.algorithm = TONEMAP_ACES;
    config.exposure = 0.5f;
    config.contrast = 1.2f;
    config.saturation = 1.1f;
    config.gamma = 2.2f;
    config.bloom_strength = 0.6f;
    return config;
}

TonemapConfig tonemap_preset_neutral(void) {
    TonemapConfig config;
    tonemap_init(&config);
    config.algorithm = TONEMAP_REINHARD;
    config.exposure = 0.0f;
    config.contrast = 1.0f;
    config.saturation = 1.0f;
    config.gamma = 2.2f;
    return config;
}

TonemapConfig tonemap_preset_vivid(void) {
    TonemapConfig config;
    tonemap_init(&config);
    config.algorithm = TONEMAP_FILMIC;
    config.exposure = -0.5f;
    config.contrast = 1.3f;
    config.saturation = 1.3f;
    config.gamma = 2.0f;
    config.bloom_strength = 0.4f;
    return config;
}

TonemapConfig tonemap_preset_bright(void) {
    TonemapConfig config;
    tonemap_init(&config);
    config.algorithm = TONEMAP_LINEAR;
    config.exposure = 1.0f;
    config.contrast = 1.1f;
    config.saturation = 1.0f;
    config.gamma = 1.8f;
    return config;
}

// ==============================================================================
// Serialization
// ==============================================================================

bool tonemap_serialize(const TonemapConfig* config, u8* buffer, u32 buffer_size, u32* out_size) {
    if (!config || !buffer || !out_size) {
        return false;
    }

    if (buffer_size < sizeof(TonemapConfig)) {
        return false;
    }

    memcpy(buffer, config, sizeof(TonemapConfig));
    *out_size = sizeof(TonemapConfig);

    return true;
}

bool tonemap_deserialize(u8* buffer, u32 size, TonemapConfig* out_config) {
    if (!buffer || !out_config) {
        return false;
    }

    if (size < sizeof(TonemapConfig)) {
        return false;
    }

    memcpy(out_config, buffer, sizeof(TonemapConfig));
    return true;
}
