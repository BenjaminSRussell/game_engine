import Foundation

public protocol ReloadableModule {
    func onReload()
}

@MainActor
public final class HotReloader {
    public static let shared = HotReloader()
    
    private var activeLibrary: UnsafeMutableRawPointer?
    private var modulePath: String?
    
    private init() {}
    
    public func loadModule(at path: String) {
        if let current = activeLibrary {
            dlclose(current)
        }
        
        modulePath = path
        activeLibrary = dlopen(path, RTLD_NOW)
        
        if activeLibrary != nil {
            Logger.info("Successfully loaded module: \(path)")
        } else {
            let error = String(cString: dlerror())
            Logger.error("Failed to load module: \(error)")
        }
    }
    
    public func getFunction<T>(name: String) -> T? {
        guard let library = activeLibrary else { return nil }
        
        if let symbol = dlsym(library, name) {
            return unsafeBitCast(symbol, to: T.self)
        }
        
        return nil
    }
}
