import Foundation
import UIKit
import SwiftUI

/// Comprehensive HUD management system for iOS
class HUDSystem: ObservableObject {
    
    static let shared = HUDSystem()
    
    // MARK: - Types
    
    enum HUDAnchor {
        case topLeft, topCenter, topRight
        case centerLeft, center, centerRight
        case bottomLeft, bottomCenter, bottomRight
    }
    
    struct HUDLayout {
        var screenSize: CGSize
        var safeAreaInsets: UIEdgeInsets
        var dpiScale: CGFloat
    }
    
    // MARK: - Published State
    @Published var layout: HUDLayout
    @Published var elements: [HUDElement] = []
    @Published var debugMode: Bool = false
    
    private var elementRegistry: [String: HUDElement] = [:]
    
    // MARK: - Initialization
    
    private init() {
        // Initialize with default screen size
        let screen = UIScreen.main.bounds.size
        let insets = UIApplication.shared.windows.first?.safeAreaInsets ?? .zero
        let scale = UIScreen.main.scale
        
        layout = HUDLayout(
            screenSize: screen,
            safeAreaInsets: insets,
            dpiScale: scale
        )
        
        registerDefaultElements()
    }
    
    // MARK: - Public API
    
    /// Update screen size and safe area
    func updateLayout(screenSize: CGSize, safeArea: UIEdgeInsets) {
        layout.screenSize = screenSize
        layout.safeAreaInsets = safeArea
        recalculatePositions()
    }
    
    /// Register a new HUD element
    func registerElement(_ element: HUDElement) {
        elementRegistry[element.id]! = element
        elements.append(element)
        recalculatePositions()
    }
    
    /// Remove HUD element
    func removeElement(id: String) {
        elementRegistry.removeValue(forKey: id)
        elements.removeAll { $0.id == id }
    }
    
    /// Set element visibility
    func setElementVisible(id: String, visible: Bool) {
        if let index = elements.firstIndex(where: { $0.id == id }) {
            elements[index].isVisible = visible
        }
    }
    
    /// Calculate position for an element based on its anchor
    func calculatePosition(for element: HUDElement) -> CGPoint {
        let screen = layout.screenSize
        let insets = layout.safeAreaInsets
        let offset = element.offset
        
        // Base position from anchor
        var position = CGPoint.zero
        
        switch element.anchor {
        case .topLeft:
            position = CGPoint(x: insets.left, y: insets.top)
        case .topCenter:
            position = CGPoint(x: screen.width / 2, y: insets.top)
        case .topRight:
            position = CGPoint(x: screen.width - insets.right, y: insets.top)
        case .centerLeft:
            position = CGPoint(x: insets.left, y: screen.height / 2)
        case .center:
            position = CGPoint(x: screen.width / 2, y: screen.height / 2)
        case .centerRight:
            position = CGPoint(x: screen.width - insets.right, y: screen.height / 2)
        case .bottomLeft:
            position = CGPoint(x: insets.left, y: screen.height - insets.bottom)
        case .bottomCenter:
            position = CGPoint(x: screen.width / 2, y: screen.height - insets.bottom)
        case .bottomRight:
            position = CGPoint(x: screen.width - insets.right, y: screen.height - insets.bottom)
        }
        
        // Apply offset with DPI scaling
        position.x += offset.x * layout.dpiScale
        position.y += offset.y * layout.dpiScale
        
        return position
    }
    
    /// Scale value for current DPI
    func scaleForDPI(_ value: CGFloat) -> CGFloat {
        return value * layout.dpiScale
    }
    
    // MARK: - Private Methods
    
    private func registerDefaultElements() {
        // Health bar
        let healthBar = HUDElement(
            id: "health_bar",
            type: .healthBar,
            anchor: .bottomLeft,
            offset: CGPoint(x: 10, y: -60),
            size: CGSize(width: 200, height: 20),
            zOrder: 10
        )
        registerElement(healthBar)
        
        // Hunger bar
        let hungerBar = HUDElement(
            id: "hunger_bar",
            type: .hungerBar,
            anchor: .bottomRight,
            offset: CGPoint(x: -210, y: -60),
            size: CGSize(width: 200, height: 20),
            zOrder: 10
        )
        registerElement(hungerBar)
        
        // Crosshair
        let crosshair = HUDElement(
            id: "crosshair",
            type: .crosshair,
            anchor: .center,
            offset: .zero,
            size: CGSize(width: 32, height: 32),
            zOrder: 100
        )
        registerElement(crosshair)
        
        // Hotbar
        let hotbar = HUDElement(
            id: "hotbar",
            type: .hotbar,
            anchor: .bottomCenter,
            offset: CGPoint(x: 0, y: -10),
            size: CGSize(width: 360, height: 40),
            zOrder: 50
        )
        registerElement(hotbar)
    }
    
    private func recalculatePositions() {
        objectWillChange.send()
    }
}

// MARK: - HUD Element

struct HUDElement: Identifiable {
    let id: String
    let type: HUDElementType
    var anchor: HUDSystem.HUDAnchor
    var offset: CGPoint
    var size: CGSize
    var zOrder: Int
    var isVisible: Bool = true
    var alpha: CGFloat = 1.0
    var data: HUDElementData?
    
    init(id: String, type: HUDElementType, anchor: HUDSystem.HUDAnchor, offset: CGPoint, size: CGSize, zOrder: Int) {
        self.id = id
        self.type = type
        self.anchor = anchor
        self.offset = offset
        self.size = size
        self.zOrder = zOrder
    }
}

enum HUDElementType {
    case healthBar
    case hungerBar
    case hotbar
    case crosshair
    case minimap
    case statusEffects
    case notifications
    case custom
}

struct HUDElementData {
    var health: Float = 100.0
    var maxHealth: Float = 100.0
    var hunger: Float = 100.0
    var maxHunger: Float = 100.0
    var selectedSlot: Int = 0
}

// MARK: - SwiftUI View

struct HUDView: View {
    @ObservedObject var hudSystem = HUDSystem.shared
    
    var body: some View {
        ZStack {
            ForEach(hudSystem.elements.sorted(by: { $0.zOrder < $1.zOrder })) { element in
                if element.isVisible {
                    renderElement(element)
                        .position(hudSystem.calculatePosition(for: element))
                        .opacity(element.alpha)
                }
            }
            
            if hudSystem.debugMode {
                debugOverlay
            }
        }
        .ignoresSafeArea()
    }
    
    @ViewBuilder
    private func renderElement(_ element: HUDElement) -> some View {
        switch element.type {
        case .healthBar:
            HealthBarView(data: element.data ?? HUDElementData())
                .frame(width: element.size.width, height: element.size.height)
        case .hungerBar:
            HungerBarView(data: element.data ?? HUDElementData())
                .frame(width: element.size.width, height: element.size.height)
        case .hotbar:
            HotbarView(data: element.data ?? HUDElementData())
                .frame(width: element.size.width, height: element.size.height)
        case .crosshair:
            CrosshairView()
                .frame(width: element.size.width, height: element.size.height)
        default:
            EmptyView()
        }
    }
    
    private var debugOverlay: some View {
        VStack {
            Text("HUD Debug")
                .font(.caption)
                .padding(4)
                .background(Color.black.opacity(0.7))
                .foregroundColor(.white)
                .cornerRadius(4)
            
            Spacer()
        }
    }
}
