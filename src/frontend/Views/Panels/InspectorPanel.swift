import SwiftUI

// MARK: - Inspector Panel

struct InspectorPanel: View {
    @ObservedObject var selectionManager: SelectionManager
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Inspector")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                if !selectionManager.selectedEntities.isEmpty {
                    Text("\(selectionManager.selectedEntities.count) selected")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            Divider()
            
            // Content
            ScrollView {
                if selectionManager.selectedEntities.isEmpty {
                    EmptyInspectorView()
                } else if selectionManager.selectedEntities.count == 1 {
                    let entityID = selectionManager.selectedEntities.first!
                    SingleEntityInspector(entityID: entityID)
                } else {
                    MultiEntityInspector(entityIDs: Array(selectionManager.selectedEntities))
                }
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Empty Inspector

struct EmptyInspectorView: View {
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Image(systemName: "cube.transparent")
                .font(.system(size: 48, weight: .thin))
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            Text("Nothing Selected")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Text("Select an entity from the hierarchy to view and edit its properties")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textTertiary)
                .multilineTextAlignment(.center)
                .padding(.horizontal, DesignSystem.Spacing.lg)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .padding(.top, 100)
    }
}

// MARK: - Single Entity Inspector

struct SingleEntityInspector: View {
    let entityID: UUID
    @State private var entityName: String = ""
    @State private var components: [ComponentBridgeData] = []
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.lg) {
            // Entity Header
            EntityHeaderSection(entityID: entityID, name: $entityName)
            
            Divider()
            
            // Transform Component
            if let transform = EngineBridge.shared.entities[entityID]?.transform {
                TransformInspectorSection(entityID: entityID, transform: transform)
            }
            
            Divider()
            
            // Mesh Renderer Component
            if let meshRenderer = EngineBridge.shared.entities[entityID]?.meshRenderer {
                MeshRendererInspectorSection(entityID: entityID, meshRenderer: meshRenderer)
            }
            
            Divider()
            
            // Physics Component
            if let physics = EngineBridge.shared.entities[entityID]?.physics {
                PhysicsInspectorSection(entityID: entityID, physics: physics)
            }
            
            Divider()
            
            // Add Component Button
            AddComponentSection(entityID: entityID)
            
            Spacer()
        }
        .padding(DesignSystem.Spacing.md)
        .onAppear {
            loadEntityData()
        }
        .onChange(of: entityID) { _ in
            loadEntityData()
        }
    }
    
    private func loadEntityData() {
        if let entity = EngineBridge.shared.entities[entityID] {
            entityName = entity.name
        }
        
        components = EngineBridge.shared.getComponents(for: entityID) ?? []
    }
}

// MARK: - Multi Entity Inspector

struct MultiEntityInspector: View {
    let entityIDs: [UUID]
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.lg) {
            Text("Multiple Selection")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Text("\(entityIDs.count) entities selected")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Divider()
            
            // Common components section would go here
            Text("Common properties editing coming soon...")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            Spacer()
        }
        .padding(DesignSystem.Spacing.md)
    }
}

// MARK: - Entity Header Section

struct EntityHeaderSection: View {
    let entityID: UUID
    @Binding var name: String
    @State private var isRenaming = false
    @State private var tempName = ""
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            HStack {
                if isRenaming {
                    TextField("Entity Name", text: $tempName)
                        .textFieldStyle(.roundedBorder)
                        .font(DesignSystem.Typography.body)
                        .onSubmit {
                            EngineBridge.shared.setEntityName(entityID, name: tempName)
                            name = tempName
                            isRenaming = false
                        }
                        .onEscape {
                            tempName = name
                            isRenaming = false
                        }
                } else {
                    Text(name)
                        .font(DesignSystem.Typography.headline)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .onTapGesture {
                            tempName = name
                            isRenaming = true
                        }
                }
                
                Spacer()
                
                Button(action: {
                    EngineBridge.shared.deleteEntity(entityID)
                }) {
                    Image(systemName: "trash")
                        .font(.system(size: 14))
                        .foregroundColor(DesignSystem.Colors.accentError)
                }
                .buttonStyle(.plain)
            }
            
            Text("ID: \(entityID.uuidString.prefix(8))...")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textTertiary)
        }
        .onAppear {
            tempName = name
        }
    }
}

// MARK: - Transform Inspector Section

struct TransformInspectorSection: View {
    let entityID: UUID
    @State private var transform: TransformData
    
    init(entityID: UUID, transform: TransformData) {
        self.entityID = entityID
        self._transform = State(initialValue: transform)
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            SectionHeader(title: "Transform", icon: "arrow.up.and.down.and.arrow.left.and.right")
            
            VStack(spacing: DesignSystem.Spacing.sm) {
                Vector3Field(
                    label: "Position",
                    value: $transform.position,
                    onChange: { EngineBridge.shared.setTransform(entityID, transform: transform) }
                )
                
                Vector3Field(
                    label: "Rotation",
                    value: $transform.rotation,
                    onChange: { EngineBridge.shared.setTransform(entityID, transform: transform) }
                )
                
                Vector3Field(
                    label: "Scale",
                    value: $transform.scale,
                    onChange: { EngineBridge.shared.setTransform(entityID, transform: transform) }
                )
            }
        }
    }
}

// MARK: - Mesh Renderer Inspector Section

struct MeshRendererInspectorSection: View {
    let entityID: UUID
    @State private var meshRenderer: MeshRendererData
    
    init(entityID: UUID, meshRenderer: MeshRendererData) {
        self.entityID = entityID
        self._meshRenderer = State(initialValue: meshRenderer)
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            SectionHeader(title: "Mesh Renderer", icon: "cube.fill")
            
            VStack(spacing: DesignSystem.Spacing.sm) {
                ToggleField(
                    label: "Cast Shadows",
                    value: $meshRenderer.castShadows,
                    onChange: { EngineBridge.shared.setMeshRenderer(entityID, data: meshRenderer) }
                )
                
                ToggleField(
                    label: "Receive Shadows",
                    value: $meshRenderer.receiveShadows,
                    onChange: { EngineBridge.shared.setMeshRenderer(entityID, data: meshRenderer) }
                )
            }
        }
    }
}

// MARK: - Physics Inspector Section

struct PhysicsInspectorSection: View {
    let entityID: UUID
    @State private var physics: PhysicsData
    
    init(entityID: UUID, physics: PhysicsData) {
        self.entityID = entityID
        self._physics = State(initialValue: physics)
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            SectionHeader(title: "Physics", icon: "atom")
            
            VStack(spacing: DesignSystem.Spacing.sm) {
                FloatField(
                    label: "Mass",
                    value: $physics.mass,
                    onChange: { EngineBridge.shared.setPhysics(entityID, data: physics) }
                )
                
                ToggleField(
                    label: "Use Gravity",
                    value: $physics.useGravity,
                    onChange: { EngineBridge.shared.setPhysics(entityID, data: physics) }
                )
                
                ToggleField(
                    label: "Is Kinematic",
                    value: $physics.isKinematic,
                    onChange: { EngineBridge.shared.setPhysics(entityID, data: physics) }
                )
                
                FloatField(
                    label: "Drag",
                    value: $physics.drag,
                    onChange: { EngineBridge.shared.setPhysics(entityID, data: physics) }
                )
                
                FloatField(
                    label: "Angular Drag",
                    value: $physics.angularDrag,
                    onChange: { EngineBridge.shared.setPhysics(entityID, data: physics) }
                )
            }
        }
    }
}

// MARK: - Add Component Section

struct AddComponentSection: View {
    let entityID: UUID
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            Menu {
                Button("Add Mesh Renderer") {
                    let data = MeshRendererData(
                        material: "Default",
                        castShadows: true,
                        receiveShadows: true
                    )
                    EngineBridge.shared.setMeshRenderer(entityID, data: data)
                }
                
                Button("Add Physics") {
                    let data = PhysicsData()
                    EngineBridge.shared.setPhysics(entityID, data: data)
                }
                
                Divider()
                
                Button("Add Camera") {
                    // Camera component
                }
                
                Button("Add Light") {
                    // Light component
                }
                
                Button("Add Audio Source") {
                    // Audio source component
                }
                
                Button("Add Script") {
                    // Script component
                }
            } label: {
                HStack {
                    Image(systemName: "plus.circle")
                    Text("Add Component")
                }
                .font(DesignSystem.Typography.callout)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
        }
    }
}

// MARK: - Helper Views

struct SectionHeader: View {
    let title: String
    let icon: String
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            Image(systemName: icon)
                .font(.system(size: 14, weight: .medium))
                .foregroundColor(DesignSystem.Colors.accentPrimary)
            
            Text(title)
                .font(DesignSystem.Typography.headline)
                .foregroundColor(DesignSystem.Colors.textPrimary)
        }
    }
}

struct Vector3Field: View {
    let label: String
    @Binding var value: SIMD3<Float>
    let onChange: () -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
            Text(label)
                .font(DesignSystem.Typography.callout)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack(spacing: DesignSystem.Spacing.xs) {
                FloatField("", value: $value.x, onChange: onChange)
                FloatField("", value: $value.y, onChange: onChange)
                FloatField("", value: $value.z, onChange: onChange)
            }
        }
    }
}

struct FloatField: View {
    let label: String
    @Binding var value: Float
    let onChange: () -> Void
    
    init(_ label: String, value: Binding<Float>, onChange: @escaping () -> Void) {
        self.label = label
        self._value = value
        self.onChange = onChange
    }
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            if !label.isEmpty {
                Text(label)
                    .font(DesignSystem.Typography.callout)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .frame(width: 80, alignment: .leading)
            }
            
            TextField("", value: $value, format: .number.precision(.fractionLength(2)))
                .textFieldStyle(.roundedBorder)
                .font(DesignSystem.Typography.callout)
                .onChange(of: value) { _ in
                    onChange()
                }
        }
    }
}

struct ToggleField: View {
    let label: String
    @Binding var value: Bool
    let onChange: () -> Void
    
    var body: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.callout)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            Toggle("", isOn: $value)
                .labelsHidden()
                .onChange(of: value) { _ in
                    onChange()
                }
        }
    }
}
