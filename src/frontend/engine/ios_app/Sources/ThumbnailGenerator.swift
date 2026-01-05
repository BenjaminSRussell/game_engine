import Foundation
import UIKit
import CoreGraphics
import AVFoundation

/// Service for generating thumbnails for various asset types
class ThumbnailGenerator {
    
    static let shared = ThumbnailGenerator()
    
    // MARK: - Types
    
    enum ThumbnailSize {
        case small      // 64x64
        case medium     // 128x128
        case large      // 256x256
        
        var dimension: CGFloat {
            switch self {
            case .small: return 64
            case .medium: return 128
            case .large: return 256
            }
        }
    }
    
    struct CacheKey: Hashable {
        let path: String
        let size: ThumbnailSize
    }
    
    // MARK: - Properties
    
    private var memoryCache: [CacheKey: UIImage] = [:]
    private let cacheQueue = DispatchQueue(label: "com.minecraft.thumbnailgenerator.cache")
    private let generationQueue = DispatchQueue(label: "com.minecraft.thumbnailgenerator.generation", qos: .userInitiated, attributes: .concurrent)
    
    private let diskCacheURL: URL
    
    // MARK: - Initialization
    
    private init() {
        // Set up disk cache directory
        let cacheDir = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask).first!
        diskCacheURL = cacheDir.appendingPathComponent("AssetThumbnails")
        
        do {
            try FileManager.default.createDirectory(at: diskCacheURL, withIntermediateDirectories: true)
        } catch {
            print("Failed to create thumbnail cache directory: \(error)")
        }
    }
    
    // MARK: - Public API
    
    /// Generate a thumbnail for an asset asynchronously
    func generateThumbnail(
        for assetPath: String,
        size: ThumbnailSize = .medium,
        completion: @escaping (UIImage?) -> Void
    ) {
        let key = CacheKey(path: assetPath, size: size)
        
        // Check memory cache
        if let cached = cacheQueue.sync(execute: { memoryCache[key] }) {
            completion(cached)
            return
        }
        
        // Check disk cache
        if let diskCached = loadFromDiskCache(key: key) {
            cacheQueue.async { [weak self] in
                self?.memoryCache[key] = diskCached
            }
            completion(diskCached)
            return
        }
        
        // Generate thumbnail
        generationQueue.async { [weak self] in
            guard let self = self else { return }
            
            let thumbnail = self.generateThumbnailSync(for: assetPath, size: size)
            
            if let thumbnail = thumbnail {
                // Cache in memory
                self.cacheQueue.async {
                    self.memoryCache[key] = thumbnail
                }
                
                // Cache to disk
                self.saveToDiskCache(thumbnail: thumbnail, key: key)
            }
            
            DispatchQueue.main.async {
                completion(thumbnail)
            }
        }
    }
    
    /// Generate thumbnail synchronously (for batch processing)
    func generateThumbnailSync(for assetPath: String, size: ThumbnailSize) -> UIImage? {
        let url = URL(fileURLWithPath: assetPath)
        let ext = url.pathExtension.lowercased()
        
        switch ext {
        case "png", "jpg", "jpeg", "tga", "bmp":
            return generateImageThumbnail(url: url, size: size)
        case "obj", "fbx", "gltf", "glb":
            return generateModelThumbnail(url: url, size: size)
        case "wav", "mp3", "ogg", "m4a":
            return generateAudioThumbnail(url: url, size: size)
        case "mat", "shader":
            return generateMaterialThumbnail(url: url, size: size)
        default:
            return generateDefaultThumbnail(size: size)
        }
    }
    
    /// Clear all caches
    func clearCache() {
        cacheQueue.async { [weak self] in
            self?.memoryCache.removeAll()
        }
        
        try? FileManager.default.removeItem(at: diskCacheURL)
        try? FileManager.default.createDirectory(at: diskCacheURL, withIntermediateDirectories: true)
    }
    
    // MARK: - Thumbnail Generation
    
    private func generateImageThumbnail(url: URL, size: ThumbnailSize) -> UIImage? {
        guard let image = UIImage(contentsOfFile: url.path) else { return nil }
        return resizeImage(image, to: size.dimension)
    }
    
    private func generateModelThumbnail(url: URL, size: ThumbnailSize) -> UIImage? {
        // Create a basic SceneKit scene
        let scene = SCNScene()
        
        // Load the model
        // Note: In a real app we would load the specific node or generic scene
        // For this implementation we'll add a primitive if load fails, or try to load the reference
        
        do {
            let modelScene = try SCNScene(url: url, options: nil)
            for node in modelScene.rootNode.childNodes {
                scene.rootNode.addChildNode(node)
                
                // Add default material if missing
                if node.geometry?.firstMaterial == nil {
                    node.geometry?.firstMaterial = SCNMaterial()
                    node.geometry?.firstMaterial?.diffuse.contents = UIColor.systemGray
                }
            }
        } catch {
            print("Failed to load model for thumbnail: \(error)")
            // Fallback to cube
            let box = SCNBox(width: 1, height: 1, length: 1, chamferRadius: 0.1)
            let node = SCNNode(geometry: box)
            node.geometry?.firstMaterial?.diffuse.contents = UIColor.systemBlue
            scene.rootNode.addChildNode(node)
        }
        
        // Add lighting
        let lightNode = SCNNode()
        lightNode.light = SCNLight()
        lightNode.light?.type = .omni
        lightNode.position = SCNVector3(x: 10, y: 10, z: 10)
        scene.rootNode.addChildNode(lightNode)
        
        let ambientLight = SCNNode()
        ambientLight.light = SCNLight()
        ambientLight.light?.type = .ambient
        ambientLight.light?.color = UIColor.darkGray
        scene.rootNode.addChildNode(ambientLight)
        
        // Setup camera
        let cameraNode = SCNNode()
        cameraNode.camera = SCNCamera()
        cameraNode.position = SCNVector3(x: 2, y: 2, z: 3)
        cameraNode.look(at: SCNVector3Zero)
        scene.rootNode.addChildNode(cameraNode)
        
        // Render offscreen
        let renderer = SCNRenderer(device: MTLCreateSystemDefaultDevice(), options: nil)
        renderer.scene = scene
        renderer.pointOfView = cameraNode
        
        let targetSize = CGSize(width: size.dimension, height: size.dimension)
        return renderer.snapshot(atTime: 0, with: targetSize, antialiasingMode: .multisampling4X)
    }
    
    private func generateAudioThumbnail(url: URL, size: ThumbnailSize) -> UIImage? {
        // Generate waveform visualization
        guard let waveform = generateWaveform(for: url, size: size.dimension) else {
            return createPlaceholderThumbnail(
                icon: "waveform",
                color: .systemGreen,
                size: size.dimension
            )
        }
        return waveform
    }
    
    private func generateMaterialThumbnail(url: URL, size: ThumbnailSize) -> UIImage? {
        // Render a sphere with the material applied
        let scene = SCNScene()
        
        // Create sphere
        let sphere = SCNSphere(radius: 0.8)
        let sphereNode = SCNNode(geometry: sphere)
        scene.rootNode.addChildNode(sphereNode)
        
        // Try to load material texture if it's an image file pretending to be a mat
        // Or if it's a proprietary format we would parse it here.
        // For this demo, we'll apply a color derived from the filename hash
        let material = SCNMaterial()
        // Improve this later to actually parse .mat files
        material.diffuse.contents = UIColor(
            hue: CGFloat(abs(url.path.hashValue) % 100) / 100.0,
            saturation: 0.8,
            brightness: 0.8,
            alpha: 1.0
        )
        material.lightingModel = .physicallyBased
        material.metalness.contents = 0.5
        material.roughness.contents = 0.2
        sphere.materials = [material]
        
        // Lighting
        let lightNode = SCNNode()
        lightNode.light = SCNLight()
        lightNode.light?.type = .directional
        lightNode.eulerAngles = SCNVector3(-Float.pi/4, Float.pi/4, 0)
        scene.rootNode.addChildNode(lightNode)
        
        // Camera
        let cameraNode = SCNNode()
        cameraNode.camera = SCNCamera()
        cameraNode.position = SCNVector3(x: 0, y: 0, z: 2.5)
        scene.rootNode.addChildNode(cameraNode)
        
        // Render
        let renderer = SCNRenderer(device: MTLCreateSystemDefaultDevice(), options: nil)
        renderer.scene = scene
        renderer.pointOfView = cameraNode
        renderer.autoenablesDefaultLighting = true
        
        let targetSize = CGSize(width: size.dimension, height: size.dimension)
        return renderer.snapshot(atTime: 0, with: targetSize, antialiasingMode: .multisampling4X)
    }
    
    private func generateDefaultThumbnail(size: ThumbnailSize) -> UIImage? {
        return createPlaceholderThumbnail(
            icon: "doc",
            color: .systemGray,
            size: size.dimension
        )
    }
    
    // MARK: - Waveform Generation
    
    private func generateWaveform(for url: URL, size: CGFloat) -> UIImage? {
        guard let audioFile = try? AVAudioFile(forReading: url) else { return nil }
        
        let format = audioFile.processingFormat
        let frameCount = AVAudioFrameCount(audioFile.length)
        
        guard let buffer = AVAudioPCMBuffer(pcmFormat: format, frameCapacity: frameCount) else {
            return nil
        }
        
        try? audioFile.read(into: buffer)
        
        guard let channelData = buffer.floatChannelData else { return nil }
        let channelDataValue = channelData.pointee
        
        // Sample the waveform
        let sampleCount = Int(size)
        let samplesPerPixel = Int(frameCount) / sampleCount
        var samples: [Float] = []
        
        for i in 0..<sampleCount {
            let startIndex = i * samplesPerPixel
            let endIndex = min(startIndex + samplesPerPixel, Int(frameCount))
            
            var maxSample: Float = 0
            for j in startIndex..<endIndex {
                let sample = abs(channelDataValue[j])
                maxSample = max(maxSample, sample)
            }
            samples.append(maxSample)
        }
        
        // Render waveform
        return renderWaveform(samples: samples, size: CGSize(width: size, height: size))
   }
    
    private func renderWaveform(samples: [Float], size: CGSize) -> UIImage {
        let renderer = UIGraphicsImageRenderer(size: size)
        
        return renderer.image { context in
            let cgContext = context.cgContext
            
            // Background
            UIColor.black.setFill()
            cgContext.fill(CGRect(origin: .zero, size: size))
            
            // Waveform
            UIColor.systemGreen.setFill()
            
            let barWidth = size.width / CGFloat(samples.count)
            let centerY = size.height / 2
            
            for (index, sample) in samples.enumerated() {
                let x = CGFloat(index) * barWidth
                let barHeight = CGFloat(sample) * size.height * 0.8
                
                let rect = CGRect(
                    x: x,
                    y: centerY - barHeight / 2,
                    width: max(barWidth - 1, 1),
                    height: barHeight
                )
                cgContext.fill(rect)
            }
        }
    }
    
    // MARK: - Image Utilities
    
    private func resizeImage(_ image: UIImage, to size: CGFloat) -> UIImage {
        let targetSize = CGSize(width: size, height: size)
        let renderer = UIGraphicsImageRenderer(size: targetSize)
        
        return renderer.image { _ in
            image.draw(in: CGRect(origin: .zero, size: targetSize))
        }
    }
    
    private func createPlaceholderThumbnail(icon: String, color: UIColor, size: CGFloat) -> UIImage {
        let targetSize = CGSize(width: size, height: size)
        let renderer = UIGraphicsImageRenderer(size: targetSize)
        
        return renderer.image { context in
            // Background
            color.withAlphaComponent(0.2).setFill()
            context.fill(CGRect(origin: .zero, size: targetSize))
            
            // Icon
            let config = UIImage.SymbolConfiguration(pointSize: size * 0.4, weight: .regular)
            if let symbolImage = UIImage(systemName: icon, withConfiguration: config) {
                color.setFill()
                
                let imageSize = symbolImage.size
                let x = (size - imageSize.width) / 2
                let y = (size - imageSize.height) / 2
                
                symbolImage.draw(at: CGPoint(x: x, y: y))
            }
        }
    }
    
    // MARK: - Disk Cache
    
    private func diskCacheURL(for key: CacheKey) -> URL {
        let filename = "\(key.path.md5)_\(key.size.dimension).png"
        return diskCacheURL.appendingPathComponent(filename)
    }
    
    private func loadFromDiskCache(key: CacheKey) -> UIImage? {
        let url = diskCacheURL(for: key)
        return UIImage(contentsOfFile: url.path)
    }
    
    private func saveToDiskCache(thumbnail: UIImage, key: CacheKey) {
        guard let data = thumbnail.pngData() else { return }
        let url = diskCacheURL(for: key)
        
        try? data.write(to: url)
    }
}

// MARK: - String MD5 Extension

extension String {
    var md5: String {
        // Simple hash for cache key (not cryptographic)
        var hash = self.hashValue
        return String(format: "%08x", abs(hash))
    }
}
