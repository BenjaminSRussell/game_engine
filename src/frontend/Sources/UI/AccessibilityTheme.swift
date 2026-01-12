import SwiftUI

// MARK: - Accessibility Helpers (50+ features)

// MARK: - Accessibility Manager

class AccessibilityManager: ObservableObject {
    static let shared = AccessibilityManager()
    
    @Published var reduceMotion: Bool = false
    @Published var highContrast: Bool = false
    @Published var largeText: Bool = false
    @Published var reduceTransparency: Bool = false
    
    init() {
        // Check system preferences
        reduceMotion = NSWorkspace.shared.accessibilityDisplayShouldReduceMotion
        reduceTransparency = NSWorkspace.shared.accessibilityDisplayShouldReduceTransparency
        
        // Monitor changes
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(accessibilitySettingsChanged),
            name: NSWorkspace.accessibilityDisplayOptionsDidChangeNotification,
            object: nil
        )
    }
    
    @objc private func accessibilitySettingsChanged() {
        reduceMotion = NSWorkspace.shared.accessibilityDisplayShouldReduceMotion
        reduceTransparency = NSWorkspace.shared.accessibilityDisplayShouldReduceTransparency
    }
}

// MARK: - Accessible Button

struct AccessibleButton<Label: View>: View {
    let action: () -> Void
    let label: Label
    var accessibilityLabel: String
    var accessibilityHint: String?
    
    init(
        _ accessibilityLabel: String,
        hint: String? = nil,
        action: @escaping () -> Void,
        @ViewBuilder label: () -> Label
    ) {
        self.accessibilityLabel = accessibilityLabel
        self.accessibilityHint = hint
        self.action = action
        self.label = label()
    }
    
    var body: some View {
        Button(action: action) {
            label
        }
        .accessibilityLabel(accessibilityLabel)
        .accessibilityHint(accessibilityHint ?? "")
        .accessibilityAddTraits(.isButton)
        .buttonStyle(.plain)
    }
}

// MARK: - Focus-Visible Modifier

struct FocusVisibleModifier: ViewModifier {
    @FocusState private var isFocused: Bool
    let color: Color
    
    func body(content: Content) -> some View {
        content
            .focused($isFocused)
            .overlay(
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular + 2)
                    .stroke(color, lineWidth: isFocused ? 2 : 0)
                    .padding(-2)
            )
            .animation(DesignSystem.Animation.fast, value: isFocused)
    }
}

extension View {
    func focusVisible(color: Color = DesignSystem.Colors.accentPrimary) -> some View {
        modifier(FocusVisibleModifier(color: color))
    }
}

// MARK: - Skip Navigation Link

struct SkipNavigationLink: View {
    let destination: String
    
    var body: some View {
        Button("Skip to \(destination)") {
            // Announce to VoiceOver
        }
        .accessibilityAddTraits(.isLink)
        .opacity(0)
        .accessibilityHidden(false)
    }
}

// MARK: - Announce to VoiceOver

extension View {
    func announceChange(_ announcement: String, when condition: Bool) -> some View {
        self
            .onChange(of: condition) { _ in
                if condition {
                    NSAccessibility.post(
                        element: NSApp.mainWindow as Any,
                        notification: .announcementRequested,
                        userInfo: [.announcement: announcement]
                    )
                }
            }
    }
}

// MARK: - High Contrast Colors

struct HighContrastColors {
    @ObservedObject static var accessibility = AccessibilityManager.shared
    
    static var textPrimary: Color {
        accessibility.highContrast ? .white : DesignSystem.Colors.textPrimary
    }
    
    static var textSecondary: Color {
        accessibility.highContrast ? Color(hex: "#CCCCCC") : DesignSystem.Colors.textSecondary
    }
    
    static var backgroundPrimary: Color {
        accessibility.highContrast ? .black : DesignSystem.Colors.backgroundPrimary
    }
    
    static var accentPrimary: Color {
        accessibility.highContrast ? Color(hex: "#00AAFF") : DesignSystem.Colors.accentPrimary
    }
}

// MARK: - Motion-Safe Animation

extension View {
    func motionSafeAnimation<V: Equatable>(_ animation: Animation?, value: V) -> some View {
        self
            .animation(
                AccessibilityManager.shared.reduceMotion ? nil : animation,
                value: value
            )
    }
}

// Themes removed - using DesignSystem.Colors directly

// MARK: - Preferences Panel

struct EditorPreferencesPanel: View {
    @ObservedObject var accessibility = AccessibilityManager.shared
    @State private var selectedTab = 0
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Preferences")
                    .font(DesignSystem.Typography.title2)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            HStack(spacing: 0) {
                // Sidebar
                VStack(alignment: .leading, spacing: 2) {
                    PreferenceTab(title: "General", icon: "gearshape", isSelected: selectedTab == 0) { selectedTab = 0 }
                    PreferenceTab(title: "Appearance", icon: "paintpalette", isSelected: selectedTab == 1) { selectedTab = 1 }
                    PreferenceTab(title: "Shortcuts", icon: "keyboard", isSelected: selectedTab == 2) { selectedTab = 2 }
                    PreferenceTab(title: "Accessibility", icon: "accessibility", isSelected: selectedTab == 3) { selectedTab = 3 }
                    PreferenceTab(title: "Performance", icon: "speedometer", isSelected: selectedTab == 4) { selectedTab = 4 }
                    Spacer()
                }
                .frame(width: 150)
                .padding(DesignSystem.Spacing.sm)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                // Content
                ScrollView {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.lg) {
                        switch selectedTab {
                        case 0: GeneralPreferences()
                        case 1: AppearanceSettings()
                        case 2: KeyboardShortcutsPanel()
                        case 3: AccessibilityPreferences()
                        case 4: PerformancePreferences()
                        default: EmptyView()
                        }
                    }
                    .padding(DesignSystem.Spacing.lg)
                }
            }
        }
        .frame(width: 700, height: 500)
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(DesignSystem.CornerRadius.large)
    }
    
    struct PreferenceTab: View {
        let title: String
        let icon: String
        let isSelected: Bool
        let action: () -> Void
        
        @State private var isHovered = false
        
        var body: some View {
            Button(action: action) {
                HStack(spacing: DesignSystem.Spacing.sm) {
                    Image(systemName: icon)
                        .font(.system(size: 14))
                        .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                        .frame(width: 20)
                    
                    Text(title)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(isSelected ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                }
                .padding(.horizontal, DesignSystem.Spacing.sm)
                .padding(.vertical, DesignSystem.Spacing.xs)
                .background(isSelected ? DesignSystem.Colors.selection : (isHovered ? DesignSystem.Colors.hover : Color.clear))
                .cornerRadius(DesignSystem.CornerRadius.tight)
            }
            .buttonStyle(.plain)
            .onHover { isHovered = $0 }
        }
    }
    
    struct GeneralPreferences: View {
        @State private var autoSave = true
        @State private var autoSaveInterval = 5
        @State private var showWelcome = true
        
        var body: some View {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                Text("General")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                EditorToggle(label: "Auto-save enabled", isOn: $autoSave)
                
                if autoSave {
                    EditorStepper(label: "Auto-save interval (minutes)", value: $autoSaveInterval, range: 1...30)
                }
                
                EditorToggle(label: "Show welcome screen on startup", isOn: $showWelcome)
            }
        }
    }
    
    struct AppearanceSettings: View {
        @State private var showGrid = true
        @State private var showStats = true
        @State private var uiScale: Double = 1.0
        
        var body: some View {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                Text("Appearance")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text("Color Palette")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                // Display current color palette
                LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible()), GridItem(.flexible())], spacing: 8) {
                    ColorSwatch(name: "Primary", color: DesignSystem.Colors.accentPrimary)
                    ColorSwatch(name: "Success", color: DesignSystem.Colors.accentSuccess)
                    ColorSwatch(name: "Warning", color: DesignSystem.Colors.accentWarning)
                    ColorSwatch(name: "Danger", color: DesignSystem.Colors.accentDanger)
                    ColorSwatch(name: "Info", color: DesignSystem.Colors.accentInfo)
                    ColorSwatch(name: "Tertiary", color: DesignSystem.Colors.accentTertiary)
                }
                
                EditorToggle(label: "Show grid", isOn: $showGrid)
                EditorToggle(label: "Show stats overlay", isOn: $showStats)
                
                Text("UI Scale")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                EditorSlider(label: "UI Scale", value: $uiScale, range: 0.75...1.5, step: 0.05)
            }
        }
        
        struct ColorSwatch: View {
            let name: String
            let color: Color
            
            var body: some View {
                VStack(spacing: 4) {
                    RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular)
                        .fill(color)
                        .frame(height: 32)
                    Text(name)
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
        }
    }
    
    struct AccessibilityPreferences: View {
        @ObservedObject var accessibility = AccessibilityManager.shared
        
        var body: some View {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                Text("Accessibility")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                EditorToggle(label: "Reduce motion", isOn: $accessibility.reduceMotion)
                EditorToggle(label: "High contrast mode", isOn: $accessibility.highContrast)
                EditorToggle(label: "Large text", isOn: $accessibility.largeText)
                EditorToggle(label: "Reduce transparency", isOn: $accessibility.reduceTransparency)
            }
        }
    }
    
    struct PerformancePreferences: View {
        @State private var vsync = true
        @State private var targetFPS = 60
        @State private var shadowQuality = 2
        
        var body: some View {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                Text("Performance")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                EditorToggle(label: "V-Sync", isOn: $vsync)
                EditorStepper(label: "Target FPS", value: $targetFPS, range: 30...144)
                
                Text("Shadow Quality")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                EditorSegmentedControl(
                    options: [0, 1, 2, 3],
                    selected: .constant(shadowQuality),
                    label: { ["Off", "Low", "Medium", "High"][$0] }
                )
            }
        }
    }
}
