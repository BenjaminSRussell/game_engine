import SwiftUI
import Combine

// MARK: - Preference Manager
class PreferenceManager: ObservableObject {
    static let shared = PreferenceManager()
    
    @AppStorage("editor.theme") var theme: String = "dark"
    @AppStorage("editor.fontSize") var fontSize: Int = 12
    @AppStorage("editor.autoSave") var autoSave: Bool = true
    @AppStorage("editor.autoSaveInterval") var autoSaveInterval: Int = 300 // seconds
    
    @AppStorage("viewport.gridEnabled") var gridEnabled: Bool = true
    @AppStorage("viewport.gridSpacing") var gridSpacing: Double = 1.0
    @AppStorage("viewport.gizmoSize") var gizmoSize: Double = 1.0
    
    @Published var searchQuery: String = ""
    
    // Validates preferences within acceptable bounds
    func validate() -> Bool {
        if fontSize < 8 || fontSize > 32 { return false }
        if autoSaveInterval < 10 { return false }
        if gridSpacing <= 0 { return false }
        if gizmoSize <= 0.1 { return false }
        return true
    }
    
    // Resets all user preferences to default values
    func resetToDefaults() {
        theme = "dark"
        fontSize = 12
        autoSave = true
        autoSaveInterval = 300
        gridEnabled = true
        gridSpacing = 1.0
        gizmoSize = 1.0
        
        // Ensure defaults are valid
        _ = validate()
    }
}

// MARK: - Preferences View
struct PreferenceView: View {
    @ObservedObject var prefs = PreferenceManager.shared
    @State private var selectedCategory: String = "General"
    
    let categories = ["General", "Appearance", "Editor", "Viewport", "Performance", "Input"]
    
    var body: some View {
        NavigationSplitView {
            List(categories, id: \.self, selection: $selectedCategory) { category in
                NavigationLink(value: category) {
                    Text(category)
                }
            }
            .navigationTitle("Preferences")
        } detail: {
            VStack(spacing: 0) {
                // Search Bar
                HStack {
                    Image(systemName: "magnifyingglass")
                        .foregroundColor(.gray)
                    TextField("Search preferences...", text: $prefs.searchQuery)
                        .textFieldStyle(.plain)
                }
                .padding()
                .background(Color(.windowBackgroundColor))
                
                Divider()
                
                ScrollView {
                    VStack(alignment: .leading, spacing: 20) {
                        switch selectedCategory {
                        case "General":
                            GeneralPrefsView()
                        case "Appearance":
                            AppearancePrefsView()
                        case "Editor":
                            EditorPrefsView()
                        case "Viewport":
                            ViewportPrefsView()
                        default:
                            Text("\(selectedCategory) settings coming soon...")
                                .foregroundColor(.gray)
                        }
                    }
                    .padding()
                }
            }
        }
        .frame(minWidth: 600, minHeight: 400)
    }
}

struct GeneralPrefsView: View {
    @ObservedObject var prefs = PreferenceManager.shared
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Toggle("Auto-Save", isOn: $prefs.autoSave)
            if prefs.autoSave {
                HStack {
                    Text("Auto-Save Interval (seconds)")
                    TextField("", value: $prefs.autoSaveInterval, formatter: NumberFormatter())
                        .frame(width: 60)
                }
            }
        }
    }
}

struct AppearancePrefsView: View {
    @ObservedObject var prefs = PreferenceManager.shared
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Picker("Theme", selection: $prefs.theme) {
                Text("Dark").tag("dark")
                Text("Light").tag("light")
                Text("High Contrast").tag("highContrast")
            }
            Stepper("Editor Font Size: \(prefs.fontSize)", value: $prefs.fontSize, in: 8...32)
        }
    }
}

struct EditorPrefsView: View {
    var body: some View {
        Text("Editor specific settings...")
    }
}

struct ViewportPrefsView: View {
    @ObservedObject var prefs = PreferenceManager.shared
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Toggle("Show Grid", isOn: $prefs.gridEnabled)
            HStack {
                Text("Grid Spacing")
                Slider(value: $prefs.gridSpacing, in: 0.1...10.0)
            }
            HStack {
                Text("Gizmo Size")
                Slider(value: $prefs.gizmoSize, in: 0.5...2.0)
            }
        }
    }
}
