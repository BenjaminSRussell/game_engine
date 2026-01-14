import SwiftUI
import UniformTypeIdentifiers  // For UTType

// MARK: - Content Browser Panel

struct ContentBrowserPanel: View {
    @StateObject private var viewModel = ContentBrowserViewModel()
    @State private var searchText = ""
    @State private var viewMode: ViewMode = .grid
    @State private var selectedAssets: Set<UUID> = []
    
    enum ViewMode {
        case grid, list
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Title bar with breadcrumbs
            HStack {
                Text("Content Browser")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                // View mode toggle
                HStack(spacing: 0) {
                    Button(action: { viewMode = .grid }) {
                        Image(systemName: "square.grid.2x2")
                            .foregroundColor(viewMode == .grid ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                            .frame(width: 28, height: 28)
                    }
                    .buttonStyle(.plain)
                    
                    Button(action: { viewMode = .list }) {
                        Image(systemName: "list.bullet")
                            .foregroundColor(viewMode == .list ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                            .frame(width: 28, height: 28)
                    }
                    .buttonStyle(.plain)
                }
                
                EditorIconButton(icon: "square.and.arrow.down", tooltip: "Import Asset") {
                    viewModel.importAsset()
                }
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Breadcrumb navigation
            BreadcrumbNavigation(path: $viewModel.currentPath)
                .padding(.horizontal, DesignSystem.Spacing.sm)
                .padding(.vertical, DesignSystem.Spacing.xs)
            
            EditorDivider()
            
            // Search and filters
            HStack(spacing: DesignSystem.Spacing.sm) {
                EditorSearchBar(text: $searchText, placeholder: "Search assets...")
                
                Menu {
                    ForEach(AssetType.allCases, id: \.self) { type in
                        Button(type.rawValue.capitalized) {
                            viewModel.filterByType(type)
                        }
                    }
                } label: {
                    HStack {
                        Image(systemName: "line.3.horizontal.decrease.circle")
                        Text("Filter")
                    }
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, DesignSystem.Spacing.sm)
                    .padding(.vertical, DesignSystem.Spacing.xs)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
                }
                .menuStyle(.borderlessButton)
                
                // Sort Menu
                Menu {
                    Button(action: { viewModel.sortOption = .name }) {
                        Label("Name", systemImage: "textformat")
                    }
                    Button(action: { viewModel.sortOption = .date }) {
                        Label("Date Modified", systemImage: "calendar")
                    }
                    Button(action: { viewModel.sortOption = .size }) {
                        Label("Size", systemImage: "scalemass")
                    }
                } label: {
                    HStack {
                        Image(systemName: "arrow.up.arrow.down.circle")
                        Text(viewModel.sortOption == .date ? "Recent" : "Sort") // Simplified "Recent" label for date sort
                    }
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, DesignSystem.Spacing.sm)
                    .padding(.vertical, DesignSystem.Spacing.xs)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
                }
                .menuStyle(.borderlessButton)
            }
            .padding(DesignSystem.Spacing.sm)
            
            EditorDivider()
            
            // Asset grid/list
            ScrollView {
                if viewMode == .grid {
                    AssetGridView(assets: filteredAssets, selectedAssets: $selectedAssets)
                } else {
                    AssetListView(assets: filteredAssets, selectedAssets: $selectedAssets)
                }
            }
            .background(DesignSystem.Colors.backgroundSecondary)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
    
    private var filteredAssets: [AssetItem] {
        let filtered = viewModel.assets.filter { asset in
            let matchesSearch = searchText.isEmpty || asset.name.lowercased().contains(searchText.lowercased())
            let matchesType = viewModel.selectedFilter == nil || asset.type == viewModel.selectedFilter
            return matchesSearch && matchesType
        }
        return viewModel.sortAssets(filtered)
    }
}

// MARK: - Breadcrumb Navigation

private struct BreadcrumbNavigation: View {
    @Binding var path: [String]
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            ForEach(Array(path.enumerated()), id: \.offset) { index, folder in
                Button(action: {
                    // Navigate to this folder
                    path = Array(path.prefix(index + 1))
                }) {
                    Text(folder)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(index == path.count - 1 ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                
                if index < path.count - 1 {
                    Image(systemName: "chevron.right")
                        .font(.system(size: 10))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            
            Spacer()
        }
    }
}

// MARK: - View Model

class ContentBrowserViewModel: ObservableObject {
    @Published var assets: [AssetItem] = []
    @Published var currentPath: [String] = ["Assets"]
    @Published var selectedFilter: AssetType?
    
    @Published var rotation: Double = 0
    @Published var sortOption: SortOption = .name
    
    enum SortOption {
        case name, date, size
    }
    
    init() {
        loadDemoAssets()
    }
    
    func loadDemoAssets() {
        assets = [
            // Folders
            // Note: AssetType in AssetBrowser.swift doesn't have .folder yet, treating as .other for now or need to add it
            // Using .other for folders as placeholder since AssetType in AssetBrowser.swift doesn't have explicitly .folder
            // Actually AssetType has .other. Let's assume directories are handled differently or add .folder to AssetType in AssetBrowser.swift.
             
            // Using existing types from AssetBrowser.swift
            AssetItem(name: "grass_diffuse.png", type: .texture, size: 2048576, modifiedDate: Date().addingTimeInterval(-86400), path: "Assets/Textures/grass_diffuse.png"),
            AssetItem(name: "dirt_diffuse.png", type: .texture, size: 1536000, modifiedDate: Date().addingTimeInterval(-172800), path: "Assets/Textures/dirt_diffuse.png"),
            AssetItem(name: "stone_diffuse.png", type: .texture, size: 1843200, modifiedDate: Date().addingTimeInterval(-3600), path: "Assets/Textures/stone_diffuse.png"),
            AssetItem(name: "player.obj", type: .mesh, size: 524288, modifiedDate: Date().addingTimeInterval(-604800), path: "Assets/Models/player.obj"),
            AssetItem(name: "terrain_material.mat", type: .material, size: 4096, modifiedDate: Date().addingTimeInterval(-43200), path: "Assets/Materials/terrain_material.mat"),
            AssetItem(name: "ambient_sound.ogg", type: .audio, size: 3145728, modifiedDate: Date().addingTimeInterval(-120), path: "Assets/Audio/ambient_sound.ogg"),
        ]
    }
    
    func sortAssets(_ assets: [AssetItem]) -> [AssetItem] {
        switch sortOption {
        case .name: return assets.sorted { $0.name < $1.name }
        case .date: return assets.sorted { $0.modifiedDate > $1.modifiedDate }
        case .size: return assets.sorted { $0.size > $1.size }
        }
    }
    
    func filterByType(_ type: AssetType) {
        if selectedFilter == type {
            selectedFilter = nil
        } else {
            selectedFilter = type
        }
    }
    
    func importAsset() {
        // Show file picker and import asset
        #if os(macOS)
        let panel = NSOpenPanel()
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = true
        panel.allowedContentTypes = [
            .image, .audio, .movie,
            UTType(filenameExtension: "obj")!,
            UTType(filenameExtension: "fbx")!,
            UTType(filenameExtension: "gltf")!
        ]
        
        panel.begin { [weak self] response in
            guard let self = self else { return }
            if response == .OK {
                for url in panel.urls {
                    do {
                        let destPath = self.currentPath.joined(separator: "/") + "/" + url.lastPathComponent
                        try FileManager.default.copyItem(at: url, to: URL(fileURLWithPath: destPath))
                        print("[Swift] Imported asset: \(url.lastPathComponent)")
                    } catch {
                        print("[Swift] Import error: \(error)")
                    }
                }
                self.refreshAssets()
            }
        }
        #endif
    }
    
    func refreshAssets() {
        // Reload assets from filesystem
        // For now, just reload demo assets
        loadDemoAssets()
    }
}

// MARK: - Asset Grid View
private struct AssetGridView: View {
    let assets: [AssetItem]
    @Binding var selectedAssets: Set<UUID>
    
    let columns = [
        GridItem(.adaptive(minimum: 100, maximum: 120), spacing: DesignSystem.Spacing.md)
    ]
    
    var body: some View {
        LazyVGrid(columns: columns, spacing: DesignSystem.Spacing.md) {
            ForEach(assets) { asset in
                AssetGridItem(asset: asset, isSelected: selectedAssets.contains(asset.id))
                    .onTapGesture {
                        if NSEvent.modifierFlags.contains(.command) {
                            if selectedAssets.contains(asset.id) {
                                selectedAssets.remove(asset.id)
                            } else {
                                selectedAssets.insert(asset.id)
                            }
                        } else {
                            selectedAssets = [asset.id]
                        }
                    }
            }
        }
        .padding(DesignSystem.Spacing.md)
    }
}

// MARK: - Asset Grid Item
private struct AssetGridItem: View {
    let asset: AssetItem
    let isSelected: Bool
    @State private var isHovering = false
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.xs) {
            ZStack {
                RoundedRectangle(cornerRadius: 6)
                    .fill(DesignSystem.Colors.backgroundTertiary)
                    .aspectRatio(1.0, contentMode: .fit)
                
                Image(systemName: asset.type.icon)
                    .font(.system(size: 40))
                    .foregroundColor(asset.type.color)
            }
            .overlay(
                RoundedRectangle(cornerRadius: 6)
                    .stroke(isSelected ? DesignSystem.Colors.accentPrimary : (isHovering ? DesignSystem.Colors.textSecondary : Color.clear), lineWidth: 2)
            )
            
            Text(asset.name)
                .font(DesignSystem.Typography.small)
                .foregroundColor(isSelected ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                .lineLimit(2)
                .multilineTextAlignment(.center)
        }
        .onHover { isHovering = $0 }
    }
}

// MARK: - Asset List View
private struct AssetListView: View {
    let assets: [AssetItem]
    @Binding var selectedAssets: Set<UUID>
    
    var body: some View {
        LazyVStack(spacing: 0) {
            ForEach(assets) { asset in
                AssetListRow(asset: asset, isSelected: selectedAssets.contains(asset.id))
                    .onTapGesture {
                        if NSEvent.modifierFlags.contains(.command) {
                            if selectedAssets.contains(asset.id) {
                                selectedAssets.remove(asset.id)
                            } else {
                                selectedAssets.insert(asset.id)
                            }
                        } else {
                            selectedAssets = [asset.id]
                        }
                    }
            }
        }
    }
}

// MARK: - Asset List Row
private struct AssetListRow: View {
    let asset: AssetItem
    let isSelected: Bool
    @State private var isHovering = false
    
    var body: some View {
        HStack {
            Image(systemName: asset.type.icon)
                .foregroundColor(asset.type.color)
                .frame(width: 20)
            
            Text(asset.name)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Spacer()
            
            Text(asset.type.rawValue)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 80, alignment: .leading)
            
            Text(asset.formattedSize)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 80, alignment: .trailing)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(isSelected ? DesignSystem.Colors.selection : (isHovering ? DesignSystem.Colors.hover : Color.clear))
        .onHover { isHovering = $0 }
    }
}

// MARK: - Preview
#Preview {
    ContentBrowserPanel()
        .frame(height: 300)
}

