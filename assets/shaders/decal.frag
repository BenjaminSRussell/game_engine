#version 450

layout(location = 0) in vec4 inClipPos;

// Global (Set 0)
layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 projection;
    mat4 view;
    mat4 invProjection;
    mat4 invView;
    vec2 screenSize;
} global;

// G-Buffer Depth (Set 0, Binding 1)
layout(set = 0, binding = 1) uniform sampler2D texDepth;

// Decal Textures (Set 1)
layout(set = 1, binding = 0) uniform sampler2D texAlbedo;
layout(set = 1, binding = 1) uniform sampler2D texNormal;
layout(set = 1, binding = 2) uniform sampler2D texMaterial; // Met/Rough/AO

// Decal Params (Push Constants)
layout(push_constant) uniform PushConsts {
    mat4 model;
    mat4 invModel; // World -> Decal Space
    vec4 colorTint;
    float normalOpacity;
    float roughnessScale;
    float metallicScale;
    float decalId; // For randomization if needed
} push;

// Outputs to G-Buffer
layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outMaterial;

vec3 reconstructWorldPos(vec2 uv, float depth) {
    vec4 clipVal = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 viewVal = global.invProjection * clipVal;
    viewVal /= viewVal.w;
    vec4 worldVal = global.invView * viewVal;
    return worldVal.xyz;
}

vec3 unpackNormal(vec2 enc) {
    // Basic unpacking assuming 0..1 storage
    vec3 n;
    n.xy = enc * 2.0 - 1.0;
    n.z = sqrt(max(0.0, 1.0 - dot(n.xy, n.xy)));
    return n;
}

void main() {
    // 1. Calculate Screen UV
    vec2 screenUV = (inClipPos.xy / inClipPos.w) * 0.5 + 0.5;
    
    // 2. Sample Scene Depth
    float sceneDepth = texture(texDepth, screenUV).r;
    
    // 3. Reconstruct World Position
    vec3 worldPos = reconstructWorldPos(screenUV, sceneDepth);
    
    // 4. Transform to Decal Object Space
    vec4 objectPos = push.invModel * vec4(worldPos, 1.0);
    
    // 5. Volume Clip Test (Unit Cube is -0.5 to 0.5)
    // Using slight epsilon to avoid artifacts at edges
    vec3 clipInput = abs(objectPos.xyz);
    if (clipInput.x > 0.5 || clipInput.y > 0.5 || clipInput.z > 0.5) {
        discard;
    }
    
    // 6. Calculate Decal UVs
    // Decal is projected along Z axis? usually +Y or +Z.
    // Let's assume projection is onto the XY plane of the decal box (Planar projection)
    // UV range 0..1 mapped from -0.5..0.5
    vec2 decalUV = objectPos.xy + 0.5; 
    
    // 7. Sample Decal Textures
    vec4 albedoSample = texture(texAlbedo, decalUV);
    vec3 normalSample = texture(texNormal, decalUV).xyz * 2.0 - 1.0;
    vec4 materialSample = texture(texMaterial, decalUV); // R=Met, G=Rough, B=AO
    
    // Apply Mask/Tint
    vec4 finalAlbedo = albedoSample * push.colorTint;
    
    // Alpha Test / Soft Blend
    // We can use the texture alpha for blending weight
    float alpha = finalAlbedo.a;
    
    // Falloff at edges of the cube (Z-axis fade)
    // float distFade = 1.0 - abs(objectPos.z * 2.0);
    // distFade = clamp(distFade * 5.0, 0.0, 1.0); // sharp fade near boundaries
    // alpha *= distFade;
    
    if (alpha <= 0.01) discard;
    
    // Output
    // Setup for blending:
    // If GL_BLEND is enabled with glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), 
    // we output the color and alpha.
    
    outAlbedo = vec4(finalAlbedo.rgb, alpha);
    
    // Normal Blending is tricky in deferred. 
    // We ideally want to read existing normal, blend, and write back. 
    // BUT we can't read/write same target easily without specialized extensions (Fbfetch).
    // Standard approach: Render Decals into G-Buffer *copy* or use logic op?
    // OR: just overwrite if opaque-ish?
    // Real deferred decals usually use hardware blending for Albedo/PBR, 
    // but Normal blending often requires just overwriting or special 'DBuffer' pass.
    // For this task, we'll assume standard Alpha Blending for G-Buffer targets.
    
    // Rotate normal map to match decal orientation
    // We need TBN.
    // T = normalize(push.model * vec4(1,0,0,0)).xyz;
    // B = normalize(push.model * vec4(0,1,0,0)).xyz; 
    // N = normalize(push.model * vec4(0,0,1,0)).xyz;
    // ... wait, this is volume projection.
    
    // Simply rotating the normal sample by the decal Model rotation is usually "correct enough" 
    // for planar projection if the underlying surface is flat-ish.
    // Ideally we blend with underlying surface normal.
    
    mat3 normalMatrix = mat3(push.model);
    vec3 worldNormal = normalize(normalMatrix * normalSample);
    
    outNormal = vec4(worldNormal * 0.5 + 0.5, alpha * push.normalOpacity); // Encoded output
    
    // Material params
    // Simply mix
    outMaterial = vec4(materialSample.rgb, alpha); 
}
