// SwiftUI/AssetBrowser.swift
// Asset management and import pipeline
import SwiftUI
import UniformTypeIdentifiers

struct AssetBrowserView: View {
    @State private var assets: [Asset] = []
    @State private var selectedFolder: AssetFolder? = .textures
    @State private var selectedAsset: Asset?
    @State private var searchText = ""
    
    var body: some View {
        NavigationSplitView {
            // Left: Folder tree
            List(selection: $selectedFolder) {
                Section("Content") {
                    NavigationLink(value: AssetFolder.textures) {
                        Label("Textures", systemImage: "photo.fill.on.rectangle.fill")
                    }
                    NavigationLink(value: AssetFolder.materials) {
                        Label("Materials", systemImage: "paintpalette.fill")
                    }
                    NavigationLink(value: AssetFolder.meshes) {
                        Label("Meshes", systemImage: "cube.fill")
                    }
                    NavigationLink(value: AssetFolder.sounds) {
                        Label("Sounds", systemImage: "waveform")
                    }
                    NavigationLink(value: AssetFolder.animations) {
                        Label("Animations", systemImage: "figure.walk")
                    }
                }
            }
            .listStyle(.sidebar)
        } content: {
            // Center: Asset grid
            VStack(spacing: 0) {
                // Toolbar
                HStack {
                    TextField("Search", text: $searchText)
                        .textFieldStyle(.roundedBorder)
                        .frame(maxWidth: 300)
                    
                    Spacer()
                    
                    Menu {
                        Button("Import FBX/GLTF...") { importMesh() }
                        Button("Import Texture...") { importTexture() }
                        Button("Import Audio...") { importAudio() }
                    } label: {
                        Label("Import", systemImage: "plus.circle.fill")
                    }
                }
                .padding()
                
                Divider()
                
                // Asset grid
                ScrollView {
                    LazyVGrid(columns: [
                        GridItem(.adaptive(minimum: 120), spacing: 16)
                    ], spacing: 16) {
                        ForEach(filteredAssets) { asset in
                            AssetThumbnail(asset: asset)
                                .onTapGesture {
                                    selectedAsset = asset
                                }
                        }
                    }
                    .padding()
                }
            }
        } detail: {
            // Right: Asset inspector
            if let asset = selectedAsset {
                AssetInspectorView(asset: asset)
            } else {
                Text("No asset selected")
                    .foregroundColor(.secondary)
            }
        }
        .onAppear {
            loadAssets()
        }
    }
    
    var filteredAssets: [Asset] {
        assets.filter { asset in
            searchText.isEmpty || asset.name.localizedCaseInsensitiveContains(searchText)
        }
    }
    
    func loadAssets() {
        // Load from asset database
        assets = [
            Asset(name: "Brick_Albedo", type: .texture),
            Asset(name: "Metal_Material", type: .material),
            Asset(name: "Character_Mesh", type: .mesh)
        ]
    }
    
    func importMesh() {
        let panel = NSOpenPanel()
        panel.allowedContentTypes = [UTType(filenameExtension: "fbx")!,
                                     UTType(filenameExtension: "gltf")!]
        panel.begin { response in
            if response == .OK, let url = panel.url {
                // Import mesh
                print("Importing mesh: \\(url)")
            }
        }
    }
    
    func importTexture() {
        let panel = NSOpenPanel()
        panel.allowedContentTypes = [.png, .jpeg, .tiff]
        panel.begin { response in
            if response == .OK, let url = panel.url {
                print("Importing texture: \\(url)")
            }
        }
    }
    
    func importAudio() {
        let panel = NSOpenPanel()
        panel.allowedContentTypes = [.wav, .mp3, .aiff]
        panel.begin { response in
            if response == .OK, let url = panel.url {
                print("Importing audio: \\(url)")
            }
        }
    }
}

struct AssetThumbnail: View {
    let asset: Asset
    
    var body: some View {
        VStack {
            ZStack {
                RoundedRectangle(cornerRadius: 8)
                    .fill(Color.gray.opacity(0.2))
                    .frame(width: 100, height: 100)
                
                Image(systemName: asset.type.icon)
                    .font(.largeTitle)
                    .foregroundColor(.secondary)
            }
            
            Text(asset.name)
                .font(.caption)
                .lineLimit(2)
                .frame(width: 100)
        }
    }
}

struct AssetInspectorView: View {
    let asset: Asset
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Asset Info")
                .font(.headline)
            
            Form {
                Section {
                    LabeledContent("Name", value: asset.name)
                    LabeledContent("Type", value: asset.type.rawValue)
                    LabeledContent("Size", value: "2.5 MB")
                    LabeledContent("Modified", value: "Today")
                }
                
                Section("Metadata") {
                    // Asset-specific properties
                    if asset.type == .texture {
                        LabeledContent("Resolution", value: "2048x2048")
                        LabeledContent("Format", value: "PNG")
                        LabeledContent("Compression", value: "BC7")
                    }
                }
            }
            .formStyle(.grouped)
            
            Spacer()
        }
        .padding()
    }
}

enum AssetFolder: Hashable {
    case textures, materials, meshes, sounds, animations
}

struct Asset: Identifiable {
    let id = UUID()
    var name: String
    var type: AssetType
}

enum AssetType: String {
    case texture = "Texture"
    case material = "Material"
    case mesh = "Mesh"
    case sound = "Sound"
    case animation = "Animation"
    
    var icon: String {
        switch self {
        case .texture: return "photo.fill"
        case .material: return "paintpalette.fill"
        case .mesh: return "cube.fill"
        case .sound: return "waveform"
        case .animation: return "figure.walk"
        }
    }
}
