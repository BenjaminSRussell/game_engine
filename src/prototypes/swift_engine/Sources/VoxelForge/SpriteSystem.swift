import Foundation
import simd

public struct SpriteComponent: Component {
    public var textureIndex: UInt32
    public var currentFrame: Int
    public var frameCount: Int
    public var animationTimer: Float
    public var frameDuration: Float
    
    public init(textureIndex: UInt32, frameCount: Int, frameDuration: Float = 0.1) {
        self.textureIndex = textureIndex
        self.currentFrame = 0
        self.frameCount = frameCount
        self.animationTimer = 0
        self.frameDuration = frameDuration
    }
}

@MainActor
public final class SpriteSystem {
    public static let shared = SpriteSystem()
    
    private init() {}
    
    public func update(deltaTime: Float, ecs: ECSManager) async {
        let querySig: Set<String> = ["SpriteComponent"]
        let entities = await ecs.query(signature: querySig)
        
        for (_, components) in entities {
            guard var sprite = components["SpriteComponent"] as? SpriteComponent else { continue }
            
            sprite.animationTimer += deltaTime
            if sprite.animationTimer >= sprite.frameDuration {
                sprite.animationTimer = 0
                sprite.currentFrame = (sprite.currentFrame + 1) % sprite.frameCount
            }
            
            // In a real implementation, we would update the component back in the ECS
        }
    }
}

public struct AttachmentComponent: Component {
    public var parentID: EntityID
    public var socketOffset: SIMD3<Float>
    
    public init(parentID: EntityID, socketOffset: SIMD3<Float> = .zero) {
        self.parentID = parentID
        self.socketOffset = socketOffset
    }
}
