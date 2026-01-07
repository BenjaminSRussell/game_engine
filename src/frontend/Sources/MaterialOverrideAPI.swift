// MaterialOverrideAPI.swift
// Swift wrapper for material overrides

import Foundation

public class MaterialOverrideAPI {
    public static let shared = MaterialOverrideAPI()
    private init() {}
    
    // MARK: - Types
    
    public enum OverrideMode: Int32, CaseIterable {
        case none = 0
        case wireframe = 1
        case clay = 2
        case normal = 3
        case albedo = 4
        case roughness = 5
        case metallic = 6
        
        var displayName: String {
            switch self {
            case .none: return "None"
            case .wireframe: return "Wireframe"
            case .clay: return "Clay"
            case .normal: return "Normals"
            case .albedo: return "Albedo"
            case .roughness: return "Roughness"
            case .metallic: return "Metallic"
            }
        }
    }
    
    // MARK: - Control
    
    public var mode: OverrideMode {
        get { return OverrideMode(rawValue: material_override_get_mode()) ?? .none }
        set { material_override_set_mode(newValue.rawValue) }
    }
    
    public func setWireframeProperties(width: Float, color: (r: Float, g: Float, b: Float, a: Float)) {
        material_override_set_wireframe_width(width)
        material_override_set_wireframe_color(color.r, color.g, color.b, color.a)
    }
}

// MARK: - C Bridging

@_silgen_name("material_override_set_mode")
private func material_override_set_mode(_ mode: Int32)

@_silgen_name("material_override_get_mode")
private func material_override_get_mode() -> Int32

@_silgen_name("material_override_set_wireframe_width")
private func material_override_set_wireframe_width(_ width: Float)

@_silgen_name("material_override_set_wireframe_color")
private func material_override_set_wireframe_color(_ r: Float, _ g: Float, _ b: Float, _ a: Float)
