// SwiftUI/EditorApp.swift
// Premium SwiftUI Editor - Main Application
import SwiftUI
import MetalKit

@main
struct EngineEditorApp: App {
    @StateObject private var editorState = EditorState()
    
    var body: some Scene {
        WindowGroup {
            EditorMainView()
                .environmentObject(editorState)
                .frame(minWidth: 1200, minHeight: 800)
        }
        .commands {
            EditorCommands()
        }
        
        // Asset Browser window
        WindowGroup("Asset Browser") {
            AssetBrowserView()
                .environmentObject(editorState)
        }
        
        // Material Editor window
        WindowGroup("Material Editor") {
            MaterialEditorView()
                .environmentObject(editorState)
        }
    }
}

// Main editor layout
struct EditorMainView: View {
    @EnvironmentObject var editorState: EditorState
    @State private var selectedTab: EditorTab = .viewport
    
    enum EditorTab {
        case viewport, blueprint, sequencer, profiler
    }
    
    var body: some View {
        HSplitView {
            // Left sidebar - Scene hierarchy
            SceneHierarchyView()
                .frame(minWidth: 200, idealWidth: 250, maxWidth: 400)
            
            // Center - Main content
            VStack(spacing: 0) {
                // Tab bar
                Picker("", selection: $selectedTab) {
                    Label("Viewport", systemImage: "cube.fill").tag(EditorTab.viewport)
                    Label("Blueprint", systemImage: "flowchart.fill").tag(EditorTab.blueprint)
                    Label("Sequencer", systemImage: "film.fill").tag(EditorTab.sequencer)
                    Label("Profiler", systemImage: "chart.bar.fill").tag(EditorTab.profiler)
                }
                .pickerStyle(.segmented)
                .padding(8)
                
                // Main content area
                Group {
                    switch selectedTab {
                    case .viewport:
                        MetalViewportView()
                    case .blueprint:
                        BlueprintEditorView()
                    case .sequencer:
                        SequencerView()
                    case .profiler:
                        ProfilerView()
                    }
                }
            }
            
            // Right sidebar - Inspector
            InspectorView()
                .frame(minWidth: 250, idealWidth: 300, maxWidth: 500)
        }
        .toolbar {
            EditorToolbar()
        }
    }
}

// Scene hierarchy (left panel)
struct SceneHierarchyView: View {
    @EnvironmentObject var editorState: EditorState
    @State private var selectedEntity: UUID?
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Scene")
                    .font(.headline)
                Spacer()
                Button(action: { /* Add entity */ }) {
                    Image(systemName: "plus.circle.fill")
                }
            }
            .padding(8)
            .background(Color(nsColor: .controlBackgroundColor))
            
            List(selection: $selectedEntity) {
                ForEach(editorState.entities) { entity in
                    HStack {
                        Image(systemName: entity.icon)
                        Text(entity.name)
                        Spacer()
                        if !entity.visible {
                            Image(systemName: "eye.slash")
                                .foregroundColor(.secondary)
                        }
                    }
                    .contextMenu {
                        Button("Duplicate") { }
                        Button("Delete") { }
                    }
                }
            }
        }
    }
}

// Metal viewport (center - main 3D view)
struct MetalViewportView: View {
    @StateObject private var renderer = MetalRenderer()
    @State private var viewportSize: CGSize = .zero
    
    var body: some View {
        GeometryReader { geometry in
            MetalView(renderer: renderer)
                .overlay(alignment: .topLeading) {
                    ViewportControls()
                        .padding()
                }
                .overlay(alignment: .bottomTrailing) {
                    ViewportStats(renderer: renderer)
                        .padding()
                }
                .onAppear {
                    viewportSize = geometry.size
                }
        }
    }
}

struct ViewportControls: View {
    var body: some View {
        HStack(spacing: 12) {
            // View mode
            Menu {
                Button("Lit") { }
                Button("Wireframe") { }
                Button("Normals") { }
                Button("Overdraw") { }
            } label: {
                Label("Lit", systemImage: "lightbulb.fill")
            }
            
            // Render features
            Toggle(isOn: .constant(true)) {
                Image(systemName: "sparkles")
            }
            .help("MetalFX Upscaling")
            
            Toggle(isOn: .constant(true)) {
                Image(systemName: "sun.max.fill")
            }
            .help("Lumen GI")
        }
        .padding(8)
        .background(.ultraThinMaterial)
        .cornerRadius(8)
    }
}

struct ViewportStats: View {
    @ObservedObject var renderer: MetalRenderer
    
    var body: some View {
        VStack(alignment: .trailing, spacing: 4) {
            Text("FPS: \\(renderer.fps, specifier: "%.1f")")
            Text("Draw Calls: \\(renderer.drawCalls)")
            Text("Tris: \\(renderer.triangles / 1000)k")
        }
        .font(.system(.caption, design: .monospaced))
        .padding(6)
        .background(.ultraThinMaterial)
        .cornerRadius(6)
    }
}

// Inspector panel (right)
struct InspectorView: View {
    @EnvironmentObject var editorState: EditorState
    
    var body: some View {
        VStack(spacing: 0) {
            Text("Inspector")
                .font(.headline)
                .frame(maxWidth: .infinity, alignment: .leading)
                .padding(8)
                .background(Color(nsColor: .controlBackgroundColor))
            
            if let entity = editorState.selectedEntity {
                ScrollView {
                    VStack(alignment: .leading, spacing: 16) {
                        // Transform
                        GroupBox("Transform") {
                            VStack(spacing: 8) {
                                Vec3Field(label: "Position", value: .constant(Vec3()))
                                Vec3Field(label: "Rotation", value: .constant(Vec3()))
                                Vec3Field(label: "Scale", value: .constant(Vec3(1, 1, 1)))
                            }
                        }
                        
                        // Components
                        ForEach(entity.components) { component in
                            ComponentInspector(component: component)
                        }
                    }
                    .padding()
                }
            } else {
                Spacer()
                Text("No selection")
                    .foregroundColor(.secondary)
                Spacer()
            }
        }
    }
}

struct Vec3Field: View {
    let label: String
    @Binding var value: Vec3
    
    var body: some View {
        HStack {
            Text(label)
                .frame(width: 70, alignment: .leading)
            TextField("X", value: $value.x, format: .number)
                .textFieldStyle(.roundedBorder)
            TextField("Y", value: $value.y, format: .number)
                .textFieldStyle(.roundedBorder)
            TextField("Z", value: $value.z, format: .number)
                .textFieldStyle(.roundedBorder)
        }
    }
}

// Supporting types
struct Vec3 {
    var x: Float = 0
    var y: Float = 0
    var z: Float = 0
}

class EditorState: ObservableObject {
    @Published var entities: [Entity] = []
    @Published var selectedEntity: Entity?
}

struct Entity: Identifiable {
    let id = UUID()
    var name: String
    var icon: String
    var visible: Bool = true
    var components: [Component] = []
}

struct Component: Identifiable {
    let id = UUID()
    var type: String
}
