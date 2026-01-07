import Metal
import MetalKit

@MainActor
public final class TextureManager {
    public static let shared = TextureManager()
    
    private var textures: [String: MTLTexture] = [:]
    private var textureHeap: [MTLTexture] = []
    
    private init() {}
    
    public func loadTexture(virtualPath: String, device: MTLDevice) async -> Int {
        if let existing = textures[virtualPath], let index = textureHeap.firstIndex(where: { $0 === existing }) {
            return index
        }
        
        let loader = MTKTextureLoader(device: device)
        let url = VirtualFileSystem.shared.resolve(virtualPath: virtualPath)
        
        do {
            let texture = try await loader.newTexture(URL: url, options: [
                .origin: MTKTextureLoader.Origin.bottomLeft,
                .generateMipmaps: true
            ])
            textures[virtualPath] = texture
            textureHeap.append(texture)
            return textureHeap.count - 1
        } catch {
            Logger.error("Failed to load texture at \(virtualPath): \(error)")
            return -1
        }
    }
    
    public func getTexture(at index: Int) -> MTLTexture? {
        guard index >= 0 && index < textureHeap.count else { return nil }
        return textureHeap[index]
    }
    
    public func getAllTextures() -> [MTLTexture] {
        return textureHeap
    }
}
