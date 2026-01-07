// PhysicsDebugAPI.swift
// Swift wrapper for physics debug visualization

import Foundation
import SwiftUI

/// Physics debug visualization API
public class PhysicsDebugAPI {
    public static let shared = PhysicsDebugAPI()
    private init() {}
    
    // MARK: - Enable/Disable
    
    /// Enable or disable all physics debug rendering
    public func setEnabled(_ enabled: Bool) {
        physics_debug_set_enabled(enabled)
    }
    
    /// Check if physics debug rendering is currently enabled
    public func isEnabled() -> Bool {
        return physics_debug_is_enabled()
    }
    
    // MARK: - Visualization Modes
    
    /// Show contact points between colliding objects
    public func showContacts(_ enabled: Bool) {
        physics_debug_show_contacts(enabled)
    }
    
    /// Show constraint connections (joints, hinges, etc.)
    public func showConstraints(_ enabled: Bool) {
        physics_debug_show_constraints(enabled)
    }
    
    /// Show axis-aligned bounding boxes
    public func showAABBs(_ enabled: Bool) {
        physics_debug_show_aabbs(enabled)
    }
    
    /// Show oriented bounding boxes
    public func showOBBs(_ enabled: Bool) {
        physics_debug_show_obbs(enabled)
    }
    
    /// Show collision shapes (wireframe)
    public func showShapes(_ enabled: Bool) {
        physics_debug_show_shapes(enabled)
    }
    
    /// Show velocity vectors
    public func showVelocities(_ enabled: Bool) {
        physics_debug_show_velocities(enabled)
    }
    
    /// Show force vectors
    public func showForces(_ enabled: Bool) {
        physics_debug_show_forces(enabled)
    }
    
    /// Show center of mass markers
    public func showCenterOfMass(_ enabled: Bool) {
        physics_debug_show_center_of_mass(enabled)
    }
    
    // MARK: - Colors
    
    /// Set color for contact point rendering
    public func setContactColor(_ color: Color) {
        let (r, g, b) = color.rgbComponents
        physics_debug_set_contact_color(r, g, b)
    }
    
    /// Set color for constraint rendering
    public func setConstraintColor(_ color: Color) {
        let (r, g, b) = color.rgbComponents
        physics_debug_set_constraint_color(r, g, b)
    }
    
    /// Set color for AABB rendering
    public func setAABBColor(_ color: Color) {
        let (r, g, b) = color.rgbComponents
        physics_debug_set_aabb_color(r, g, b)
    }
    
    /// Set color for velocity vector rendering
    public func setVelocityColor(_ color: Color) {
        let (r, g, b) = color.rgbComponents
        physics_debug_set_velocity_color(r, g, b)
    }
    
    // MARK: - Scales
    
    /// Set size of contact point markers
    public func setContactSize(_ size: Float) {
        physics_debug_set_contact_size(size)
    }
    
    /// Set scale multiplier for velocity vectors
    public func setVelocityScale(_ scale: Float) {
        physics_debug_set_velocity_scale(scale)
    }
    
    /// Set scale multiplier for force vectors
    public func setForceScale(_ scale: Float) {
        physics_debug_set_force_scale(scale)
    }
    
    // MARK: - Statistics
    
    /// Physics debug statistics
    public struct Stats {
        public var activeBodies: UInt32
        public var sleepingBodies: UInt32
        public var contactCount: UInt32
        public var constraintCount: UInt32
        public var islandCount: UInt32
    }
    
    /// Get current physics statistics
    public func getStats() -> Stats {
        let cStats = UnsafeMutablePointer<PhysicsDebugStats_C>.allocate(capacity: 1)
        defer { cStats.deallocate() }
        physics_debug_get_stats(cStats)
        
        return Stats(
            activeBodies: cStats.pointee.active_bodies,
            sleepingBodies: cStats.pointee.sleeping_bodies,
            contactCount: cStats.pointee.contact_count,
            constraintCount: cStats.pointee.constraint_count,
            islandCount: cStats.pointee.island_count
        )
    }
}

// MARK: - C Bridging

private typealias PhysicsDebugStats_C = (
    active_bodies: UInt32,
    sleeping_bodies: UInt32,
    contact_count: UInt32,
    constraint_count: UInt32,
    island_count: UInt32
)

@_silgen_name("physics_debug_set_enabled")
private func physics_debug_set_enabled(_ enabled: Bool)

@_silgen_name("physics_debug_is_enabled")
private func physics_debug_is_enabled() -> Bool

@_silgen_name("physics_debug_show_contacts")
private func physics_debug_show_contacts(_ enabled: Bool)

@_silgen_name("physics_debug_show_constraints")
private func physics_debug_show_constraints(_ enabled: Bool)

@_silgen_name("physics_debug_show_aabbs")
private func physics_debug_show_aabbs(_ enabled: Bool)

@_silgen_name("physics_debug_show_obbs")
private func physics_debug_show_obbs(_ enabled: Bool)

@_silgen_name("physics_debug_show_shapes")
private func physics_debug_show_shapes(_ enabled: Bool)

@_silgen_name("physics_debug_show_velocities")
private func physics_debug_show_velocities(_ enabled: Bool)

@_silgen_name("physics_debug_show_forces")
private func physics_debug_show_forces(_ enabled: Bool)

@_silgen_name("physics_debug_show_center_of_mass")
private func physics_debug_show_center_of_mass(_ enabled: Bool)

@_silgen_name("physics_debug_set_contact_color")
private func physics_debug_set_contact_color(_ r: Float, _ g: Float, _ b: Float)

@_silgen_name("physics_debug_set_constraint_color")
private func physics_debug_set_constraint_color(_ r: Float, _ g: Float, _ b: Float)

@_silgen_name("physics_debug_set_aabb_color")
private func physics_debug_set_aabb_color(_ r: Float, _ g: Float, _ b: Float)

@_silgen_name("physics_debug_set_velocity_color")
private func physics_debug_set_velocity_color(_ r: Float, _ g: Float, _ b: Float)

@_silgen_name("physics_debug_set_contact_size")
private func physics_debug_set_contact_size(_ size: Float)

@_silgen_name("physics_debug_set_velocity_scale")
private func physics_debug_set_velocity_scale(_ scale: Float)

@_silgen_name("physics_debug_set_force_scale")
private func physics_debug_set_force_scale(_ scale: Float)

@_silgen_name("physics_debug_get_stats")
private func physics_debug_get_stats(_ stats: UnsafeMutablePointer<PhysicsDebugStats_C>)

// MARK: - Color Extension

private extension Color {
    var rgbComponents: (Float, Float, Float) {
        #if canImport(UIKit)
        var r: CGFloat = 0, g: CGFloat = 0, b: CGFloat = 0, a: CGFloat = 0
        UIColor(self).getRed(&r, green: &g, blue: &b, alpha: &a)
        return (Float(r), Float(g), Float(b))
        #elseif canImport(AppKit)
        let nsColor = NSColor(self)
        guard let rgb = nsColor.usingColorSpace(.deviceRGB) else {
            return (0, 0, 0)
        }
        return (Float(rgb.redComponent), Float(rgb.greenComponent), Float(rgb.blueComponent))
        #endif
    }
}
