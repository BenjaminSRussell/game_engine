import SwiftUI

// MARK: - Ecosystem Panel

struct EcosystemPanel: View {
    @StateObject private var viewModel = EcosystemViewModel()
    @State private var selectedTab: EcosystemTab = .species
    
    enum EcosystemTab: String, CaseIterable {
        case species = "Species Database"
        case behavior = "Behavior Trees"
        case debug = "Debug Overlays"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Tab Bar
            HStack(spacing: 0) {
                ForEach(EcosystemTab.allCases, id: \.self) { tab in
                    Button(action: { selectedTab = tab }) {
                        VStack(spacing: 8) {
                            Text(tab.rawValue)
                                .font(DesignSystem.Typography.bodyBold)
                                .foregroundColor(selectedTab == tab ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                            
                            Rectangle()
                                .fill(selectedTab == tab ? DesignSystem.Colors.accentPrimary : Color.clear)
                                .frame(height: 2)
                        }
                        .padding(.horizontal, DesignSystem.Spacing.md)
                        .padding(.top, DesignSystem.Spacing.sm)
                    }
                    .buttonStyle(.plain)
                    .frame(maxWidth: .infinity)
                }
            }
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Content
            ScrollView {
                VStack(spacing: DesignSystem.Spacing.lg) {
                    switch selectedTab {
                    case .species:
                        SpeciesEditorView(viewModel: viewModel)
                    case .behavior:
                        BehaviorTreeEditorView(viewModel: viewModel)
                    case .debug:
                        EcosystemDebugView(viewModel: viewModel)
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
    }
}

// MARK: - Sub-Views

private struct SpeciesEditorView: View {
    @ObservedObject var viewModel: EcosystemViewModel
    @State private var selectedSpeciesId: UUID?
    
    var body: some View {
        HStack(alignment: .top, spacing: DesignSystem.Spacing.md) {
            // Sidebar List
            VStack(spacing: DesignSystem.Spacing.sm) {
                ForEach(viewModel.speciesList) { species in
                    Button(action: { selectedSpeciesId = species.id }) {
                        HStack {
                            Text(species.name)
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(selectedSpeciesId == species.id ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                            Spacer()
                        }
                        .padding(8)
                        .background(selectedSpeciesId == species.id ? DesignSystem.Colors.selection : DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    }
                    .buttonStyle(.plain)
                }
                
                EditorButton("Add Species", icon: "plus") {
                    viewModel.addSpecies()
                }
            }
            .frame(width: 200)
            
            // Detail Editor
            VStack(spacing: DesignSystem.Spacing.md) {
                if let species = viewModel.speciesList.first(where: { $0.id == selectedSpeciesId }) {
                    EditorCollapsibleSection("General", isExpanded: true) {
                        Text("Name: \(species.name)") // Placeholder for binding
                        // In a real app we'd bind to properties of the species in the array
                    }
                    
                    EditorCollapsibleSection("Needs", isExpanded: true) {
                        Text("Metabolism Rate: \(String(format: "%.2f", species.metabolismRate))")
                        Text("Food Type: \(species.diet.rawValue)")
                    }
                    
                    EditorCollapsibleSection("Model", isExpanded: true) {
                        HStack {
                            Text("Model Path:")
                            Spacer()
                            Text(species.modelPath)
                                .font(.caption.monospaced())
                        }
                    }
                } else {
                    Text("Select a species to edit")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(maxWidth: .infinity, alignment: .center)
                        .padding()
                }
            }
            .padding()
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(8)
        }
    }
}

private struct BehaviorTreeEditorView: View {
    @ObservedObject var viewModel: EcosystemViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            HStack {
                Text("Behavior Tree: Default Predator")
                    .font(DesignSystem.Typography.h3)
                Spacer()
                EditorButton("Open Visual Editor", icon: "arrow.up.right.square") {
                    // Action to open full node graph
                }
            }
            
            // Placeholder for node graph preview
            ZStack {
                Color.black.opacity(0.3)
                VStack(spacing: 20) {
                     NodePlaceholder(label: "Selector", color: .purple)
                     HStack(spacing: 40) {
                         NodePlaceholder(label: "Find Food", color: .green)
                         NodePlaceholder(label: "Patrol", color: .blue)
                     }
                }
            }
            .frame(height: 300)
            .cornerRadius(8)
        }
    }
}

private struct NodePlaceholder: View {
    let label: String
    let color: Color
    
    var body: some View {
        Text(label)
            .padding()
            .background(color.opacity(0.6))
            .cornerRadius(8)
            .overlay(RoundedRectangle(cornerRadius: 8).stroke(Color.white.opacity(0.5)))
    }
}

private struct EcosystemDebugView: View {
    @ObservedObject var viewModel: EcosystemViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Debug Overlays", isExpanded: true) {
                Toggle("Show Needs", isOn: $viewModel.debugShowNeeds)
                    .toggleStyle(EditorToggleStyle())
                Toggle("Show Paths", isOn: $viewModel.debugShowPaths)
                    .toggleStyle(EditorToggleStyle())
                Toggle("Show Perception Radius", isOn: $viewModel.debugShowPerception)
                    .toggleStyle(EditorToggleStyle())
                Toggle("Show State Labels", isOn: $viewModel.debugShowStateLabels)
                    .toggleStyle(EditorToggleStyle())
            }
        }
    }
}

// MARK: - View Model

class EcosystemViewModel: ObservableObject {
    struct Species: Identifiable {
        let id = UUID()
        var name: String
        var metabolismRate: Float
        var diet: DietType
        var modelPath: String
    }
    
    enum DietType: String {
        case herbivore = "Herbivore"
        case carnivore = "Carnivore"
        case omnivore = "Omnivore"
    }
    
    @Published var speciesList: [Species] = [
        Species(name: "Deer", metabolismRate: 0.5, diet: .herbivore, modelPath: "animals/deer.obj"),
        Species(name: "Wolf", metabolismRate: 0.8, diet: .carnivore, modelPath: "animals/wolf.obj")
    ]
    
    // Debug
    @Published var debugShowNeeds: Bool = false
    @Published var debugShowPaths: Bool = false
    @Published var debugShowPerception: Bool = false
    @Published var debugShowStateLabels: Bool = false
    
    func addSpecies() {
        speciesList.append(Species(name: "New Species", metabolismRate: 0.5, diet: .herbivore, modelPath: ""))
    }
}
