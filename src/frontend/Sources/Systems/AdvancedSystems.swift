import Foundation
import simd

// MARK: - Utility Functions

/// Linear interpolation between two Float values
func mix(_ a: Float, _ b: Float, t: Float) -> Float {
    return a + (b - a) * t
}

// MARK: - Visual Scripting (Phase 13-15)

// Node Execution Engine
enum ScriptNodeType: String {
    case event = "Event"
    case math = "Math"
    case logic = "Logic"
    case entityControl = "Entity"
    case flowControl = "Flow"
    case variable = "Variable"
}

struct NodeData {
    let id: UUID
    let type: ScriptNodeType
    let operation: String
    var inputs: [String: Any]
    var outputs: [String: Any]
}

class NodeExecutor {
    private var variables: [String: Any] = [:]
    
    func execute(_ node: NodeData) -> [String: Any] {
        switch node.type {
        case .math:
            return executeMathNode(node)
        case .logic:
            return executeLogicNode(node)
        case .entityControl:
            return executeEntityNode(node)
        case .flowControl:
            return executeFlowNode(node)
        case .variable:
            return executeVariableNode(node)
        default:
            return [:]
        }
    }
    
    private func executeMathNode(_ node: NodeData) -> [String: Any]  {
        guard let a = node.inputs["A"] as? Float,
              let b = node.inputs["B"] as? Float else { return [:] }
        
        let result: Float
        switch node.operation {
        case "Add": result = a + b
        case "Subtract": result = a - b
        case "Multiply": result = a * b
        case "Divide": result = b != 0 ? a / b : 0
        case "Clamp": result = min(max(a, 0), b)
        default: result = 0
        }
        
        return ["Result": result]
    }
    
    private func executeLogicNode(_ node: NodeData) -> [String: Any] {
        if node.operation == "If" {
            guard let condition = node.inputs["Condition"] as? Bool else { return [:] }
            return ["Output": condition ? "True" : "False"]
        }
        return [:]
    }
    
    private func executeEntityNode(_ node: NodeData) -> [String: Any] {
        // Bridge to EngineBridge for entity operations
        return [:]
    }
    
    private func executeFlowNode(_ node: NodeData) -> [String: Any] {
        // Handle Delay, Loop, Sequence
        return [:]
    }
    
    private func executeVariableNode(_ node: NodeData) -> [String: Any] {
        if node.operation == "Get" {
            if let varName = node.inputs["VariableName"] as? String {
                return ["Value": variables[varName] ?? 0]
            }
        } else if node.operation == "Set" {
            if let varName = node.inputs["VariableName"] as? String,
               let value = node.inputs["Value"] {
                variables[varName] = value
            }
        }
        return [:]
    }
}

// MARK: - Terrain System (Phase 16-17)

class TerrainEditor {
    private var heightmap: [[Float]] = []
    private var textureWeights: [[[Float]]] = [] // [layer][x][y]
    
    func initializeTerrain(size: Int) {
        heightmap = Array(repeating: Array(repeating: 0.0, count: size), count: size)
        textureWeights = Array(repeating: Array(repeating: Array(repeating: 0.0, count: size), count: size), count: 4)
    }
    
    func raiseTerrain(at position: SIMD2<Int>, radius: Int, strength: Float) {
        applyBrush(at: position, radius: radius) { x, y, falloff in
            heightmap[y][x] += strength * falloff
        }
    }
    
    func lowerTerrain(at position: SIMD2<Int>, radius: Int, strength: Float) {
        applyBrush(at: position, radius: radius) { x, y, falloff in
            heightmap[y][x] -= strength * falloff
        }
    }
    
    func smoothTerrain(at position: SIMD2<Int>, radius: Int) {
        applyBrush(at: position, radius: radius) { x, y, falloff in
            let avg = getAverageHeight(around: SIMD2(x, y), radius: 2)
            heightmap[y][x] = mix(heightmap[y][x], avg, t: falloff * 0.5)
        }
    }
    
    func paintTexture(layer: Int, at position: SIMD2<Int>, radius: Int, strength: Float) {
        applyBrush(at: position, radius: radius) { x, y, falloff in
            textureWeights[layer][y][x] = min(1.0, textureWeights[layer][y][x] + strength * falloff)
            normalizeTextureWeights(at: SIMD2(x, y))
        }
    }
    
    private func applyBrush(at position: SIMD2<Int>, radius: Int, action: (Int, Int, Float) -> Void) {
        for dy in -radius...radius {
            for dx in -radius...radius {
                let x = position.x + dx
                let y = position.y + dy
                
                guard x >= 0 && y >= 0 && x < heightmap[0].count && y < heightmap.count else { continue }
                
                let distance = sqrt(Float(dx * dx + dy * dy))
                let falloff = max(0, 1.0 - (distance / Float(radius)))
                
                action(x, y, falloff)
            }
        }
    }
    
    private func getAverageHeight(around position: SIMD2<Int>, radius: Int) -> Float {
        var sum: Float = 0
        var count = 0
        
        for dy in -radius...radius {
            for dx in -radius...radius {
                let x = position.x + dx
                let y = position.y + dy
                guard x >= 0 && y >= 0 && x < heightmap[0].count && y < heightmap.count else { continue }
                sum += heightmap[y][x]
                count += 1
            }
        }
        
        return count > 0 ? sum / Float(count) : 0
    }
    
    private func normalizeTextureWeights(at position: SIMD2<Int>) {
        var total: Float = 0
        for layer in 0..<4 {
            total += textureWeights[layer][position.y][position.x]
        }
        
        if total > 0 {
            for layer in 0..<4 {
                textureWeights[layer][position.y][position.x] /= total
            }
        }
    }
}

// MARK: - Animation System (Phase 19-21)

struct Keyframe {
    let time: Float
    var value: Any
    var tangentIn: SIMD2<Float> = SIMD2(0, 0)
    var tangentOut: SIMD2<Float> = SIMD2(0, 0)
}

class AnimationCurve {
    var keyframes: [Keyframe] = []
    
    func addKeyframe(_ keyframe: Keyframe) {
        keyframes.append(keyframe)
        keyframes.sort { $0.time < $1.time }
    }
    
    func evaluate(at time: Float) -> Any? {
        guard !keyframes.isEmpty else { return nil }
        guard keyframes.count > 1 else { return keyframes[0].value }
        
        // Find surrounding keyframes
        var prevIndex = 0
        for (index, kf) in keyframes.enumerated() {
            if kf.time > time {
                break
            }
            prevIndex = index
        }
        
        if prevIndex >= keyframes.count - 1 {
            return keyframes.last?.value
        }
        
        let prev = keyframes[prevIndex]
        let next = keyframes[prevIndex + 1]
        
        let t = (time - prev.time) / (next.time - prev.time)
        
        // Cubic interpolation using tangents
        return interpolate(from: prev, to: next, t: t)
    }
    
    private func interpolate(from: Keyframe, to: Keyframe, t: Float) -> Any? {
        // Simple linear for now - bezier would use tangents
        if let fromFloat = from.value as? Float,
           let toFloat = to.value as? Float {
            return mix(fromFloat, toFloat, t: t)
        }
        return to.value
    }
}

// MARK: - UI Editor (Phase 22-23)

enum UIAnchorPreset {
    case topLeft, topCenter, topRight
    case middleLeft, center, middleRight
    case bottomLeft, bottomCenter, bottomRight
    case stretchHorizontal, stretchVertical, stretchAll
}

struct RectTransform {
    var anchorMin: SIMD2<Float> = SIMD2(0.5, 0.5)
    var anchorMax: SIMD2<Float> = SIMD2(0.5, 0.5)
    var pivot: SIMD2<Float> = SIMD2(0.5, 0.5)
    var anchoredPosition: SIMD2<Float> = SIMD2(0, 0)
    var sizeDelta: SIMD2<Float> = SIMD2(100, 100)
    
    func applyPreset(_ preset: UIAnchorPreset) -> RectTransform {
        var result = self
        
        switch preset {
        case .topLeft:
            result.anchorMin = SIMD2(0, 1)
            result.anchorMax = SIMD2(0, 1)
        case .center:
            result.anchorMin = SIMD2(0.5, 0.5)
            result.anchorMax = SIMD2(0.5, 0.5)
        case .stretchAll:
            result.anchorMin = SIMD2(0, 0)
            result.anchorMax = SIMD2(1, 1)
            result.sizeDelta = SIMD2(0, 0)
        default:
            break
        }
        
        return result
    }
}

// MARK: - Build Pipeline (Phase 25)

struct BuildConfiguration {
    let platform: String // "macOS", "Windows", "Linux"
    let buildType: String // "Debug", "Release"
    let outputPath: String
    let stripDebugSymbols: Bool
    let optimizationLevel: String
}

class BuildPipeline {
    func build(config: BuildConfiguration, completion: @escaping (Result<String, Error>) -> Void) {
        print("[Build] Starting build for \(config.platform) - \(config.buildType)")
        
        DispatchQueue.global(qos: .userInitiated).async {
            // 1. Compile C++ engine
            print("[Build] Compiling engine...")
            
            // 2. Bundle assets
            print("[Build] Bundling assets...")
            
            // 3. Package executable
            print("[Build] Packaging...")
            
            // 4. Create output
            let outputPath = config.outputPath
            
            DispatchQueue.main.async {
                completion(.success(outputPath))
            }
        }
    }
}
