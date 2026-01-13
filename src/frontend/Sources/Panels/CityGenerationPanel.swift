import SwiftUI

// MARK: - City Generation Panel

struct CityGenerationPanel: View {
    @StateObject private var viewModel = CityGeneratorViewModel()
    @State private var selectedTab: CityTab = .generation
    
    enum CityTab: String, CaseIterable {
        case generation = "Generation"
        case rules = "Rules"
        case preview = "Preview"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Tab Bar
            HStack(spacing: 0) {
                ForEach(CityTab.allCases, id: \.self) { tab in
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
                    case .generation:
                        GenerationControlView(viewModel: viewModel)
                    case .rules:
                        CityRuleEditorView(viewModel: viewModel)
                    case .preview:
                        CityPreviewView(viewModel: viewModel)
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
    }
}

// MARK: - Sub-Views

private struct GenerationControlView: View {
    @ObservedObject var viewModel: CityGeneratorViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("General Parameters", isExpanded: true) {
                EditorNumericField(label: "City Size (Blocks)", value: $viewModel.citySize, range: 100...5000, step: 100)
                EditorNumericField(label: "Road Density", value: $viewModel.roadDensity, range: 0...1, step: 0.1)
                EditorNumericField(label: "Building Density", value: $viewModel.buildingDensity, range: 0...1, step: 0.1)
                
                Picker("Architectural Style", selection: $viewModel.architecturalStyle) {
                    ForEach(CityGeneratorViewModel.ArchitectureStyle.allCases, id: \.self) { style in
                        Text(style.rawValue).tag(style)
                    }
                }
            }
            
            EditorCollapsibleSection("Actions", isExpanded: true) {
                HStack {
                    EditorButton("Generate", icon: "hammer.fill") {
                        viewModel.generate()
                    }
                    EditorButton("Cancel", icon: "xmark.circle") {
                        viewModel.cancel()
                    }
                }
                
                if viewModel.isGenerating {
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Generating: \(Int(viewModel.progress * 100))%")
                        ProgressView(value: viewModel.progress)
                    }
                }
            }
        }
    }
}

private struct CityRuleEditorView: View {
    @ObservedObject var viewModel: CityGeneratorViewModel
    
    var body: some View {
         VStack(spacing: DesignSystem.Spacing.md) {
             Text("Building Rules & Constraints")
                 .font(DesignSystem.Typography.h3)
             
             // Placeholder for a node-based or list-based rule editor
             // Could re-use NodeGraphEditor here if appropriate
             ZStack {
                 RoundedRectangle(cornerRadius: 8)
                     .stroke(DesignSystem.Colors.border, style: StrokeStyle(lineWidth: 1, dash: [5]))
                     .frame(height: 200)
                 
                 Text("Rule Graph / List Editor Placeholder")
                     .foregroundColor(DesignSystem.Colors.textTertiary)
             }
             
             // List of simple constraints
             ForEach($viewModel.rules) { $rule in
                 HStack {
                     TextField("Rule Name", text: $rule.name)
                         .textFieldStyle(.roundedBorder)
                     Toggle("Active", isOn: $rule.isActive)
                         .toggleStyle(EditorToggleStyle())
                 }
                 .padding(DesignSystem.Spacing.sm)
                 .background(DesignSystem.Colors.backgroundTertiary)
                 .cornerRadius(4)
             }
             
             EditorButton("Add Rule") {
                 viewModel.rules.append(CityGeneratorViewModel.CityRule(name: "New Rule"))
             }
         }
    }
}

private struct CityPreviewView: View {
    @ObservedObject var viewModel: CityGeneratorViewModel
    
    var body: some View {
        VStack {
            // Interactive 2D map or 3D view placeholder
            ZStack {
                Color.black
                Text("Interactive Map Preview")
                    .foregroundColor(.white)
            }
            .frame(height: 400)
            .cornerRadius(8)
            
            HStack {
                Text("Selected Block: \(viewModel.selectedBlockId ?? "None")")
                Spacer()
                EditorButton("Regenerate Block") {
                    if let id = viewModel.selectedBlockId {
                        viewModel.regenerateBlock(id)
                    }
                }
                .disabled(viewModel.selectedBlockId == nil)
            }
            .padding()
            .background(DesignSystem.Colors.backgroundTertiary)
        }
    }
}

// MARK: - View Model

class CityGeneratorViewModel: ObservableObject {
    enum ArchitectureStyle: String, CaseIterable {
        case modern = "Modern"
        case medieval = "Medieval"
        case scifi = "Sci-Fi"
        case industrial = "Industrial"
    }
    
    struct CityRule: Identifiable {
        let id = UUID()
        var name: String
        var isActive: Bool = true
    }
    
    @Published var citySize: Float = 1000
    @Published var roadDensity: Float = 0.5
    @Published var buildingDensity: Float = 0.7
    @Published var architecturalStyle: ArchitectureStyle = .modern
    
    @Published var isGenerating: Bool = false
    @Published var progress: Float = 0.0
    
    @Published var rules: [CityRule] = [
        CityRule(name: "Max Height: 200m"),
        CityRule(name: "Park Frequency: Low")
    ]
    
    @Published var selectedBlockId: String? = nil
    
    func generate() {
        isGenerating = true
        progress = 0.0
        
        // Simulation
        Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { timer in
            self.progress += 0.05
            if self.progress >= 1.0 {
                timer.invalidate()
                self.isGenerating = false
                self.progress = 1.0
            }
        }
    }
    
    func cancel() {
        isGenerating = false
        progress = 0.0
    }
    
    func regenerateBlock(_ id: String) {
        print("Regenerating block \(id)")
    }
}
