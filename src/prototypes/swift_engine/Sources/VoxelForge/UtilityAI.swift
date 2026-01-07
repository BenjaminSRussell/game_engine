import Foundation

public protocol Consideration: Sendable {
    func score(entity: EntityID, ecs: ECSManager) async -> Float
}

public struct HungerConsideration: Consideration {
    public func score(entity: EntityID, ecs: ECSManager) async -> Float {
        // Return higher score if entity is hungry
        return 0.5
    }
}

@MainActor
public final class UtilityAI {
    public static let shared = UtilityAI()
    
    private init() {}
    
    public func decide(entity: EntityID, options: [Consideration], ecs: ECSManager) async -> Consideration? {
        var bestOption: Consideration?
        var highestScore: Float = -1.0
        
        for option in options {
            let score = await option.score(entity: entity, ecs: ecs)
            if score > highestScore {
                highestScore = score
                bestOption = option
            }
        }
        
        return bestOption
    }
}
