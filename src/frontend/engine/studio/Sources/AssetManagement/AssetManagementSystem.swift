import Foundation
import Combine
import AppKit

// MARK: - File System Watcher (Phase 6)

class FileSystemWatcher: ObservableObject {
    @Published var changedPaths: [String] = []
    
    private var monitoredURL: URL?
    private var fileDescriptor: CInt = -1
    private var dispatchSource: DispatchSourceFileSystemObject?
    
    func startWatching(path: String) {
        let url = URL(fileURLWithPath: path)
        monitoredURL = url
        
        let fd = open(path, O_EVTONLY)
        guard fd >= 0 else {
            print("[FileWatcher] Failed to open path: \(path)")
            return
        }
        
        fileDescriptor = fd
        
        let source = DispatchSource.makeFileSystemObjectSource(
            fileDescriptor: fd,
            eventMask: [.write, .delete, .rename, .extend],
            queue: DispatchQueue.global(qos: .background)
        )
        
        source.setEventHandler { [weak self] in
            self?.handleFileSystemEvent()
        }
        
        source.setCancelHandler { [fd] in
            close(fd)
        }
        
        source.resume()
        dispatchSource = source
        
        print("[FileWatcher] Watching: \(path)")
    }
    
    func stopWatching() {
        dispatchSource?.cancel()
        dispatchSource = nil
        
        if fileDescriptor >= 0 {
            close(fileDescriptor)
            fileDescriptor = -1
        }
    }
    
    private func handleFileSystemEvent() {
        guard let url = monitoredURL else { return }
        
        DispatchQueue.main.async {
            self.changedPaths.append(url.path)
            print("[FileWatcher] Change detected in: \(url.path)")
        }
    }
    
    deinit {
        stopWatching()
    }
}

// MARK: - Asset Metadata Cache (Phase 7)

struct AssetMetadata: Codable {
    let path: String
    let type: AssetType
    let size: Int64
    let modifiedDate: Date
    let thumbnailPath: String?
    let importSettings: [String: String]
    
    var id: String { path }
}

class AssetMetadataCache {
    private let cacheURL: URL
    private var cache: [String: AssetMetadata] = [:]
    
    init(cacheDirectory: URL) {
        self.cacheURL = cacheDirectory.appendingPathComponent("asset_cache.json")
        loadCache()
    }
    
    func loadCache() {
        guard FileManager.default.fileExists(atPath: cacheURL.path) else { return }
        
        do {
            let data = try Data(contentsOf: cacheURL)
            let decoder = JSONDecoder()
            decoder.dateDecodingStrategy = .iso8601
            let metadataArray = try decoder.decode([AssetMetadata].self, from: data)
            cache = Dictionary(uniqueKeysWithValues: metadataArray.map { ($0.path, $0) })
            print("[AssetCache] Loaded \(cache.count) items")
        } catch {
            print("[AssetCache] Failed to load: \(error)")
        }
    }
    
    func saveCache() {
        do {
            let encoder = JSONEncoder()
            encoder.dateEncodingStrategy = .iso8601
            encoder.outputFormatting = .prettyPrinted
            let data = try encoder.encode(Array(cache.values))
            try data.write(to: cacheURL)
            print("[AssetCache] Saved \(cache.count) items")
        } catch {
            print("[AssetCache] Failed to save: \(error)")
        }
    }
    
    func get(_ path: String) -> AssetMetadata? {
        return cache[path]
    }
    
    func set(_ metadata: AssetMetadata) {
        cache[metadata.path] = metadata
    }
    
    func remove(_ path: String) {
        cache.removeValue(forKey: path)
    }
    
    func all() -> [AssetMetadata] {
        return Array(cache.values)
    }
}

// MARK: - Thumbnail Generator (Phase 7)

class ThumbnailGenerator {
    static let shared = ThumbnailGenerator()
    private let thumbnailSize = CGSize(width: 128, height: 128)
    
    func generateThumbnail(for assetPath: String, outputPath: String) -> Bool {
        let url = URL(fileURLWithPath: assetPath)
        let ext = url.pathExtension.lowercased()
        
        switch ext {
        case "png", "jpg", "jpeg", "tiff", "bmp":
            return generateImageThumbnail(url: url, outputPath: outputPath)
        default:
            return false
        }
    }
    
    private func generateImageThumbnail(url: URL, outputPath: String) -> Bool {
        #if os(macOS)
        guard let image = NSImage(contentsOf: url) else { return false }
        
        let thumbnail = NSImage(size: thumbnailSize)
        thumbnail.lockFocus()
        
        image.draw(in: NSRect(origin: .zero, size: thumbnailSize),
                   from: NSRect(origin: .zero, size: image.size),
                   operation: .copy,
                   fraction: 1.0)
        
        thumbnail.unlockFocus()
        
        guard let tiffData = thumbnail.tiffRepresentation,
              let bitmapRep = NSBitmapImageRep(data: tiffData),
              let pngData = bitmapRep.representation(using: .png, properties: [:]) else {
            return false
        }
        
        do {
            try pngData.write(to: URL(fileURLWithPath: outputPath))
            return true
        } catch {
            print("[Thumbnail] Failed to save: \(error)")
            return false
        }
        #else
        return false
        #endif
    }
}

// MARK: - Asset Importer (Phase 9)

class AssetImporter {
    static let shared = AssetImporter()
    
    func importAsset(from sourceURL: URL, to projectURL: URL, completion: @escaping (Result<AssetMetadata, Error>) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            let destURL = projectURL.appendingPathComponent(sourceURL.lastPathComponent)
            
            do {
                // Copy file
                try FileManager.default.copyItem(at: sourceURL, to: destURL)
                
                // Get file attributes
                let attributes = try FileManager.default.attributesOfItem(atPath: destURL.path)
                let size = attributes[.size] as? Int64 ?? 0
                let modDate = attributes[.modificationDate] as? Date ?? Date()
                
                // Determine asset type
                let type = self.detectAssetType(url: destURL)
                
                // Generate thumbnail
                let thumbnailPath = self.generateThumbnailPath(for: destURL)
                _ = ThumbnailGenerator.shared.generateThumbnail(for: destURL.path, outputPath: thumbnailPath)
                
                // Create metadata
                let metadata = AssetMetadata(
                    path: destURL.path,
                    type: type,
                    size: size,
                    modifiedDate: modDate,
                    thumbnailPath: thumbnailPath,
                    importSettings: [:]
                )
                
                DispatchQueue.main.async {
                    completion(.success(metadata))
                }
            } catch {
                DispatchQueue.main.async {
                    completion(.failure(error))
                }
            }
        }
    }
    
    private func detectAssetType(url: URL) -> AssetType {
        let ext = url.pathExtension.lowercased()
        
        switch ext {
        case "png", "jpg", "jpeg", "tiff", "bmp":
            return .texture
        case "obj", "fbx", "gltf", "glb":
            return .mesh
        case "wav", "mp3", "ogg":
            return .audio
        case "mat":
            return .material
        case "prefab":
            return .prefab
        default:
            return .other
        }
    }
    
    private func generateThumbnailPath(for url: URL) -> String {
        let thumbnailDir = url.deletingLastPathComponent().appendingPathComponent(".thumbnails")
        try? FileManager.default.createDirectory(at: thumbnailDir, withIntermediateDirectories: true)
        return thumbnailDir.appendingPathComponent(url.lastPathComponent).appendingPathExtension("png").path
    }
}
