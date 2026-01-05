import SwiftUI

// MARK: - Prefab Editor

class PrefabManager: ObservableObject {
    static let shared = PrefabManager()
    
    @Published var prefabs: [PrefabAsset] = []
    @Published var selectedPrefab: UUID?
    
    init() {
        // Demo data
        prefabs = [
            PrefabAsset(name: "PlayerCharacter", path: "Assets/Prefabs/Player.prefab", category: "Characters"),
            PrefabAsset(name: "Enemy_Zombie", path: "Assets/Prefabs/Zombie.prefab", category: "Enemies"),
            PrefabAsset(name: "Environment_Tree", path: "Assets/Prefabs/Tree_01.prefab", category: "Environment"),
            PrefabAsset(name: "Prop_Crate", path: "Assets/Prefabs/Crate.prefab", category: "Props")
        ]
    }
    
    func createPrefab() {
        let newPrefab = PrefabAsset(name: "New Prefab \(prefabs.count)", path: "Assets/Prefabs/New.prefab", category: "Uncategorized")
        prefabs.append(newPrefab)
    }
}

struct PrefabAsset: Identifiable {
    let id = UUID()
    var name: String
    var path: String
    var category: String
    var thumbnail: String = "cube"
    var instanceCount: Int = 0
    var hasOverrides: Bool = false
}

struct PrefabEditorView: View {
    @ObservedObject var manager = PrefabManager.shared
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Prefabs")
                    .font(DesignSystem.Typography.h2)
                
                Spacer()
                
                Button(action: { manager.createPrefab() }) {
                    Label("Create Prefab", systemImage: "plus")
                }
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            HSplitView {
                // Prefab List
                VStack(alignment: .leading, spacing: 0) {
                    ScrollView {
                        LazyVStack(spacing: 4) {
                            ForEach(manager.prefabs) { prefab in
                                PrefabRow(prefab: prefab, isSelected: manager.selectedPrefab == prefab.id) {
                                    manager.selectedPrefab = prefab.id
                                }
                            }
                        }
                        .padding(DesignSystem.Spacing.sm)
                    }
                }
                .frame(minWidth: 200, maxWidth: 300)
                
                // Prefab Inspector
                if let selectedId = manager.selectedPrefab, let prefab = manager.prefabs.first(where: { $0.id == selectedId }) {
                    PrefabInspector(prefab: prefab)
                } else {
                    VStack {
                        Image(systemName: "cube")
                            .font(.system(size: 48))
                            .foregroundColor(DesignSystem.Colors.textDisabled)
                        Text("Select a prefab to edit")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .background(DesignSystem.Colors.backgroundSecondary)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

struct PrefabRow: View {
    let prefab: PrefabAsset
    let isSelected: Bool
    let action: () -> Void
    
    var body: some View {
        HStack {
            Image(systemName: prefab.thumbnail)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .frame(width: 24, height: 24)
            
            VStack(alignment: .leading) {
                Text(prefab.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Text(prefab.path)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
            
            if prefab.hasOverrides {
                Image(systemName: "exclamationmark.triangle")
                    .foregroundColor(.orange)
                    .font(.caption)
            }
        }
        .padding(8)
        .background(isSelected ? DesignSystem.Colors.selection : Color.clear)
        .cornerRadius(4)
        .contentShape(Rectangle())
        .onTapGesture(perform: action)
    }
}

struct PrefabInspector: View {
    let prefab: PrefabAsset
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text(prefab.name)
                .font(DesignSystem.Typography.h2)
                .padding(DesignSystem.Spacing.md)
                .frame(maxWidth: .infinity, alignment: .leading)
                .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            ScrollView {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                    Group {
                        Text("Info")
                            .font(DesignSystem.Typography.bodyBold)
                        
                        Text("Path: \(prefab.path)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Text("Category: \(prefab.category)")
                            .font(DesignSystem.Typography.body)
                        
                        Text("Instances: \(prefab.instanceCount)")
                            .font(DesignSystem.Typography.body)
                    }
                    
                    EditorDivider()
                    
                    Group {
                        Text("Actions")
                            .font(DesignSystem.Typography.bodyBold)
                        
                        Button("Open in Editor") { }
                        Button("Instantiate in Scene") { }
                        Button("Update from Selected") { }
                        Button("Revert Overrides") { }
                            .foregroundColor(.red)
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}
