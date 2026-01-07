import SwiftUI

struct DocumentationView: View {
    @State private var selectedPage: String? = "introduction"
    @State private var pageContent: String = ""
    
    var body: some View {
        NavigationSplitView {
            List(selection: $selectedPage) {
                Section("Guide") {
                    NavigationLink(value: "introduction") { Label("Introduction", systemImage: "book.pages") }
                    NavigationLink(value: "getting-started") { Label("Getting Started", systemImage: "play.circle") }
                    NavigationLink(value: "installation") { Label("Installation", systemImage: "arrow.down.circle") }
                }
                Section("Core Concepts") {
                    NavigationLink(value: "architecture") { Label("Architecture", systemImage: "square.stack.3d.up") }
                    NavigationLink(value: "ecs") { Label("ECS System", systemImage: "capsule") }
                    NavigationLink(value: "rendering") { Label("Rendering Pipeline", systemImage: "paintpalette") }
                }
            }
            .navigationTitle("Documentation")
            .listStyle(.sidebar)
        } detail: {
            if let page = selectedPage {
                ScrollView {
                    VStack(alignment: .leading, spacing: 20) {
                        Text(pageTitle(for: page))
                            .font(.system(size: 32, weight: .bold))
                        
                        Text(pageContent)
                            .font(.system(size: 16))
                            .lineSpacing(6)
                            .foregroundStyle(.secondary)
                        
                        Spacer()
                    }
                    .padding(40)
                    .frame(maxWidth: .infinity, alignment: .leading)
                }
                .onChange(of: selectedPage) { newValue in
                    if let newValue = newValue {
                        loadContent(for: newValue)
                    }
                }
            } else {
                VStack {
                    Image(systemName: "book")
                        .font(.system(size: 48))
                        .foregroundColor(.gray)
                    Text("Select a page")
                        .font(.headline)
                        .foregroundColor(.secondary)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
    }
    
    func pageTitle(for id: String) -> String {
        switch id {
        case "introduction": return "Introduction to VoxelForge"
        case "getting-started": return "Getting Started"
        case "installation": return "Installation Guide"
        case "architecture": return "Engine Architecture"
        case "ecs": return "Entity Component System"
        case "rendering": return "Rendering Pipeline"
        default: return id.capitalized
        }
    }
    
    func loadContent(for id: String) {
        if let url = Bundle.main.url(forResource: id, withExtension: "md", subdirectory: "Docs"),
           let content = try? String(contentsOf: url) {
            self.pageContent = content
        } else {
            self.pageContent = "Content for '\(id)' is coming soon."
        }
    }
}
