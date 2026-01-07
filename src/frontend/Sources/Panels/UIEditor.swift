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
    
    init() {
        // Demo elements
        elements = [
            UIElement(type: .panel, name: "MainPanel", rect: CGRect(x: 100, y: 100, width: 400, height: 300)),
            UIElement(type: .button, name: "PlayButton", rect: CGRect(x: 150, y: 200, width: 120, height: 40)),
            UIElement(type: .text, name: "TitleText", rect: CGRect(x: 120, y: 120, width: 200, height: 30)),
            UIElement(type: .image, name: "Logo", rect: CGRect(x: 200, y: 50, width: 100, height: 100))
        ]
    }
    
    func addElement(type: UIElementType) {
        let element = UIElement(type: type, name: "\(type.rawValue)_\(elements.count)", rect: CGRect(x: 100, y: 100, width: 100, height: 40))
        elements.append(element)
        selectedElement = element.id
    }
    
    func deleteSelected() {
        elements.removeAll { $0.id == selectedElement }
        selectedElement = nil
    }
}

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
