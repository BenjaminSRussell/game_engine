{
  "post_processing_profile": {
    "name": "cinematic_profile",
    "version": "1.0",
    "description": "Cinematic look with color grading and effects",
    
    "bloom": {
      "enabled": true,
      "intensity": 0.3,
      "threshold": 0.9,
      "soft_threshold": 0.5,
      "radius": 4.0,
      "diffusion": 7.0
    },
    
    "tone_mapping": {
      "enabled": true,
      "mode": "filmic",
      "exposure": 1.2,
      "contrast": 1.1,
      "saturation": 1.05,
      "temperature": 0.05,
      "tint": 0.0
    },
    
    "color_grading": {
      "enabled": true,
      "lut_texture": "textures/lut/cinematic.png",
      "lut_contribution": 0.8,
      "shadows": {
        "tint": [-0.02, 0.0, 0.03, 1.0],
        "offset": [0.0, 0.0, 0.05, 1.0]
      },
      "midtones": {
        "tint": [0.0, 0.0, 0.0, 1.0],
        "offset": [0.0, 0.0, 0.0, 1.0]
      },
      "highlights": {
        "tint": [0.05, 0.02, 0.0, 1.0],
        "offset": [0.0, 0.0, 0.0, 1.0]
      }
    },
    
    "depth_of_field": {
      "enabled": true,
      "focus_distance": 10.0,
      "focal_length": 50.0,
      "aperture": 2.8,
      "bokeh_shape": "hexagon",
      "bokeh_rotation": 0.0
    },
    
    "motion_blur": {
      "enabled": true,
      "shutter_angle": 180.0,
      "sample_count": 8
    },
    
    "vignette": {
      "enabled": true,
      "intensity": 0.3,
      "smoothness": 0.4,
      "center": [0.5, 0.5],
      "color": [0.0, 0.0, 0.0, 1.0]
    },
    
    "chromatic_aberration": {
      "enabled": true,
      "intensity": 0.02
    },
    
    "film_grain": {
      "enabled": true,
      "intensity": 0.05,
      "response": 0.8
    },
    
    "ambient_occlusion": {
      "enabled": true,
      "mode": "hbao+",
      "radius": 0.5,
      "intensity": 1.0,
      "quality": "high"
    },
    
    "screen_space_reflections": {
      "enabled": true,
      "quality": "high",
      "max_roughness": 0.6,
      "thickness": 0.1
    },
    
    "taa": {
      "enabled": true,
      "jitter_spread": 0.75,
      "sharpness": 0.5,
      "stationary_blending": 0.95,
      "motion_blending": 0.85
    }
  }
}
