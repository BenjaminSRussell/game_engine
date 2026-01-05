import UIKit
import SwiftUI

class AssetEditorViewController: UIViewController {
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        setupSwiftUI()
    }
    
    private func setupSwiftUI() {
        let editorView = AssetEditorRootView()
        let hostingController = UIHostingController(rootView: editorView)
        
        addChild(hostingController)
        view.addSubview(hostingController.view)
        hostingController.view.translatesAutoresizingMaskIntoConstraints = false
        
        NSLayoutConstraint.activate([
            hostingController.view.topAnchor.constraint(equalTo: view.topAnchor),
            hostingController.view.bottomAnchor.constraint(equalTo: view.bottomAnchor),
            hostingController.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            hostingController.view.trailingAnchor.constraint(equalTo: view.trailingAnchor)
        ])
        
        hostingController.didMove(toParent: self)
    }
}

struct AssetEditorRootView: View {
    @State private var selectedTab: EditorTab = .browser
    @State private var isBuilding: Bool = false
    @State private var buildProgress: Double = 0.0
    @State private var buildStatus: String = "Engine Ready"
    @ObservedObject private var docking = DockingManager.shared
    
    enum EditorTab {
        case sprite, model, world, material, logic, browser
    }
    
    var body: some View {
        ZStack {
            // Modern Dark Gradient Background
            LinearGradient(
                gradient: Gradient(colors: [Color(white: 0.1), Color(white: 0.05)]),
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
            .ignoresSafeArea()
            
            VStack(spacing: 0) {
                // Custom Navigation Bar
                navigationBar
                
                // Dockable Editor Layout
                DockableEditorView {
                    // Center Content Area
                    Group {
                        switch selectedTab {
                        case .sprite:
                            SpriteEditorView(layerManager: AssetEditorRootView.sharedLayerManager)
                        case .model:
                            ModelEditorView()
                        case .world:
                            TerrainEditorView()
                        case .material:
                            MaterialEditorView()
                        case .logic:
                            BlueprintEditorView()
                        case .browser:
                            AssetBrowserView()
                        }
                    }
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                }
            }
        }
        .onAppear {
            setupDockingPanels()
            // Add Console to Docking Manager
            DockingManager.shared.addPanel(DockingManager.EditorPanel(title: "Engine Console", position: .bottom) {
                ConsoleView()
            })
        }
    }
    
    // MARK: - Navigation Bar
    
    private var navigationBar: some View {
        HStack {
            Text("NOVA ASSET EDITOR")
                .font(.system(size: 18, weight: .bold, design: .monospaced))
                .foregroundColor(.cyan)
                .padding(.leading)
            
            Spacer()
            
            // Nova Engine Build System
            VStack(alignment: .leading, spacing: 2) {
                HStack {
                    Text("NOVA BUILD")
                        .font(.system(size: 8, weight: .bold, design: .monospaced))
                        .foregroundColor(isBuilding ? .orange : .green)
                    Spacer()
                    Text(isBuilding ? "\(Int(buildProgress * 100))%" : buildStatus)
                        .font(.system(size: 7, design: .monospaced))
                        .foregroundColor(.gray)
                }
                ZStack(alignment: .leading) {
                    Capsule().fill(Color.white.opacity(0.1)).frame(height: 3)
                    if isBuilding {
                        Capsule()
                            .fill(Color.orange)
                            .frame(width: 80 * buildProgress, height: 3)
                    }
                }
            }
            .frame(width: 80)
            .padding(.trailing, 4)
            
            Button(action: { startBuild() }) {
                Image(systemName: isBuilding ? "stop.fill" : "hammer.fill")
                    .foregroundColor(isBuilding ? .red : .cyan)
                    .font(.system(size: 14))
            }
            .padding(.trailing, 12)
            
            // Hot Reload Status
            HStack(spacing: 6) {
                Circle()
                    .fill(Color.green)
                    .frame(width: 8, height: 8)
                    .shadow(color: .green, radius: 4)
                Text("LIVE")
                    .font(.system(size: 8, weight: .bold))
                    .foregroundColor(.green)
            }
            .padding(.horizontal, 6)
            .padding(.vertical, 3)
            .background(Color.green.opacity(0.1))
            .cornerRadius(4)
            .padding(.trailing)
            
            // Editor Mode Picker
            Picker("Editor Mode", selection: $selectedTab) {
                Text("2.5D Sprites").tag(EditorTab.sprite)
                Text("3D Voxel Forge").tag(EditorTab.model)
                Text("World Editor").tag(EditorTab.world)
                Text("Material Editor").tag(EditorTab.material)
                Text("Logic Blueprints").tag(EditorTab.logic)
                Text("Browser").tag(EditorTab.browser)
            }
            .pickerStyle(SegmentedPickerStyle())
            .frame(width: 400)
            .padding()
            
            Spacer()
            
            // View Menu
            Menu {
                Button(action: { docking.togglePanel(title: "Properties") }) {
                    Label("Properties", systemImage: "slider.horizontal.3")
                }
                Button(action: { docking.togglePanel(title: "Console") }) {
                    Label("Console", systemImage: "terminal")
                }
                Button(action: { docking.togglePanel(title: "Timeline") }) {
                    Label("Timeline", systemImage: "timeline.selection")
                }
                
                Divider()
                
                Button(action: { docking.saveLayout(name: "default") }) {
                    Label("Save Layout", systemImage: "square.and.arrow.down")
                }
                Button(action: { docking.loadLayout(name: "default") }) {
                    Label("Load Layout", systemImage: "square.and.arrow.up")
                }
                Button(action: { docking.resetLayout() }) {
                    Label("Reset Layout", systemImage: "arrow.counterclockwise")
                }
            } label: {
                Image(systemName: "sidebar.right")
                    .font(.title3)
                    .foregroundColor(.gray)
                    .padding(.trailing, 8)
            }
            
            Button(action: { /* Exit action */ }) {
                Image(systemName: "xmark.circle.fill")
                    .font(.title2)
                    .foregroundColor(.gray)
                    .padding(.trailing)
            }
        }
        .background(Color(white: 0.15).opacity(0.8))
        .overlay(Rectangle().frame(height: 1).foregroundColor(.cyan.opacity(0.3)), alignment: .bottom)
    }
    
    // MARK: - Setup Docking Panels
    
    // Use a shared layer manager for animation integration
    private static let sharedLayerManager = SpriteLayerManager(canvasSize: 32)
    
    private func setupDockingPanels() {
        // Only setup once
        guard docking.panels.isEmpty else { return }
        
        // Right Panel: Property Inspector
        docking.addPanel(
            DockingManager.EditorPanel(
                title: "Properties",
                position: .right,
                isVisible: true
            ) {
                PropertyInspectorView()
            }
        )
        
        // Bottom Panel: Console
        docking.addPanel(
            DockingManager.EditorPanel(
                title: "Console",
                position: .bottom,
                isVisible: false
            ) {
                ConsoleView()
            }
        )
        
        // Bottom Panel: Timeline (for animations)
        docking.addPanel(
            DockingManager.EditorPanel(
                title: "Timeline",
                position: .bottom,
                isVisible: true
            ) {
                TimelineView(layerManager: AssetEditorRootView.sharedLayerManager)
            }
        )
    }
    
    // MARK: - Build Logic
    
    private func startBuild() {
        if isBuilding {
            isBuilding = false
            return
        }
        
        isBuilding = true
        buildProgress = 0.0
        buildStatus = "Wait..."
        
        DockingManager.shared.log("Starting full engine build...", type: .info)
        
        Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { timer in
            if buildProgress < 1.0 {
                buildProgress += 0.02
                if buildProgress > 0.3 { buildStatus = "Shaders..." }
                if buildProgress > 0.6 { buildStatus = "Linking..." }
            } else {
                timer.invalidate()
                isBuilding = false
                buildStatus = "Ready"
                DockingManager.shared.log("Engine Build Successful!", type: .success)
            }
        }
    }
}

// MARK: - Console View

struct ConsoleView: View {
    @State private var logs: [ConsoleLog] = [
        ConsoleLog(message: "Editor initialized successfully", type: .info),
        ConsoleLog(message: "Asset browser ready", type: .success),
        ConsoleLog(message: "Watching directory: assets/", type: .info)
    ]
    
    struct ConsoleLog: Identifiable {
        let id = UUID()
        let message: String
        let type: LogType
        let timestamp = Date()
        
        enum LogType {
            case info, success, warning, error
            
            var color: Color {
                switch self {
                case .info: return .white
                case .success: return .green
                case .warning: return .yellow
                case .error: return .red
                }
            }
            
            var icon: String {
                switch self {
                case .info: return "info.circle"
                case .success: return "checkmark.circle"
                case .warning: return "exclamationmark.triangle"
                case .error: return "xmark.circle"
                }
            }
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Console Toolbar
            HStack {
                Button(action: { logs.removeAll() }) {
                    Label("Clear", systemImage: "trash")
                        .font(.caption)
                }
                .buttonStyle(PlainButtonStyle())
                .foregroundColor(.gray)
                
                Spacer()
            }
            .padding(8)
            .background(Color(white: 0.15))
            
                    Image(systemName: "trash")
                        .font(.caption2)
                }
            }
            .padding(.horizontal, 8)
            .padding(.top, 4)
            .foregroundColor(.gray)
            
            Divider().background(Color.white.opacity(0.1))
            
            ScrollView {
                VStack(alignment: .leading, spacing: 4) {
                    ForEach(logs.indices, id: \.self) { idx in
                        HStack(alignment: .top, spacing: 6) {
                            Text(logs[idx].0)
                                .font(.system(size: 11, design: .monospaced))
                                .foregroundColor(logs[idx].1.color)
                        }
                    }
                }
                .padding(8)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        .background(Color(white: 0.1))
    }
}

struct PanelContainer<Content: View>: View {
    let title: String
    let content: Content
    
    init(_ title: String, @ViewBuilder content: () -> Content) {
        self.title = title
        self.content = content()
    }
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text(title.uppercased())
                    .font(.caption2.bold())
                    .foregroundColor(.white.opacity(0.6))
                Spacer()
                Image(systemName: "line.3.horizontal")
                    .font(.caption2)
                    .foregroundColor(.gray)
            }
            .padding(8)
            .background(Color(white: 0.15))
            
            content
        }
        .background(Color(white: 0.12))
        .cornerRadius(6)
        .overlay(
            RoundedRectangle(cornerRadius: 6)
                .stroke(Color.white.opacity(0.1), lineWidth: 1)
        )
    }
}

// MARK: - Timeline View

struct TimelineView: View {
    @ObservedObject var layerManager: SpriteLayerManager
    @State private var isPlaying = false
    @State private var timer: Timer?
    
    var body: some View {
        VStack(spacing: 0) {
            // Timeline Controls
            HStack {
                Button(action: { togglePlayback() }) {
                    Image(systemName: isPlaying ? "pause.fill" : "play.fill")
                }
                
                Button(action: { layerManager.currentFrameIndex = 0 }) {
                    Image(systemName: "backward.end.fill")
                }
                
                Text("\(layerManager.currentFrameIndex + 1) / \(max(1, layerManager.frames.count))")
                    .font(.caption.monospaced())
                    .foregroundColor(.white)
                    .frame(width: 80)
                
                Slider(value: Binding(
                    get: { Double(layerManager.currentFrameIndex) },
                    set: { layerManager.currentFrameIndex = min(Int($0), layerManager.frames.count - 1) }
                ), in: 0...Double(max(0, layerManager.frames.count - 1)))
                    .accentColor(.cyan)
                
                Button(action: { layerManager.addFrame() }) {
                    Image(systemName: "plus.diamond")
                }
                
                Button(action: { layerManager.deleteFrame(at: layerManager.currentFrameIndex) }) {
                    Image(systemName: "minus.diamond")
                }
                .disabled(layerManager.frames.count <= 1)
            }
            .padding(8)
            .background(Color(white: 0.15))
            
            // Timeline Track
            ScrollView(.horizontal) {
                HStack(spacing: 0) {
                    ForEach(0..<layerManager.frames.count, id: \.self) { frame in
                        Rectangle()
                            .fill(frame == layerManager.currentFrameIndex ? Color.cyan : Color.white.opacity(0.1))
                            .frame(width: 40, height: 40)
                            .border(Color.white.opacity(0.1), width: 0.5)
                            .overlay(
                                Text("\(frame + 1)")
                                    .font(.caption2.monospaced())
                                    .foregroundColor(frame == layerManager.currentFrameIndex ? .black : .gray)
                            )
                            .onTapGesture {
                                layerManager.currentFrameIndex = frame
                            }
                    }
                }
            }
            .background(Color.black.opacity(0.3))
        }
    }
    
    private func togglePlayback() {
        isPlaying.toggle()
        if isPlaying {
            timer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
                layerManager.currentFrameIndex = (layerManager.currentFrameIndex + 1) % layerManager.frames.count
            }
        } else {
            timer?.invalidate()
            timer = nil
        }
    }
}

// MARK: - Placeholder Content Views

// MARK: - Metal Renderer
class VoxelRenderer: NSObject, MTKViewDelegate {
    var device: MTLDevice
    var commandQueue: MTLCommandQueue?
    
    init?(device: MTLDevice) {
        self.device = device
        self.commandQueue = device.makeCommandQueue()
        super.init()
    }
    
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}
    
    func draw(in view: MTKView) {
        guard let drawable = view.currentDrawable,
              let renderPassDescriptor = view.currentRenderPassDescriptor,
              let commandQueue = commandQueue,
              let commandBuffer = commandQueue.makeCommandBuffer(),
              let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor) else { return }
        
        // Clear screen with dark "void" color
        renderEncoder.endEncoding()
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}

// MARK: - Views

struct SpriteEditorView: View {
    @ObservedObject var layerManager: SpriteLayerManager
    @StateObject private var toolSettings = ToolSettings()
    @State private var showSaveAlert = false
    @State private var canvasSize: Int = 32
    @State private var zoom: CGFloat = 1.0
    @State private var showGrid = true
    @State private var onionSkinning = true
    @State private var showParallaxPreview = false
    
    private var drawingEngine: DrawingEngine {
        DrawingEngine(layerManager: layerManager, toolSettings: toolSettings)
    }
    
    var body: some View {
        HStack(spacing: 0) {
            // Left: Tool Palette
            ToolPaletteView(toolSettings: toolSettings, layerManager: layerManager)
                .padding()
            
            // Center: Canvas Area
            VStack(spacing: 0) {
                // Canvas Toolbar
                canvasToolbar
                
                // Main Canvas
                GeometryReader { geo in
                    let availableSize = min(geo.size.width, geo.size.height) * 0.9
                    let cellSize = (availableSize / CGFloat(canvasSize)) * zoom
                    
                    ZStack {
                        // Checkerboard background
                        CanvasBackground(gridSize: canvasSize)
                            .frame(width: availableSize * zoom, height: availableSize * zoom)
                        
                        // Onion Skin (Previous Frame)
                        if onionSkinning && layerManager.currentFrameIndex > 0 {
                            CanvasView(
                                pixels: layerManager.getCompositeImage(forFrame: layerManager.currentFrameIndex - 1),
                                gridSize: canvasSize,
                                cellSize: cellSize,
                                showGrid: false,
                                isInteractive: false
                            )
                            .opacity(0.3)
                            .frame(width: availableSize * zoom, height: availableSize * zoom)
                        }
                        
                        // Composite layer view
                        CanvasView(
                            pixels: layerManager.getCompositeImage(),
                            gridSize: canvasSize,
                            cellSize: cellSize,
                            showGrid: showGrid,
                            selection: layerManager.selection,
                            onDraw: { point in
                                handleDrawing(at: point, cellSize: cellSize)
                            }
                        )
                        .frame(width: availableSize * zoom, height: availableSize * zoom)
                    }
                    .position(x: geo.size.width / 2, y: geo.size.height / 2)
                }
                
                // Parallax Preview (2.5D)
                if showParallaxPreview {
                    ParallaxPreviewView(layerManager: layerManager)
                        .frame(height: 200)
                        .background(Color.black.opacity(0.5))
                        .transition(.move(edge: .bottom))
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(Color(white: 0.08))
            
            // Right: Layer Panel
            LayerPanelView(layerManager: layerManager)
        }
    }
    
    // MARK: - Canvas Toolbar
    
    private var canvasToolbar: some View {
        HStack {
            // Canvas size selector
            Menu {
                Button("16x16") { resizeCanvas(16) }
                Button("32x32") { resizeCanvas(32) }
                Button("64x64") { resizeCanvas(64) }
                Button("128x128") { resizeCanvas(128) }
            } label: {
                HStack {
                    Text("\(canvasSize)x\(canvasSize)")
                        .font(.caption.monospaced())
                    Image(systemName: "chevron.down")
                        .font(.caption2)
                }
                .foregroundColor(.white)
                .padding(.horizontal, 12)
                .padding(.vertical, 6)
                .background(Color.white.opacity(0.1))
                .cornerRadius(6)
            }
            
            Divider()
                .frame(height: 20)
                .background(Color.gray)
            
            // Zoom controls
            Button(action: { zoom = max(0.5, zoom - 0.25) }) {
                Image(systemName: "minus.magnifyingglass")
            }
            
            Text("\(Int(zoom * 100))%")
                .font(.caption.monospaced())
                .frame(width: 50)
            
            Button(action: { zoom = min(4.0, zoom + 0.25) }) {
                Image(systemName: "plus.magnifyingglass")
            }
            
            Button(action: { zoom = 1.0 }) {
                Text("Reset")
                    .font(.caption)
            }
            
            Divider()
                .frame(height: 20)
                .background(Color.gray)
            
            // Grid toggle
            Toggle("Grid", isOn: $showGrid)
                .toggleStyle(SwitchToggleStyle(tint: .cyan))
                .font(.caption)
            
            Toggle("Onion", isOn: $onionSkinning)
                .toggleStyle(SwitchToggleStyle(tint: .cyan))
                .font(.caption)
            
            Toggle("2.5D", isOn: $showParallaxPreview)
                .toggleStyle(SwitchToggleStyle(tint: .cyan))
                .font(.caption)
            
            Spacer()
            
            // Save button
            Button(action: { showSaveAlert = true }) {
                Label("Export", systemImage: "square.and.arrow.up")
                    .font(.caption.weight(.semibold))
                    .padding(.horizontal, 12)
                    .padding(.vertical, 6)
                    .background(Color.cyan.opacity(0.2))
                    .foregroundColor(.cyan)
                    .cornerRadius(6)
            }
            .alert(isPresented: $showSaveAlert) {
                Alert(
                    title: Text("Export Sprite"),
                    message: Text("Sprite exported to engine assets."),
                    dismissButton: .default(Text("OK"))
                )
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(Color(white: 0.15))
        .overlay(Rectangle().frame(height: 1).foregroundColor(Color.white.opacity(0.1)), alignment: .bottom)
    }
    
    // MARK: - Helper Methods
    
    private func resizeCanvas(_ newSize: Int) {
        canvasSize = newSize
        layerManager.canvasSize = newSize
        // Recreate frames and layers
        layerManager.frames.removeAll()
        layerManager.addFrame()
    }
    
    private func handleDrawing(at point: CGPoint, cellSize: CGFloat) {
        let x = Int(point.x / cellSize)
        let y = Int(point.y / cellSize)
        let canvasPoint = CGPoint(x: x, y: y)
        
        drawingEngine.beginStroke(at: canvasPoint)
        drawingEngine.endStroke(at: canvasPoint)
    }
}

// MARK: - Canvas View

struct CanvasView: View {
    let pixels: [Color]
    let gridSize: Int
    let cellSize: CGFloat
    let showGrid: Bool
    var selection: Set<Int> = []
    var isInteractive: Bool = true
    let onDraw: (CGPoint) -> Void
    
    init(pixels: [Color], gridSize: Int, cellSize: CGFloat, showGrid: Bool, selection: Set<Int> = [], isInteractive: Bool = true, onDraw: @escaping (CGPoint) -> Void = { _ in }) {
        self.pixels = pixels
        self.gridSize = gridSize
        self.cellSize = cellSize
        self.showGrid = showGrid
        self.selection = selection
        self.isInteractive = isInteractive
        self.onDraw = onDraw
    }
    
    var body: some View {
        LazyVGrid(
            columns: Array(repeating: GridItem(.fixed(cellSize), spacing: 0), count: gridSize),
            spacing: 0
        ) {
            ForEach(0..<pixels.count, id: \.self) { index in
                Rectangle()
                    .fill(pixels[index])
                    .frame(width: cellSize, height: cellSize)
                    .border(showGrid ? Color.white.opacity(0.1) : Color.clear, width: 0.5)
                    .overlay(
                        selection.contains(index) ? 
                        Rectangle()
                            .stroke(Color.cyan, lineWidth: 1)
                            .background(Color.cyan.opacity(0.2))
                        : nil
                    )
            }
        }
        .gesture(
            isInteractive ? 
            DragGesture(minimumDistance: 0)
                .onChanged { value in
                    onDraw(value.location)
                }
            : nil
        )
        .border(Color.cyan.opacity(0.5), width: 2)
    }
}

struct CanvasBackground: View {
    let gridSize: Int
    var body: some View {
        VStack(spacing: 0) {
            ForEach(0..<gridSize, id: \.self) { row in
                HStack(spacing: 0) {
                    ForEach(0..<gridSize, id: \.self) { col in
                        Rectangle()
                            .fill((row + col) % 2 == 0 ? Color.white.opacity(0.05) : Color.black.opacity(0.05))
                    }
                }
            }
        }
    }
}

struct ModelEditorView: View {
    @StateObject private var voxelModel = VoxelModel(size: 32)
    @StateObject private var toolSettings = ToolSettings()
    @State private var selectedTool: Tool = .brush
    @State private var rotation: SIMD3<Float> = [0.4, 0.4, 0]
    @State private var zoom: Float = 45.0
    @State private var symmetryX = false
    @State private var symmetryY = false
    @State private var symmetryZ = false
    
    enum Tool {
        case brush, eraser, paint, move
    }
    
    var body: some View {
        ZStack {
            // 3D Viewport (Metal)
            VoxelViewport(model: voxelModel, rotation: rotation, zoom: zoom, tool: selectedTool, color: toolSettings.primaryColor, symmetry: (symmetryX, symmetryY, symmetryZ))
                .edgesIgnoringSafeArea(.all)
                .gesture(
                    DragGesture()
                        .onChanged { value in
                            rotation.y += Float(value.translation.width) * 0.001
                            rotation.x += Float(value.translation.height) * 0.001
                        }
                )
            
            // HUD Overlays
            VStack {
                // Top Bar: Stats & Options
                HStack {
                    VStack(alignment: .leading) {
                        Text("VOXEL FORGE")
                            .font(.headline)
                            .foregroundColor(.cyan)
                        Text("Size: \(voxelModel.size)³")
                            .font(.caption.monospaced())
                            .foregroundColor(.white)
                    }
                    .padding()
                    .background(Color.black.opacity(0.5))
                    .cornerRadius(8)
                    
                    Spacer()
                    
                    // Symmetry Controls
                    HStack {
                        SymmetryToggle(label: "X", isOn: $symmetryX)
                        SymmetryToggle(label: "Y", isOn: $symmetryY)
                        SymmetryToggle(label: "Z", isOn: $symmetryZ)
                    }
                    .padding(8)
                    .background(Color.black.opacity(0.5))
                    .cornerRadius(8)
                }
                .padding()
                
                Spacer()
                
                // Bottom Toolbar: Tools
                HStack(spacing: 15) {
                    ToolButton(icon: "paintbrush.fill", label: "Brush", isSelected: selectedTool == .brush) { selectedTool = .brush }
                    ToolButton(icon: "eraser.fill", label: "Erase", isSelected: selectedTool == .eraser) { selectedTool = .eraser }
                    ToolButton(icon: "paintpalette.fill", label: "Paint", isSelected: selectedTool == .paint) { selectedTool = .paint }
                    
                    Divider().frame(height: 30).background(Color.gray)
                    
                    ColorPicker("", selection: $toolSettings.primaryColor)
                        .labelsHidden()
                    
                    Button(action: { voxelModel.clear() }) {
                        Image(systemName: "trash")
                            .foregroundColor(.red)
                    }
                }
                .padding()
                .background(VisualEffectBlur(blurStyle: .systemUltraThinMaterialDark))
                .cornerRadius(16)
                .overlay(
                    RoundedRectangle(cornerRadius: 16)
                        .stroke(Color.white.opacity(0.1), lineWidth: 1)
                )
                .padding(.bottom, 20)
            }
        }
    }
}

struct SymmetryToggle: View {
    let label: String
    @Binding var isOn: Bool
    var body: some View {
        Toggle(label, isOn: $isOn)
            .toggleStyle(ButtonToggleStyle())
            .font(.caption.bold())
            .foregroundColor(isOn ? .cyan : .gray)
            .frame(width: 30)
    }
}

struct ToolButton: View {
    let icon: String
    let label: String
    let isSelected: Bool
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            VStack(spacing: 4) {
                Image(systemName: icon)
                    .font(.system(size: 24))
                Text(label)
                    .font(.caption2)
            }
            .foregroundColor(isSelected ? .cyan : .gray)
            .frame(width: 50, height: 50)
            .background(isSelected ? Color.cyan.opacity(0.1) : Color.clear)
            .cornerRadius(8)
        }
    }
}

struct VoxelViewport: UIViewRepresentable {
    @ObservedObject var model: VoxelModel
    var rotation: SIMD3<Float>
    var zoom: Float
    var tool: ModelEditorView.Tool
    var color: Color
    var symmetry: (x: Bool, y: Bool, z: Bool)
    
    func makeUIView(context: Context) -> MTKView {
        let mtkView = MTKView()
        mtkView.device = MTLCreateSystemDefaultDevice()
        mtkView.clearColor = MTLClearColor(red: 0.05, green: 0.05, blue: 0.07, alpha: 1.0)
        mtkView.depthStencilPixelFormat = .depth32Float
        
        let renderer = AdvancedVoxelRenderer(device: mtkView.device!, model: model)
        context.coordinator.renderer = renderer
        mtkView.delegate = renderer
        
        // Add Tap Gesture for Editing
        let tap = UITapGestureRecognizer(target: context.coordinator, action: #selector(Coordinator.handleTap(_:)))
        mtkView.addGestureRecognizer(tap)
        
        return mtkView
    }
    
    func updateUIView(_ uiView: MTKView, context: Context) {
        context.coordinator.renderer?.rotation = rotation
        context.coordinator.renderer?.zoom = zoom
        context.coordinator.tool = tool
        context.coordinator.color = color
        context.coordinator.symmetry = symmetry
        context.coordinator.renderer?.updateMesh()
    }
    
    func makeCoordinator() -> Coordinator {
        Coordinator(model: model)
    }
    
    class Coordinator {
        var model: VoxelModel
        var renderer: AdvancedVoxelRenderer?
        var tool: ModelEditorView.Tool = .brush
        var color: Color = .white
        var symmetry: (x: Bool, y: Bool, z: Bool) = (false, false, false)
        
        init(model: VoxelModel) {
            self.model = model
        }
        
        @objc func handleTap(_ gesture: UITapGestureRecognizer) {
            guard let view = gesture.view as? MTKView, let renderer = renderer else { return }
            let point = gesture.location(in: view)
            
            let ray = renderer.getRay(from: point, in: view.bounds.size)
            model.editVoxel(at: ray, tool: tool, color: color, symmetry: symmetry)
        }
    }
}

struct VisualEffectBlur: UIViewRepresentable {
    var blurStyle: UIBlurEffect.Style
    func makeUIView(context: Context) -> UIVisualEffectView {
        return UIVisualEffectView(effect: UIBlurEffect(style: blurStyle))
    }
    func updateUIView(_ uiView: UIVisualEffectView, context: Context) {}
}

struct AssetBrowserView: View {
    @State private var searchText = ""
    @State private var filter: AssetFilter = .all
    @State private var viewMode: ViewMode = .grid
    @State private var sortBy: SortOption = .name
    @State private var sortAscending = true
    @State private var thumbnailSize: CGFloat = 128
    @State private var selectedAssets: Set<String> = []
    @State private var showImportDialog = false
    
    @StateObject private var bridge = AssetBrowserBridge.shared
    
    enum AssetFilter: String, CaseIterable, Identifiable {
        case all = "All"
        case textures = "Textures"
        case models = "Models"
        case audio = "Audio"
        case materials = "Materials"
        var id: String { self.rawValue }
        
        var assetType: AssetBrowserBridge.AssetType {
            switch self {
            case .all: return .all
            case .textures: return .texture
            case .models: return .model
            case .audio: return .audio
            case .materials: return .material
            }
        }
    }
    
    enum ViewMode: String, CaseIterable {
        case grid = "Grid"
        case list = "List"
        case tiles = "Tiles"
    }
    
    enum SortOption: String, CaseIterable {
        case name = "Name"
        case type = "Type"
        case date = "Date"
        case size = "Size"
    }
    
    var filteredAssets: [AssetBrowserBridge.BrowserAsset] {
        let assets = bridge.getAssets()
        
        return assets
            .filter { asset in
                // Filter by type
                if filter != .all && asset.type != filter.assetType {
                    return false
                }
                
                // Filter by search
                if !searchText.isEmpty && !asset.name.localizedCaseInsensitiveContains(searchText) {
                    return false
                }
                
                return true
            }
            .sorted { a, b in
                let result: Bool
                switch sortBy {
                case .name:
                    result = a.name < b.name
                case .type:
                    result = a.type.rawValue < b.type.rawValue
                case .date:
                    result = a.lastModified < b.lastModified
                case .size:
                    result = a.fileSize < b.fileSize
                }
                return sortAscending ? result : !result
            }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Enhanced Toolbar
            toolbarView
                .padding()
                .background(Color.black.opacity(0.2))
            
            // Stats Bar
            statsBar
                .padding(.horizontal)
                .padding(.vertical, 8)
                .background(Color.white.opacity(0.05))
            
            // Asset Content
            ZStack {
                ScrollView {
                    if viewMode == .grid {
                        gridView
                    } else if viewMode == .list {
                        listView
                    } else {
                        tilesView
                    }
                }
                .opacity(bridge.isIndexing ? 0.3 : 1.0)
                .blur(radius: bridge.isIndexing ? 2 : 0)
                
                if bridge.isIndexing {
                    VStack(spacing: 15) {
                        ProgressView()
                            .scaleEffect(1.5)
                            .accentColor(.cyan)
                        
                        VStack(spacing: 5) {
                            Text("SCANNING ASSETS...")
                                .font(.system(size: 14, weight: .black, design: .monospaced))
                                .foregroundColor(.cyan)
                            Text("Deep indexing project workspace...")
                                .font(.caption2)
                                .foregroundColor(.gray)
                        }
                    }
                    .padding(30)
                    .background(Color.black.opacity(0.8))
                    .cornerRadius(12)
                    .overlay(
                        RoundedRectangle(cornerRadius: 12)
                            .stroke(Color.cyan.opacity(0.3), lineWidth: 1)
                    )
                    .transition(.opacity)
                }
            }
        }
        .onChange(of: filter) { newFilter in
            bridge.setFilter(newFilter.assetType)
        }
        .onChange(of: searchText) { newSearch in
            bridge.setSearchQuery(newSearch)
        }
        .onChange(of: thumbnailSize) { newSize in
            bridge.setThumbnailSize(Float(newSize))
        }
    }
    
    // MARK: - Toolbar
    
    private var toolbarView: some View {
        HStack(spacing: 15) {
            // Search Box
            HStack {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(.gray)
                TextField("Search assets...", text: $searchText)
                    .textFieldStyle(PlainTextFieldStyle())
                    .foregroundColor(.white)
                
                if !searchText.isEmpty {
                    Button(action: { searchText = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(.gray)
                    }
                }
            }
            .padding(10)
            .background(Color.white.opacity(0.1))
            .cornerRadius(8)
            
            // Filter Dropdown
            Picker("Filter", selection: $filter) {
                ForEach(AssetFilter.allCases) { filterOption in
                    Text(filterOption.rawValue).tag(filterOption)
                }
            }
            .pickerStyle(SegmentedPickerStyle())
            .frame(width: 350)
            
            // View Mode
            Picker("View", selection: $viewMode) {
                ForEach(ViewMode.allCases, id: \.self) { mode in
                    Text(mode.rawValue).tag(mode)
                }
            }
            .pickerStyle(SegmentedPickerStyle())
            .frame(width: 200)
            
            Spacer()
            
            // Sort Options
            Menu {
                ForEach(SortOption.allCases, id: \.self) { option in
                    Button(action: {
                        if sortBy == option {
                            sortAscending.toggle()
                        } else {
                            sortBy = option
                            sortAscending = true
                        }
                    }) {
                        HStack {
                            Text(option.rawValue)
                            if sortBy == option {
                                Image(systemName: sortAscending ? "arrow.up" : "arrow.down")
                            }
                        }
                    }
                }
            } label: {
                HStack {
                    Image(systemName: "arrow.up.arrow.down")
                    Text("Sort")
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 8)
                .background(Color.white.opacity(0.1))
                .foregroundColor(.white)
                .cornerRadius(8)
            }
            
            // Thumbnail Size (for grid view)
            if viewMode == .grid {
                HStack {
                    Image(systemName: "square.grid.2x2")
                        .foregroundColor(.gray)
                    Slider(value: $thumbnailSize, in: 64...256, step: 16)
                        .frame(width: 100)
                        .accentColor(.cyan)
                }
            }
            
            // Actions
            Button(action: { bridge.refreshAssets() }) {
                Image(systemName: "arrow.clockwise")
                    .padding(8)
                    .background(Color.white.opacity(0.1))
                    .foregroundColor(.white)
                    .cornerRadius(8)
            }
            
            Button(action: { showImportDialog = true }) {
                Label("Import", systemImage: "square.and.arrow.down")
                    .font(.headline)
                    .padding(.horizontal, 15)
                    .padding(.vertical, 8)
                    .background(Color.cyan.opacity(0.2))
                    .foregroundColor(.cyan)
                    .cornerRadius(8)
            }
        }
    }
    
    // MARK: - Stats Bar
    
    private var statsBar: some View {
        let stats = bridge.getStats()
        
        return HStack(spacing: 20) {
            Text("\(filteredAssets.count) of \(stats.totalAssets) assets")
                .font(.caption)
                .foregroundColor(.gray)
            
            if selectedAssets.count > 0 {
                Text("•")
                    .foregroundColor(.gray)
                Text("\(selectedAssets.count) selected")
                    .font(.caption)
                    .foregroundColor(.cyan)
            }
            
            Spacer()
            
            Text("Scan time: \(String(format: "%.2f", stats.scanTime))ms")
                .font(.caption.monospaced())
                .foregroundColor(.gray)
        }
    }
    
    // MARK: - Grid View
    
    private var gridView: some View {
        LazyVGrid(columns: [GridItem(.adaptive(minimum: thumbnailSize), spacing: 20)], spacing: 20) {
            // Create New Cards
            CreateNewCard(icon: "plus.square.dashed", label: "New Sprite", color: .cyan)
            CreateNewCard(icon: "plus.cube.dashed", label: "New Model", color: .purple)
            
            // Asset Cards
            ForEach(bridge.assets) { asset in
                EnhancedAssetCard(
                    asset: asset,
                    isSelected: selectedAssets.contains(asset.filePath),
                    thumbnailSize: thumbnailSize
                )
                .onTapGesture {
                    toggleSelection(asset.filePath)
                }
            }
        }
        .padding()
    }
    
    // MARK: - List View
    
    private var listView: some View {
        LazyVStack(spacing: 1) {
            ForEach(bridge.assets) { asset in
                AssetListRow(
                    asset: asset,
                    isSelected: selectedAssets.contains(asset.filePath)
                )
                .onTapGesture {
                    toggleSelection(asset.filePath)
                }
            }
        }
        .padding()
    }
    
    // MARK: - Tiles View
    
    private var tilesView: some View {
        LazyVStack(spacing: 12) {
            ForEach(bridge.assets) { asset in
                AssetTileRow(
                    asset: asset,
                    isSelected: selectedAssets.contains(asset.filePath)
                )
                .onTapGesture {
                    toggleSelection(asset.filePath)
                }
            }
        }
        .padding()
    }
    
    // MARK: - Helper Methods
    
    private func toggleSelection(_ filePath: String) {
        if selectedAssets.contains(filePath) {
            selectedAssets.remove(filePath)
        } else {
            selectedAssets.insert(filePath)
        }
    }
}

// MARK: - Enhanced Asset Card

struct EnhancedAssetCard: View {
    let asset: AssetBrowserBridge.BrowserAsset
    let isSelected: Bool
    let thumbnailSize: CGFloat
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Thumbnail Preview
            ZStack {
                Color.black.opacity(0.3)
                
                if asset.hasThumbnail {
                    Image(systemName: iconForAssetType(asset.type))
                        .font(.system(size: thumbnailSize * 0.4))
                        .foregroundColor(colorForAssetType(asset.type))
                } else {
                    VStack {
                        Image(systemName: iconForAssetType(asset.type))
                            .font(.system(size: thumbnailSize * 0.3))
                            .foregroundColor(colorForAssetType(asset.type).opacity(0.5))
                        Text("No Preview")
                            .font(.caption2)
                            .foregroundColor(.gray)
                    }
                }
            }
            .frame(height: thumbnailSize)
            
            // Info
            VStack(alignment: .leading, spacing: 4) {
                Text(asset.name)
                    .font(.caption.weight(.medium))
                    .foregroundColor(.white)
                    .lineLimit(2)
                    .fixedSize(horizontal: false, vertical: true)
                
                HStack(spacing: 4) {
                    Image(systemName: iconForAssetType(asset.type))
                        .font(.caption2)
                    Text(formatFileSize(asset.fileSize))
                        .font(.caption2)
                    Spacer()
                    if asset.hasThumbnail {
                        Image(systemName: "photo")
                            .font(.caption2)
                    }
                }
                .foregroundColor(.gray)
            }
            .padding(10)
            .background(Color.white.opacity(0.05))
        }
        .cornerRadius(12)
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(isSelected ? Color.cyan : Color.white.opacity(0.1), lineWidth: isSelected ? 2 : 1)
        )
        .shadow(color: isSelected ? Color.cyan.opacity(0.3) : Color.clear, radius: 8)
    }
    
    private func iconForAssetType(_ type: AssetBrowserBridge.AssetType) -> String {
        switch type {
        case .texture: return "photo"
        case .model: return "cube"
        case .audio: return "waveform"
        case .material: return "paintpalette"
        default: return "doc"
        }
    }
    
    private func colorForAssetType(_ type: AssetBrowserBridge.AssetType) -> Color {
        switch type {
        case .texture: return .cyan
        case .model: return .purple
        case .audio: return .green
        case .material: return .orange
        default: return .gray
        }
    }
    
    private func formatFileSize(_ bytes: UInt64) -> String {
        let kb = Double(bytes) / 1024
        let mb = kb / 1024
        
        if mb >= 1 {
            return String(format: "%.1f MB", mb)
        } else {
            return String(format: "%.0f KB", kb)
        }
    }
}

// MARK: - List Row

struct AssetListRow: View {
    let asset: AssetBrowserBridge.BrowserAsset
    let isSelected: Bool
    
    var body: some View {
        HStack(spacing: 12) {
            // Icon
            Image(systemName: iconForAssetType(asset.type))
                .font(.title3)
                .foregroundColor(colorForAssetType(asset.type))
                .frame(width: 40)
            
            // Name
            Text(asset.name)
                .foregroundColor(.white)
                .lineLimit(1)
            
            Spacer()
            
            // Type
            Text(typeLabel(asset.type))
                .font(.caption)
                .foregroundColor(.gray)
                .frame(width: 80, alignment: .leading)
            
            // Size
            Text(formatFileSize(asset.fileSize))
                .font(.caption.monospaced())
                .foregroundColor(.gray)
                .frame(width: 80, alignment: .trailing)
            
            // Date
            Text(formatDate(asset.lastModified))
                .font(.caption)
                .foregroundColor(.gray)
                .frame(width: 100, alignment: .trailing)
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 8)
        .background(isSelected ? Color.cyan.opacity(0.1) : Color.clear)
        .cornerRadius(6)
    }
    
    private func iconForAssetType(_ type: AssetBrowserBridge.AssetType) -> String {
        switch type {
        case .texture: return "photo"
        case .model: return "cube"
        case .audio: return "waveform"
        case .material: return "paintpalette"
        default: return "doc"
        }
    }
    
    private func colorForAssetType(_ type: AssetBrowserBridge.AssetType) -> Color {
        switch type {
        case .texture: return .cyan
        case .model: return .purple
        case .audio: return .green
        case .material: return .orange
        default: return .gray
        }
    }
    
    private func typeLabel(_ type: AssetBrowserBridge.AssetType) -> String {
        switch type {
        case .texture: return "Texture"
        case .model: return "Model"
        case .audio: return "Audio"
        case .material: return "Material"
        default: return "Unknown"
        }
    }
    
    private func formatFileSize(_ bytes: UInt64) -> String {
        let kb = Double(bytes) / 1024
        let mb = kb / 1024
        
        if mb >= 1 {
            return String(format: "%.1f MB", mb)
        } else {
            return String(format: "%.0f KB", kb)
        }
    }
    
    private func formatDate(_ timestamp: UInt64) -> String {
        let date = Date(timeIntervalSince1970: TimeInterval(timestamp))
        let formatter = RelativeDateTimeFormatter()
        formatter.unitsStyle = .abbreviated
        return formatter.localizedString(for: date, relativeTo: Date())
    }
}

// MARK: - Tile Row

struct AssetTileRow: View {
    let asset: AssetBrowserBridge.BrowserAsset
    let isSelected: Bool
    
    var body: some View {
        HStack(spacing: 16) {
            // Large Thumbnail
            ZStack {
                Color.black.opacity(0.3)
                Image(systemName: iconForAssetType(asset.type))
                    .font(.system(size: 40))
                    .foregroundColor(colorForAssetType(asset.type))
            }
            .frame(width: 80, height: 80)
            .cornerRadius(8)
            
            // Info
            VStack(alignment: .leading, spacing: 6) {
                Text(asset.name)
                    .font(.headline)
                    .foregroundColor(.white)
                
                HStack(spacing: 12) {
                    Label(typeLabel(asset.type), systemImage: iconForAssetType(asset.type))
                        .font(.caption)
                        .foregroundColor(colorForAssetType(asset.type))
                    
                    Text(formatFileSize(asset.fileSize))
                        .font(.caption.monospaced())
                        .foregroundColor(.gray)
                }
                
                Text(asset.filePath)
                    .font(.caption2)
                    .foregroundColor(.gray)
                    .lineLimit(1)
            }
            
            Spacer()
        }
        .padding(12)
        .background(isSelected ? Color.cyan.opacity(0.1) : Color.white.opacity(0.05))
        .cornerRadius(12)
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(isSelected ? Color.cyan : Color.white.opacity(0.1), lineWidth: isSelected ? 2 : 1)
        )
    }
    
    private func iconForAssetType(_ type: AssetBrowserBridge.AssetType) -> String {
        switch type {
        case .texture: return "photo"
        case .model: return "cube"
        case .audio: return "waveform"
        case .material: return "paintpalette"
        default: return "doc"
        }
    }
    
    private func colorForAssetType(_ type: AssetBrowserBridge.AssetType) -> Color {
        switch type {
        case .texture: return .cyan
        case .model: return .purple
        case .audio: return .green
        case .material: return .orange
        default: return .gray
        }
    }
    
    private func typeLabel(_ type: AssetBrowserBridge.AssetType) -> String {
        switch type {
        case .texture: return "Texture"
        case .model: return "Model"
        case .audio: return "Audio"
        case .material: return "Material"
        default: return "Unknown"
        }
    }
    
    private func formatFileSize(_ bytes: UInt64) -> String {
        let kb = Double(bytes) / 1024
        let mb = kb / 1024
        
        if mb >= 1 {
            return String(format: "%.1f MB", mb)
        } else {
            return String(format: "%.0f KB", kb)
        }
    }
}


struct AssetItem: Identifiable {
    let id = UUID()
    let name: String
    let type: AssetBrowserView.AssetType
    let date: String
    let icon: String
}

struct CreateNewCard: View {
    let icon: String
    let label: String
    let color: Color
    
    var body: some View {
        VStack(spacing: 12) {
            Image(systemName: icon)
                .font(.system(size: 40))
                .foregroundColor(color)
            Text(label)
                .font(.headline)
                .foregroundColor(color)
        }
        .frame(height: 180)
        .frame(maxWidth: .infinity)
        .background(color.opacity(0.1))
        .cornerRadius(12)
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(color.opacity(0.5), style: StrokeStyle(lineWidth: 2, dash: [5]))
        )
    }
}

struct AssetCard: View {
    let asset: AssetItem
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Icon Preview
            ZStack {
                Color.black.opacity(0.3)
                Image(systemName: asset.icon)
                    .font(.system(size: 40))
                    .foregroundColor(asset.type == .sprite ? .cyan : .purple)
            }
            .frame(height: 120)
            
            // Info
            VStack(alignment: .leading, spacing: 4) {
                Text(asset.name)
                    .font(.headline)
                    .foregroundColor(.white)
                    .lineLimit(1)
                
                HStack {
                    Image(systemName: asset.type == .sprite ? "paintpalette" : "cube")
                        .font(.caption2)
                    Text(asset.type.rawValue)
                        .font(.caption2)
                    Spacer()
                    Text(asset.date)
                        .font(.caption2)
                }
                .foregroundColor(.gray)
            }
            .padding(10)
            .background(Color.white.opacity(0.05))
        }
        .cornerRadius(12)
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(Color.white.opacity(0.1), lineWidth: 1)
        )
    }
}
