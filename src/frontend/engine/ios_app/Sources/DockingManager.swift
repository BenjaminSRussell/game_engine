import SwiftUI

/// Docking system for editor panels
class DockingManager: ObservableObject {
    
    static let shared = DockingManager()
    
    @Published var panels: [EditorPanel] = []
    @Published var layout: LayoutConfiguration = .default
    
    enum PanelPosition {
        case left
        case right
        case bottom
        case center
        case floating
    }
    
    struct EditorPanel: Identifiable {
        let id = UUID()
        var title: String
        var position: PanelPosition
        var isVisible: Bool
        var width: CGFloat?
        var height: CGFloat?
        var content: AnyView
        
        init<Content: View>(title: String, position: PanelPosition, isVisible: Bool = true, @ViewBuilder content: () -> Content) {
            self.title = title
            self.position = position
            self.isVisible = isVisible
            self.content = AnyView(content())
        }
    }
    
    struct LayoutConfiguration {
        var leftPanelWidth: CGFloat
        var rightPanelWidth: CGFloat
        var bottomPanelHeight: CGFloat
        
        static let `default` = LayoutConfiguration(
            leftPanelWidth: 250,
            rightPanelWidth: 300,
            bottomPanelHeight: 200
        )
    }
    
    private init() {
        setupDefaultPanels()
    }
    
    // MARK: - Panel Management
    
    func addPanel(_ panel: EditorPanel) {
        panels.append(panel)
    }
    
    func removePanel(id: UUID) {
        panels.removeAll { $0.id == id }
    }
    
    func togglePanel(title: String) {
        if let index = panels.firstIndex(where: { $0.title == title }) {
            panels[index].isVisible.toggle()
        }
    }
    
    func showPanel(title: String) {
        if let index = panels.firstIndex(where: { $0.title == title }) {
            panels[index].isVisible = true
        }
    }
    
    func hidePanel(title: String) {
        if let index = panels.firstIndex(where: { $0.title == title }) {
            panels[index].isVisible = false
        }
    }
    
    func getPanels(at position: PanelPosition) -> [EditorPanel] {
        return panels.filter { $0.position == position && $0.isVisible }
    }
    
    // MARK: - Layout Management
    
    func saveLayout(name: String) {
        // Save current layout configuration
        UserDefaults.standard.set(layout.leftPanelWidth, forKey: "layout_\(name)_leftWidth")
        UserDefaults.standard.set(layout.rightPanelWidth, forKey: "layout_\(name)_rightWidth")
        UserDefaults.standard.set(layout.bottomPanelHeight, forKey: "layout_\(name)_bottomHeight")
        
        // Save panel visibility
        for panel in panels {
            UserDefaults.standard.set(panel.isVisible, forKey: "layout_\(name)_panel_\(panel.title)")
        }
        
        print("DockingManager: Saved layout '\(name)'")
    }
    
    func loadLayout(name: String) {
        // Load layout configuration
        if UserDefaults.standard.object(forKey: "layout_\(name)_leftWidth") != nil {
            layout.leftPanelWidth = CGFloat(UserDefaults.standard.float(forKey: "layout_\(name)_leftWidth"))
            layout.rightPanelWidth = CGFloat(UserDefaults.standard.float(forKey: "layout_\(name)_rightWidth"))
            layout.bottomPanelHeight = CGFloat(UserDefaults.standard.float(forKey: "layout_\(name)_bottomHeight"))
            
            // Load panel visibility
            for i in 0..<panels.count {
                if UserDefaults.standard.object(forKey: "layout_\(name)_panel_\(panels[i].title)") != nil {
                    panels[i].isVisible = UserDefaults.standard.bool(forKey: "layout_\(name)_panel_\(panels[i].title)")
                }
            }
            
            print("DockingManager: Loaded layout '\(name)'")
        }
    }
    
    func resetLayout() {
        layout = .default
        for i in 0..<panels.count {
            panels[i].isVisible = true
        }
    }
    
    // MARK: - Default Panels
    
    private func setupDefaultPanels() {
        // These will be populated by the editor
        print("DockingManager: Initialized")
    }
}

/// Dockable editor view with resizable panels
struct DockableEditorView<Center: View>: View {
    @ObservedObject var docking = DockingManager.shared
    @State private var isDraggingLeft = false
    @State private var isDraggingRight = false
    @State private var isDraggingBottom = false
    
    let centerContent: Center
    
    init(@ViewBuilder centerContent: () -> Center) {
        self.centerContent = centerContent()
    }
    
    var body: some View {
        GeometryReader { geometry in
            HStack(spacing: 0) {
                // Left Panel
                if !docking.getPanels(at: .left).isEmpty {
                    VStack(spacing: 0) {
                        ForEach(docking.getPanels(at: .left)) { panel in
                            PanelContainer(panel: panel)
                        }
                    }
                    .frame(width: docking.layout.leftPanelWidth)
                    .background(Color(white: 0.12))
                    
                    // Left Resize Handle
                    ResizeHandle(isVertical: true)
                        .onDrag(isDragging: $isDraggingLeft) { delta in
                            docking.layout.leftPanelWidth = max(150, min(500, docking.layout.leftPanelWidth + delta))
                        }
                }
                
                // Center Area
                VStack(spacing: 0) {
                    // Main Content
                    centerContent
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                    
                    // Bottom Panel
                    if !docking.getPanels(at: .bottom).isEmpty {
                        // Bottom Resize Handle
                        ResizeHandle(isVertical: false)
                            .onDrag(isDragging: $isDraggingBottom) { delta in
                                docking.layout.bottomPanelHeight = max(100, min(400, docking.layout.bottomPanelHeight - delta))
                            }
                        
                        VStack(spacing: 0) {
                            ForEach(docking.getPanels(at: .bottom)) { panel in
                                PanelContainer(panel: panel)
                            }
                        }
                        .frame(height: docking.layout.bottomPanelHeight)
                        .background(Color(white: 0.12))
                    }
                }
                
                // Right Panel
                if !docking.getPanels(at: .right).isEmpty {
                    // Right Resize Handle
                    ResizeHandle(isVertical: true)
                        .onDrag(isDragging: $isDraggingRight) { delta in
                            docking.layout.rightPanelWidth = max(150, min(500, docking.layout.rightPanelWidth - delta))
                        }
                    
                    VStack(spacing: 0) {
                        ForEach(docking.getPanels(at: .right)) { panel in
                            PanelContainer(panel: panel)
                        }
                    }
                    .frame(width: docking.layout.rightPanelWidth)
                    .background(Color(white: 0.12))
                }
            }
        }
    }
}

/// Container for individual panels
struct PanelContainer: View {
    let panel: DockingManager.EditorPanel
    @State private var isCollapsed = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Panel Header
            HStack {
                Text(panel.title)
                    .font(.headline)
                    .foregroundColor(.white)
                
                Spacer()
                
                Button(action: { isCollapsed.toggle() }) {
                    Image(systemName: isCollapsed ? "chevron.down" : "chevron.up")
                        .foregroundColor(.gray)
                }
                
                Button(action: { DockingManager.shared.hidePanel(title: panel.title) }) {
                    Image(systemName: "xmark")
                        .foregroundColor(.gray)
                }
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(Color(white: 0.15))
            .overlay(Rectangle().frame(height: 1).foregroundColor(Color.white.opacity(0.1)), alignment: .bottom)
            
            // Panel Content
            if !isCollapsed {
                panel.content
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
    }
}

/// Resize handle for panels
struct ResizeHandle: View {
    let isVertical: Bool
    
    var body: some View {
        Rectangle()
            .fill(Color.clear)
            .frame(width: isVertical ? 4 : nil, height: isVertical ? nil : 4)
            .contentShape(Rectangle())
            .cursor(isVertical ? .resizeLeftRight : .resizeUpDown)
            .overlay(
                Rectangle()
                    .fill(Color.cyan.opacity(0.3))
                    .frame(width: isVertical ? 1 : nil, height: isVertical ? nil : 1)
            )
    }
    
    func onDrag(isDragging: Binding<Bool>, perform action: @escaping (CGFloat) -> Void) -> some View {
        self.gesture(
            DragGesture()
                .onChanged { value in
                    isDragging.wrappedValue = true
                    let delta = isVertical ? value.translation.width : value.translation.height
                    action(delta)
                }
                .onEnded { _ in
                    isDragging.wrappedValue = false
                }
        )
    }
}

// MARK: - Cursor Extension

extension View {
    func cursor(_ cursor: NSCursor) -> some View {
        self.onHover { hovering in
            if hovering {
                cursor.push()
            } else {
                NSCursor.pop()
            }
        }
    }
}
