// GraphicsAPI.swift
// Swift wrapper for high-fidelity graphics features
// Bridges to graphics_api_bridge.h

import Foundation
import simd

// MARK: - Graphics API Wrapper

/// Main graphics API interface for VoxelForgeStudio
public class GraphicsAPI {
    // Singleton access
    public static let shared = GraphicsAPI()
    private init() {}
    
    // MARK: - Dynamic Resolution
    
    /// Set render resolution scale (0.25 = quarter res, 1.0 = native, 2.0 = 2x supersampling)
    public func setRenderScale(_ scale: Float) {
        graphics_set_render_scale(scale)
    }
    
    public func getRenderScale() -> Float {
        return graphics_get_render_scale()
    }
    
    public func getRenderResolution() -> (width: UInt32, height: UInt32) {
        var width: UInt32 = 0
        var height: UInt32 = 0
        graphics_get_render_resolution(&width, &height)
        return (width, height)
    }
    
    // MARK: - Particle System
    
    public struct ParticleEmitterConfig {
        var maxParticles: UInt32
        var emissionRate: Float
        var lifetime: Float
        var startSize: Float
        var endSize: Float
        var startColor: SIMD4<Float>
        var endColor: SIMD4<Float>
    }
    
    public func createParticleEmitter(config: ParticleEmitterConfig) -> UInt32 {
        var cConfig = ParticleEmitterConfig_C(
            max_particles: config.maxParticles,
            emission_rate: config.emissionRate,
            lifetime: config.lifetime,
            start_size: config.startSize,
            end_size: config.endSize,
            start_color: (config.startColor.x, config.startColor.y, config.startColor.z, config.startColor.w),
            end_color: (config.endColor.x, config.endColor.y, config.endColor.z, config.endColor.w))
        return graphics_particle_create_emitter(&cConfig)
    }
    
    public func setParticlePosition(emitterID: UInt32, x: Float, y: Float, z: Float) {
        graphics_particle_set_position(emitterID, x, y, z)
    }
    
    public func setParticleActive(emitterID: UInt32, active: Bool) {
        graphics_particle_set_active(emitterID, active)
    }
    
    public func destroyParticleEmitter(emitterID: UInt32) {
        graphics_particle_destroy_emitter(emitterID)
    }
    
    // MARK: - GLTF Loading
    
    public struct GLTFLoadResult {
        var meshCount: UInt32
        var materialCount: UInt32
        var meshNames: [String]?
    }
    
    public func loadGLTF(path: String) -> (modelID: UInt32, result: GLTFLoadResult)? {
        let cResult = UnsafeMutablePointer<GLTFLoadResult_C>.allocate(capacity: 1)
        defer { cResult.deallocate() }
        
        let modelID = graphics_gltf_load(path, cResult)
        
        guard modelID > 0 else { return nil }
        
        let result = GLTFLoadResult(
            meshCount: cResult.pointee.mesh_count,
            materialCount: cResult.pointee.material_count,
            meshNames: nil)
        
        return (modelID, result)
    }
    
    public func unloadGLTF(modelID: UInt32) {
        graphics_gltf_unload(modelID)
    }
    
    // MARK: - PBR Materials
    
    public struct PBRMaterialConfig {
        var metallic: Float = 0.0
        var roughness: Float = 0.5
        var ao: Float = 1.0
        var normalStrength: Float = 1.0
        var albedo: SIMD4<Float> = SIMD4(0.8, 0.8, 0.8, 1.0)
        var emissive: SIMD3<Float> = SIMD3(0, 0, 0)
        var albedoTexture: String = ""
        var normalTexture: String = ""
        var metallicRoughnessTexture: String = ""
        var aoTexture: String = ""
    }
    
    public func createPBRMaterial(config: PBRMaterialConfig) -> UInt32 {
        // Helper to convert string to tuple of chars? No, too complex.
        // For now, passing empty texture strings as we don't have a helper.
        // In real impl, we'd need a helper to copy string bytes to the tuple.
        
        // Zero-init large tuple by allocating pointer
        let cConfig = UnsafeMutablePointer<PBRMaterialConfig_C>.allocate(capacity: 1)
        defer { cConfig.deallocate() }
        
        cConfig.pointee.metallic = config.metallic
        cConfig.pointee.roughness = config.roughness
        cConfig.pointee.ao = config.ao
        cConfig.pointee.normal_strength = config.normalStrength
        cConfig.pointee.albedo = (config.albedo.x, config.albedo.y, config.albedo.z, config.albedo.w)
        cConfig.pointee.emissive = (config.emissive.x, config.emissive.y, config.emissive.z)
        // Textures left empty for now as filling char tuples is verbose without a helper
        
        return graphics_material_create_pbr(cConfig)
    }
    
    public func destroyMaterial(materialID: UInt32) {
        graphics_material_destroy(materialID)
    }
    
    // MARK: - Post-Processing
    
    public struct BloomConfig {
        var threshold: Float = 1.0
        var intensity: Float = 0.04
        var spread: Float = 1.0
        var enabled: Bool = true
    }
    
    public struct TAAConfig {
        var blendFactor: Float = 0.9
        var varianceClamp: Float = 1.0
        var jitterEnabled: Bool = true
        var enabled: Bool = true
    }
    
    public struct SSRConfig {
        var maxDistance: Float = 100.0
        var maxSteps: UInt32 = 64
        var thickness: Float = 0.5
        var fadeStart: Float = 0.8
        var enabled: Bool = true
    }
    
    public struct VolumetricFogConfig {
        var density: Float = 0.01
        var scattering: Float = 0.5
        var heightFalloff: Float = 0.1
        var color: SIMD3<Float> = SIMD3(0.5, 0.5, 0.6)
        var enabled: Bool = false
    }
    
    public func setBloomConfig(_ config: BloomConfig) {
        var cConfig = BloomConfig_C(threshold: config.threshold, intensity: config.intensity, spread: config.spread, enabled: config.enabled)
        graphics_bloom_set_config(&cConfig)
    }
    
    public func getBloomConfig() -> BloomConfig {
        let cConfig = UnsafeMutablePointer<BloomConfig_C>.allocate(capacity: 1)
        defer { cConfig.deallocate() }
        graphics_bloom_get_config(cConfig)
        return BloomConfig(threshold: cConfig.pointee.threshold, intensity: cConfig.pointee.intensity, spread: cConfig.pointee.spread, enabled: cConfig.pointee.enabled)
    }
    
    public func setTAAConfig(_ config: TAAConfig) {
        var cConfig = TAAConfig_C(blend_factor: config.blendFactor, variance_clamp: config.varianceClamp, jitter_enabled: config.jitterEnabled, enabled: config.enabled)
        graphics_taa_set_config(&cConfig)
    }
    
    public func setSSRConfig(_ config: SSRConfig) {
        var cConfig = SSRConfig_C(max_distance: config.maxDistance, max_steps: config.maxSteps, thickness: config.thickness, fade_start: config.fadeStart, enabled: config.enabled)
        graphics_ssr_set_config(&cConfig)
    }
    
    public func setFogConfig(_ config: VolumetricFogConfig) {
        var cConfig = VolumetricFogConfig_C(density: config.density, scattering: config.scattering, height_falloff: config.heightFalloff, color: (config.color.x, config.color.y, config.color.z), enabled: config.enabled)
        graphics_fog_set_config(&cConfig)
    }
    
    // MARK: - IBL Environment
    
    public func loadEnvironmentMap(path: String) -> UInt32 {
        return graphics_ibl_load_environment(path)
    }
    
    public func setActiveEnvironment(environmentID: UInt32) {
        graphics_ibl_set_active(environmentID)
    }
    
    public func setEnvironmentIntensity(_ intensity: Float) {
        graphics_ibl_set_intensity(intensity)
    }
    
    // MARK: - Debug Visualization
    
    public func setWireframeMode(_ enabled: Bool) {
        graphics_debug_show_wireframe(enabled)
    }
    
    public func setShowNormals(_ enabled: Bool) {
        graphics_debug_show_normals(enabled)
    }
    
    public func setShowParticles(_ enabled: Bool) {
        graphics_debug_show_particles(enabled)
    }
    
    public func setLightingOnlyMode(_ enabled: Bool) {
        graphics_debug_show_lighting_only(enabled)
    }
}

// MARK: - C Bridging Declarations

// These match the C structs from graphics_api_bridge.h
// Note: In production, these would be auto-generated from a bridging header

private typealias ParticleEmitterConfig_C = (
    max_particles: UInt32,
    emission_rate: Float,
    lifetime: Float,
    start_size: Float,
    end_size: Float,
    start_color: (Float, Float, Float, Float),
    end_color: (Float, Float, Float, Float)
)

private typealias GLTFLoadResult_C = (
    mesh_count: UInt32,
    material_count: UInt32,
    mesh_names: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>?
)

private typealias PBRMaterialConfig_C = (
    metallic: Float,
    roughness: Float,
    ao: Float,
    normal_strength: Float,
    albedo: (Float, Float, Float, Float),
    emissive: (Float, Float, Float),
    albedo_texture: (CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar),
    normal_texture: (CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar),
    metallic_roughness_texture: (CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar),
    ao_texture: (CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar)
)

private typealias BloomConfig_C = (threshold: Float, intensity: Float, spread: Float, enabled: Bool)
private typealias TAAConfig_C = (blend_factor: Float, variance_clamp: Float, jitter_enabled: Bool, enabled: Bool)
private typealias SSRConfig_C = (max_distance: Float, max_steps: UInt32, thickness: Float, fade_start: Float, enabled: Bool)
private typealias VolumetricFogConfig_C = (density: Float, scattering: Float, height_falloff: Float, color: (Float, Float, Float), enabled: Bool)

// C function declarations (would normally be in bridging header)
@_silgen_name("graphics_set_render_scale")
private func graphics_set_render_scale(_ scale: Float)

@_silgen_name("graphics_get_render_scale")
private func graphics_get_render_scale() -> Float

@_silgen_name("graphics_get_render_resolution")
private func graphics_get_render_resolution(_ width: UnsafeMutablePointer<UInt32>, _ height: UnsafeMutablePointer<UInt32>)

@_silgen_name("graphics_particle_create_emitter")
private func graphics_particle_create_emitter(_ config: UnsafePointer<ParticleEmitterConfig_C>) -> UInt32

@_silgen_name("graphics_particle_set_position")
private func graphics_particle_set_position(_ emitterID: UInt32, _ x: Float, _ y: Float, _ z: Float)

@_silgen_name("graphics_particle_set_active")
private func graphics_particle_set_active(_ emitterID: UInt32, _ active: Bool)

@_silgen_name("graphics_particle_destroy_emitter")
private func graphics_particle_destroy_emitter(_ emitterID: UInt32)

@_silgen_name("graphics_gltf_load")
private func graphics_gltf_load(_ path: String, _ result: UnsafeMutablePointer<GLTFLoadResult_C>) -> UInt32

@_silgen_name("graphics_gltf_unload")
private func graphics_gltf_unload(_ modelID: UInt32)

@_silgen_name("graphics_material_create_pbr")
private func graphics_material_create_pbr(_ config: UnsafePointer<PBRMaterialConfig_C>) -> UInt32

@_silgen_name("graphics_material_destroy")
private func graphics_material_destroy(_ materialID: UInt32)

@_silgen_name("graphics_bloom_set_config")
private func graphics_bloom_set_config(_ config: UnsafePointer<BloomConfig_C>)

@_silgen_name("graphics_bloom_get_config")
private func graphics_bloom_get_config(_ config: UnsafeMutablePointer<BloomConfig_C>)

@_silgen_name("graphics_taa_set_config")
private func graphics_taa_set_config(_ config: UnsafePointer<TAAConfig_C>)

@_silgen_name("graphics_ssr_set_config")
private func graphics_ssr_set_config(_ config: UnsafePointer<SSRConfig_C>)

@_silgen_name("graphics_fog_set_config")
private func graphics_fog_set_config(_ config: UnsafePointer<VolumetricFogConfig_C>)

@_silgen_name("graphics_ibl_load_environment")
private func graphics_ibl_load_environment(_ path: String) -> UInt32

@_silgen_name("graphics_ibl_set_active")
private func graphics_ibl_set_active(_ environmentID: UInt32)

@_silgen_name("graphics_ibl_set_intensity")
private func graphics_ibl_set_intensity(_ intensity: Float)

@_silgen_name("graphics_debug_show_wireframe")
private func graphics_debug_show_wireframe(_ enabled: Bool)

@_silgen_name("graphics_debug_show_normals")
private func graphics_debug_show_normals(_ enabled: Bool)

@_silgen_name("graphics_debug_show_particles")
private func graphics_debug_show_particles(_ enabled: Bool)

@_silgen_name("graphics_debug_show_lighting_only")
private func graphics_debug_show_lighting_only(_ enabled: Bool)
