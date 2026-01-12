import SwiftUI

// MARK: - UI Editor (TODO-5901 to TODO-6060)

class UIEditorManager: ObservableObject {
    static let shared = UIEditorManager()
    
    @Published var selectedElement: UUID?
    @Published var elements: [UIElement] = []
    @Published var canvasSize: CGSize = CGSize(width: 1920, height: 1080)
    @Published var zoom: CGFloat = 0.5
    @Published var showGrid: Bool = true
    @Published var snapToGrid: Bool = true
    @Published var gridSize: CGFloat = 10
    @Published var activeTool: UIEditorTool = .select
    @Published var isPlaying: Bool = false
    @Published var currentTime: Double = 0.0
    @Published var playbackSpeed: Float = 1.0
    @Published var showRulers: Bool = true
    @Published var showGuides: Bool = true
    @Published var guides: [UIGuide] = []
    @Published var layers: [UILayer] = []
    @Published var activeLayer: Int = 0
    @Published var componentLibrary: [UIComponent] = []
    @Published var eventSystem: UIEventSystem = UIEventSystem()
    @Published var animationSystem: UIAnimationSystem = UIAnimationSystem()
    @Published var layoutSystem: UILayoutSystem = UILayoutSystem()
    @Published var themeSystem: UIThemeSystem = UIThemeSystem()
    
    enum UIEditorTool: String, CaseIterable {
        case select = "Select"
        case move = "Move"
        case rotate = "Rotate"
        case scale = "Scale"
        case rectangle = "Rectangle"
        case circle = "Circle"
        case text = "Text"
        case image = "Image"
        case line = "Line"
        case pen = "Pen"
        case eyedropper = "Eyedropper"
        case zoom = "Zoom"
        case pan = "Pan"
    }
    
    init() {
        setupDefaultLayers()
        setupDefaultComponents()
        setupDefaultGuides()
    }
    
    // MARK: - Element Management
    
    func addElement(type: UIElementType) {
        let element = UIElement(type: type, name: "\(type.rawValue)_\(elements.count)", rect: CGRect(x: 100, y: 100, width: 100, height: 40))
        elements.append(element)
        selectedElement = element.id
    }
    
    func deleteSelected() {
        elements.removeAll { $0.id == selectedElement }
        selectedElement = nil
    }
    
    func duplicateSelected() {
        guard let selectedId = selectedElement,
              let original = elements.first(where: { $0.id == selectedId }) else { return }
        
        var duplicate = original
        duplicate.id = UUID()
        duplicate.name = "\(original.name)_copy"
        duplicate.rect.offsetBy(dx: 20, dy: 20)
        elements.append(duplicate)
        selectedElement = duplicate.id
    }
    
    // MARK: - Layer Management
    
    private func setupDefaultLayers() {
        layers = [
            UILayer(name: "Background", isVisible: true, isLocked: false, opacity: 1.0),
            UILayer(name: "UI Elements", isVisible: true, isLocked: false, opacity: 1.0),
            UILayer(name: "Overlay", isVisible: true, isLocked: false, opacity: 0.8)
        ]
    }
    
    func addLayer(name: String) {
        let layer = UILayer(name: name, isVisible: true, isLocked: false, opacity: 1.0)
        layers.append(layer)
        activeLayer = layers.count - 1
    }
    
    func deleteLayer(at index: Int) {
        guard layers.count > 1 else { return }
        layers.remove(at: index)
        if activeLayer >= layers.count {
            activeLayer = layers.count - 1
        }
    }
    
    // MARK: - Component System
    
    private func setupDefaultComponents() {
        componentLibrary = [
            UIComponent(name: "Button", type: .button, category: .controls),
            UIComponent(name: "Panel", type: .panel, category: .containers),
            UIComponent(name: "Text", type: .text, category: .display),
            UIComponent(name: "Image", type: .image, category: .display),
            UIComponent(name: "Slider", type: .slider, category: .controls),
            UIComponent(name: "Toggle", type: .toggle, category: .controls),
            UIComponent(name: "Progress", type: .progress, category: .display),
            UIComponent(name: "List", type: .list, category: .containers)
        ]
    }
    
    // MARK: - Guide System
    
    private func setupDefaultGuides() {
        guides = [
            UIGuide(position: 960, orientation: .vertical, color: .red),
            UIGuide(position: 540, orientation: .horizontal, color: .red)
        ]
    }
    
    func addGuide(position: Float, orientation: UIGuideOrientation) {
        let guide = UIGuide(position: position, orientation: orientation, color: .blue)
        guides.append(guide)
    }
    
    // MARK: - Animation System
    
    func createAnimation(for elementId: UUID) {
        let animation = UIAnimation(
            elementId: elementId,
            duration: 1.0,
            keyframes: [],
            easing: .easeInOut,
            isLooping: false
        )
        animationSystem.animations.append(animation)
    }
    
    func playAnimation() {
        isPlaying = true
        currentTime = 0.0
    }
    
    func stopAnimation() {
        isPlaying = false
        currentTime = 0.0
    }
    
    // MARK: - Layout System
    
    func applyAutoLayout() {
        layoutSystem.arrangeElements(elements: elements, in: canvasSize)
    }
    
    func applyGridLayout(columns: Int, spacing: CGFloat) {
        layoutSystem.gridLayout(elements: elements, columns: columns, spacing: spacing)
    }
    
    func applyFlowLayout() {
        layoutSystem.flowLayout(elements: elements, containerSize: canvasSize)
    }
}

// MARK: - Supporting Data Structures

struct UIElement: Identifiable {
    let id = UUID()
    var type: UIElementType
    var name: String
    var rect: CGRect
    var anchorMin: CGPoint = CGPoint(x: 0.5, y: 0.5)
    var anchorMax: CGPoint = CGPoint(x: 0.5, y: 0.5)
    var pivot: CGPoint = CGPoint(x: 0.5, y: 0.5)
    var rotation: CGFloat = 0
    var color: Color = .white
    var isInteractable: Bool = true
    var opacity: Float = 1.0
    var isVisible: Bool = true
    var layerId: Int = 0
    var constraints: [UIConstraint] = []
    var animations: [UUID] = []
    var customProperties: [String: Any] = [:]
}

enum UIElementType: String, CaseIterable {
    case panel = "Panel"
    case button = "Button"
    case text = "Text"
    case image = "Image"
    case slider = "Slider"
    case toggle = "Toggle"
    case progress = "Progress"
    case list = "List"
    case input = "Input"
    case scroll = "Scroll"
    case video = "Video"
    case canvas = "Canvas"
    case container = "Container"
}

struct UILayer: Identifiable {
    let id = UUID()
    var name: String
    var isVisible: Bool = true
    var isLocked: Bool = false
    var opacity: Float = 1.0
    var elements: [UUID] = []
}

struct UIGuide: Identifiable {
    let id = UUID()
    var position: Float
    var orientation: UIGuideOrientation
    var color: Color
}

enum UIGuideOrientation {
    case horizontal
    case vertical
}

struct UIComponent {
    let name: String
    let type: UIElementType
    let category: UIComponentCategory
    var icon: String
    var description: String
    var defaultProperties: [String: Any]
    
    init(name: String, type: UIElementType, category: UIComponentCategory) {
        self.name = name
        self.type = type
        self.category = category
        self.icon = type.rawValue.lowercased()
        self.description = "\(name) component for UI design"
        self.defaultProperties = [:]
    }
}

enum UIComponentCategory: String, CaseIterable {
    case controls = "Controls"
    case containers = "Containers"
    case display = "Display"
    case navigation = "Navigation"
    case input = "Input"
    case media = "Media"
}

struct UIConstraint {
    let type: UIConstraintType
    let target: UUID?
    let value: Float
    var isActive: Bool = true
}

enum UIConstraintType {
    case width
    case height
    case aspectRatio
    case minWidth
    case maxWidth
    case minHeight
    case maxHeight
    case left
    case right
    case top
    case bottom
    case centerX
    case centerY
}

// MARK: - Animation System

struct UIAnimation: Identifiable {
    let id = UUID()
    let elementId: UUID
    var duration: Double
    var keyframes: [UIKeyframe]
    var easing: UIAnimationEasing
    var isLooping: Bool
    var delay: Double = 0.0
}

struct UIKeyframe {
    let time: Double
    let properties: [String: Any]
    let easing: UIAnimationEasing?
}

enum UIAnimationEasing {
    case linear
    case easeIn
    case easeOut
    case easeInOut
    case easeInQuad
    case easeOutQuad
    case easeInOutQuad
    case easeInCubic
    case easeOutCubic
    case easeInOutCubic
}

class UIAnimationSystem: ObservableObject {
    @Published var animations: [UIAnimation] = []
    @Published var isPlaying: Bool = false
    @Published var currentTime: Double = 0.0
    
    func update(deltaTime: Double) {
        guard isPlaying else { return }
        
        currentTime += deltaTime
        
        // Update all playing animations
        for animation in animations {
            updateAnimation(animation, deltaTime: deltaTime)
        }
    }
    
    private func updateAnimation(_ animation: UIAnimation, deltaTime: Double) {
        // Animation update logic
        let normalizedTime = (currentTime.truncatingRemainder(dividingBy: animation.duration)) / animation.duration
        let easedTime = applyEasing(normalizedTime, easing: animation.easing)
        
        // Apply interpolated values to element
        // This would connect to the actual UI elements
    }
    
    private func applyEasing(_ t: Double, easing: UIAnimationEasing) -> Double {
        switch easing {
        case .linear: return t
        case .easeIn: return t * t
        case .easeOut: return 1 - (1 - t) * (1 - t)
        case .easeInOut: return t < 0.5 ? 2 * t * t : 1 - pow(-2 * t + 2, 2) / 2
        case .easeInQuad: return t * t
        case .easeOutQuad: return 1 - (1 - t) * (1 - t)
        case .easeInOutQuad: return t < 0.5 ? 2 * t * t : 1 - pow(-2 * t + 2, 2) / 2
        case .easeInCubic: return t * t * t
        case .easeOutCubic: return 1 - pow(1 - t, 3)
        case .easeInOutCubic: return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2
        }
    }
}

// MARK: - Event System

struct UIEventSystem {
    var eventListeners: [String: [UIEventListener]] = [:]
    var eventQueue: [UIEvent] = []
    
    func addListener(for event: String, listener: UIEventListener) {
        if eventListeners[event] == nil {
            eventListeners[event] = []
        }
        eventListeners[event]?.append(listener)
    }
    
    func removeListener(for event: String, listener: UIEventListener) {
        eventListeners[event]?.removeAll { $0.id == listener.id }
    }
    
    func dispatchEvent(_ event: UIEvent) {
        eventQueue.append(event)
        
        // Notify listeners
        if let listeners = eventListeners[event.type] {
            for listener in listeners {
                listener.handler(event)
            }
        }
    }
}

struct UIEvent {
    let type: String
    let source: UUID?
    let data: [String: Any]
    let timestamp: Date
}

struct UIEventListener {
    let id = UUID()
    let handler: (UIEvent) -> Void
}

// MARK: - Layout System

class UILayoutSystem {
    func arrangeElements(elements: [UIElement], in canvasSize: CGSize) {
        // Auto-arrangement logic
        // Could implement various layout algorithms
    }
    
    func gridLayout(elements: [UIElement], columns: Int, spacing: CGFloat) {
        // Grid layout arrangement
        var currentRow = 0
        var currentCol = 0
        let elementWidth = (canvasSize.width - spacing * CGFloat(columns - 1)) / CGFloat(columns)
        
        for element in elements {
            let x = CGFloat(currentCol) * (elementWidth + spacing)
            let y = CGFloat(currentRow) * (50 + spacing) // Assuming 50px height
            
            element.rect.origin = CGPoint(x: x, y: y)
            
            currentCol += 1
            if currentCol >= columns {
                currentCol = 0
                currentRow += 1
            }
        }
    }
    
    func flowLayout(elements: [UIElement], containerSize: CGSize) {
        // Flow layout arrangement
        var x: CGFloat = 0
        var y: CGFloat = 0
        let rowHeight: CGFloat = 50
        
        for element in elements {
            if x + element.rect.width > containerSize.width {
                x = 0
                y += rowHeight
            }
            
            element.rect.origin = CGPoint(x: x, y: y)
            x += element.rect.width + 10 // 10px spacing
        }
    }
}

// MARK: - Theme System

class UIThemeSystem: ObservableObject {
    @Published var currentTheme: UITheme = UITheme.default
    @Published var customThemes: [UITheme] = []
    
    func applyTheme(_ theme: UITheme) {
        currentTheme = theme
        // Apply theme to all UI elements
    }
    
    func createCustomTheme(name: String, colors: [String: Color]) -> UITheme {
        return UITheme(
            name: name,
            backgroundColor: colors["background"] ?? .white,
            foregroundColor: colors["foreground"] ?? .black,
            accentColor: colors["accent"] ?? .blue,
            borderColor: colors["border"] ?? .gray,
            shadowColor: colors["shadow"] ?? .black.opacity(0.2)
        )
    }
}

struct UITheme {
    let name: String
    let backgroundColor: Color
    let foregroundColor: Color
    let accentColor: Color
    let borderColor: Color
    let shadowColor: Color
    
    static let `default` = UITheme(
        name: "Default",
        backgroundColor: .white,
        foregroundColor: .black,
        accentColor: .blue,
        borderColor: .gray,
        shadowColor: .black.opacity(0.2)
    )
    
    static let dark = UITheme(
        name: "Dark",
        backgroundColor: .black,
        foregroundColor: .white,
        accentColor: .blue,
        borderColor: .gray,
        shadowColor: .white.opacity(0.2)
    )
}

enum UIElementType: String, CaseIterable {
    case panel, button, text, image, slider, toggle, scrollView, inputField, dropdown
    
    var icon: String {
        switch self {
        case .panel: return "rectangle"
        case .button: return "button.horizontal"
        case .text: return "textformat"
        case .image: return "photo"
        case .slider: return "slider.horizontal.3"
        case .toggle: return "checkmark.square"
        case .scrollView: return "scroll"
        case .inputField: return "text.cursor"
        case .dropdown: return "chevron.down.square"
        }
    }
}

// MARK: - UI Editor View
struct UIEditorView: View {
    @ObservedObject var manager = UIEditorManager.shared
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            UIEditorToolbar(manager: manager)
            
            EditorDivider()
            
            HSplitView {
                // Hierarchy
                UIHierarchyPanel(manager: manager)
                    .frame(minWidth: 180, maxWidth: 250)
                
                // Canvas
                UICanvas(manager: manager)
                
                // Properties
                UIPropertiesPanel(manager: manager)
                    .frame(minWidth: 200, maxWidth: 300)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - UI Editor Toolbar
struct UIEditorToolbar: View {
    @ObservedObject var manager: UIEditorManager
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            // Add element menu
            Menu {
                ForEach(UIElementType.allCases, id: \.self) { type in
                    Button(action: { manager.addElement(type: type) }) {
                        Label(type.rawValue.capitalized, systemImage: type.icon)
                    }
                }
            } label: {
                Label("Add", systemImage: "plus")
            }
            
            Divider().frame(height: 20)
            
            // Canvas size presets
            Picker("", selection: .constant("1080p")) {
                Text("1920x1080").tag("1080p")
                Text("1280x720").tag("720p")
                Text("2560x1440").tag("1440p")
                Text("3840x2160").tag("4k")
            }
            .frame(width: 120)
            
            Divider().frame(height: 20)
            
            Toggle(isOn: $manager.showGrid) {
                Image(systemName: "grid")
            }
            .toggleStyle(.button)
            
            Toggle(isOn: $manager.snapToGrid) {
                Image(systemName: "rectangle.arrowtriangle.2.inward")
            }
            .toggleStyle(.button)
            
            Spacer()
            
            // Zoom
            HStack(spacing: 4) {
                Button(action: { manager.zoom = max(0.25, manager.zoom - 0.25) }) {
                    Image(systemName: "minus.magnifyingglass")
                }
                .buttonStyle(.plain)
                
                Text("\(Int(manager.zoom * 100))%")
                    .font(DesignSystem.Typography.mono)
                    .frame(width: 40)
                
                Button(action: { manager.zoom = min(2.0, manager.zoom + 0.25) }) {
                    Image(systemName: "plus.magnifyingglass")
                }
                .buttonStyle(.plain)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
}

// MARK: - UI Hierarchy Panel
struct UIHierarchyPanel: View {
    @ObservedObject var manager: UIEditorManager
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text("Hierarchy")
                .font(DesignSystem.Typography.bodyBold)
                .padding(DesignSystem.Spacing.sm)
                .frame(maxWidth: .infinity, alignment: .leading)
                .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            List(manager.elements) { element in
                HStack(spacing: 8) {
                    Image(systemName: element.type.icon)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                    Text(element.name)
                        .font(DesignSystem.Typography.body)
                }
                .padding(.vertical, 2)
                .background(manager.selectedElement == element.id ? DesignSystem.Colors.selection : Color.clear)
                .onTapGesture {
                    manager.selectedElement = element.id
                }
            }
            .listStyle(.plain)
        }
    }
}

// MARK: - UI Canvas
struct UICanvas: View {
    @ObservedObject var manager: UIEditorManager
    
    var body: some View {
        GeometryReader { geo in
            ZStack {
                // Background
                Color(red: 0.12, green: 0.12, blue: 0.14)
                
                // Canvas area
                ZStack {
                    // Canvas background
                    Rectangle()
                        .fill(Color(red: 0.15, green: 0.15, blue: 0.17))
                        .frame(width: manager.canvasSize.width * manager.zoom, height: manager.canvasSize.height * manager.zoom)
                    
                    // Grid
                    if manager.showGrid {
                        UICanvasGrid(size: manager.canvasSize, zoom: manager.zoom, gridSize: manager.gridSize)
                    }
                    
                    // Elements
                    ForEach($manager.elements) { $element in
                        UIElementView(element: $element, isSelected: manager.selectedElement == element.id, zoom: manager.zoom)
                            .onTapGesture {
                                manager.selectedElement = element.id
                            }
                    }
                }
                .frame(width: manager.canvasSize.width * manager.zoom, height: manager.canvasSize.height * manager.zoom)
            }
        }
    }
}

struct UICanvasGrid: View {
    let size: CGSize
    let zoom: CGFloat
    let gridSize: CGFloat
    
    var body: some View {
        Canvas { context, canvasSize in
            let scaledGridSize = gridSize * zoom
            
            for x in stride(from: 0, to: size.width * zoom, by: scaledGridSize) {
                var path = Path()
                path.move(to: CGPoint(x: x, y: 0))
                path.addLine(to: CGPoint(x: x, y: size.height * zoom))
                context.stroke(path, with: .color(Color.white.opacity(0.05)), lineWidth: 1)
            }
            
            for y in stride(from: 0, to: size.height * zoom, by: scaledGridSize) {
                var path = Path()
                path.move(to: CGPoint(x: 0, y: y))
                path.addLine(to: CGPoint(x: size.width * zoom, y: y))
                context.stroke(path, with: .color(Color.white.opacity(0.05)), lineWidth: 1)
            }
        }
        .frame(width: size.width * zoom, height: size.height * zoom)
    }
}

struct UIElementView: View {
    @Binding var element: UIElement
    let isSelected: Bool
    let zoom: CGFloat
    
    var body: some View {
        ZStack {
            // Element representation
            Group {
                switch element.type {
                case .panel:
                    RoundedRectangle(cornerRadius: 8 * zoom)
                        .fill(Color.gray.opacity(0.3))
                case .button:
                    RoundedRectangle(cornerRadius: 4 * zoom)
                        .fill(Color.blue)
                        .overlay(
                            Text("Button")
                                .font(.system(size: 12 * zoom))
                                .foregroundColor(.white)
                        )
                case .text:
                    Text("Text Label")
                        .font(.system(size: 14 * zoom))
                        .foregroundColor(.white)
                case .image:
                    Rectangle()
                        .fill(Color.purple.opacity(0.5))
                        .overlay(
                            Image(systemName: "photo")
                                .font(.system(size: 24 * zoom))
                                .foregroundColor(.white.opacity(0.5))
                        )
                default:
                    Rectangle()
                        .fill(element.color.opacity(0.5))
                }
            }
            .frame(width: element.rect.width * zoom, height: element.rect.height * zoom)
            
            // Selection outline
            if isSelected {
                Rectangle()
                    .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
                    .frame(width: element.rect.width * zoom, height: element.rect.height * zoom)
                
                // Resize handles
                ForEach(0..<4, id: \.self) { corner in
                    Circle()
                        .fill(DesignSystem.Colors.accentPrimary)
                        .frame(width: 8, height: 8)
                        .position(handlePosition(for: corner, in: element.rect, zoom: zoom))
                }
            }
        }
        .position(
            x: element.rect.midX * zoom,
            y: element.rect.midY * zoom
        )
        .gesture(
            DragGesture()
                .onChanged { value in
                    element.rect.origin.x += value.translation.width / zoom
                    element.rect.origin.y += value.translation.height / zoom
                }
        )
    }
    
    func handlePosition(for corner: Int, in rect: CGRect, zoom: CGFloat) -> CGPoint {
        switch corner {
        case 0: return CGPoint(x: -rect.width * zoom / 2, y: -rect.height * zoom / 2)
        case 1: return CGPoint(x: rect.width * zoom / 2, y: -rect.height * zoom / 2)
        case 2: return CGPoint(x: rect.width * zoom / 2, y: rect.height * zoom / 2)
        case 3: return CGPoint(x: -rect.width * zoom / 2, y: rect.height * zoom / 2)
        default: return .zero
        }
    }
}

// MARK: - UI Properties Panel
struct UIPropertiesPanel: View {
    @ObservedObject var manager: UIEditorManager
    
    var selectedElement: UIElement? {
        manager.elements.first { $0.id == manager.selectedElement }
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text("Properties")
                .font(DesignSystem.Typography.bodyBold)
                .padding(DesignSystem.Spacing.sm)
                .frame(maxWidth: .infinity, alignment: .leading)
                .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            if let element = selectedElement, let index = manager.elements.firstIndex(where: { $0.id == element.id }) {
                ScrollView {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                        // Name
                        StringPropertyEditor(label: "Name", value: $manager.elements[index].name)
                        
                        EditorDivider()
                        
                        // Rect Transform
                        Text("Rect Transform")
                            .font(DesignSystem.Typography.bodyBold)
                        
                        HStack {
                            Text("X")
                            TextField("", value: Binding(
                                get: { Double(manager.elements[index].rect.origin.x) },
                                set: { manager.elements[index].rect.origin.x = CGFloat($0) }
                            ), format: .number)
                                .textFieldStyle(.roundedBorder)
                            Text("Y")
                            TextField("", value: Binding(
                                get: { Double(manager.elements[index].rect.origin.y) },
                                set: { manager.elements[index].rect.origin.y = CGFloat($0) }
                            ), format: .number)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        HStack {
                            Text("W")
                            TextField("", value: Binding(
                                get: { Double(manager.elements[index].rect.size.width) },
                                set: { manager.elements[index].rect.size.width = CGFloat($0) }
                            ), format: .number)
                                .textFieldStyle(.roundedBorder)
                            Text("H")
                            TextField("", value: Binding(
                                get: { Double(manager.elements[index].rect.size.height) },
                                set: { manager.elements[index].rect.size.height = CGFloat($0) }
                            ), format: .number)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        EditorDivider()
                        
                        // Color
                        ColorPropertyEditor(label: "Color", value: $manager.elements[index].color)
                        
                        // Interactable
                        BoolPropertyEditor(label: "Interactable", value: $manager.elements[index].isInteractable)
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
            } else {
                VStack {
                    Image(systemName: "cursorarrow.click")
                        .font(.largeTitle)
                        .foregroundColor(DesignSystem.Colors.textDisabled)
                    Text("Select an element")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}
