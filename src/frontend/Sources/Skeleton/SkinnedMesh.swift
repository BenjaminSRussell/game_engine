import SwiftUI
import simd

// MARK: - Skinned Mesh
/// Mesh with bone weight bindings for skeletal deformation

struct SkinnedMesh: Identifiable {
    let id: UUID
    var name: String
    
    // Vertex data
    var vertices: [SkinnedVertex]
    var indices: [UInt32]
    
    // Skeleton binding
    var skeletonID: UUID?
    var boneMapping: [Int: UUID]  // Index to bone ID mapping
    
    // Material reference
    var materialPath: String = ""
    
    // MARK: - Initialization
    
    init(id: UUID = UUID(), name: String = "Skinned Mesh") {
        self.id = id
        self.name = name
        self.vertices = []
        self.indices = []
        self.boneMapping = [:]
    }
    
    // MARK: - Vertex Operations
    
    /// Add a vertex with bone weights
    mutating func addVertex(
        position: SIMD3<Float>,
        normal: SIMD3<Float> = SIMD3<Float>(0, 1, 0),
        uv: SIMD2<Float> = .zero,
        boneWeights: BoneWeights = BoneWeights()
    ) {
        let vertex = SkinnedVertex(
            position: position,
            normal: normal,
            uv: uv,
            boneWeights: boneWeights
        )
        vertices.append(vertex)
    }
    
    /// Set bone weights for a vertex
    mutating func setVertexWeights(_ vertexIndex: Int, weights: BoneWeights) {
        guard vertexIndex < vertices.count else { return }
        vertices[vertexIndex].boneWeights = weights.normalized()
    }
    
    // MARK: - Deformation
    
    /// Calculate deformed vertex positions based on skeleton pose
    func deformedVertices(skeleton: Skeleton) -> [SIMD3<Float>] {
        return vertices.map { vertex in
            deformVertex(vertex, skeleton: skeleton)
        }
    }
    
    private func deformVertex(_ vertex: SkinnedVertex, skeleton: Skeleton) -> SIMD3<Float> {
        var result = SIMD3<Float>.zero
        let weights = vertex.boneWeights
        
        // Apply weighted bone transforms
        for i in 0..<4 {
            let boneIndex = weights.boneIndices[i]
            let weight = weights.weights[i]
            
            guard weight > 0,
                  let boneID = boneMapping[boneIndex],
                  let worldTransform = skeleton.worldTransforms[boneID] else {
                continue
            }
            
            let inverseBindPose = skeleton.inverseBindPose(for: boneID)
            let skinMatrix = worldTransform * inverseBindPose
            
            let pos4 = SIMD4<Float>(vertex.position.x, vertex.position.y, vertex.position.z, 1)
            let transformed = skinMatrix * pos4
            
            result += SIMD3<Float>(transformed.x, transformed.y, transformed.z) * weight
        }
        
        return result
    }
}

// MARK: - Skinned Vertex

struct SkinnedVertex {
    var position: SIMD3<Float>
    var normal: SIMD3<Float>
    var uv: SIMD2<Float>
    var boneWeights: BoneWeights
    
    init(
        position: SIMD3<Float>,
        normal: SIMD3<Float> = SIMD3<Float>(0, 1, 0),
        uv: SIMD2<Float> = .zero,
        boneWeights: BoneWeights = BoneWeights()
    ) {
        self.position = position
        self.normal = normal
        self.uv = uv
        self.boneWeights = boneWeights
    }
}

// MARK: - Bone Weights

/// Stores up to 4 bone influences per vertex
struct BoneWeights {
    var boneIndices: [Int] = [0, 0, 0, 0]
    var weights: [Float] = [0, 0, 0, 0]
    
    init() {}
    
    init(bones: [(index: Int, weight: Float)]) {
        // Sort by weight descending, take top 4
        let sorted = bones.sorted { $0.weight > $1.weight }.prefix(4)
        
        for (i, bone) in sorted.enumerated() {
            boneIndices[i] = bone.index
            weights[i] = bone.weight
        }
        
        // Normalize
        let _ = normalized()
    }
    
    /// Ensure weights sum to 1.0
    func normalized() -> BoneWeights {
        var result = self
        let sum = weights.reduce(0, +)
        
        if sum > 0 {
            for i in 0..<4 {
                result.weights[i] = weights[i] / sum
            }
        }
        
        return result
    }
    
    /// Get dominant bone index
    var dominantBone: Int {
        var maxWeight: Float = 0
        var maxIndex = 0
        
        for i in 0..<4 {
            if weights[i] > maxWeight {
                maxWeight = weights[i]
                maxIndex = boneIndices[i]
            }
        }
        
        return maxIndex
    }
}

// MARK: - Mesh Wrapper

/// High-level wrapper for attaching meshes to skeletons
class MeshWrapper: ObservableObject, Identifiable {
    let id: UUID
    @Published var name: String
    @Published var mesh: SkinnedMesh
    @Published var skeletonID: UUID?
    
    // Multiple mesh parts for characters
    @Published var parts: [MeshPart] = []
    
    init(id: UUID = UUID(), name: String = "Character Mesh") {
        self.id = id
        self.name = name
        self.mesh = SkinnedMesh(name: name)
    }
    
    // MARK: - Auto Weight Generation
    
    /// Generate bone weights based on vertex proximity to bones
    func generateWeightsFromBoneProximity(skeleton: Skeleton, maxInfluences: Int = 4, falloff: Float = 2.0) {
        let boneIDs = skeleton.allBoneIDsDepthFirst()
        
        // Create bone index mapping
        var indexMapping: [UUID: Int] = [:]
        for (index, boneID) in boneIDs.enumerated() {
            indexMapping[boneID] = index
            mesh.boneMapping[index] = boneID
        }
        
        mesh.skeletonID = skeleton.id
        
        // For each vertex, calculate weights based on distance to bones
        for vertexIndex in 0..<mesh.vertices.count {
            let vertexPos = mesh.vertices[vertexIndex].position
            var influences: [(index: Int, weight: Float)] = []
            
            for boneID in boneIDs {
                let bonePos = skeleton.boneWorldPosition(boneID)
                let tailPos = skeleton.boneTailWorldPosition(boneID)
                
                // Distance to bone segment
                let distance = distanceToLineSegment(point: vertexPos, start: bonePos, end: tailPos)
                
                // Weight based on distance (inverse falloff)
                let weight = 1.0 / pow(max(distance, 0.001), falloff)
                
                if let index = indexMapping[boneID] {
                    influences.append((index: index, weight: weight))
                }
            }
            
            // Take top N influences
            let topInfluences = Array(influences.sorted { $0.weight > $1.weight }.prefix(maxInfluences))
            mesh.setVertexWeights(vertexIndex, weights: BoneWeights(bones: topInfluences))
        }
        
        print("[MeshWrapper] Generated weights for \(mesh.vertices.count) vertices from \(boneIDs.count) bones")
    }
    
    private func distanceToLineSegment(point: SIMD3<Float>, start: SIMD3<Float>, end: SIMD3<Float>) -> Float {
        let line = end - start
        let len = length(line)
        
        if len < 0.0001 {
            return length(point - start)
        }
        
        let t = max(0, min(1, dot(point - start, line) / (len * len)))
        let projection = start + t * line
        
        return length(point - projection)
    }
}

// MARK: - Mesh Part

struct MeshPart: Identifiable {
    let id: UUID
    var name: String
    var meshPath: String
    var materialPath: String
    var isVisible: Bool = true
    
    init(name: String, meshPath: String = "", materialPath: String = "") {
        self.id = UUID()
        self.name = name
        self.meshPath = meshPath
        self.materialPath = materialPath
    }
}

// MARK: - Skinned Mesh Manager

class SkinnedMeshManager: ObservableObject {
    static let shared = SkinnedMeshManager()
    
    @Published var meshWrappers: [UUID: MeshWrapper] = [:]
    @Published var activeMeshID: UUID?
    
    var activeMesh: MeshWrapper? {
        guard let id = activeMeshID else { return nil }
        return meshWrappers[id]
    }
    
    func createMeshWrapper(name: String = "New Mesh") -> MeshWrapper {
        let wrapper = MeshWrapper(name: name)
        meshWrappers[wrapper.id] = wrapper
        activeMeshID = wrapper.id
        return wrapper
    }
}
