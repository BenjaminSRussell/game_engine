import SwiftUI

/// Property inspector panel for editing selected assets
struct PropertyInspectorView: View {
    @State private var selectedAsset: InspectableAsset?
    @State private var expandedSections: Set<String> = ["Transform", "Material"]
    
    var body: some View {
        ScrollView {
            VStack(spacing: 0) {
                if let asset = selectedAsset {
                    // Asset Header
                    assetHeader(asset)
                    
                    // Property Sections
                    if asset.hasTransform {
                        PropertySection(title: "Transform", isExpanded: expandedSections.contains("Transform")) {
                            transformProperties(asset)
                        } onToggle: {
                            toggleSection("Transform")
                        }
                    }
                    
                    if asset.hasMaterial {
                        PropertySection(title: "Material", isExpanded: expandedSections.contains("Material")) {
                            materialProperties(asset)
                        } onToggle: {
                            toggleSection("Material")
                        }
                    }
                    
                    if asset.hasGeometry {
                        PropertySection(title: "Geometry", isExpanded: expandedSections.contains("Geometry")) {
                            geometryProperties(asset)
                        } onToggle: {
                            toggleSection("Geometry")
                        }
                    }
                    
                    if asset.hasPhysics {
                        PropertySection(title: "Physics", isExpanded: expandedSections.contains("Physics")) {
                            physicsProperties(asset)
                        } onToggle: {
                            toggleSection("Physics")
                        }
                    }
                } else {
                    // No Selection
                    VStack(spacing: 12) {
                        Image(systemName: "doc.text.magnifyingglass")
                            .font(.system(size: 48))
                            .foregroundColor(.gray)
                        Text("No Asset Selected")
                            .font(.headline)
                            .foregroundColor(.gray)
                        Text("Select an asset to view its properties")
                            .font(.caption)
                            .foregroundColor(.gray)
                    }
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .padding()
                }
            }
        }
        .background(Color(white: 0.1))
    }
    
    // MARK: - Asset Header
    
    private func assetHeader(_ asset: InspectableAsset) -> some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Image(systemName: asset.icon)
                    .font(.title2)
                    .foregroundColor(.cyan)
                
                VStack(alignment: .leading, spacing: 2) {
                    Text(asset.name)
                        .font(.headline)
                        .foregroundColor(.white)
                    Text(asset.type)
                        .font(.caption)
                        .foregroundColor(.gray)
                }
                
                Spacer()
            }
            .padding()
            .background(Color(white: 0.15))
        }
    }
    
    // MARK: - Transform Properties
    
    private func transformProperties(_ asset: InspectableAsset) -> some View {
        VStack(spacing: 12) {
            PropertyRow(label: "Position") {
                HStack(spacing: 8) {
                    PropertyField(label: "X", value: .constant(0.0))
                    PropertyField(label: "Y", value: .constant(0.0))
                    PropertyField(label: "Z", value: .constant(0.0))
                }
            }
            
            PropertyRow(label: "Rotation") {
                HStack(spacing: 8) {
                    PropertyField(label: "X", value: .constant(0.0))
                    PropertyField(label: "Y", value: .constant(0.0))
                    PropertyField(label: "Z", value: .constant(0.0))
                }
            }
            
            PropertyRow(label: "Scale") {
                HStack(spacing: 8) {
                    PropertyField(label: "X", value: .constant(1.0))
                    PropertyField(label: "Y", value: .constant(1.0))
                    PropertyField(label: "Z", value: .constant(1.0))
                }
            }
        }
    }
    
    // MARK: - Material Properties
    
    private func materialProperties(_ asset: InspectableAsset) -> some View {
        VStack(spacing: 12) {
            PropertyRow(label: "Base Color") {
                ColorPicker("", selection: .constant(Color.white))
                    .labelsHidden()
            }
            
            PropertyRow(label: "Metallic") {
                Slider(value: .constant(0.0), in: 0...1)
                    .accentColor(.cyan)
            }
            
            PropertyRow(label: "Roughness") {
                Slider(value: .constant(0.5), in: 0...1)
                    .accentColor(.cyan)
            }
            
            PropertyRow(label: "Emissive") {
                Toggle("", isOn: .constant(false))
                    .labelsHidden()
            }
        }
    }
    
    // MARK: - Geometry Properties
    
    private func geometryProperties(_ asset: InspectableAsset) -> some View {
        VStack(spacing: 12) {
            PropertyRow(label: "Vertices") {
                Text("1,234")
                    .font(.caption.monospaced())
                    .foregroundColor(.gray)
            }
            
            PropertyRow(label: "Triangles") {
                Text("2,468")
                    .font(.caption.monospaced())
                    .foregroundColor(.gray)
            }
            
            PropertyRow(label: "UV Channels") {
                Text("2")
                    .font(.caption.monospaced())
                    .foregroundColor(.gray)
            }
        }
    }
    
    // MARK: - Physics Properties
    
    private func physicsProperties(_ asset: InspectableAsset) -> some View {
        VStack(spacing: 12) {
            PropertyRow(label: "Collision") {
                Toggle("", isOn: .constant(true))
                    .labelsHidden()
            }
            
            PropertyRow(label: "Mass") {
                PropertyField(label: "kg", value: .constant(1.0))
            }
            
            PropertyRow(label: "Friction") {
                Slider(value: .constant(0.5), in: 0...1)
                    .accentColor(.cyan)
            }
        }
    }
    
    // MARK: - Helper Methods
    
    private func toggleSection(_ section: String) {
        if expandedSections.contains(section) {
            expandedSections.remove(section)
        } else {
            expandedSections.insert(section)
        }
    }
}

// MARK: - Property Section

struct PropertySection<Content: View>: View {
    let title: String
    let isExpanded: Bool
    let content: Content
    let onToggle: () -> Void
    
    init(title: String, isExpanded: Bool, @ViewBuilder content: () -> Content, onToggle: @escaping () -> Void) {
        self.title = title
        self.isExpanded = isExpanded
        self.content = content()
        self.onToggle = onToggle
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Section Header
            Button(action: onToggle) {
                HStack {
                    Image(systemName: isExpanded ? "chevron.down" : "chevron.right")
                        .font(.caption)
                        .foregroundColor(.gray)
                    
                    Text(title)
                        .font(.subheadline.weight(.semibold))
                        .foregroundColor(.white)
                    
                    Spacer()
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 8)
                .background(Color(white: 0.15))
            }
            .buttonStyle(PlainButtonStyle())
            
            // Section Content
            if isExpanded {
                VStack(spacing: 0) {
                    content
                }
                .padding(12)
                .background(Color(white: 0.12))
            }
        }
    }
}

// MARK: - Property Row

struct PropertyRow<Content: View>: View {
    let label: String
    let content: Content
    
    init(label: String, @ViewBuilder content: () -> Content) {
        self.label = label
        self.content = content()
    }
    
    var body: some View {
        HStack {
            Text(label)
                .font(.caption)
                .foregroundColor(.gray)
                .frame(width: 80, alignment: .leading)
            
            content
                .frame(maxWidth: .infinity)
        }
    }
}

// MARK: - Property Field

struct PropertyField: View {
    let label: String
    @Binding var value: Double
    
    var body: some View {
        HStack(spacing: 4) {
            Text(label)
                .font(.caption2)
                .foregroundColor(.gray)
                .frame(width: 12)
            
            TextField("", value: $value, format: .number)
                .textFieldStyle(PlainTextFieldStyle())
                .font(.caption.monospaced())
                .foregroundColor(.white)
                .padding(4)
                .background(Color.black.opacity(0.3))
                .cornerRadius(4)
        }
    }
}

// MARK: - Inspectable Asset Model

struct InspectableAsset {
    let name: String
    let type: String
    let icon: String
    let hasTransform: Bool
    let hasMaterial: Bool
    let hasGeometry: Bool
    let hasPhysics: Bool
    
    static let example = InspectableAsset(
        name: "TestModel.fbx",
        type: "3D Model",
        icon: "cube",
        hasTransform: true,
        hasMaterial: true,
        hasGeometry: true,
        hasPhysics: true
    )
}
