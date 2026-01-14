import SwiftUI

struct ProjectExplorerView: View {
    @State private var items: [FileSystemItem] = [
        FileSystemItem(name: "Assets", icon: "folder", children: [
            FileSystemItem(name: "Textures", icon: "folder", children: [
                FileSystemItem(name: "grass.png", icon: "photo"),
                FileSystemItem(name: "dirt.png", icon: "photo")
            ]),
            FileSystemItem(name: "Models", icon: "folder", children: [
                FileSystemItem(name: "player.obj", icon: "cube")
            ])
        ]),
        FileSystemItem(name: "Scripts", icon: "folder", children: [
            FileSystemItem(name: "PlayerController.lua", icon: "doc.text")
        ]),
        FileSystemItem(name: "Config.json", icon: "gear")
    ]
    
    var body: some View {
        List(items, children: \.children) { item in
            Label(item.name, systemImage: item.icon)
        }
        .listStyle(.sidebar)
        .navigationTitle("Project")
    }
}

struct FileSystemItem: Identifiable {
    let id = UUID()
    let name: String
    let icon: String
    var children: [FileSystemItem]?
}
