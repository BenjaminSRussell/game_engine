import Foundation
import Combine

/// Bridge between Swift UI and C engine for asset management
class AssetBrowserBridge: ObservableObject {
    
    static let shared = AssetBrowserBridge()
    
    // MARK: - Types
    
    enum AssetType: Int32 {
        case unknown = 0
        case texture = 1
        case model = 2
        case audio = 3
        case material = 4
        case all = -1
    }
    
    struct BrowserAsset: Identifiable {
        let id = UUID()
        let name: String
        let filePath: String
        let type: AssetType
        let fileSize: UInt64
        let lastModified: UInt64
        let hasThumbnail: Bool
        let thumbnailSize: UInt32
        var thumbnail: UIImage?
        var isFavorite: Bool = false
        var tags: [String] = []
    }
    
    struct BrowserStats {
        var totalAssets: UInt32 = 0
        var filteredAssets: UInt32 = 0
        var scanTime: Float = 0.0
    }
    
    // MARK: - Published State
    @Published var assets: [BrowserAsset] = []
    @Published var isIndexing: Bool = false
    @Published var stats: BrowserStats = BrowserStats()
    @Published var searchQuery: String = ""
    @Published var currentFilter: AssetType = .all
    
    private var allAssets: [BrowserAsset] = []
    private var favorites: Set<String> = []
    private let thumbnailGenerator = ThumbnailGenerator.shared
    
    private init() {
        loadFavorites()
        initializeAssetBrowser()
    }
    
    // MARK: - Public API
    
    func refreshAssets() {
        isIndexing = true
        stats.scanTime = 0.0
        
        Task {
            let start = Date()
            
            // Call C engine to scan assets
            // For now, scan assets directory directly
            let scannedAssets = await scanAssetsDirectory()
            
            // Generate thumbnails asynchronously
            await loadThumbnails(for: scannedAssets)
            
            await MainActor.run {
                self.allAssets = scannedAssets
                self.applyFilters()
                self.isIndexing = false
                self.stats.totalAssets = UInt32(scannedAssets.count)
                self.stats.scanTime = Float(Date().timeIntervalSince(start) * 1000)
                
                print("AssetBrowserBridge: Indexed \(scannedAssets.count) assets in \(self.stats.scanTime)ms")
            }
        }
    }
    
    func setFilter(_ type: AssetType) {
        currentFilter = type
        applyFilters()
    }
    
    func setSearchQuery(_ query: String) {
        searchQuery = query
        applyFilters()
    }
    
    func setThumbnailSize(_ size: Float) {
        // Update thumbnail size preference
        // Thumbnails will be regenerated as needed
    }
    
    func toggleFavorite(assetPath: String) {
        if favorites.contains(assetPath) {
            favorites.remove(assetPath)
        } else {
            favorites.insert(assetPath)
        }
        saveFavorites()
        
        // Update asset
        if let index = allAssets.firstIndex(where: { $0.filePath == assetPath }) {
            allAssets[index].isFavorite = favorites.contains(assetPath)
        }
        applyFilters()
    }
    
    func addTag(to assetPath: String, tag: String) {
        if let index = allAssets.firstIndex(where: { $0.filePath == assetPath }) {
            if !allAssets[index].tags.contains(tag) {
                allAssets[index].tags.append(tag)
            }
        }
        applyFilters()
    }
    
    func removeTag(from assetPath: String, tag: String) {
        if let index = allAssets.firstIndex(where: { $0.filePath == assetPath }) {
            allAssets[index].tags.removeAll { $0 == tag }
        }
        applyFilters()
    }

    // MARK: - Import / Export
    
    func importAssets(at urls: [URL]) {
        // Verify C engine status
        print("Importing \(urls.count) assets...")
        
        Task {
            // Simulate copy process
            try? await Task.sleep(nanoseconds: 1_000_000_000)
            
            // Refresh
            await MainActor.run {
                self.refreshAssets()
            }
        }
    }
    
    func exportAssets(_ assets: [BrowserAsset], to destination: URL) {
        print("Exporting \(assets.count) assets to \(destination.path)")
        // Implement copy logic here
    }
    
    // MARK: - Private Methods
    
    private func initializeAssetBrowser() {
        refreshAssets()
    }
    
    private func scanAssetsDirectory() async -> [BrowserAsset] {
        let assetsPath = "/Users/benjaminrussell/Desktop/Minecraft v2/assets"
        var scannedAssets: [BrowserAsset] = []
        
        guard let enumerator = FileManager.default.enumerator(atPath: assetsPath) else {
            return []
        }
        
        let validExtensions = ["png", "jpg", "jpeg", "obj", "fbx", "gltf", "wav", "mp3", "ogg", "mat"]
        
        while let file = enumerator.nextObject() as? String {
            let filePath = (assetsPath as NSString).appendingPathComponent(file)
            let ext = (file as NSString).pathExtension.lowercased()
            
            guard validExtensions.contains(ext) else { continue }
            
            // Get file attributes
            guard let attributes = try? FileManager.default.attributesOfItem(atPath: filePath) else {
                continue
            }
            
            let type = detectAssetType(extension: ext)
            let fileSize = attributes[.size] as? UInt64 ?? 0
            let modDate = attributes[.modificationDate] as? Date ?? Date()
            let isFav = favorites.contains(filePath)
            
            let asset = BrowserAsset(
                name: (file as NSString).lastPathComponent,
                filePath: filePath,
                type: type,
                fileSize: fileSize,
                lastModified: UInt64(modDate.timeIntervalSince1970),
                hasThumbnail: false,
                thumbnailSize: 128,
                thumbnail: nil,
                isFavorite: isFav,
                tags: []
            )
            
            scannedAssets.append(asset)
        }
        
        return scannedAssets
    }
    
    private func loadThumbnails(for assets: [BrowserAsset]) async {
        // Load thumbnails in background
        for (index, asset) in assets.enumerated() {
            thumbnailGenerator.generateThumbnail(for: asset.filePath, size: .medium) { [weak self] thumbnail in
                guard let self = self else { return }
                
                Task { @MainActor in
                    if index < self.allAssets.count {
                        var updatedAsset = self.allAssets[index]
                        updatedAsset.thumbnail = thumbnail
                        self.allAssets[index] = updatedAsset
                        self.applyFilters()
                    }
                }
            }
        }
    }
    
    private func applyFilters() {
        var filtered = allAssets
        
        // Filter by type
        if currentFilter != .all {
            filtered = filtered.filter { $0.type == currentFilter }
        }
        
        // Filter by search query
        if !searchQuery.isEmpty {
            filtered = filtered.filter { asset in
                // Fuzzy search on name
                asset.name.localizedCaseInsensitiveContains(searchQuery) ||
                // Search in tags
                asset.tags.contains { $0.localizedCaseInsensitiveContains(searchQuery) }
            }
        }
        
        assets = filtered
        stats.filteredAssets = UInt32(filtered.count)
    }
    
    private func detectAssetType(extension ext: String) -> AssetType {
        switch ext {
        case "png", "jpg", "jpeg", "tga", "bmp":
            return .texture
        case "obj", "fbx", "gltf", "glb":
            return .model
        case "wav", "mp3", "ogg", "m4a":
            return .audio
        case "mat", "shader":
            return .material
        default:
            return .unknown
        }
    }
    
    // MARK: - Persistence
    
    private func loadFavorites() {
        if let data = UserDefaults.standard.data(forKey: "AssetBrowserFavorites"),
           let favorites = try? JSONDecoder().decode(Set<String>.self, from: data) {
            self.favorites = favorites
        }
    }
    
    private func saveFavorites() {
        if let data = try? JSONEncoder().encode(favorites) {
            UserDefaults.standard.set(data, forKey: "AssetBrowserFavorites")
        }
    }
}
