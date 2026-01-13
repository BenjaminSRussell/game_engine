import SwiftUI

// MARK: - Weather Panel

struct WeatherPanel: View {
    @StateObject private var viewModel = WeatherViewModel()
    @State private var selectedTab: WeatherTab = .zones
    
    enum WeatherTab: String, CaseIterable {
        case zones = "Zones"
        case timeOfDay = "Time of Day"
        case clouds = "Volumetric Clouds"
        case effects = "Precipitation & Effects"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Tab Bar
            HStack(spacing: 0) {
                ForEach(WeatherTab.allCases, id: \.self) { tab in
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
                    case .zones:
                        WeatherZoneView(viewModel: viewModel)
                    case .timeOfDay:
                        TimeOfDayEditorView(viewModel: viewModel)
                    case .clouds:
                        CloudEditorView(viewModel: viewModel)
                    case .effects:
                        WeatherEffectsView(viewModel: viewModel)
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
    }
}

// MARK: - Sub-Views

private struct WeatherZoneView: View {
    @ObservedObject var viewModel: WeatherViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Weather Zones", isExpanded: true) {
                // List of zones
                VStack(spacing: 1) {
                    ForEach(viewModel.zones) { zone in
                        HStack {
                            Text(zone.name)
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(DesignSystem.Colors.textPrimary)
                            Spacer()
                            Text(zone.type.rawValue.capitalized)
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                        .padding(DesignSystem.Spacing.sm)
                        .background(DesignSystem.Colors.backgroundTertiary)
                    }
                }
                .cornerRadius(4)
                
                EditorButton("Add New Zone", icon: "plus") {
                    viewModel.addZone()
                }
            }
        }
    }
}

private struct TimeOfDayEditorView: View {
    @ObservedObject var viewModel: WeatherViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Cycle Control", isExpanded: true) {
                HStack {
                    Text("Current Time")
                    Spacer()
                    Text(viewModel.timeString)
                        .font(.body.monospaced())
                }
                Slider(value: $viewModel.currentTime, in: 0...24)
                
                Toggle("Auto Cycle", isOn: $viewModel.autoCycle)
                    .toggleStyle(EditorToggleStyle())
                
                EditorNumericField(label: "Day Length (min)", value: $viewModel.dayLengthMinutes, range: 1...120, step: 1)
            }
            
            EditorCollapsibleSection("Sky Colors", isExpanded: true) {
                 CurveEditor(points: $viewModel.skyColorCurvePoints)
                    .frame(height: 150)
            }
        }
    }
}

private struct CloudEditorView: View {
    @ObservedObject var viewModel: WeatherViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Shape & Noise", isExpanded: true) {
                EditorNumericField(label: "Coverage", value: $viewModel.cloudCoverage, range: 0...1, step: 0.05)
                EditorNumericField(label: "Density", value: $viewModel.cloudDensity, range: 0...2, step: 0.1)
                EditorNumericField(label: "Noise Scale", value: $viewModel.cloudNoiseScale, range: 0.1...10, step: 0.1)
            }
            
            EditorCollapsibleSection("Lighting", isExpanded: true) {
               EditorNumericField(label: "Absorption", value: $viewModel.cloudAbsorption, range: 0...5, step: 0.1)
               ColorPicker("Scatter Color", selection: $viewModel.cloudScatterColor)
            }
        }
    }
}

private struct WeatherEffectsView: View {
    @ObservedObject var viewModel: WeatherViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Precipitation", isExpanded: true) {
                 EditorNumericField(label: "Rain Intensity", value: $viewModel.rainIntensity, range: 0...1, step: 0.05)
                 EditorNumericField(label: "Snow Intensity", value: $viewModel.snowIntensity, range: 0...1, step: 0.05)
                 EditorNumericField(label: "Accumulation Rate", value: $viewModel.accumulationRate, range: 0...1, step: 0.1)
            }
            
            EditorCollapsibleSection("Storms", isExpanded: true) {
                 EditorNumericField(label: "Lightning Frequency", value: $viewModel.lightningFrequency, range: 0...1, step: 0.05)
                 EditorButton("Trigger Lightning") {
                     viewModel.triggerLightning()
                 }
            }
        }
    }
}

// MARK: - View Model

class WeatherViewModel: ObservableObject {
    // Zones
    struct WeatherZone: Identifiable {
        let id = UUID()
        var name: String
        var type: WeatherVFXAPI.WeatherType
    }
    
    @Published var zones: [WeatherZone] = [
        WeatherZone(name: "Global Default", type: .clear),
        WeatherZone(name: "Mountain Peak", type: .snow)
    ]
    
    // Time of Day
    @Published var currentTime: Float = 12.0
    @Published var autoCycle: Bool = true
    @Published var dayLengthMinutes: Float = 20.0
    @Published var skyColorCurvePoints: [SIMD2<Float>] = [
        SIMD2<Float>(0.0, 0.1), // Midnight
        SIMD2<Float>(0.25, 0.8), // Sunrise
        SIMD2<Float>(0.5, 1.0), // Noon
        SIMD2<Float>(0.75, 0.8), // Sunset
        SIMD2<Float>(1.0, 0.1)  // Midnight
    ]
    
    var timeString: String {
        let hours = Int(currentTime)
        let minutes = Int((currentTime - Float(hours)) * 60)
        return String(format: "%02d:%02d", hours, minutes)
    }
    
    // Clouds
    @Published var cloudCoverage: Float = 0.5
    @Published var cloudDensity: Float = 1.0
    @Published var cloudNoiseScale: Float = 1.0
    @Published var cloudAbsorption: Float = 0.5
    @Published var cloudScatterColor: Color = .white
    
    // Effects
    @Published var rainIntensity: Float = 0.0 {
        didSet { WeatherVFXAPI.shared.rainIntensity = rainIntensity }
    }
    @Published var snowIntensity: Float = 0.0 {
        didSet { WeatherVFXAPI.shared.snowIntensity = snowIntensity }
    }
    @Published var accumulationRate: Float = 0.1
    @Published var lightningFrequency: Float = 0.0
    
    func addZone() {
        zones.append(WeatherZone(name: "New Zone", type: .clear))
    }
    
    func triggerLightning() {
        WeatherVFXAPI.shared.triggerLightning(at: (x: 0, y: 100, z: 0))
    }
}
