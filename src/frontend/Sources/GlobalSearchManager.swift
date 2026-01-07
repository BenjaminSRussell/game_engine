import SwiftUI
import Combine

// MARK: - Global Search Manager
class GlobalSearchManager: ObservableObject {
    static let shared = GlobalSearchManager()
    
    @Published var isSearching: Bool = false
    @Published var searchText: String = ""
    @Published var results: [SearchResult] = []
    
    func performSearch() {
        guard !searchText.isEmpty else {
            results = []
            return
        }
        
        // Search categories: Entities, Assets, Menus, Documents
        var newResults: [SearchResult] = []
        
        // 1. Search Entities
        let entities = EngineBridge.shared.entities.values.filter { 
            $0.name.localizedCaseInsensitiveContains(searchText) 
        }
        for entity in entities {
            newResults.append(SearchResult(title: entity.name, type: .entity, targetID: entity.id))
        }
        
        // 2. Search Menus
        let menus = [
            "New Project", "Open Project", "Save Project",
            "New Scene", "Open Scene", "Save Scene",
            "Undo", "Redo", "Cut", "Copy", "Paste",
            "Create Material", "Create Script", "Create Shader",
            "Import Asset", "Build Project"
        ].filter {
            $0.localizedCaseInsensitiveContains(searchText)
        }
        for menu in menus {
            newResults.append(SearchResult(title: menu, type: .menu))
        }
        
        self.results = newResults
    }
}

// MARK: - Search Models
struct SearchResult: Identifiable {
    let id = UUID()
    let title: String
    let type: ResultType
    var targetID: UUID? = nil
    
    enum ResultType {
        case entity, asset, menu, document
        
        var icon: String {
            switch self {
            case .entity: return "cube"
            case .asset: return "doc"
            case .menu: return "command"
            case .document: return "book"
            }
        }
    }
}

// MARK: - Search Command Palette
struct SearchCommandPalette: View {
    @ObservedObject var search = GlobalSearchManager.shared
    @Environment(\.dismiss) var dismiss
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Image(systemName: "magnifyingglass")
                    .font(.title2)
                    .foregroundColor(.gray)
                
                TextField("Search anything (entities, assets, tools)...", text: $search.searchText)
                    .textFieldStyle(.plain)
                    .font(.title2)
                    .onSubmit {
                        search.performSearch()
                    }
            }
            .padding()
            
            Divider()
            
            if search.results.isEmpty {
                VStack(spacing: 8) {
                    Image(systemName: "sparkles")
                        .font(.largeTitle)
                        .foregroundColor(.gray)
                    Text("Try searching for 'Player' or 'Material'")
                        .foregroundColor(.gray)
                }
                .frame(maxHeight: .infinity)
            } else {
                List(search.results) { result in
                    Button(action: {
                        // Action based on type
                        dismiss()
                    }) {
                        HStack {
                            Image(systemName: result.type.icon)
                                .frame(width: 24)
                            Text(result.title)
                            Spacer()
                            Text(String(describing: result.type).capitalized)
                                .font(.caption)
                                .foregroundColor(.gray)
                        }
                    }
                    .buttonStyle(.plain)
                    .padding(.vertical, 4)
                }
            }
        }
        .frame(width: 500, height: 400)
        .background(DesignSystem.Colors.backgroundPrimary)
        .onChange(of: search.searchText) { _ in
            search.performSearch()
        }
    }
}
