import Foundation
import simd

public struct NetworkStateComponent: Component {
    public var lastServerPosition: SIMD3<Float>
    public var lastServerTimestamp: TimeInterval
    public var predictedPosition: SIMD3<Float>
    
    public init(position: SIMD3<Float>, timestamp: TimeInterval) {
        self.lastServerPosition = position
        self.lastServerTimestamp = timestamp
        self.predictedPosition = position
    }
}

@MainActor
public final class NetworkInterpolationSystem {
    public static let shared = NetworkInterpolationSystem()
    
    private init() {}
    
    public func update(deltaTime: Double, ecs: ECSManager) async {
        let querySig: Set<String> = ["TransformComponent", "NetworkStateComponent"]
        let entities = await ecs.query(signature: querySig)
        
        for (_, components) in entities {
            guard var transform = components["TransformComponent"] as? TransformComponent,
                  let netState = components["NetworkStateComponent"] as? NetworkStateComponent else { continue }
            
            // Linear Interpolation: Lerp from current position towards server position
            // This is a simplified version of the logic
            let t = Float(min(1.0, deltaTime * 10.0)) // 10Hz/100ms smoothing
            transform.position = mix(transform.position, netState.lastServerPosition, t)
        }
    }
    
    private func mix(_ a: SIMD3<Float>, _ b: SIMD3<Float>, _ t: Float) -> SIMD3<Float> {
        return a * (1.0 - t) + b * t
    }
}
