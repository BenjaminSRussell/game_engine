import SwiftUI
import Combine

/// NovaBlueprints: Visual Scripting Data Architecture

enum BlueprintPinType: String, Codable {
    case exec    // Thick white wire for execution flow
    case bool    // Red
    case float   // Cyan
    case int     // Green
    case string  // Magenta
    case vector  // Yellow
    case entity  // Blue
}

enum BlueprintNodeType: String, CaseIterable, Identifiable {
    // Events
    case eventBeginPlay = "On Begin Play"
    case eventTick = "On Tick"
    case eventInteract = "On Interact"
    
    // Control Flow
    case branch = "Branch (If)"
    case sequence = "Sequence"
    case forLoop = "For Loop"
    case delay = "Delay"
    
    // Actions
    case setPosition = "Set Position"
    case playSound = "Play Sound"
    case spawnEntity = "Spawn Entity"
    case logMessage = "Log Message"
    
    // Variables
    case getVar = "Get Variable"
    case setVar = "Set Variable"
    
    var id: String { self.rawValue }
    
    var category: String {
        switch self {
        case .eventBeginPlay, .eventTick, .eventInteract: return "Events"
        case .branch, .sequence, .forLoop, .delay: return "Control Flow"
        case .setPosition, .playSound, .spawnEntity, .logMessage: return "Actions"
        case .getVar, .setVar: return "Variables"
        }
    }
}

struct BlueprintPin: Identifiable {
    let id = UUID()
    let name: String
    let type: BlueprintPinType
    let isOutput: Bool
}

class BlueprintNode: ObservableObject, Identifiable {
    let id = UUID()
    let type: BlueprintNodeType
    @Published var position: CGPoint
    @Published var properties: [String: Any] = [:]
    @Published var isSelected: Bool = false
    @Published var isExecuting: Bool = false
    
    var inputs: [BlueprintPin] = []
    var outputs: [BlueprintPin] = []
    
    init(type: BlueprintNodeType, position: CGPoint) {
        self.type = type
        self.position = position
        setupPins()
    }
    
    private func setupPins() {
        switch type {
        case .eventBeginPlay:
            outputs.append(BlueprintPin(name: "", type: .exec, isOutput: true))
            
        case .branch:
            inputs.append(BlueprintPin(name: "", type: .exec, isOutput: false))
            inputs.append(BlueprintPin(name: "Condition", type: .bool, isOutput: false))
            outputs.append(BlueprintPin(name: "True", type: .exec, isOutput: true))
            outputs.append(BlueprintPin(name: "False", type: .exec, isOutput: true))
            
        case .logMessage:
            inputs.append(BlueprintPin(name: "", type: .exec, isOutput: false))
            inputs.append(BlueprintPin(name: "Message", type: .string, isOutput: false))
            outputs.append(BlueprintPin(name: "", type: .exec, isOutput: true))
            
        case .setPosition:
            inputs.append(BlueprintPin(name: "", type: .exec, isOutput: false))
            inputs.append(BlueprintPin(name: "Target", type: .entity, isOutput: false))
            inputs.append(BlueprintPin(name: "Position", type: .vector, isOutput: false))
            outputs.append(BlueprintPin(name: "", type: .exec, isOutput: true))
            
        default: break
        }
    }
}

struct BlueprintLink: Identifiable {
    let id = UUID()
    let fromNodeID: UUID
    let fromPinID: UUID
    let toNodeID: UUID
    let toPinID: UUID
    let type: BlueprintPinType
}

struct BlueprintVariable: Identifiable {
    let id = UUID()
    var name: String
    var type: BlueprintPinType
    var value: Any // This would ideally be a more specific enum or protocol
}

class BlueprintGraph: ObservableObject {
    @Published var nodes: [BlueprintNode] = []
    @Published var links: [BlueprintLink] = []
    @Published var variables: [BlueprintVariable] = []
    @Published var selectedNodeID: UUID?
    @Published var targetEntityID: String = "ENTITY_127"
    
    func addNode(type: BlueprintNodeType, at position: CGPoint) {
        let node = BlueprintNode(type: type, position: position)
        nodes.append(node)
    }
    
    func simulate() {
        // Simple simulation: highlight nodes in execution order
        Task {
            for node in nodes {
                await MainActor.run { node.isExecuting = true }
                try? await Task.sleep(nanoseconds: 500_000_000)
                await MainActor.run { node.isExecuting = false }
            }
        }
    }
    
    func connect(fromNode: BlueprintNode, fromPin: BlueprintPin, toNode: BlueprintNode, toPin: BlueprintPin) {
        // Validation: Types must match, Exec can only have one output link (usually)
        let link = BlueprintLink(fromNodeID: fromNode.id, fromPinID: fromPin.id, toNodeID: toNode.id, toPinID: toPin.id, type: fromPin.type)
        links.append(link)
    }
}
