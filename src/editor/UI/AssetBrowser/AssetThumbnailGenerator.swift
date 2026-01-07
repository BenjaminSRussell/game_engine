import SwiftUI
import QuickLookThumbnailing
import AVFoundation

/// Generates thumbnails asynchronously for supported asset types
@available(macOS 14.0, *)
public actor AssetThumbnailGenerator {
    public static let shared = AssetThumbnailGenerator()
    private var cache: [String: NSImage] = [:]
    
    private init() {}
    
    public enum ThumbnailError: Error {
        case fileNotFound
        case notSupported
        case generationFailed
    }
    
    /// Generate thumbnail for a file at path
    public func generateThumbnail(for path: String, size: CGSize = CGSize(width: 128, height: 128)) async throws -> NSImage {
        // Check cache
        if let cached = cache[path] {
            return cached
        }
        
        let url = URL(fileURLWithPath: path)
        let ext = url.pathExtension.lowercased()
        
        var image: NSImage?
        
        switch ext {
        case "png", "jpg", "jpeg", "tga", "bmp":
            // ImageIO / Native
            if let nsImage = NSImage(contentsOf: url) {
                image = nsImage
            }
            
        case "wav", "mp3", "ogg":
            // Audio waveform (placeholder generation)
            // Real waveform generation requires reading audio samples
            image = generateWaveformIcon()
            
        case "vox", "obj", "gltf":
            // 3D Model - Use QuickLook if supported, or fallback
            // QL might not support .vox without a plugin
            image = await generateQuickLookThumbnail(for: url, size: size)
            
        default:
            // Attempt QuickLook for generic types
            image = await generateQuickLookThumbnail(for: url, size: size)
        }
        
        guard let finalImage = image else {
            throw ThumbnailError.generationFailed
        }
        
        // Cache result
        cache[path] = finalImage
        return finalImage
    }
    
    private func generateQuickLookThumbnail(for url: URL, size: CGSize) async -> NSImage? {
        let request = QLThumbnailGenerator.Request(
            fileAt: url,
            size: size,
            scale: 2.0,
            representationTypes: .thumbnail
        )
        
        do {
            let thumbnail = try await QLThumbnailGenerator.shared.generateBestRepresentation(for: request)
            return NSImage(cgImage: thumbnail.cgImage, size: size)
        } catch {
            print("QL Thumbnail failed for \(url.lastPathComponent): \(error)")
            return nil
        }
    }
    
    private func generateWaveformIcon() -> NSImage {
        // Generate a programmatic placeholder for audio
        let size = CGSize(width: 128, height: 128)
        let img = NSImage(size: size)
        img.lockFocus()
        
        guard let ctx = NSGraphicsContext.current?.cgContext else { return img }
        
        ctx.setFillColor(NSColor.systemBlue.withAlphaComponent(0.2).cgColor)
        ctx.fill(CGRect(origin: .zero, size: size))
        
        ctx.setStrokeColor(NSColor.systemBlue.cgColor)
        ctx.setLineWidth(2)
        
        let path = CGMutablePath()
        let midY = size.height / 2
        path.move(to: CGPoint(x: 0, y: midY))
        
        for x in stride(from: 0.0, to: size.width, by: 4.0) {
            let amp = CGFloat.random(in: -20...20)
            path.addLine(to: CGPoint(x: x, y: midY + amp))
        }
        path.addLine(to: CGPoint(x: size.width, y: midY))
        
        ctx.addPath(path)
        ctx.strokePath()
        
        img.unlockFocus()
        return img
    }
}

/// SwiftUI View that asynchronously loads a thumbnail
@available(macOS 14.0, *)
public struct AsyncAssetThumbnail: View {
    let path: String
    let fallbackIcon: String
    
    @State private var image: NSImage?
    @State private var isLoading = false
    
    public init(path: String, fallbackIcon: String) {
        self.path = path
        self.fallbackIcon = fallbackIcon
    }
    
    public var body: some View {
        Group {
            if let image = image {
                Image(nsImage: image)
                    .resizable()
                    .aspectRatio(contentMode: .fit)
            } else {
                ZStack {
                    RoundedRectangle(cornerRadius: 8)
                        .fill(Color.primary.opacity(0.1))
                    
                    if isLoading {
                        ProgressView()
                            .scaleEffect(0.5)
                    } else {
                        Image(systemName: fallbackIcon)
                            .font(.system(size: 40))
                            .foregroundColor(.primary.opacity(0.6))
                    }
                }
            }
        }
        .onAppear {
            loadThumbnail()
        }
    }
    
    private func loadThumbnail() {
        guard !path.isEmpty else { return }
        isLoading = true
        
        Task {
            do {
                let generated = try await AssetThumbnailGenerator.shared.generateThumbnail(for: path)
                await MainActor.run {
                    self.image = generated
                    self.isLoading = false
                }
            } catch {
                await MainActor.run {
                    self.isLoading = false
                }
            }
        }
    }
}
