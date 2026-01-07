import Foundation

public enum AIState: String, Codable, Sendable {
    case idle
    case wandering
    case chasing
    case escaping
}

public struct AIComponent: Component {
    public var currentState: AIState
    public var targetEntity: EntityID?
    
    public init(state: AIState = .idle) {
        self.currentState = state
    }
}

@MainActor
public final class AISystem {
    public static let shared = AISystem()
    
    private init() {}
    
    public func update(deltaTime: Float, ecs: ECSManager) async {
        let querySig: Set<String> = ["AIComponent", "TransformComponent"]
        let entities = await ecs.query(signature: querySig)
        
        for (_, components) in entities {
            guard var ai = components["AIComponent"] as? AIComponent else { continue }
            
            _ = components["TransformComponent"] as? TransformComponent
            
            switch ai.currentState {
            case .idle:
                // Chance to switch to wandering
                if Float.random(in: 0...1) < 0.01 {
                    ai.currentState = .wandering
                }
            case .wandering:
                // Logic to move towards a random point
                break
            case .chasing:
                // Logic to move towards targetEntity
                break
            case .escaping:
                // Logic to move away from danger
                break
            }
            
            // In a real implementation, we would update the component back in the ECS
        }
    }
}
