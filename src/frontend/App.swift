import SwiftUI

@main
@available(macOS 14.0, *)
struct VoxelForgeStudioApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
        .windowStyle(.hiddenTitleBar)
        .windowToolbarStyle(.unified)
        .commands {
            // File Menu
            SwiftUI.CommandGroup(replacing: .newItem) {
                Button("New Project...") {
                    // Handled by Dashboard view state
                }
                Button("New Scene") {
                    EngineBridge.shared.newScene()
                }
                .keyboardShortcut("n", modifiers: .command)
                
                Divider()
                
                Button("Open Project...") {
                    let panel = NSOpenPanel()
                    panel.canChooseDirectories = true
                    panel.begin { response in
                        if response == .OK {
                            print("Open Project: \(panel.url?.path ?? "")")
                        }
                    }
                }
                Button("Open Scene...") {
                    let panel = NSOpenPanel()
                    panel.allowedContentTypes = [.data] // Using .data generic for now
                    panel.begin { response in
                        if response == .OK {
                            EngineBridge.shared.loadScene(path: panel.url?.path ?? "")
                        }
                    }
                }
                .keyboardShortcut("o", modifiers: .command)
                
                Divider()
                
                Button("Save Scene") {
                    EngineBridge.shared.saveScene(path: "/tmp/current_scene.voxel")
                }
                .keyboardShortcut("s", modifiers: .command)
                
                Button("Save As...") {
                    let panel = NSSavePanel()
                    panel.begin { response in
                        if response == .OK {
                            EngineBridge.shared.saveScene(path: panel.url?.path ?? "")
                        }
                    }
                }
                .keyboardShortcut("s", modifiers: [.command, .shift])
            }
            
            // Edit Menu (Undo/Redo)
            SwiftUI.CommandGroup(replacing: .undoRedo) {
                Button("Undo") {
                    CommandManager.shared.undo()
                }
                .keyboardShortcut("z", modifiers: .command)
                .disabled(!CommandManager.shared.canUndo)
                
                Button("Redo") {
                    CommandManager.shared.redo()
                }
                .keyboardShortcut("z", modifiers: [.command, .shift])
                .disabled(!CommandManager.shared.canRedo)
                
                Divider()
                
                Button("Cut") {
                    NSApp.sendAction(#selector(NSText.cut(_:)), to: nil, from: nil)
                }
                .keyboardShortcut("x", modifiers: .command)
                Button("Copy") {
                    NSApp.sendAction(#selector(NSText.copy(_:)), to: nil, from: nil)
                }
                .keyboardShortcut("c", modifiers: .command)
                Button("Paste") {
                    NSApp.sendAction(#selector(NSText.paste(_:)), to: nil, from: nil)
                }
                .keyboardShortcut("v", modifiers: .command)
                Button("Delete") {
                    NSApp.sendAction(#selector(NSText.delete(_:)), to: nil, from: nil)
                }
                .keyboardShortcut(.delete, modifiers: .command)
            }
            
            // Assets Menu
            CommandMenu("Assets") {
                Button("Create Material") { print("Create Material") }
                Button("Create Script") { print("Create Script") }
                Button("Create Shader") { print("Create Shader") }
                Divider()
                Button("Import New Asset...") {
                    let panel = NSOpenPanel()
                    panel.begin { _ in }
                }
                Button("Refresh") {
                    // Refresh logic
                }
                .keyboardShortcut("r", modifiers: .command)
            }
            
            // GameObject Menu
            CommandMenu("GameObject") {
                Button("Create Empty") { }
                .keyboardShortcut("n", modifiers: [.command, .shift])
                Divider()
                Menu("3D Object") {
                    Button("Cube") { }
                    Button("Sphere") { }
                    Button("Plane") { }
                }
                Button("Effects") { }
                Button("Light") { }
                Button("Audio") { }
                Button("Camera") { }
            }
            
            // Window Menu
            CommandGroup(after: .windowList) {
                Divider()
                Button("Reset Layout") { }
                Button("Scene Hierarchy") { }
                Button("Inspector") { }
                Button("Project Explorer") { }
                Button("Console") { }
            }
        }
    }
}
