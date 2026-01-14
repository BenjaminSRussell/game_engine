import SwiftUI
import CEngineCore

/// Property Inspector with dynamic binding to C component data
@available(macOS 14.0, *)
public struct PropertyInspector: View {
    @Binding var selectedEntity: SwiftEntity?
    let world: UnsafeMutablePointer<World>?
    
    @State private var properties: [PropertyItem] = []
    
    public init(selectedEntity: Binding<SwiftEntity?>, world: UnsafeMutablePointer<World>?) {
        self._selectedEntity = selectedEntity
        self.world = world
    }
    
    public var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Header
            HStack {
                Text("Inspector")
                    .font(.headline)
                Spacer()
                if selectedEntity != nil {
                    Button(action: { selectedEntity = nil }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(.secondary)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding()
            
            Divider()
            
            if let entity = selectedEntity {
                ScrollView {
                    VStack(alignment: .leading, spacing: 16) {
                        // Entity info
                        Section {
                            InspectorPropertyRow(label: "Entity ID", value: "\(entity.id)")
                            InspectorPropertyRow(label: "Generation", value: "\(entity.generation)")
                        }
                        
                        Divider()
                        
                        // Components
                        Text("Components")
                            .font(.caption)
                            .foregroundColor(.secondary)
                            .padding(.horizontal)
                        
                        ForEach(properties) { property in
                            ComponentPropertyView(property: property)
                        }
                        
                       if properties.isEmpty {
                            HStack {
                                Spacer()
                                VStack(spacing: 8) {
                                    Image(systemName: "cube")
                                        .font(.system(size: 32))
                                        .foregroundColor(.secondary)
                                    Text("No components")
                                        .font(.caption)
                                        .foregroundColor(.secondary)
                                }
                                Spacer()
                            }
                            .padding(.vertical, 32)
                        }
                    }
                    .padding()
                }
            } else {
                // Empty state
                VStack(spacing: 12) {
                    Image(systemName: "hand.tap")
                        .font(.system(size: 48))
                        .foregroundColor(.secondary)
                    Text("Select an entity")
                        .font(.headline)
                        .foregroundColor(.secondary)
                    Text("Click an entity in the viewport to inspect its properties")
                        .font(.caption)
                        .foregroundColor(.secondary)
                        .multilineTextAlignment(.center)
                        .frame(maxWidth: 200)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            }
        }
        .onChange(of: selectedEntity) { _ in
            loadProperties()
        }
        .onAppear {
            loadProperties()
        }
    }
    
    private func loadProperties() {
        guard let entity = selectedEntity, let world = world else {
            properties = []
            return
        }
        
        // Live data fetching from ECS
        var loadedProperties: [PropertyItem] = []
        
        // 1. Position (Component ID 1 - Assuming Transform/Position)
        if let posPtr = ecs_bridge_get_component_data(world, entity.toCEntity(), 1 /* POSITION */) {
            let vec = posPtr.assumingMemoryBound(to: SIMD3<Float>.self).pointee
            loadedProperties.append(PropertyItem(name: "Position", type: .vector3, value: String(format: "%.2f, %.2f, %.2f", vec.x, vec.y, vec.z)))
        }
        
        // 2. Velocity (Component ID 2 - Assuming Physics/Velocity)
        if let velPtr = ecs_bridge_get_component_data(world, entity.toCEntity(), 2 /* VELOCITY */) {
            let vec = velPtr.assumingMemoryBound(to: SIMD3<Float>.self).pointee
            loadedProperties.append(PropertyItem(name: "Velocity", type: .vector3, value: String(format: "%.2f, %.2f, %.2f", vec.x, vec.y, vec.z)))
        }
        
        // 3. Render State (Component ID 3) - Example Bool checking
        if let _ = ecs_bridge_get_component_data(world, entity.toCEntity(), 3 /* RENDER */) {
            loadedProperties.append(PropertyItem(name: "Renderable", type: .bool, value: "true"))
        }
        
        // 4. GOAP State (Bridge call)
        let goapState = ecs_bridge_get_goap_state(world, entity.toCEntity())
        if let goal = goapState.current_goal {
            loadedProperties.append(PropertyItem(name: "Current Goal", type: .string, value: String(cString: goal)))
        }
        loadedProperties.append(PropertyItem(name: "Planning Time", type: .float, value: String(format: "%.2f ms", goapState.planning_time_ms)))

        // Fallback for demo if no live data found
        if loadedProperties.isEmpty {
             loadedProperties = [
                PropertyItem(name: "Entity Info", type: .string, value: "ID: \(entity.id)")
            ]
        }
        
        properties = loadedProperties
    }
}

// MARK: - Inspector Property Row

struct InspectorPropertyRow: View {
    let label: String
    let value: String
    
    var body: some View {
        HStack {
            Text(label)
                .font(.caption)
                .foregroundColor(.secondary)
            Spacer()
            Text(value)
                .font(.system(.caption, design: .monospaced))
        }
        .padding(.horizontal)
    }
}

// MARK: - Component Property View

struct ComponentPropertyView: View {
    let property: PropertyItem
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(property.name)
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                propertyValueView
                Spacer()
            }
        }
        .padding(.horizontal)
        .padding(.vertical, 4)
    }
    
    @ViewBuilder
    private var propertyValueView: some View {
        switch property.type {
        case .float, .int:
            Text(property.value)
                .font(.system(.body, design: .monospaced))
        case .vector3:
            HStack(spacing: 8) {
                ForEach(property.value.split(separator: ","), id: \.self) { component in
                    Text(component.trimmingCharacters(in: .whitespaces))
                        .font(.system(.caption, design: .monospaced))
                        .padding(.horizontal, 6)
                        .padding(.vertical, 2)
                        .background(Color.primary.opacity(0.1))
                        .cornerRadius(4)
                }
            }
        case .bool:
            Toggle("", isOn: .constant(property.value == "true"))
                .labelsHidden()
                .disabled(true)
        case .string:
            Text("\"\(property.value)\"")
                .font(.system(.body, design: .monospaced))
                .foregroundColor(.green)
        }
    }
}

// MARK: - Data Models

public struct PropertyItem: Identifiable {
    public let id = UUID()
    public let name: String
    public let type: PropertyType
    public let value: String
    
    public enum PropertyType {
        case float
        case int
        case vector3
        case bool
        case string
    }
}

// MARK: - Preview

struct PropertyInspector_Previews: PreviewProvider {
    static var previews: some View {
        GlassPlate {
            PropertyInspector(
                selectedEntity: .constant(SwiftEntity(from: Entity(id: 42, generation: 1))),
                world: nil
            )
        }
        .frame(width: 300, height: 600)
    }
}
