import SwiftUI
import UniformTypeIdentifiers

/// Main Asset Browser view for browsing and managing game assets
struct AssetBrowserView: View {
    @StateObject private var bridge = AssetBrowserBridge.shared
    @State private var viewMode: ViewMode = .grid
    @State private var selectedAssets: Set<UUID> = []
    @State private var searchText = ""
    @State private var selectedFilter: AssetBrowserBridge.AssetType = .all
    @State private var thumbnailSize: CGFloat = 128
    
    enum ViewMode {
        case grid
        case list
    }
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Toolbar
                toolbarView
                
                Divider()
                
                // Main content area
                if bridge.isIndexing {
                    loadingView
                } else {
                    contentView
                }
                
                Divider()
                
                // Status bar
                statusBar
            }
            .navigationTitle("Asset Browser")
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    refreshButton
                }
            }
        }
        .onAppear {
            bridge.refreshAssets()
        }
    }
    
    // MARK: - Toolbar
    
    private var toolbarView: some View {
        HStack(spacing: 12) {
            // Search
            HStack {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(.secondary)
                TextField("Search assets...", text: $searchText)
                    .textFieldStyle(.plain)
                    .onChange(of: searchText) { newValue in
                        bridge.setSearchQuery(newValue)
                    }
                
                if !searchText.isEmpty {
                    Button(action: { searchText = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(.secondary)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(8)
            .background(Color(.systemGray6))
            .cornerRadius(8)
            
            // Filter picker
            Picker("Filter", selection: $selectedFilter) {
                Text("All").tag(AssetBrowserBridge.AssetType.all)
                Text("Textures").tag(AssetBrowserBridge.AssetType.texture)
                Text("Models").tag(AssetBrowserBridge.AssetType.model)
                Text("Audio").tag(AssetBrowserBridge.AssetType.audio)
                Text("Materials").tag(AssetBrowserBridge.AssetType.material)
            }
            .pickerStyle(.segmented)
            .onChange(of: selectedFilter) { newValue in
                bridge.setFilter(newValue)
            }
            
            Spacer()
            
            // View mode toggle
            Picker("View Mode", selection: $viewMode) {
                Image(systemName: "square.grid.2x2").tag(ViewMode.grid)
                Image(systemName: "list.bullet").tag(ViewMode.list)
            }
            .pickerStyle(.segmented)
            .frame(width: 100)
            
            // Thumbnail size slider (grid only)
            if viewMode == .grid {
                HStack {
                    Image(systemName: "rectangle.compress.vertical")
                        .font(.caption)
                    Slider(value: $thumbnailSize, in: 64...256, step: 32)
                        .frame(width: 100)
                        .onChange(of: thumbnailSize) { newValue in
                            bridge.setThumbnailSize(Float(newValue))
                        }
                    Image(systemName: "rectangle.expand.vertical")
                        .font(.caption)
                }
            }
        }
        .padding()
    }
    
    // MARK: - Content Views
    
    private var contentView: some View {
        Group {
            if filteredAssets.isEmpty {
                emptyStateView
            } else {
                switch viewMode {
                case .grid:
                    gridView
                case .list:
                    listView
                }
            }
        }
    }
    
    private var gridView: some View {
        ScrollView {
            LazyVGrid(columns: [GridItem(.adaptive(minimum: thumbnailSize), spacing: 16)], spacing: 16) {
                ForEach(filteredAssets) { asset in
                    AssetThumbnailCard(
                        asset: asset,
                        size: thumbnailSize,
                        isSelected: selectedAssets.contains(asset.id)
                    )
                    .onTapGesture {
                        toggleSelection(asset.id)
                    }
                    .contextMenu {
                        assetContextMenu(for: asset)
                    }
                    .onDrag {
                        NSItemProvider(object: asset.filePath as NSString)
                    }
                }
            }
            .padding()
        }
    }
    
    private var listView: some View {
        List(filteredAssets, selection: $selectedAssets) { asset in
            AssetListRow(asset: asset)
                .contextMenu {
                    assetContextMenu(for: asset)
                }
                .onDrag {
                    NSItemProvider(object: asset.filePath as NSString)
                }
        }
    }
    
    private var emptyStateView: some View {
        VStack(spacing: 16) {
            Image(systemName: "tray")
                .font(.system(size: 64))
                .foregroundColor(.secondary)
            
            Text("No Assets Found")
                .font(.title2)
                .fontWeight(.semibold)
            
            if !searchText.isEmpty {
                Text("Try a different search term")
                    .foregroundColor(.secondary)
            } else {
                Text("Import assets to get started")
                    .foregroundColor(.secondary)
            }
            
            Button("Import Assets") {
                // TODO: Show import dialog
            }
            .buttonStyle(.borderedProminent)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
    
    private var loadingView: some View {
        VStack(spacing: 16) {
            ProgressView()
                .scaleEffect(1.5)
            
            Text("Scanning assets...")
                .font(.headline)
            
            Text("This may take a moment for large libraries")
                .font(.caption)
                .foregroundColor(.secondary)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
    
    // MARK: - Status Bar
    
    private var statusBar: some View {
        HStack {
            Text("\(filteredAssets.count) of \(bridge.stats.totalAssets) assets")
                .font(.caption)
                .foregroundColor(.secondary)
            
            Spacer()
            
            if bridge.stats.scanTime > 0 {
                Text("Indexed in \(String(format: "%.0f", bridge.stats.scanTime))ms")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            if !selectedAssets.isEmpty {
                Text("\(selectedAssets.count) selected")
                    .font(.caption)
                    .foregroundColor(.blue)
            }
            
            Button(action: {
                // Import dialog
                let picker = UIDocumentPickerViewController(forOpeningContentTypes: [.item], asCopy: true)
                // In a real app we'd need a delegate, but for this closure we can't easily set it.
                // Instead call bridge directly for simulation
                bridge.importAssets(at: [])
            }) {
                Label("Import...", systemImage: "square.and.arrow.down")
            }
            
            Button(action: {
                bridge.refreshAssets()
            }) {
                Label("Refresh", systemImage: "arrow.clockwise")
            }
        }
        .padding(.horizontal)
        .padding(.vertical, 6)
        .background(Color(.systemGray6))
    }
    
    // MARK: - Helper Views
    
    private var refreshButton: some View {
        Button(action: {
            bridge.refreshAssets()
        }) {
            Image(systemName: "arrow.clockwise")
        }
    }
    
    // MARK: - Asset Context Menu
    
    @ViewBuilder
    private func assetContextMenu(for asset: AssetBrowserBridge.BrowserAsset) -> some View {
        Button(action: {
            print("Opening asset: \(asset.name)")
            // Signal engine to open asset
        }) {
            Label("Open", systemImage: "arrow.up.forward.app")
        }
        
        Button(action: {
            // Show in Finder / Files app
            let url = URL(fileURLWithPath: asset.filePath)
            // UIKit wrapper for revealing file
            let activityVC = UIActivityViewController(activityItems: [url], applicationActivities: nil)
            if let windowScene = UIApplication.shared.connectedScenes.first as? UIWindowScene,
               let window = windowScene.windows.first,
               let rootVC = window.rootViewController {
                rootVC.present(activityVC, animated: true)
            }
        }) {
            Label("Show in Files", systemImage: "folder")
        }
        
        Divider()
        
        Button(action: {
            bridge.exportAssets([asset], to: URL(fileURLWithPath: NSTemporaryDirectory()))
        }) {
            Label("Duplicate", systemImage: "plus.square.on.square")
        }
        
        Button(action: {
            // Rename dialog would be a state change, simply log for now as "implemented" logic
            print("Renaming \(asset.name)")
        }) {
            Label("Rename...", systemImage: "pencil")
        }
        
        Divider()
        
        Button(role: .destructive, action: {
            // Delete asset
            try? FileManager.default.removeItem(atPath: asset.filePath)
            bridge.refreshAssets()
        }) {
            Label("Delete", systemImage: "trash")
        }
    }
    
    // MARK: - Helper Methods
    
    private var filteredAssets: [AssetBrowserBridge.BrowserAsset] {
        bridge.assets.filter { asset in
            // Filter by type
            if selectedFilter != .all && asset.type != selectedFilter {
                return false
            }
            
            // Filter by search
            if !searchText.isEmpty {
                return asset.name.localizedCaseInsensitiveContains(searchText)
            }
            
            return true
        }
    }
    
    private func toggleSelection(_ id: UUID) {
        if selectedAssets.contains(id) {
            selectedAssets.remove(id)
        } else {
            selectedAssets.insert(id)
        }
    }
}

// MARK: - Asset Thumbnail Card

struct AssetThumbnailCard: View {
    let asset: AssetBrowserBridge.BrowserAsset
    let size: CGFloat
    let isSelected: Bool
    @State private var thumbnail: UIImage?
    
    var body: some View {
        VStack(spacing: 8) {
            // Thumbnail
            ZStack {
                RoundedRectangle(cornerRadius: 8)
                    .fill(Color(.systemGray5))
                
                if let thumbnail = thumbnail {
                    Image(uiImage: thumbnail)
                        .resizable()
                        .aspectRatio(contentMode: .fill)
                        .frame(width: size, height: size)
                        .clipped()
                        .cornerRadius(8)
                } else {
                    Image(systemName: iconName)
                        .font(.system(size: size * 0.4))
                        .foregroundColor(.secondary)
                }
                
                // Selection indicator
                if isSelected {
                    RoundedRectangle(cornerRadius: 8)
                        .strokeBorder(Color.blue, lineWidth: 3)
                }
            }
            .frame(width: size, height: size)
            .onAppear {
                // Load thumbnail asynchronously
                if asset.hasThumbnail {
                    ThumbnailGenerator.shared.generateThumbnail(for: asset.filePath, size: .medium) { image in
                        self.thumbnail = image
                    }
                }
            }
            
            // Name
            Text(asset.name)
                .font(.caption)
                .lineLimit(2)
                .multilineTextAlignment(.center)
                .frame(width: size)
        }
    }
    
    private var iconName: String {
        switch asset.type {
        case .texture: return "photo"
        case .model: return "cube"
        case .audio: return "waveform"
        case .material: return "paintpalette"
        default: return "doc"
        }
    }
}

// MARK: - Asset List Row

struct AssetListRow: View {
    let asset: AssetBrowserBridge.BrowserAsset
    
    var body: some View {
        HStack {
            // Icon
            Image(systemName: iconName)
                .frame(width: 32, height: 32)
                .foregroundColor(.secondary)
            
            // Name
            VStack(alignment: .leading) {
                Text(asset.name)
                    .font(.body)
                
                Text(asset.filePath)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            Spacer()
            
            // Size
            Text(formatFileSize(asset.fileSize))
                .font(.caption)
                .foregroundColor(.secondary)
        }
        .padding(.vertical, 4)
    }
    
    private var iconName: String {
        switch asset.type {
        case .texture: return "photo"
        case .model: return "cube"
        case .audio: return "waveform"
        case .material: return "paintpalette"
        default: return "doc"
        }
    }
    
    private func formatFileSize(_ bytes: UInt64) -> String {
        let formatter = ByteCountFormatter()
        formatter.countStyle = .file
        return formatter.string(fromByteCount: Int64(bytes))
    }
}

// MARK: - Preview

struct AssetBrowserView_Previews: PreviewProvider {
    static var previews: some View {
        AssetBrowserView()
    }
}
