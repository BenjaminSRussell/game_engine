import Foundation

public struct ModMetadata: Codable, Sendable {
    public let name: String
    public let version: String
    public let author: String
}

public protocol Mod: ReloadableModule {
    var metadata: ModMetadata { get }
}

@MainActor
public final class ModManager {
    public static let shared = ModManager()
    
    private var mods: [Mod] = []
    
    public func scanForMods() {
        let fileManager = FileManager.default
        let appSupport = fileManager.urls(for: .applicationSupportDirectory, in: .userDomainMask).first!
        let modsFolder = appSupport.appendingPathComponent("VoxelForge/Mods")
        
        do {
            if !fileManager.fileExists(atPath: modsFolder.path) {
                try fileManager.createDirectory(at: modsFolder, withIntermediateDirectories: true)
            }
            
            let files = try fileManager.contentsOfDirectory(at: modsFolder, includingPropertiesForKeys: nil)
            for file in files where file.pathExtension == "dylib" {
                Task { @MainActor in
                    HotReloader.shared.loadModule(at: file.path)
                }
            }
        } catch {
            Logger.error("Failed to scan for mods: \(error)")
        }
    }
}

/// Plugin FFI Bridge
@_cdecl("VoxelForge_GetEngineVersion")
public func getEngineVersion() -> UnsafePointer<CChar> {
    let version = "1.0.0"
    return UnsafePointer(strdup(version)!)
}
