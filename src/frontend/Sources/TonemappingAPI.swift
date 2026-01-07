// TonemappingAPI.swift
// Swift wrapper for HDR Tonemapping

import Foundation

public class TonemappingAPI {
    public static let shared = TonemappingAPI()
    private init() {}
    
    // MARK: - Types
    
    public enum Operator: Int32 {
        case none = 0
        case reinhard = 1
        case filmic = 2
        case aces = 3
        case uncharted2 = 4
    }
    
    // MARK: - Tonemapping Control
    
    public var `operator`: Operator {
        get { return Operator(rawValue: tonemap_get_operator()) ?? .aces }
        set { tonemap_set_operator(newValue.rawValue) }
    }
    
    public var exposure: Float {
        get { return tonemap_get_exposure() }
        set { tonemap_set_exposure(newValue) }
    }
    
    public var whitePoint: Float {
        get { return tonemap_get_white_point() }
        set { tonemap_set_white_point(newValue) }
    }
}

// MARK: - C Bridging

@_silgen_name("tonemap_set_operator")
private func tonemap_set_operator(_ op: Int32)

@_silgen_name("tonemap_get_operator")
private func tonemap_get_operator() -> Int32

@_silgen_name("tonemap_set_exposure")
private func tonemap_set_exposure(_ exposure: Float)

@_silgen_name("tonemap_get_exposure")
private func tonemap_get_exposure() -> Float

@_silgen_name("tonemap_set_white_point")
private func tonemap_set_white_point(_ whitePoint: Float)

@_silgen_name("tonemap_get_white_point")
private func tonemap_get_white_point() -> Float
