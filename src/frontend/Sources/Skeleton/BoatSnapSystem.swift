import SwiftUI
import simd

// MARK: - Snap Point System
/// Modular attachment points for boat components

enum SnapPointType: String, CaseIterable, Identifiable {
    case propeller = "Propeller Mount"
    case rudder = "Rudder Post"
    case motor = "Motor Mount"
    case mast = "Mast Step"
    case helm = "Helm Station"
    case seat = "Seat Mount"
    case cleat = "Cleat"
    case anchor = "Anchor Roller"
    case cabin = "Cabin Mount"
    case storage = "Storage Compartment"
    case deck = "Deck Equipment"
    
    var id: String { rawValue }
    
    var color: Color {
        switch self {
        case .propeller: return .orange
        case .rudder: return .blue
        case .motor: return .red
        case .mast: return .brown
        case .helm: return .purple
        case .seat: return .green
        case .cleat, .anchor: return .cyan
        case .cabin: return .yellow
        case .storage: return .gray
        case .deck: return .orange
        }
    }
    
    var icon: String {
        switch self {
        case .propeller: return "fan.fill"
        case .rudder: return "arrow.down.circle.fill"
        case .motor: return "engine.combustion.fill"
        case .mast: return "arrow.up.circle.fill"
        case .helm: return "steeringwheel"
        case .seat: return "chair.fill"
        case .cleat: return "link.circle.fill"
        case .anchor: return "anchor"
        case .cabin: return "house.fill"
        case .storage: return "archivebox.fill"
        case .deck: return "lifepreserver.fill"
        }
    }
    
    var snapRadius: Float {
        switch self {
        case .propeller, .rudder, .motor: return 0.2  // Precise positioning
        case .mast: return 0.3
        default: return 0.5  // More forgiving for interior items
        }
    }
}

struct SnapPoint: Identifiable {
    let id = UUID()
    var type: SnapPointType
    var localPosition: SIMD3<Float>
    var localRotation: simd_quatf = simd_quatf(angle: 0, axis: [0, 1, 0])
    var isOccupied: Bool = false
    var occupyingComponentID: UUID?
    var autoCreateBone: Bool = true  // Auto-create skeleton bone when component snaps
    
    var indicatorColor: Color {
        isOccupied ? .gray : type.color
    }
}

// MARK: - Boat Component Base
/// Base protocol for all snap-on boat components

protocol BoatComponent: Identifiable {
    var id: UUID { get }
    var name: String { get }
    var snapType: SnapPointType { get }
    var linkedBoneID: UUID? { get set }
    var modelAsset: String { get }
    var mass: Float { get }
}

// MARK: - Interior Assets
/// Snap-on interior components for boat building

struct InteriorAsset: BoatComponent, Identifiable {
    let id = UUID()
    var name: String
    var snapType: SnapPointType
    var linkedBoneID: UUID?
    var modelAsset: String
    var mass: Float = 10.0
    var category: AssetCategory
    var dimensions: SIMD3<Float> = SIMD3<Float>(0.5, 0.5, 0.5)
    var snapToFloor: Bool = true
    var snapToGrid: Bool = true
    var gridSize: Float = 0.25  // 25cm grid
    
    enum AssetCategory: String, CaseIterable {
        case seating = "Seating"
        case helm = "Helm & Controls"
        case cabin = "Cabin"
        case storage = "Storage"
        case galley = "Galley (Kitchen)"
        case deck = "Deck Equipment"
        case safety = "Safety Equipment"
        case electronics = "Electronics"
    }
}

// MARK: - Boat Builder System
/// Manages snap points and component placement

class BoatBuilderSystem: ObservableObject {
    static let shared = BoatBuilderSystem()
    
    @Published var snapPoints: [SnapPoint] = []
    @Published var placedComponents: [UUID: any BoatComponent] = [:]
    @Published var highlightedSnapPoint: UUID?
    @Published var selectedHullShape: HullShape = .deepV
    @Published var selectedPropellerType: PropellerType = .threeBladeFixed
    
    /// Find nearest available snap point for component
    func findNearestSnapPoint(position: SIMD3<Float>, type: SnapPointType) -> SnapPoint? {
        snapPoints
            .filter { $0.type == type && !$0.isOccupied }
            .min(by: {
                distance($0.localPosition, position) < distance($1.localPosition, position)
            })
    }
    
    /// Snap component to point and create bone
    func snapComponent(_ component: inout any BoatComponent, to snapPoint: SnapPoint) {
        guard let idx = snapPoints.firstIndex(where: { $0.id == snapPoint.id }) else { return }
        
        // Auto-create bone at snap point
        if snapPoint.autoCreateBone, let skeleton = SkeletonManager.shared.activeSkeleton {
            let boneName = "\(snapPoint.type.rawValue)_\(component.name)"
            let boneID = skeleton.addBone(
                name: boneName,
                position: snapPoint.localPosition,
                length: 0.3
            )
            component.linkedBoneID = boneID
        }
        
        // Mark snap point as occupied
        snapPoints[idx].isOccupied = true
        snapPoints[idx].occupyingComponentID = component.id
        
        // Add to placed components
        placedComponents[component.id] = component
        
        print("[BoatBuilder] Snapped \(component.name) to \(snapPoint.type.rawValue)")
    }
    
    /// Generate default snap points based on hull shape
    func generateDefaultSnapPoints(hull: HullShape, length: Float, beam: Float) {
        snapPoints.removeAll()
        
        // Stern snap points (propeller, rudder, motor)
        let sternZ = -length / 2
        snapPoints.append(SnapPoint(
            type: .propeller,
            localPosition: SIMD3<Float>(0, -0.5, sternZ - 0.2)
        ))
        snapPoints.append(SnapPoint(
            type: .rudder,
            localPosition: SIMD3<Float>(0, -0.4, sternZ)
        ))
        snapPoints.append(SnapPoint(
            type: .motor,
            localPosition: SIMD3<Float>(0, 0.3, sternZ + 0.5)
        ))
        
        // Deck snap points (helm, seats, cleats)
        snapPoints.append(SnapPoint(
            type: .helm,
            localPosition: SIMD3<Float>(0, 1.0, length * 0.1)
        ))
        
        // Port and starboard seats
        snapPoints.append(SnapPoint(
            type: .seat,
            localPosition: SIMD3<Float>(beam * 0.3, 0.8, length * 0.1)
        ))
        snapPoints.append(SnapPoint(
            type: .seat,
            localPosition: SIMD3<Float>(-beam * 0.3, 0.8, length * 0.1)
        ))
        
        // Bow anchor
        snapPoints.append(SnapPoint(
            type: .anchor,
            localPosition: SIMD3<Float>(0, 0.5, length / 2)
        ))
        
        // Cleats (4 corners)
        for x in [-1, 1] {
            for z in [-1, 1] {
                snapPoints.append(SnapPoint(
                    type: .cleat,
                    localPosition: SIMD3<Float>(
                        Float(x) * beam * 0.4,
                        0.3,
                        Float(z) * length * 0.3
                    )
                ))
            }
        }
        
        // Mast (for sailboats)
        if hull == .catamaran || hull == .trimaran {
            snapPoints.append(SnapPoint(
                type: .mast,
                localPosition: SIMD3<Float>(0, 0.2, 0)
            ))
        }
        
        print("[BoatBuilder] Generated \(snapPoints.count) snap points for \(hull.rawValue)")
    }
    
    /// Remove component and free snap point
    func removeComponent(_ componentID: UUID) {
        guard let component = placedComponents[componentID] else { return }
        
        // Free snap point
        if let snapIdx = snapPoints.firstIndex(where: { $0.occupyingComponentID == componentID }) {
            snapPoints[snapIdx].isOccupied = false
            snapPoints[snapIdx].occupyingComponentID = nil
        }
        
        // Remove bone if it exists
        if let boneID = component.linkedBoneID,
           let skeleton = SkeletonManager.shared.activeSkeleton {
            skeleton.removeBone(boneID)
        }
        
        placedComponents.removeValue(forKey: componentID)
    }
}

// MARK: - Asset Library
/// Pre-defined interior assets for boat building

class InteriorAssetLibrary: ObservableObject {
    static let shared = InteriorAssetLibrary()
    
    @Published var assets: [InteriorAsset] = [
        // Seating
        InteriorAsset(
            name: "Captain's Chair",
            snapType: .seat,
            modelAsset: "models/interior/captains_chair",
            mass: 15,
            category: .seating,
            dimensions: SIMD3<Float>(0.6, 1.0, 0.6)
        ),
        InteriorAsset(
            name: "Bench Seat",
            snapType: .seat,
            modelAsset: "models/interior/bench",
            mass: 20,
            category: .seating,
            dimensions: SIMD3<Float>(1.5, 0.5, 0.6)
        ),
        InteriorAsset(
            name: "Fold-Down Seat",
            snapType: .seat,
            modelAsset: "models/interior/fold_seat",
            mass: 8,
            category: .seating,
            dimensions: SIMD3<Float>(0.5, 0.4, 0.5)
        ),
        
        // Helm & Controls
        InteriorAsset(
            name: "Helm Wheel",
            snapType: .helm,
            modelAsset: "models/interior/wheel",
            mass: 5,
            category: .helm,
            dimensions: SIMD3<Float>(0.4, 0.4, 0.2),
            snapToFloor: false
        ),
        InteriorAsset(
            name: "Throttle Control",
            snapType: .helm,
            modelAsset: "models/interior/throttle",
            mass: 3,
            category: .helm,
            dimensions: SIMD3<Float>(0.3, 0.3, 0.2),
            snapToFloor: false
        ),
        InteriorAsset(
            name: "Navigation Panel",
            snapType: .helm,
            modelAsset: "models/interior/nav_panel",
            mass: 8,
            category: .electronics,
            dimensions: SIMD3<Float>(0.5, 0.3, 0.1),
            snapToFloor: false
        ),
        
        // Storage
        InteriorAsset(
            name: "Storage Locker",
            snapType: .storage,
            modelAsset: "models/interior/locker",
            mass: 25,
            category: .storage,
            dimensions: SIMD3<Float>(0.6, 1.2, 0.5)
        ),
        InteriorAsset(
            name: "Cooler",
            snapType: .storage,
            modelAsset: "models/interior/cooler",
            mass: 15,
            category: .storage,
            dimensions: SIMD3<Float>(0.8, 0.5, 0.5)
        ),
        
        // Cabin
        InteriorAsset(
            name: "Bunk Bed",
            snapType: .cabin,
            modelAsset: "models/interior/bunk",
            mass: 40,
            category: .cabin,
            dimensions: SIMD3<Float>(2.0, 0.5, 0.8)
        ),
        InteriorAsset(
            name: "Table",
            snapType: .cabin,
            modelAsset: "models/interior/table",
            mass: 12,
            category: .cabin,
            dimensions: SIMD3<Float>(0.8, 0.7, 0.8)
        ),
        
        // Galley
        InteriorAsset(
            name: "Sink",
            snapType: .cabin,
            modelAsset: "models/interior/sink",
            mass: 18,
            category: .galley,
            dimensions: SIMD3<Float>(0.5, 0.3, 0.4),
            snapToFloor: false
        ),
        InteriorAsset(
            name: "Stove",
            snapType: .cabin,
            modelAsset: "models/interior/stove",
            mass: 22,
            category: .galley,
            dimensions: SIMD3<Float>(0.5, 0.3, 0.5),
            snapToFloor: false
        ),
        
        // Deck Equipment
        InteriorAsset(
            name: "Life Ring",
            snapType: .deck,
            modelAsset: "models/deck/life_ring",
            mass: 3,
            category: .safety,
            dimensions: SIMD3<Float>(0.7, 0.7, 0.1),
            snapToFloor: false
        ),
        InteriorAsset(
            name: "Fire Extinguisher",
            snapType: .cabin,
            modelAsset: "models/safety/extinguisher",
            mass: 5,
            category: .safety,
            dimensions: SIMD3<Float>(0.15, 0.4, 0.15),
            snapToFloor: false
        ),
        
        // Electronics
        InteriorAsset(
            name: "GPS Display",
            snapType: .helm,
            modelAsset: "models/electronics/gps",
            mass: 2,
            category: .electronics,
            dimensions: SIMD3<Float>(0.3, 0.2, 0.1),
            snapToFloor: false
        ),
        InteriorAsset(
            name: "Radio",
            snapType: .helm,
            modelAsset: "models/electronics/radio",
            mass: 3,
            category: .electronics,
            dimensions: SIMD3<Float>(0.25, 0.15, 0.15),
            snapToFloor: false
        ),
    ]
    
    func getAssets(for category: InteriorAsset.AssetCategory) -> [InteriorAsset] {
        assets.filter { $0.category == category }
    }
}
