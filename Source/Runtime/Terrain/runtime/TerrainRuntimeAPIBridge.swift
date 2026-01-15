import Foundation

/// Swift bridge for Terrain Runtime API to enable frontend integration
@objc public class TerrainRuntimeAPIBridge: NSObject {
    
    @objc public static let shared = TerrainRuntimeAPIBridge()
    
    private let terrainAPI = TerrainRuntimeAPI.shared
    
    @objc public func initializeTerrain(width: Int32, height: Int32) -> Bool {
        do {
            let terrainData = TerrainData(size: SIMD2<Int>(Int(width), Int(height)))
            try terrainAPI.initialize(with: terrainData)
            return true
        } catch {
            print("Failed to initialize terrain: \(error)")
            return false
        }
    }
    
    @objc public func modifyHeight(x: Float, y: Float, delta: Float, radius: Float) -> Bool {
        Task {
            do {
                _ = try await terrainAPI.modifyHeight(at: SIMD2<Float>(x, y), delta: delta, radius: radius)
            } catch {
                print("Failed to modify height: \(error)")
            }
        }
        return true
    }
    
    @objc public func paintTexture(x: Float, y: Float, textureIndex: Int32, radius: Float) -> Bool {
        Task {
            do {
                _ = try await terrainAPI.paintTexture(at: SIMD2<Float>(x, y), textureIndex: Int(textureIndex), radius: radius)
            } catch {
                print("Failed to paint texture: \(error)")
            }
        }
        return true
    }
    
    @objc public func getHeight(x: Float, y: Float) -> Float {
        return terrainAPI.getHeight(at: SIMD2<Float>(x, y)) ?? 0.0
    }
    
    @objc public func isTerrainInitialized() -> Bool {
        return terrainAPI.isInitialized
    }
    
    @objc public func shutdownTerrain() {
        terrainAPI.shutdown()
    }
}
