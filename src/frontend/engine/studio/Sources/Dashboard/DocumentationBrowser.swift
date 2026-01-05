import SwiftUI

struct DocumentationBrowser: View {
    
    struct DocPage: Identifiable, Hashable {
        let id = UUID()
        let title: String
        let category: String
        let content: String
    }
    
    @State private var searchText = ""
    @State private var selection: DocPage?
    
    // Mock Documentation Data
    let pages = [
        DocPage(title: "Getting Started", category: "General", content: "# Welcome to Minecraft v2 Engine\nThis engine provides a powerful C core with a Swift frontend.\n\n## Quick Start\n1. Create a Project\n2. Import Assets\n3. Press Play"),
        DocPage(title: "Asset Management", category: "Editor", content: "Details on how to use the Asset Browser. Drag and drop supported."),
        DocPage(title: "ECS Architecture", category: "Engine", content: "Entities, Components, Systems. The core triad of data-driven development."),
        DocPage(title: "Rendering Pipeline", category: "Graphics", content: "Vulkan/Metal abstraction layer details using the unified renderer.")
    ]
    
    var filteredPages: [DocPage] {
        if searchText.isEmpty {
            return pages
        } else {
            return pages.filter { $0.title.localizedCaseInsensitiveContains(searchText) || $0.content.localizedCaseInsensitiveContains(searchText) }
        }
    }
    
    var body: some View {
        NavigationSplitView {
            List(selection: $selection) {
                ForEach(Dictionary(grouping: filteredPages, by: { $0.category }).sorted(by: { $0.0 < $1.0 }), id: \.key) { category, docs in
                    Section(header: Text(category)) {
                        ForEach(docs) { doc in
                            NavigationLink(value: doc) {
                                Text(doc.title)
                            }
                        }
                    }
                }
            }
            .searchable(text: $searchText)
            .navigationTitle("Documentation")
        } detail: {
            if let doc = selection {
                DocDetailView(doc: doc)
            } else {
                Text("Select a topic")
                    .foregroundColor(.secondary)
            }
        }
    }
}

struct DocDetailView: View {
    let doc: DocumentationBrowser.DocPage
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                Text(doc.title)
                    .font(.largeTitle)
                    .bold()
                
                Divider()
                
                // Simple Markdown-ish renderer
                Text(doc.content)
                    .font(.body)
                    .lineSpacing(6)
            }
            .padding()
            .frame(maxWidth: 800, alignment: .leading)
        }
    }
}

struct DocumentationBrowser_Previews: PreviewProvider {
    static var previews: some View {
        DocumentationBrowser()
    }
}
