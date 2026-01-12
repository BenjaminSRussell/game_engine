import SwiftUI

// MARK: - Content Browser Panel

struct ContentBrowserPanel: View {
    @State private var selectedCategory: AssetCategory = .all
    @State private var searchTerm = ""
    @State private var viewMode: ViewMode = .grid
    @State private var selectedAssets: Set<String> = []
    
    enum AssetCategory: String, CaseIterable {
        case all = "All"
        case models = "Models"
        case materials = "Materials"
        case textures = "Textures"
        case audio = "Audio"
        case scripts = "Scripts"
        case scenes = "Scenes"
        case prefabs = "Prefabs"
        
        var icon: String {
            switch self {
            case .all: return "square.grid.2x2"
            case .models: return "cube"
            case .materials: return "paintpalette"
            case .textures: return "photo"
            case .audio: return "speaker.wave.2"
            case .scripts: return "doc.text"
            case .scenes: return "folder"
            case .prefabs: return "cube.transparent"
            }
        }
    }
    
    enum ViewMode: String, CaseIterable {
        case grid = "Grid"
        case list = "List"
        
        var icon: String {
            switch self {
            case .grid: return "square.grid.3x3"
            case .list: return "list.bullet"
            }
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            ContentBrowserHeader(
                selectedCategory: $selectedCategory,
                searchTerm: $searchTerm,
                viewMode: $viewMode
            )
            
            Divider()
            
            // Main content
            HStack(spacing: 0) {
                // Sidebar
                ContentBrowserSidebar(selectedCategory: $selectedCategory)
                    .frame(width: 200)
                
                Divider()
                
                // Asset grid/list
                ContentBrowserContent(
                    category: selectedCategory,
                    searchTerm: searchTerm,
                    viewMode: viewMode,
                    selectedAssets: $selectedAssets
                )
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Content Browser Header

struct ContentBrowserHeader: View {
    @Binding var selectedCategory: ContentBrowserPanel.AssetCategory
    @Binding var searchTerm: String
    @Binding var viewMode: ContentBrowserPanel.ViewMode
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            // Search bar
            HStack(spacing: DesignSystem.Spacing.xs) {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                    .font(.system(size: 12))
                
                TextField("Search assets...", text: $searchTerm)
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.callout)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                if !searchTerm.isEmpty {
                    Button(action: { searchTerm = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .font(.system(size: 12))
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(.horizontal, DesignSystem.Spacing.sm)
            .padding(.vertical, DesignSystem.Spacing.xs)
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(DesignSystem.CornerRadius.tight)
            
            Spacer()
            
            // View mode toggle
            HStack(spacing: 2) {
                ForEach(ContentBrowserPanel.ViewMode.allCases, id: \.self) { mode in
                    Button(action: { viewMode = mode }) {
                        Image(systemName: mode.icon)
                            .font(.system(size: 14, weight: .medium))
                            .foregroundColor(viewMode == mode ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                            .frame(width: 28, height: 28)
                            .background(viewMode == mode ? DesignSystem.Colors.selection : Color.clear)
                            .cornerRadius(4)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(4)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(DesignSystem.CornerRadius.regular)
            
            // Import button
            EditorButton("Import", icon: "square.and.arrow.down", style: .secondary) {
                // Handle import
            }
        }
        .padding(DesignSystem.Spacing.md)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
}

// MARK: - Content Browser Sidebar

struct ContentBrowserSidebar: View {
    @Binding var selectedCategory: ContentBrowserPanel.AssetCategory
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Favorites section
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                Text("Favorites")
                    .font(DesignSystem.Typography.caption)
                    .fontWeight(.semibold)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                    .padding(.horizontal, DesignSystem.Spacing.md)
                    .padding(.top, DesignSystem.Spacing.sm)
                
                // Favorite items would go here
                Text("No favorites yet")
                    .font(DesignSystem.Typography.callout)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                    .padding(.horizontal, DesignSystem.Spacing.md)
                    .padding(.vertical, DesignSystem.Spacing.xs)
            }
            
            Divider()
                .padding(.vertical, DesignSystem.Spacing.xs)
            
            // Categories
            VStack(alignment: .leading, spacing: 2) {
                Text("Categories")
                    .font(DesignSystem.Typography.caption)
                    .fontWeight(.semibold)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                    .padding(.horizontal, DesignSystem.Spacing.md)
                    .padding(.top, DesignSystem.Spacing.sm)
                
                ForEach(ContentBrowserPanel.AssetCategory.allCases, id: \.self) { category in
                    CategoryRow(
                        category: category,
                        isSelected: selectedCategory == category
                    ) {
                        selectedCategory = category
                    }
                }
            }
            
            Spacer()
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Category Row

struct CategoryRow: View {
    let category: ContentBrowserPanel.AssetCategory
    let isSelected: Bool
    let action: () -> Void
    
    @State private var isHovered = false
    
    var body: some View {
        Button(action: action) {
            HStack(spacing: DesignSystem.Spacing.sm) {
                Image(systemName: category.icon)
                    .font(.system(size: 14, weight: .medium))
                    .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                    .frame(width: 20)
                
                Text(category.rawValue)
                    .font(DesignSystem.Typography.callout)
                    .foregroundColor(isSelected ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                
                Spacer()
            }
            .padding(.horizontal, DesignSystem.Spacing.md)
            .padding(.vertical, DesignSystem.Spacing.xs)
            .background(
                RoundedRectangle(cornerRadius: 4)
                    .fill(backgroundColor)
            )
        }
        .buttonStyle(.plain)
        .onHover { hovering in
            isHovered = hovering
        }
    }
    
    private var backgroundColor: Color {
        if isSelected {
            return DesignSystem.Colors.selectionStrong
        } else if isHovered {
            return DesignSystem.Colors.hover
        } else {
            return Color.clear
        }
    }
}

// MARK: - Content Browser Content

struct ContentBrowserContent: View {
    let category: ContentBrowserPanel.AssetCategory
    let searchTerm: String
    let viewMode: ContentBrowserPanel.ViewMode
    @Binding var selectedAssets: Set<String>
    
    private var filteredAssets: [AssetItem] {
        let allAssets = AssetItem.mockAssets(for: category)
        
        if searchTerm.isEmpty {
            return allAssets
        } else {
            return allAssets.filter { asset in
                asset.name.localizedCaseInsensitiveContains(searchTerm)
            }
        }
    }
    
    var body: some View {
        ScrollView {
            if viewMode == .grid {
                LazyVGrid(columns: [
                    GridItem(.adaptive(minimum: 80), spacing: DesignSystem.Spacing.sm)
                ], spacing: DesignSystem.Spacing.sm) {
                    ForEach(filteredAssets, id: \.id) { asset in
                        AssetGridItem(
                            asset: asset,
                            isSelected: selectedAssets.contains(asset.id)
                        ) {
                            toggleSelection(asset.id)
                        }
                    }
                }
                .padding(DesignSystem.Spacing.md)
            } else {
                LazyVStack(spacing: 2) {
                    ForEach(filteredAssets, id: \.id) { asset in
                        AssetListItem(
                            asset: asset,
                            isSelected: selectedAssets.contains(asset.id)
                        ) {
                            toggleSelection(asset.id)
                        }
                    }
                }
                .padding(.horizontal, DesignSystem.Spacing.md)
                .padding(.vertical, DesignSystem.Spacing.sm)
            }
        }
    }
    
    private func toggleSelection(_ assetId: String) {
        if selectedAssets.contains(assetId) {
            selectedAssets.remove(assetId)
        } else {
            selectedAssets.insert(assetId)
        }
    }
}

// MARK: - Asset Grid Item

struct AssetGridItem: View {
    let asset: AssetItem
    let isSelected: Bool
    let action: () -> Void
    
    @State private var isHovered = false
    
    var body: some View {
        Button(action: action) {
            VStack(spacing: DesignSystem.Spacing.xs) {
                // Thumbnail
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                    .fill(asset.thumbnailColor)
                    .aspectRatio(1, contentMode: .fit)
                    .overlay(
                        Image(systemName: asset.icon)
                            .font(.system(size: 24, weight: .medium))
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    )
                    .overlay(
                        RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                            .strokeBorder(
                                isSelected ? DesignSystem.Colors.accentPrimary : Color.clear,
                                lineWidth: 2
                            )
                    )
                
                // Name
                Text(asset.name)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .lineLimit(2)
                    .multilineTextAlignment(.center)
            }
        }
        .buttonStyle(.plain)
        .scaleEffect(isHovered ? 1.05 : 1.0)
        .animation(DesignSystem.Animations.quick, value: isHovered)
        .onHover { hovering in
            isHovered = hovering
        }
    }
}

// MARK: - Asset List Item

struct AssetListItem: View {
    let asset: AssetItem
    let isSelected: Bool
    let action: () -> Void
    
    @State private var isHovered = false
    
    var body: some View {
        Button(action: action) {
            HStack(spacing: DesignSystem.Spacing.sm) {
                // Thumbnail
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                    .fill(asset.thumbnailColor)
                    .frame(width: 32, height: 32)
                    .overlay(
                        Image(systemName: asset.icon)
                            .font(.system(size: 14, weight: .medium))
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    )
                
                // Info
                VStack(alignment: .leading, spacing: 2) {
                    Text(asset.name)
                        .font(DesignSystem.Typography.callout)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .lineLimit(1)
                    
                    Text(asset.type.displayName)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                Spacer()
                
                // Size info
                Text(asset.sizeDisplay)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            .padding(.horizontal, DesignSystem.Spacing.sm)
            .padding(.vertical, DesignSystem.Spacing.xs)
            .background(
                RoundedRectangle(cornerRadius: 4)
                    .fill(backgroundColor)
            )
        }
        .buttonStyle(.plain)
        .onHover { hovering in
            isHovered = hovering
        }
    }
    
    private var backgroundColor: Color {
        if isSelected {
            return DesignSystem.Colors.selectionStrong
        } else if isHovered {
            return DesignSystem.Colors.hover
        } else {
            return Color.clear
        }
    }
}

// MARK: - Asset Data Model

struct AssetItem {
    let id: String
    let name: String
    let type: AssetType
    let size: Int64
    let icon: String
    let thumbnailColor: Color
    
    var sizeDisplay: String {
        let formatter = ByteCountFormatter()
        formatter.countStyle = .file
        return formatter.string(fromByteCount: size)
    }
    
    static func mockAssets(for category: ContentBrowserPanel.AssetCategory) -> [AssetItem] {
        switch category {
        case .all:
            return (1...20).map { i in
                AssetItem(
                    id: "asset_\(i)",
                    name: "Asset \(i)",
                    type: AssetType.allCases.randomElement() ?? .model,
                    size: Int64.random(in: 1024...1024*1024),
                    icon: "cube",
                    thumbnailColor: DesignSystem.Colors.backgroundTertiary
                )
            }
        case .models:
            return (1...8).map { i in
                AssetItem(
                    id: "model_\(i)",
                    name: "Model \(i)",
                    type: .model,
                    size: Int64.random(in: 1024*100...1024*1024),
                    icon: "cube",
                    thumbnailColor: .blue.opacity(0.3)
                )
            }
        case .materials:
            return (1...6).map { i in
                AssetItem(
                    id: "material_\(i)",
                    name: "Material \(i)",
                    type: .material,
                    size: Int64.random(in: 1024...1024*100),
                    icon: "paintpalette",
                    thumbnailColor: .purple.opacity(0.3)
                )
            }
        case .textures:
            return (1...10).map { i in
                AssetItem(
                    id: "texture_\(i)",
                    name: "Texture \(i)",
                    type: .texture,
                    size: Int64.random(in: 1024*10...1024*500),
                    icon: "photo",
                    thumbnailColor: .green.opacity(0.3)
                )
            }
        default:
            return []
        }
    }
}

enum AssetType {
    case model, material, texture, audio, script, scene, prefab
    
    var displayName: String {
        switch self {
        case .model: return "Model"
        case .material: return "Material"
        case .texture: return "Texture"
        case .audio: return "Audio"
        case .script: return "Script"
        case .scene: return "Scene"
        case .prefab: return "Prefab"
        }
    }
}
