// MeshOptimizerAPI.swift
// Swift wrapper for Mesh Optimizer

import Foundation

public class MeshOptimizerAPI {
    public static let shared = MeshOptimizerAPI()
    private init() {}
    
    // MARK: - Mesh Optimization
    
    public func optimize(meshId: UInt64) {
        mesh_optimizer_optimize(meshId)
    }
    
    public func simplify(meshId: UInt64, targetTriangles: UInt32) {
        mesh_optimizer_simplify(meshId, targetTriangles)
    }
    
    public func generateLODs(meshId: UInt64, lodCount: UInt32) {
        mesh_optimizer_generate_lods(meshId, lodCount)
    }
    
    public func isOptimized(meshId: UInt64) -> Bool {
        return mesh_optimizer_is_optimized(meshId)
    }
}

// MARK: - C Bridging

@_silgen_name("mesh_optimizer_optimize")
private func mesh_optimizer_optimize(_ meshId: UInt64)

@_silgen_name("mesh_optimizer_simplify")
private func mesh_optimizer_simplify(_ meshId: UInt64, _ targetTriangles: UInt32)

@_silgen_name("mesh_optimizer_generate_lods")
private func mesh_optimizer_generate_lods(_ meshId: UInt64, _ lodCount: UInt32)

@_silgen_name("mesh_optimizer_is_optimized")
private func mesh_optimizer_is_optimized(_ meshId: UInt64) -> Bool
