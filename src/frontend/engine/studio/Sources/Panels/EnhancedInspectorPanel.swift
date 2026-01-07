import SwiftUI

// MARK: - Enhanced Inspector Panel
// Dynamic inspector that displays all components on selected entity

struct EnhancedInspectorPanel: View {
    @ObservedObject var selectionManager: SelectionManager
    @StateObject private var viewModel = EnhancedInspectorViewModel()
    @State private var showAddComponentMenu = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Title bar
            HStack {
                Text("Inspector")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
                
                if !selectionManager.selectedEntities.isEmpty {
                    Text("\(selectionManager.selectedEntities.count) selected")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            if let firstSelected = selectionManager.selectedEntities.first {
                ScrollView {
                    VStack(spacing: DesignSystem.Spacing.md) {
                        // Entity header
                        EntityHeaderSection(entityID: firstSelected)
                        
                        // Multi-selection warning
                        if selectionManager.selectedEntities.count > 1 {
                            MultiSelectionWarning(count: selectionManager.selectedEntities.count)
                        }
                        
                        // Components
                        ForEach(viewModel.components, id: \.id) { component in
                            ComponentSection(
                                component: component,
                                onRemove: {
                                    viewModel.removeComponent(component.id)
                                },
                                onChange: { updated in
                                    viewModel.updateComponent(updated)
                                }
                            )
                        }
                        
                        // Add Component button
                        EditorPrimaryButton("Add Component", icon: "plus") {
                            showAddComponentMenu = true
                        }
                        .padding(.horizontal, DesignSystem.Spacing.sm)
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
                .background(DesignSystem.Colors.backgroundSecondary)
                .sheet(isPresented: $showAddComponentMenu) {
                    AddComponentSheet(onAdd: { componentType in
                        viewModel.addComponent(type: componentType, to: firstSelected)
                        showAddComponentMenu = false
                    })
                }
            } else {
                // No selection state
                NoSelectionView()
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .onChange(of: selectionManager.selectedEntities) { newSelection in
            if let firstSelected = newSelection.first {
                viewModel.loadComponents(for: firstSelected)
            }
        }
        .onAppear {
            if let firstSelected = selectionManager.selectedEntities.first {
                viewModel.loadComponents(for: firstSelected)
            }
        }
    }
}

// MARK: - Entity Header Section
private struct EntityHeaderSection: View {
    let entityID: UUID
    @State private var entityName: String = "Entity"
    @State private var isActive: Bool = true
    @State private var isStatic: Bool = false
    @State private var tag: String = "Untagged"
    @State private var layer: String = "Default"
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            HStack {
                Toggle("", isOn: $isActive)
                    .labelsHidden()
                    .toggleStyle(.checkbox)
                
                TextField("Entity Name", text: $entityName)
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.h3)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .onChange(of: entityName) { newName in
                        EngineBridge.shared.setEntityName(entityID, name: newName)
                    }
            }
            
            HStack(spacing: DesignSystem.Spacing.md) {
                // Tag
                HStack(spacing: 4) {
                    Text("Tag:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Picker("", selection: $tag) {
                        Text("Untagged").tag("Untagged")
                        Text("Player").tag("Player")
                        Text("Enemy").tag("Enemy")
                        Text("Terrain").tag("Terrain")
                    }
                    .pickerStyle(.menu)
                    .font(DesignSystem.Typography.small)
                }
                
                // Layer
                HStack(spacing: 4) {
                    Text("Layer:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Picker("", selection: $layer) {
                        Text("Default").tag("Default")
                        Text("UI").tag("UI")
                        Text("Terrain").tag("Terrain")
                        Text("Water").tag("Water")
                    }
                    .pickerStyle(.menu)
                    .font(DesignSystem.Typography.small)
                }
                
                Spacer()
                
                // Static checkbox
                Toggle("Static", isOn: $isStatic)
                    .font(DesignSystem.Typography.small)
                    .toggleStyle(.checkbox)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
        .onAppear {
            loadEntityData()
        }
    }
    
    private func loadEntityData() {
        if let data = EngineBridge.shared.getEntityData(entityID) {
            entityName = data.name
            isActive = data.isActive
            isStatic = data.isStatic
            tag = data.tag
            layer = data.layer
        }
    }
}

// MARK: - Multi-Selection Warning
private struct MultiSelectionWarning: View {
    let count: Int
    
    var body: some View {
        HStack {
            Image(systemName: "info.circle")
                .foregroundColor(.blue)
            Text("\(count) entities selected. Showing first entity's components.")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(Color.blue.opacity(0.1))
        .cornerRadius(4)
    }
}

// MARK: - Component Section
private struct ComponentSection: View {
    let component: any Component
    let onRemove: () -> Void
    let onChange: (any Component) -> Void
    
    @State private var isExpanded: Bool = true
    @State private var isHovering: Bool = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Component header
            HStack {
                Button(action: { isExpanded.toggle() }) {
                    Image(systemName: isExpanded ? "chevron.down" : "chevron.right")
                        .font(.system(size: 12))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 16)
                }
                .buttonStyle(.plain)
                
                Image(systemName: component.componentType.icon)
                    .foregroundColor(component.componentType.color)
                    .font(.system(size: 14))
                
                Text(component.componentType.rawValue)
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                // Component enabled toggle
                Toggle("", isOn: .constant(component.isEnabled))
                    .labelsHidden()
                    .toggleStyle(.checkbox)
                
                // Context menu
                if component.componentType.canBeRemoved {
                    Menu {
                        Button("Reset") { }
                        Button("Copy Component") { }
                        Divider()
                        Button("Remove Component", role: .destructive) {
                            onRemove()
                        }
                    } label: {
                        Image(systemName: "ellipsis")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .menuStyle(.borderlessButton)
                    .frame(width: 20)
                }
            }
            .padding(DesignSystem.Spacing.sm)
            .background(isHovering ? DesignSystem.Colors.hover : DesignSystem.Colors.backgroundTertiary)
            .onHover { isHovering = $0 }
            
            // Component properties
            if isExpanded {
                EditorDivider()
                
                ComponentPropertiesView(component: component, onChange: onChange)
                    .padding(DesignSystem.Spacing.sm)
            }
        }
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
    }
}

// MARK: - Component Properties View
private struct ComponentPropertiesView: View {
    let component: any Component
    let onChange: (any Component) -> Void
    
    var body: some View {
        Group {
            if let transform = component as? TransformComponent {
                TransformComponentEditor(component: transform, onChange: { onChange($0) })
            } else if let meshRenderer = component as? MeshRendererComponent {
                MeshRendererComponentEditor(component: meshRenderer, onChange: { onChange($0) })
            } else if let light = component as? LightComponent {
                LightComponentEditor(component: light, onChange: { onChange($0) })
            } else if let rigidbody = component as? RigidbodyComponent {
                RigidbodyComponentEditor(component: rigidbody, onChange: { onChange($0) })
            } else if let collider = component as? ColliderComponent {
                ColliderComponentEditor(component: collider, onChange: { onChange($0) })
            } else {
                Text("No editor available for this component type")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textDisabled)
            }
        }
    }
}

// MARK: - Transform Component Editor
private struct TransformComponentEditor: View {
    @State var component: TransformComponent
    let onChange: (TransformComponent) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            Vector3PropertyEditor(label: "Position", value: $component.position)
                .onChange(of: component.position) { _ in onChange(component) }
            
            Vector3PropertyEditor(label: "Rotation", value: $component.rotation)
                .onChange(of: component.rotation) { _ in onChange(component) }
            
            Vector3PropertyEditor(label: "Scale", value: $component.scale)
                .onChange(of: component.scale) { _ in onChange(component) }
        }
    }
}

// MARK: - Mesh Renderer Component Editor
private struct MeshRendererComponentEditor: View {
    @State var component: MeshRendererComponent
    let onChange: (MeshRendererComponent) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            AssetReferenceEditor(label: "Mesh", assetPath: $component.meshPath, assetType: .mesh)
                .onChange(of: component.meshPath) { _ in onChange(component) }
            
            AssetReferenceEditor(label: "Material", assetPath: $component.materialPath, assetType: .material)
                .onChange(of: component.materialPath) { _ in onChange(component) }
            
            BoolPropertyEditor(label: "Cast Shadows", value: $component.castShadows)
                .onChange(of: component.castShadows) { _ in onChange(component) }
            
            BoolPropertyEditor(label: "Receive Shadows", value: $component.receiveShadows)
                .onChange(of: component.receiveShadows) { _ in onChange(component) }
        }
    }
}

// MARK: - Light Component Editor
private struct LightComponentEditor: View {
    @State var component: LightComponent
    let onChange: (LightComponent) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            EnumPropertyEditor(label: "Type", value: $component.lightType)
                .onChange(of: component.lightType) { _ in onChange(component) }
            
            ColorPropertyEditor(label: "Color", value: $component.color)
                .onChange(of: component.color) { _ in onChange(component) }
            
            FloatPropertyEditor(label: "Intensity", value: $component.intensity, range: 0...10, step: 0.1)
                .onChange(of: component.intensity) { _ in onChange(component) }
            
            if component.lightType == .point || component.lightType == .spot {
                FloatPropertyEditor(label: "Range", value: $component.range, range: 0...100, step: 1)
                    .onChange(of: component.range) { _ in onChange(component) }
            }
            
            if component.lightType == .spot {
                FloatPropertyEditor(label: "Spot Angle", value: $component.spotAngle, range: 1...179, step: 1)
                    .onChange(of: component.spotAngle) { _ in onChange(component) }
            }
            
            BoolPropertyEditor(label: "Cast Shadows", value: $component.castShadows)
                .onChange(of: component.castShadows) { _ in onChange(component) }
        }
    }
}

// MARK: - Rigidbody Component Editor
private struct RigidbodyComponentEditor: View {
    @State var component: RigidbodyComponent
    let onChange: (RigidbodyComponent) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            FloatPropertyEditor(label: "Mass", value: $component.mass, range: 0.01...1000, step: 0.1)
                .onChange(of: component.mass) { _ in onChange(component) }
            
            FloatPropertyEditor(label: "Drag", value: $component.drag, range: 0...10, step: 0.1)
                .onChange(of: component.drag) { _ in onChange(component) }
            
            FloatPropertyEditor(label: "Angular Drag", value: $component.angularDrag, range: 0...10, step: 0.05)
                .onChange(of: component.angularDrag) { _ in onChange(component) }
            
            BoolPropertyEditor(label: "Use Gravity", value: $component.useGravity)
                .onChange(of: component.useGravity) { _ in onChange(component) }
            
            BoolPropertyEditor(label: "Is Kinematic", value: $component.isKinematic)
                .onChange(of: component.isKinematic) { _ in onChange(component) }
        }
    }
}

// MARK: - Collider Component Editor
private struct ColliderComponentEditor: View {
    @State var component: ColliderComponent
    let onChange: (ColliderComponent) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            EnumPropertyEditor(label: "Type", value: $component.colliderType)
                .onChange(of: component.colliderType) { _ in onChange(component) }
            
            BoolPropertyEditor(label: "Is Trigger", value: $component.isTrigger)
                .onChange(of: component.isTrigger) { _ in onChange(component) }
            
            Vector3PropertyEditor(label: "Center", value: $component.center)
                .onChange(of: component.center) { _ in onChange(component) }
            
            if component.colliderType == .box {
                Vector3PropertyEditor(label: "Size", value: $component.size)
                    .onChange(of: component.size) { _ in onChange(component) }
            }
            
            if component.colliderType == .sphere || component.colliderType == .capsule {
                FloatPropertyEditor(label: "Radius", value: $component.radius, range: 0.01...100, step: 0.1)
                    .onChange(of: component.radius) { _ in onChange(component) }
            }
        }
    }
}

// MARK: - Add Component Sheet
private struct AddComponentSheet: View {
    let onAdd: (EditorComponentType) -> Void
    @Environment(\.dismiss) var dismiss
    @State private var searchText = ""
    
    var filteredTypes: [EditorComponentType] {
        if searchText.isEmpty {
            return EditorComponentType.allCases.filter { $0 != .transform }
        }
        return EditorComponentType.allCases.filter {
            $0 != .transform && $0.rawValue.localizedCaseInsensitiveContains(searchText)
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Add Component")
                    .font(DesignSystem.Typography.h2)
                Spacer()
                Button("Cancel") { dismiss() }
            }
            .padding()
            
            Divider()
            
            // Search
            EditorSearchBar(text: $searchText, placeholder: "Search components...")
                .padding()
            
            // Component list
            ScrollView {
                LazyVStack(spacing: 4) {
                    ForEach(filteredTypes, id: \.self) { type in
                        Button(action: {
                            onAdd(type)
                        }) {
                            HStack {
                                Image(systemName: type.icon)
                                    .foregroundColor(type.color)
                                    .frame(width: 24)
                                
                                Text(type.rawValue)
                                    .font(DesignSystem.Typography.body)
                                    .foregroundColor(DesignSystem.Colors.textPrimary)
                                
                                Spacer()
                            }
                            .padding(DesignSystem.Spacing.sm)
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(4)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding()
            }
        }
        .frame(width: 400, height: 500)
    }
}

// MARK: - No Selection View
private struct NoSelectionView: View {
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            Image(systemName: "cube.transparent")
                .font(.system(size: 48))
                .foregroundColor(DesignSystem.Colors.textDisabled)
            
            Text("No entity selected")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Text("Select an entity in the scene or hierarchy to inspect its properties")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textDisabled)
                .multilineTextAlignment(.center)
                .padding(.horizontal, 40)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Inspector View Model
class EnhancedInspectorViewModel: ObservableObject {
    @Published var components: [any Component] = []
    private var currentEntityID: UUID?
    
    func loadComponents(for entityID: UUID) {
        currentEntityID = entityID
        
        // Load components from EngineBridge
        if let componentData = EngineBridge.shared.getComponents(for: entityID) {
            components = componentData.compactMap { data in
                switch data.type {
                case "Transform":
                    return TransformComponent.fromEngineBridge(data)
                case "Mesh Renderer":
                    return MeshRendererComponent.fromEngineBridge(data)
                case "Light":
                    return LightComponent.fromEngineBridge(data)
                case "Rigidbody":
                    return RigidbodyComponent.fromEngineBridge(data)
                case "Collider":
                    return ColliderComponent.fromEngineBridge(data)
                default:
                    return nil
                }
            }
        } else {
            // Default: just transform
            components = [TransformComponent()]
        }
    }
    
    func updateComponent(_ component: any Component) {
        if let index = components.firstIndex(where: { $0.id == component.id }) {
            components[index] = component
            
            // Sync to EngineBridge
            if let entityID = currentEntityID {
                EngineBridge.shared.updateComponent(entityID, component: component.toEngineBridge())
            }
        }
    }
    
    func addComponent(type: EditorComponentType, to entityID: UUID) {
        let newComponent: any Component
        
        switch type {
        case .meshRenderer:
            newComponent = MeshRendererComponent()
        case .light:
            newComponent = LightComponent()
        case .rigidbody:
            newComponent = RigidbodyComponent()
        case .collider:
            newComponent = ColliderComponent()
        default:
            return
        }
        
        components.append(newComponent)
        EngineBridge.shared.addComponent(entityID, component: newComponent.toEngineBridge())
        
        NotificationManager.shared.notify("Added \(type.rawValue) component", type: .success)
    }
    
    func removeComponent(_ componentID: UUID) {
        if let index = components.firstIndex(where: { $0.id == componentID }),
           let entityID = currentEntityID {
            let component = components[index]
            components.remove(at: index)
            
            EngineBridge.shared.removeComponent(entityID, componentID: componentID)
            NotificationManager.shared.notify("Removed \(component.componentType.rawValue) component", type: .warning)
        }
    }
}
