#include "editor/sequence_editor/timeline.h"
#include "editor/sequence_editor/camera_sequencer.h"
#include "renderer/lighting/light_system.h"
#include "renderer/shadows/shadow_manager.h"
#include "renderer/environment/environment_manager.h"
#include "renderer/post_processing/color_grading.h"
#include "renderer/pbr/pbr_brdf.h"
#include <stdio.h>

/**
 * Complete Demo: Animation + Rendering Systems
 * 
 * This demo shows how to use all the Phase 5 and Phase 6 systems together
 * to create a complete animated scene with photorealistic rendering.
 */

void demo_animation_system(void) {
    printf("=== Animation System Demo ===\n\n");
    
    // Initialize timeline (30 second sequence)
    timeline_init(30.0);
    
    // Get camera sequencer
    CameraSequencer* cam_seq = timeline_get_camera_sequencer();
    
    // Create camera track
    uint32_t track_id = camera_sequencer_add_track(cam_seq, "Main Camera");
    CameraTrack* track = camera_sequencer_get_track(cam_seq, track_id);
    
    // Add camera shots
    uint32_t shot1 = camera_track_add_shot(track, 1, 0.0, 5.0);   // Camera 1: 0-5s
    uint32_t shot2 = camera_track_add_shot(track, 2, 5.0, 10.0);  // Camera 2: 5-15s
    uint32_t shot3 = camera_track_add_shot(track, 1, 15.0, 15.0); // Back to camera 1
    
    // Configure first shot with DOF
    CameraShot* shot = camera_track_get_shot(track, shot1);
    camera_shot_set_dof(shot, 10.0f, 2.8f);
    shot->transition_type = TRANSITION_CUT;
    
    // Second shot with camera shake
    shot = camera_track_get_shot(track, shot2);
    camera_shot_set_shake(shot, 0.3f, 2.0f);
    shot->transition_type = TRANSITION_DISSOLVE;
    shot->transition_duration = 1.0;
    
    // Enable composition guides on all shots
    for (uint32_t i = 0; i < track->shot_count; i++) {
        track->shots[i].show_rule_of_thirds = true;
        track->shots[i].show_safe_area = true;
    }
    
    printf("✓ Created 3 camera shots\n");
    printf("✓ Configured transitions and effects\n\n");
}

void demo_rendering_system(void) {
    printf("=== Rendering System Demo ===\n\n");
    
    // Environment setup
    printf("Setting up environment...\n");
    EnvironmentManager* env = environment_manager_create();
    environment_set_time_of_day(env, 14.5f); // 2:30 PM
    
    Skybox* sky = environment_get_skybox(env);
    skybox_set_turbidity(sky, 2.0f);
    skybox_enable_stars(sky, false); // Daytime
    skybox_enable_moon(sky, false);
    
    printf("✓ Environment created (2:30 PM)\n");
    
    // Lighting setup
    printf("\nSetting up lights...\n");
    LightSystem* lights = light_system_create();
    
    // Sun (main directional light)
    uint32_t sun_id = light_system_add_light(lights, LIGHT_TYPE_DIRECTIONAL);
    Light* sun = light_system_get_light(lights, sun_id);
    light_set_direction(sun, 0.4f, -0.7f, 0.3f);
    light_set_color(sun, 1.0f, 0.95f, 0.85f);
    light_set_intensity(sun, 4.0f);
    light_set_shadow_enabled(sun, true);
    light_set_shadow_resolution(sun, 2048);
    printf("  ✓ Sun (directional) with shadows\n");
    
    // Key light (spot)
    uint32_t key_id = light_system_add_light(lights, LIGHT_TYPE_SPOT);
    Light* key = light_system_get_light(lights, key_id);
    light_set_position(key, 5.0f, 8.0f, 5.0f);
    light_set_direction(key, -0.5f, -1.0f, -0.5f);
    light_set_color(key, 1.0f, 0.9f, 0.8f);
    light_set_intensity(key, 15.0f);
    light_set_spot_angles(key, 0.3f, 0.5f);
    light_set_shadow_enabled(key, true);
    printf("  ✓ Key light (spot)\n");
    
    // Fill light (area)
    uint32_t fill_id = light_system_add_light(lights, LIGHT_TYPE_AREA);
    Light* fill = light_system_get_light(lights, fill_id);
    light_set_position(fill, -5.0f, 5.0f, 8.0f);
    light_set_area_size(fill, 4.0f, 4.0f);
    light_set_color(fill, 0.6f, 0.7f, 1.0f);
    light_set_intensity(fill, 3.0f);
    printf("  ✓ Fill light (area)\n");
    
    // Rim light (point)
    uint32_t rim_id = light_system_add_light(lights, LIGHT_TYPE_POINT);
    Light* rim = light_system_get_light(lights, rim_id);
    light_set_position(rim, -3.0f, 3.0f, -5.0f);
    light_set_color(rim, 0.8f, 0.9f, 1.0f);
    light_set_intensity(rim, 10.0f);
    light_set_range(rim, 15.0f);
    printf("  ✓ Rim light (point)\n");
    
    printf("\n✓ Total: %u lights\n", light_system_get_visible_count(lights));
    
    // Shadow system
    printf("\nSetting up shadows...\n");
    ShadowManager* shadows = shadow_manager_create(4096);
    
    // Allocate shadow maps
    uint32_t sun_shadow = shadow_manager_allocate_shadow_map(shadows, SHADOW_MAP_CASCADE, 2048);
    uint32_t key_shadow = shadow_manager_allocate_shadow_map(shadows, SHADOW_MAP_2D, 1024);
    
    printf("✓ Shadow atlas: 4096x4096\n");
    printf("✓ Allocated %u shadow maps\n", shadow_manager_get_allocated_count(shadows));
    printf("✓ Memory usage: %.2f MB\n", shadow_manager_get_memory_usage(shadows) / (1024.0f * 1024.0f));
    
    // IBL setup
    printf("\nSetting up IBL...\n");
    IBLSystem* ibl = environment_get_ibl_system(env);
    
    uint32_t probe_id = ibl_system_add_probe(ibl);
    IBLProbe* probe = ibl_system_get_probe(ibl, probe_id);
    ibl_probe_set_position(probe, 0.0f, 2.0f, 0.0f);
    
    float box_min[3] = {-15.0f, 0.0f, -15.0f};
    float box_max[3] = {15.0f, 10.0f, 15.0f};
    ibl_probe_set_box(probe, box_min, box_max);
    ibl_probe_set_intensity(probe, 1.2f);
    
    ibl_system_set_global_probe(ibl, probe_id);
    printf("✓ Global IBL probe configured\n");
    
    // Material examples
    printf("\nCreating materials...\n");
    
    PBRMaterial metal = pbr_material_metal(0.2f);
    printf("  ✓ Brushed metal (roughness: 0.2)\n");
    
    PBRMaterial plastic = pbr_material_plastic(0.8f, 0.2f, 0.2f, 0.5f);
    printf("  ✓ Red plastic\n");
    
    PBRMaterial skin = pbr_material_skin();
    printf("  ✓ Skin with SSS\n");
    
    PBRMaterial paint = pbr_material_car_paint(0.0f, 0.2f, 0.8f);
    printf("  ✓ Blue car paint with clearcoat\n");
    
    // Post-processing
    printf("\nConfiguring post-processing...\n");
    ColorGradingParams grading = color_grading_preset_cinematic();
    grading.saturation = 1.15f;
    grading.contrast = 1.12f;
    grading.temperature = 0.1f; // Slightly warm
    printf("✓ Cinematic color grading\n");
    printf("  - Saturation: %.2f\n", grading.saturation);
    printf("  - Contrast: %.2f\n", grading.contrast);
    printf("  - Temperature: %.2f\n", grading.temperature);
    
    printf("\n✓ Rendering system fully configured!\n\n");
}

void demo_complete_scene(void) {
    printf("=== Complete Scene Demo ===\n\n");
    printf("This demo simulates a complete production scene setup\n\n");
    
    // Setup both systems
    demo_animation_system();
    demo_rendering_system();
    
    // Simulate playback
    printf("--- Simulating Playback ---\n");
    
    for (int frame = 0; frame < 120; frame++) {
        double delta_time = 1.0 / 60.0; // 60 FPS
        
        // Update timeline
        timeline_update(delta_time);
        
        // Get active shot
        CameraSequencer* cam_seq = timeline_get_camera_sequencer();
        CameraShot* active = camera_sequencer_get_active_shot(cam_seq);
        
        // Print status every 30 frames (0.5 seconds)
        if (frame % 30 == 0) {
            double time = timeline_get_current_time();
            printf("Frame %d (%.2fs): ", frame, time);
            
            if (active) {
                printf("Shot %u active", active->shot_id);
                if (active->effects.enable_dof) {
                    printf(" [DOF: f=%.1f, a=%.1f]", 
                           active->effects.focal_distance,
                           active->effects.aperture);
                }
                if (active->effects.enable_shake) {
                    printf(" [Shake: %.1f]", active->effects.shake_intensity);
                }
                printf("\n");
            } else {
                printf("No active shot\n");
            }
        }
    }
    
    printf("\n✓ Playback complete (2 seconds)\n");
    
    // Cleanup
    timeline_shutdown();
    
    printf("\n=== Demo Complete! ===\n");
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║  Animation + Rendering Systems Demo                   ║\n");
    printf("║  Phase 5 & 6 Complete Implementation                  ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    demo_complete_scene();
    
    return 0;
}
