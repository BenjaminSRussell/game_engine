import SwiftUI

// @main
public struct VoxelForgeApp: App {
    public init() {}
    public var body: some Scene {
        WindowGroup {
            EditorView()
        }
        .windowStyle(.hiddenTitleBar) // Modern look
        .commands {
            // Add custom menu commands here later
            SidebarCommands()
        }
    }
}

