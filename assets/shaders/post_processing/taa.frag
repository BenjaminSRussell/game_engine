// TAA Fragment Shader
#version 330 core

in vec2 v_uv;
out vec4 frag_color;

uniform sampler2D u_current_frame;
uniform sampler2D u_history_frame;
uniform sampler2D u_velocity;

uniform float u_blend_factor;
uniform float u_sharpness;
uniform int u_enable_sharpening;

void main() {
    vec2 uv = v_uv;
    
    // Read current frame color
    vec3 current = texture(u_current_frame, uv).rgb;
    
    // Read velocity and reproject
    vec2 velocity = texture(u_velocity, uv).rg;
    vec2 history_uv = uv - velocity;
    
    // Sample history with bilinear filtering
    vec3 history = texture(u_history_frame, history_uv).rgb;
    
    // Neighborhood clamping to reduce ghosting
    vec3 near_min = vec3(1e10);
    vec3 near_max = vec3(-1e10);
    
    const int radius = 1;
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            vec2 offset = vec2(x, y) / textureSize(u_current_frame, 0);
            vec3 sample_color = texture(u_current_frame, uv + offset).rgb;
            near_min = min(near_min, sample_color);
            near_max = max(near_max, sample_color);
        }
    }
    
    // Clamp history to neighborhood
    history = clamp(history, near_min, near_max);
    
    // Temporal blend
    vec3 result = mix(history, current, u_blend_factor);
    
    // Optional sharpening to recover detail lost in temporal blend
    if (u_enable_sharpening > 0) {
        vec3 center = current;
        vec3 blur = vec3(0.0);
        
        blur += textureOffset(u_current_frame, uv, ivec2(-1, 0)).rgb;
        blur += textureOffset(u_current_frame, uv, ivec2(1, 0)).rgb;
        blur += textureOffset(u_current_frame, uv, ivec2(0, -1)).rgb;
        blur += textureOffset(u_current_frame, uv, ivec2(0, 1)).rgb;
        blur *= 0.25;
        
        vec3 sharpened = center + (center - blur) * u_sharpness;
        result = mix(result, sharpened, 0.5);
    }
    
    frag_color = vec4(result, 1.0);
}
