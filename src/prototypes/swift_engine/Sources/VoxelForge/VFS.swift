import Foundation

public final class VirtualFileSystem: Sendable {
    public static let shared = VirtualFileSystem()
    
    private let rootPath: URL
    
    private init() {
        // Assume assets are in the root directory relative to the engine
        // In a real app, this might be bundled or provided via config
        self.rootPath = URL(fileURLWithPath: FileManager.default.currentDirectoryPath).appendingPathComponent("assets")
    }
    
    /// Returns the absolute path for a virtual path (e.g., "textures/blocks/dirt.png").
    public func resolve(virtualPath: String) -> URL {
        return rootPath.appendingPathComponent(virtualPath)
    }
    
    /// Loads data from a virtual path asynchronously.
    public func loadData(virtualPath: String, completion: @escaping @Sendable (Data?) -> Void) {
        let url = resolve(virtualPath: virtualPath)
        JobSystem.shared.dispatch(priority: .medium) {
            let data = try? Data(contentsOf: url)
            completion(data)
        }
    }
    
    /// Checks if a file exists at the virtual path.
    public func exists(virtualPath: String) -> Bool {
        let url = resolve(virtualPath: virtualPath)
        return FileManager.default.fileExists(atPath: url.path)
    }
}
