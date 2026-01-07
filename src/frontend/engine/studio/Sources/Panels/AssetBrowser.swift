import SwiftUI
import UniformTypeIdentifiers

// ═══════════════════════════════════════════════════════════════════════════════
// MARK: - CATEGORY 4: ASSET PIPELINE & BROWSER (TODO-1901 to TODO-2600)
// ═══════════════════════════════════════════════════════════════════════════════

// MARK: - Asset Browser UI TODOs (TODO-1901 to TODO-1950)
// TODO-1901: Grid view with adjustable thumbnail size
// TODO-1902: List view with sortable columns
// TODO-1903: Column view (macOS Finder-style)
// TODO-1904: Tree view for hierarchical assets
// TODO-1905: Thumbnail size slider
// TODO-1906: Thumbnail quality settings
// TODO-1907: Thumbnail caching system
// TODO-1908: Thumbnail background rendering
// TODO-1909: Custom thumbnail generation per-type
// TODO-1910: Asset preview panel
// TODO-1911: Asset quick look (spacebar)
// TODO-1912: Asset multi-selection
// TODO-1913: Asset drag-and-drop to scene
// TODO-1914: Asset drag-and-drop reordering
// TODO-1915: Asset copy/paste
// TODO-1916: Asset duplicate with Cmd+D
// TODO-1917: Asset rename inline
// TODO-1918: Asset delete with confirmation
// TODO-1919: Asset context menu
// TODO-1920: Asset keyboard navigation
// TODO-1921: Asset search with filters
// TODO-1922: Asset search history
// TODO-1923: Asset saved searches
// TODO-1924: Asset smart folders
// TODO-1925: Asset recent files

// MARK: - Asset Import TODOs (TODO-1951 to TODO-2000)
// TODO-1951: Drag-and-drop import
// TODO-1952: File browser import
// TODO-1953: Batch import
// TODO-1954: Import progress bar
// TODO-1955: Import cancellation
// TODO-1956: Import error handling
// TODO-1957: Import conflict resolution
// TODO-1958: Import duplicate detection
// TODO-1959: Import auto-rename
// TODO-1960: Import preset per-type
// TODO-1961: Texture import settings (compression, mipmap, format)
// TODO-1962: Model import settings (scale, rotation, materials)
// TODO-1963: Audio import settings (compression, sample rate)
// TODO-1964: FBX import with animations
// TODO-1965: OBJ import
// TODO-1966: GLTF/GLB import
// TODO-1967: Collada import
// TODO-1968: USD import
// TODO-1969: Alembic import
// TODO-1970: PNG/JPG/TGA/EXR texture import
// TODO-1971: HDR/HDRI import
// TODO-1972: PSD import with layers
// TODO-1973: SVG import
// TODO-1974: Font import (TTF, OTF)
// TODO-1975: Audio import (WAV, MP3, OGG, FLAC)
// TODO-1976: Video import
// TODO-1977: CSV/JSON data import
// TODO-1978: Shader import
// TODO-1979: Material import
// TODO-1980: Prefab import

// MARK: - Asset Metadata TODOs (TODO-2001 to TODO-2050)
// TODO-2001: Asset metadata storage (.meta files)
// TODO-2002: Asset GUID system
// TODO-2003: Asset reference tracking
// TODO-2004: Asset dependency graph
// TODO-2005: Asset circular dependency detection
// TODO-2006: Asset missing reference detection
// TODO-2007: Asset unused asset detection
// TODO-2008: Asset tags system
// TODO-2009: Asset labels/colors
// TODO-2010: Asset custom properties
// TODO-2011: Asset creation date
// TODO-2012: Asset modification date
// TODO-2013: Asset author/creator
// TODO-2014: Asset description
// TODO-2015: Asset copyright info
// TODO-2016: Asset license info
// TODO-2017: Asset version number
// TODO-2018: Asset changelog
// TODO-2019: Asset rating system
// TODO-2020: Asset usage statistics
// TODO-2021: Asset file size tracking
// TODO-2022: Asset memory usage estimation
// TODO-2023: Asset platform compatibility
// TODO-2024: Asset quality level
// TODO-2025: Asset LOD information

// MARK: - Asset Organization TODOs (TODO-2051 to TODO-2100)
// TODO-2051: Folder creation
// TODO-2052: Folder renaming
// TODO-2053: Folder deletion
// TODO-2054: Folder color coding
// TODO-2055: Folder icons
// TODO-2056: Folder templates
// TODO-2057: Folder favorites
// TODO-2058: Folder recent
// TODO-2059: Folder breadcrumb navigation
// TODO-2060: Folder path copying
// TODO-2061: Folder reveal in Finder
// TODO-2062: Collections/albums
// TODO-2063: Smart collections (auto-update)
// TODO-2064: Collection sharing
// TODO-2065: Asset bundles
// TODO-2066: Asset packages
// TODO-2067: Asset libraries
// TODO-2068: Asset marketplace integration
// TODO-2069: Asset cloud storage
// TODO-2070: Asset local cache

// MARK: - Asset Processing TODOs (TODO-2101 to TODO-2150)
// TODO-2101: Asset processor pipeline
// TODO-2102: Asset reimport
// TODO-2103: Asset reimport all
// TODO-2104: Asset force reimport
// TODO-2105: Asset import cache
// TODO-2106: Asset background processing
// TODO-2107: Asset processing queue
// TODO-2108: Asset processing priority
// TODO-2109: Asset processing parallelization
// TODO-2110: Texture compression (BC, ASTC, ETC)
// TODO-2111: Texture mipmap generation
// TODO-2112: Texture atlas generation
// TODO-2113: Texture sprite sheet packing
// TODO-2114: Mesh optimization
// TODO-2115: Mesh LOD generation
// TODO-2116: Mesh tangent generation
// TODO-2117: Mesh normal recalculation
// TODO-2118: Mesh UV unwrapping
// TODO-2119: Mesh collision generation
// TODO-2120: Audio compression
// TODO-2121: Audio normalization
// TODO-2122: Audio trimming silence
// TODO-2123: Audio format conversion
// TODO-2124: Shader compilation
// TODO-2125: Shader variant generation

// MARK: - Asset Dependencies TODOs (TODO-2151 to TODO-2200)
// TODO-2151: Dependency viewer
// TODO-2152: Dependency graph visualization
// TODO-2153: Find references to asset
// TODO-2154: Find dependencies of asset
// TODO-2155: Replace asset references
// TODO-2156: Update broken references
// TODO-2157: Dependency validation
// TODO-2158: Circular dependency warning
// TODO-2159: Missing dependency error
// TODO-2160: Dependency export
// TODO-2161: Dependency packaging
// TODO-2162: Addressable asset system
// TODO-2163: Asset bundle system
// TODO-2164: Asset streaming
// TODO-2165: Asset lazy loading

// MARK: - Asset Version Control TODOs (TODO-2201 to TODO-2250)
// TODO-2201: Asset versioning
// TODO-2202: Asset history
// TODO-2203: Asset diff viewer
// TODO-2204: Asset revert to version
// TODO-2205: Asset branching
// TODO-2206: Asset merging
// TODO-2207: Asset conflict resolution
// TODO-2208: Asset locking (checkout)
// TODO-2209: Asset check-in/check-out
// TODO-2210: Asset status indicators
// TODO-2211: Git LFS integration
// TODO-2212: Perforce integration
// TODO-2213: SVN integration
// TODO-2214: Asset collaboration
// TODO-2215: Asset sharing

// MARK: - Asset Export TODOs (TODO-2251 to TODO-2300)
// TODO-2251: Export selected assets
// TODO-2252: Export with dependencies
// TODO-2253: Export as package
// TODO-2254: Export format selection
// TODO-2255: Export settings per-type
// TODO-2256: Export to Unity package
// TODO-2257: Export to Unreal package
// TODO-2258: Export to Godot package
// TODO-2259: Export to web formats
// TODO-2260: Export optimization

// MARK: - Asset Validation TODOs (TODO-2301 to TODO-2350)
// TODO-2301: Asset validation rules
// TODO-2302: Asset naming conventions
// TODO-2303: Asset size limits
// TODO-2304: Asset format validation
// TODO-2305: Asset integrity check
// TODO-2306: Asset performance analysis
// TODO-2307: Asset memory profiling
// TODO-2308: Asset optimization suggestions
// TODO-2309: Asset quality assurance
// TODO-2310: Asset compliance checking

// MARK: - Asset Database TODOs (TODO-2351 to TODO-2400)
// TODO-2351: Asset database indexing
// TODO-2352: Asset database search
// TODO-2353: Asset database caching
// TODO-2354: Asset database optimization
// TODO-2355: Asset database backup
// TODO-2356: Asset database recovery
// TODO-2357: Asset database migration
// TODO-2358: Asset database synchronization
// TODO-2359: Asset database statistics
// TODO-2360: Asset database maintenance

// MARK: - Asset Browser Manager
class AssetBrowserManager: ObservableObject {
    static let shared = AssetBrowserManager()
    
    @Published var currentPath: URL = URL(fileURLWithPath: "/tmp/project/Assets")
    @Published var assets: [AssetItem] = []
    @Published var favorites: [AssetItem] = []
    @Published var recentAssets: [AssetItem] = []
    @Published var searchQuery: String = ""
    @Published var viewMode: ViewMode = .grid
    @Published var sortBy: SortOption = .name
    @Published var filterType: AssetFilterType = .all
    
    enum ViewMode: String, CaseIterable {
        case grid, list
    }
    
    enum SortOption: String, CaseIterable {
        case name, type, date, size
    }
    
    enum AssetFilterType: String, CaseIterable {
        case all, textures, materials, meshes, audio, scripts, prefabs, scenes
    }
    
    init() {
        loadDemoAssets()
    }
    
    func loadDemoAssets() {
        assets = [
            AssetItem(name: "Player.prefab", type: .prefab, size: 24576),
            AssetItem(name: "Enemy.prefab", type: .prefab, size: 18432),
            AssetItem(name: "Ground.mat", type: .material, size: 4096),
            AssetItem(name: "Metal.mat", type: .material, size: 4096),
            AssetItem(name: "Wood.mat", type: .material, size: 4096),
            AssetItem(name: "Diffuse.png", type: .texture, size: 1048576),
            AssetItem(name: "Normal.png", type: .texture, size: 1048576),
            AssetItem(name: "Roughness.png", type: .texture, size: 524288),
            AssetItem(name: "Character.fbx", type: .mesh, size: 2097152),
            AssetItem(name: "Weapon.fbx", type: .mesh, size: 524288),
            AssetItem(name: "Footsteps.wav", type: .audio, size: 262144),
            AssetItem(name: "Explosion.wav", type: .audio, size: 524288),
            AssetItem(name: "PlayerController.swift", type: .script, size: 8192),
            AssetItem(name: "EnemyAI.swift", type: .script, size: 12288),
            AssetItem(name: "Level01.scene", type: .scene, size: 65536)
        ]
    }
    
    var filteredAssets: [AssetItem] {
        var result = assets
        
        // Filter by type
        if filterType != .all {
            result = result.filter { $0.type == AssetType(rawValue: filterType.rawValue) }
        }
        
        // Filter by search
        if !searchQuery.isEmpty {
            result = result.filter { $0.name.localizedCaseInsensitiveContains(searchQuery) }
        }
        
        // Sort
        switch sortBy {
        case .name:
            result.sort { $0.name < $1.name }
        case .type:
            result.sort { $0.type.rawValue < $1.type.rawValue }
        case .date:
            result.sort { $0.modifiedDate > $1.modifiedDate }
        case .size:
            result.sort { $0.size > $1.size }
        }
        
        return result
    }
    
    func addToFavorites(_ asset: AssetItem) {
        if !favorites.contains(where: { $0.id == asset.id }) {
            favorites.append(asset)
        }
    }
    
    func removeFromFavorites(_ asset: AssetItem) {
        favorites.removeAll { $0.id == asset.id }
    }
    
    func addToRecent(_ asset: AssetItem) {
        recentAssets.removeAll { $0.id == asset.id }
        recentAssets.insert(asset, at: 0)
        if recentAssets.count > 20 {
            recentAssets.removeLast()
        }
    }
    
    func deleteAsset(_ asset: AssetItem) {
        assets.removeAll { $0.id == asset.id }
        NotificationManager.shared.notify("Deleted '\(asset.name)'", type: .warning)
    }
    
    func duplicateAsset(_ asset: AssetItem) {
        var copy = asset
        copy.id = UUID()
        copy.name = "\(asset.name.dropLast(asset.type.extension.count + 1))_copy.\(asset.type.extension)"
        assets.append(copy)
        NotificationManager.shared.notify("Created '\(copy.name)'", type: .success)
    }
}

// MARK: - Asset Models
struct AssetItem: Identifiable {
    var id = UUID()
    var name: String
    var type: AssetType
    var size: Int
    var modifiedDate: Date = Date()
    var path: String = ""
    var isFavorite: Bool = false
    var tags: [String] = []
}

enum AssetType: String, CaseIterable {
    case texture, material, mesh, audio, script, prefab, scene, shader, animation, font, other
    
    var icon: String {
        switch self {
        case .texture: return "photo"
        case .material: return "paintpalette"
        case .mesh: return "cube"
        case .audio: return "speaker.wave.2"
        case .script: return "doc.text"
        case .prefab: return "cube.transparent"
        case .scene: return "square.3.layers.3d"
        case .shader: return "sparkle"
        case .animation: return "figure.walk"
        case .font: return "textformat"
        case .other: return "doc"
        }
    }
    
    var color: Color {
        switch self {
        case .texture: return .purple
        case .material: return .orange
        case .mesh: return .blue
        case .audio: return .green
        case .script: return .yellow
        case .prefab: return .cyan
        case .scene: return .pink
        case .shader: return .mint
        case .animation: return .indigo
        case .font: return .gray
        case .other: return .secondary
        }
    }
    
    var `extension`: String {
        switch self {
        case .texture: return "png"
        case .material: return "mat"
        case .mesh: return "fbx"
        case .audio: return "wav"
        case .script: return "swift"
        case .prefab: return "prefab"
        case .scene: return "scene"
        case .shader: return "shader"
        case .animation: return "anim"
        case .font: return "ttf"
        case .other: return "bin"
        }
    }
}

// MARK: - Enhanced Asset Browser View
struct EnhancedAssetBrowser: View {
    @ObservedObject var manager = AssetBrowserManager.shared
    @State private var selectedAssets: Set<UUID> = []
    @State private var showCreateMenu = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            AssetBrowserToolbar(manager: manager, showCreateMenu: $showCreateMenu)
            
            EditorDivider()
            
            HSplitView {
                // Folder tree (left)
                AssetFolderTree()
                    .frame(minWidth: 150, maxWidth: 250)
                
                // Asset grid/list (right)
                VStack(spacing: 0) {
                    // Breadcrumb
                    AssetBreadcrumb(path: manager.currentPath)
                    
                    EditorDivider()
                    
                    // Assets view
                    if manager.viewMode == .grid {
                        AssetGridView(assets: manager.filteredAssets, selectedAssets: $selectedAssets)
                    } else {
                        AssetListView(assets: manager.filteredAssets, selectedAssets: $selectedAssets)
                    }
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Asset Browser Toolbar
struct AssetBrowserToolbar: View {
    @ObservedObject var manager: AssetBrowserManager
    @Binding var showCreateMenu: Bool
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            // Create menu
            Menu {
                Button("Folder") { }
                Divider()
                Button("Material") { }
                Button("Shader") { }
                Button("Script") { }
                Button("Prefab") { }
                Button("Scene") { }
            } label: {
                Label("Create", systemImage: "plus")
            }
            
            // Import
            Button(action: { importAssets() }) {
                Label("Import", systemImage: "square.and.arrow.down")
            }
            
            Spacer()
            
            // Search
            EditorSearchBar(text: $manager.searchQuery, placeholder: "Search assets...")
                .frame(maxWidth: 200)
            
            // Filter
            Picker("", selection: $manager.filterType) {
                ForEach(AssetBrowserManager.AssetFilterType.allCases, id: \.self) { filter in
                    Text(filter.rawValue.capitalized).tag(filter)
                }
            }
            .pickerStyle(.menu)
            .frame(width: 100)
            
            // Sort
            Picker("", selection: $manager.sortBy) {
                ForEach(AssetBrowserManager.SortOption.allCases, id: \.self) { sort in
                    Text(sort.rawValue.capitalized).tag(sort)
                }
            }
            .pickerStyle(.menu)
            .frame(width: 80)
            
            // View mode
            Picker("", selection: $manager.viewMode) {
                Image(systemName: "square.grid.2x2").tag(AssetBrowserManager.ViewMode.grid)
                Image(systemName: "list.bullet").tag(AssetBrowserManager.ViewMode.list)
            }
            .pickerStyle(.segmented)
            .frame(width: 80)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
    
    func importAssets() {
        #if os(macOS)
        let panel = NSOpenPanel()
        panel.allowsMultipleSelection = true
        panel.canChooseDirectories = false
        panel.begin { response in
            if response == .OK {
                for url in panel.urls {
                    print("[Swift] Importing asset: \(url.lastPathComponent)")
                }
                NotificationManager.shared.notify("Imported \(panel.urls.count) asset(s)", type: .success)
            }
        }
        #endif
    }
}

// MARK: - Asset Folder Tree
struct AssetFolderTree: View {
    @State private var expandedFolders: Set<String> = ["Assets"]
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Favorites section
            Text("Favorites")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .padding(.horizontal, DesignSystem.Spacing.sm)
                .padding(.top, DesignSystem.Spacing.sm)
            
            ForEach(AssetBrowserManager.shared.favorites) { asset in
                HStack {
                    Image(systemName: "star.fill")
                        .foregroundColor(.yellow)
                        .font(.system(size: 12))
                    Text(asset.name)
                        .font(DesignSystem.Typography.body)
                }
                .padding(.horizontal, DesignSystem.Spacing.sm)
                .padding(.vertical, 2)
            }
            
            EditorDivider()
                .padding(.vertical, DesignSystem.Spacing.sm)
            
            // Folder tree
            Text("Project")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .padding(.horizontal, DesignSystem.Spacing.sm)
            
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 0) {
                    FolderTreeItem(name: "Assets", isExpanded: true, level: 0)
                    FolderTreeItem(name: "Materials", isExpanded: false, level: 1)
                    FolderTreeItem(name: "Textures", isExpanded: false, level: 1)
                    FolderTreeItem(name: "Models", isExpanded: false, level: 1)
                    FolderTreeItem(name: "Scripts", isExpanded: false, level: 1)
                    FolderTreeItem(name: "Prefabs", isExpanded: false, level: 1)
                    FolderTreeItem(name: "Scenes", isExpanded: false, level: 1)
                    FolderTreeItem(name: "Audio", isExpanded: false, level: 1)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

struct FolderTreeItem: View {
    let name: String
    let isExpanded: Bool
    let level: Int
    @State private var isHovering = false
    @State private var isSelected = false
    
    var body: some View {
        HStack(spacing: 4) {
            Spacer().frame(width: CGFloat(level) * 16)
            
            Image(systemName: isExpanded ? "chevron.down" : "chevron.right")
                .font(.system(size: 10))
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 12)
            
            Image(systemName: "folder.fill")
                .foregroundColor(.yellow)
                .font(.system(size: 14))
            
            Text(name)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Spacer()
        }
        .padding(.vertical, 4)
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .background(isSelected ? DesignSystem.Colors.selection : (isHovering ? DesignSystem.Colors.hover : Color.clear))
        .onTapGesture { isSelected.toggle() }
        .onHover { isHovering = $0 }
    }
}

// MARK: - Asset Breadcrumb
struct AssetBreadcrumb: View {
    let path: URL
    
    var body: some View {
        HStack(spacing: 4) {
            ForEach(path.pathComponents.dropFirst(), id: \.self) { component in
                if component != path.pathComponents.dropFirst().first {
                    Image(systemName: "chevron.right")
                        .font(.system(size: 10))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                Text(component)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
            }
            Spacer()
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .padding(.vertical, DesignSystem.Spacing.xs)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
}

// MARK: - Asset Grid View
struct AssetGridView: View {
    let assets: [AssetItem]
    @Binding var selectedAssets: Set<UUID>
    let columns = [GridItem(.adaptive(minimum: 80, maximum: 100), spacing: 12)]
    
    var body: some View {
        ScrollView {
            LazyVGrid(columns: columns, spacing: 12) {
                ForEach(assets) { asset in
                    AssetGridItem(asset: asset, isSelected: selectedAssets.contains(asset.id))
                        .onTapGesture {
                            selectedAssets = [asset.id]
                            AssetBrowserManager.shared.addToRecent(asset)
                        }
                        .contextMenu {
                            Button("Open") { }
                            Button("Show in Finder") { }
                            Divider()
                            Button(asset.isFavorite ? "Remove from Favorites" : "Add to Favorites") {
                                if asset.isFavorite {
                                    AssetBrowserManager.shared.removeFromFavorites(asset)
                                } else {
                                    AssetBrowserManager.shared.addToFavorites(asset)
                                }
                            }
                            Divider()
                            Button("Duplicate") {
                                AssetBrowserManager.shared.duplicateAsset(asset)
                            }
                            Button("Delete", role: .destructive) {
                                AssetBrowserManager.shared.deleteAsset(asset)
                            }
                        }
                }
            }
            .padding(DesignSystem.Spacing.md)
        }
    }
}

struct AssetGridItem: View {
    let asset: AssetItem
    let isSelected: Bool
    @State private var isHovering = false
    
    var body: some View {
        VStack(spacing: 4) {
            // Thumbnail
            ZStack {
                RoundedRectangle(cornerRadius: 8)
                    .fill(asset.type.color.opacity(0.2))
                    .frame(width: 70, height: 70)
                
                Image(systemName: asset.type.icon)
                    .font(.system(size: 28))
                    .foregroundColor(asset.type.color)
            }
            .overlay(
                RoundedRectangle(cornerRadius: 8)
                    .stroke(isSelected ? DesignSystem.Colors.accentPrimary : Color.clear, lineWidth: 2)
            )
            
            // Name
            Text(asset.name)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .lineLimit(2)
                .multilineTextAlignment(.center)
                .frame(width: 80)
        }
        .padding(4)
        .background(isHovering ? DesignSystem.Colors.hover : Color.clear)
        .cornerRadius(8)
        .onHover { isHovering = $0 }
    }
}

// MARK: - Asset List View
struct AssetListView: View {
    let assets: [AssetItem]
    @Binding var selectedAssets: Set<UUID>
    
    var body: some View {
        List(assets) { asset in
            AssetListItem(asset: asset, isSelected: selectedAssets.contains(asset.id))
                .onTapGesture {
                    selectedAssets = [asset.id]
                    AssetBrowserManager.shared.addToRecent(asset)
                }
        }
        .listStyle(.plain)
    }
}

struct AssetListItem: View {
    let asset: AssetItem
    let isSelected: Bool
    
    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: asset.type.icon)
                .foregroundColor(asset.type.color)
                .frame(width: 24)
            
            Text(asset.name)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Spacer()
            
            Text(asset.type.rawValue.capitalized)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 80)
            
            Text(formatSize(asset.size))
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 60, alignment: .trailing)
        }
        .padding(.vertical, 4)
        .background(isSelected ? DesignSystem.Colors.selection : Color.clear)
    }
    
    func formatSize(_ bytes: Int) -> String {
        if bytes < 1024 { return "\(bytes) B" }
        if bytes < 1048576 { return "\(bytes / 1024) KB" }
        return "\(bytes / 1048576) MB"
    }
}
