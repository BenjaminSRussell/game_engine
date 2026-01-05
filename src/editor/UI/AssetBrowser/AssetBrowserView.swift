import SwiftUI

/// Asset Browser - SwiftUI-native asset management
/// Mirrors asset_browser.c functionality with macOS integration
@available(macOS 14.0, *)
public struct AssetBrowserView: View {
    @State private var selectedAsset: AssetItem?
    @State private var searchText = ""
    @State private var selectedCategory: AssetCategory = .all
    
    let assets: [AssetItem]
    let onAssetSelected: (AssetItem) -> Void
    
    public init(assets: [AssetItem] = [], onAssetSelected: @escaping (AssetItem) -> Void = { _ in }) {
        self.assets = assets
        self.onAssetSelected = onAssetSelected
    }
    
    var filteredAssets: [AssetItem] {
        assets.filter { asset in
            let matchesSearch = searchText.isEmpty || asset.name.localizedCaseInsensitiveContains(searchText)
            let matchesCategory = selectedCategory == .all || asset.category == selectedCategory
            return matchesSearch && matchesCategory
        }
    }
    
    public var body: some View {
        VStack(spacing: 0) {
            // Search bar
            HStack {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(.secondary)
                TextField("Search assets...", text: $searchText)
                    .textFieldStyle(.plain)
                
                if !searchText.isEmpty {
                    Button(action: { searchText = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(.secondary)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(8)
            .background(Color.primary.opacity(0.05))
            .cornerRadius(8)
            .padding()
            
            // Category filter
            Picker("Category", selection: $selectedCategory) {
                ForEach(AssetCategory.allCases, id: \.self) { category in
                    Text(category.rawValue).tag(category)
                }
            }
            .pickerStyle(.segmented)
            .padding(.horizontal)
            
            Divider()
            
            // Asset grid
            ScrollView {
                LazyVGrid(columns: [
                    GridItem(.adaptive(minimum: 120, maximum: 150), spacing: 16)
                ], spacing: 16) {
                    ForEach(filteredAssets) { asset in
                        AssetCard(asset: asset, isSelected: selectedAsset?.id == asset.id)
                            .onTapGesture {
                                selectedAsset = asset
                                onAssetSelected(asset)
                            }
                    }
                }
                .padding()
            }
            
            if filteredAssets.isEmpty {
                VStack(spacing: 12) {
                    Image(systemName: "tray")
                        .font(.system(size: 48))
                        .foregroundColor(.secondary)
                    Text("No assets found")
                        .font(.headline)
                        .foregroundColor(.secondary)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
    }
}

// MARK: - Asset Card

struct AssetCard: View {
    let asset: AssetItem
    let isSelected: Bool
    
    var body: some View {
        VStack(spacing: 8) {
            // Thumbnail
            AsyncAssetThumbnail(path: asset.path, fallbackIcon: asset.icon)
                .frame(height: 100) // Fixed height for card
                .overlay(
                    RoundedRectangle(cornerRadius: 8)
                        .strokeBorder(isSelected ? Color.accentColor : Color.clear, lineWidth: 2)
                )
            
            // Name
            Text(asset.name)
                .font(.caption)
                .lineLimit(2)
                .multilineTextAlignment(.center)
                .frame(height: 32)
        }
        .frame(width: 120)
    }
}

// MARK: - Data Models

public struct AssetItem: Identifiable {
    public let id: UUID
    public let name: String
    public let category: AssetCategory
    public let icon: String
    public let path: String
    
    public init(id: UUID = UUID(), name: String, category: AssetCategory, icon: String, path: String) {
        self.id = id
        self.name = name
        self.category = category
        self.icon = icon
        self.path = path
    }
    
    @MainActor static let mockAssets: [AssetItem] = [
        AssetItem(name: "Stone", category: .voxels, icon: "cube.fill", path: "/assets/voxels/stone.vox"),
        AssetItem(name: "Grass", category: .voxels, icon: "cube.fill", path: "/assets/voxels/grass.vox"),
        AssetItem(name: "Wood", category: .voxels, icon: "cube.fill", path: "/assets/voxels/wood.vox"),
        AssetItem(name: "Water", category: .voxels, icon: "drop.fill", path: "/assets/voxels/water.vox"),
        AssetItem(name: "Metal", category: .materials, icon: "circle.hexagongrid.fill", path: "/assets/materials/metal.mat"),
        AssetItem(name: "Glass", category: .materials, icon: "circle.hexagongrid", path: "/assets/materials/glass.mat"),
        AssetItem(name: "Footstep", category: .audio, icon: "waveform", path: "/assets/audio/footstep.wav"),
        AssetItem(name: "Ambient", category: .audio, icon: "waveform.circle", path: "/assets/audio/ambient.wav"),
    ]
}

public enum AssetCategory: String, CaseIterable {
    case all = "All"
    case voxels = "Voxels"
    case materials = "Materials"
    case audio = "Audio"
    case scripts = "Scripts"
}

// MARK: - Preview

struct AssetBrowserView_Previews: PreviewProvider {
    static var previews: some View {
        GlassPlate {
            AssetBrowserView()
        }
        .frame(width: 400, height: 600)
    }
}
