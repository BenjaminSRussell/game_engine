import SwiftUI

// MARK: - Missing UI Components for NodeGraphEditor

// MARK: - Comment Node View

struct CommentNodeView: View {
    @Binding var node: GraphNode
    let isSelected: Bool
    let offset: CGPoint
    let zoom: CGFloat
    
    @State private var isEditing = false
    @State private var editText = ""
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header
            HStack {
                Image(systemName: "text.bubble.fill")
                    .foregroundColor(.yellow.opacity(0.8))
                    .font(.system(size: 12))
                
                if isEditing {
                    TextField("Comment title", text: $editText)
                        .textFieldStyle(PlainTextFieldStyle())
                        .font(.caption)
                        .onSubmit {
                            node.title = editText
                            isEditing = false
                        }
                } else {
                    Text(node.title.isEmpty ? "Comment" : node.title)
                        .font(.caption)
                        .fontWeight(.medium)
                        .onTapGesture {
                            editText = node.title
                            isEditing = true
                        }
                }
                
                Spacer()
            }
            
            // Content
            Text(node.comment.isEmpty ? "Click to add comment..." : node.comment)
                .font(.caption)
                .foregroundColor(.white.opacity(0.8))
                .frame(maxWidth: .infinity, alignment: .leading)
                .onTapGesture {
                    // Could trigger inline editing for comment content
                }
        }
        .padding(12)
        .background(Color.yellow.opacity(0.15))
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(isSelected ? Color.yellow : Color.clear, lineWidth: 2)
        )
        .position(
            x: node.position.x * zoom + offset.x,
            y: node.position.y * zoom + offset.y
        )
        .gesture(
            DragGesture()
                .onChanged { value in
                    node.position = CGPoint(
                        x: (value.location.x - offset.x) / zoom,
                        y: (value.location.y - offset.y) / zoom
                    )
                }
        )
    }
}

// MARK: - Enhanced Port View

struct EnhancedPortView: View {
    let port: NodePort
    let isOutput: Bool
    let nodeId: UUID
    @State private var isHovering = false
    @State private var showDefaultValueEditor = false
    @State private var showTooltip = false
    @AppStorage("showPortNames") private var showPortNames = true
    
    var body: some View {
        HStack(spacing: 4) {
            if !isOutput {
                EnhancedPortConnector(
                    type: port.type, 
                    isHovering: isHovering,
                    hasDefaultValue: port.defaultValue != nil,
                    currentValue: port.currentValue
                )
                
                // Default value indicator
                if !isOutput && port.defaultValue != nil {
                    Button(action: { showDefaultValueEditor = true }) {
                        Image(systemName: "slider.horizontal.3")
                            .font(.system(size: 8))
                            .foregroundColor(.white.opacity(0.6))
                    }
                    .buttonStyle(.plain)
                    .popover(isPresented: $showDefaultValueEditor) {
                        EnhancedPortDefaultValueEditor(port: port)
                    }
                }
            }
            
            if showPortNames {
                VStack(alignment: isOutput ? .trailing : .leading, spacing: 2) {
                    Text(port.name)
                        .font(.caption2)
                        .foregroundColor(.white.opacity(0.8))
                    
                    if let currentValue = port.currentValue {
                        Text(formatValue(currentValue))
                            .font(.caption2)
                            .foregroundColor(.white.opacity(0.5))
                    }
                }
            }
            
            if isOutput {
                EnhancedPortConnector(
                    type: port.type, 
                    isHovering: isHovering,
                    hasDefaultValue: port.defaultValue != nil,
                    currentValue: port.currentValue
                )
            }
        }
        .onHover { isHovering = $0 }
        .onHover { hovering in
            showTooltip = hovering
        }
        .popover(isPresented: $showTooltip, arrowEdge: isOutput ? .trailing : .leading) {
            EnhancedPortTooltip(port: port, nodeId: nodeId)
        }
        .contextMenu {
            Button(action: {
                UserDefaults.standard.set(!showPortNames, forKey: "showPortNames")
            }) {
                Label(showPortNames ? "Hide Port Names" : "Show Port Names", 
                      systemImage: showPortNames ? "eye.slash" : "eye")
            }
            
            if !isOutput && port.defaultValue != nil {
                Button(action: { showDefaultValueEditor = true }) {
                    Label("Edit Default Value", systemImage: "slider.horizontal.3")
                }
            }
            
            if port.hasError {
                Button("Show Error") {
                    // Show error details
                }
            }
        }
    }
    
    private func formatValue(_ value: Any) -> String {
        if let stringValue = value as? String {
            return "\"\(stringValue.prefix(10))\(stringValue.count > 10 ? "..." : "")\""
        } else if let numericValue = value as? NSNumber {
            return String(format: "%.2f", numericValue.doubleValue)
        } else if let boolValue = value as? Bool {
            return boolValue.description
        } else {
            return String(describing: value).prefix(15).description
        }
    }
}

// MARK: - Enhanced Port Connector

struct EnhancedPortConnector: View {
    let type: PortType
    let isHovering: Bool
    let hasDefaultValue: Bool
    let currentValue: Any?
    
    var body: some View {
        Group {
            switch type.shape {
            case .triangle:
                // Flow ports are triangular
                Image(systemName: "arrowtriangle.right.fill")
                    .font(.system(size: 10))
            case .dashedCircle:
                // Array ports have dashed outline
                Circle()
                    .frame(width: 10, height: 10)
                    .overlay(
                        Circle()
                            .stroke(type.color, style: StrokeStyle(lineWidth: 1, dash: [2, 2]))
                    )
                    .background(Circle().fill(Color.black.opacity(0.3)))
            case .square:
                Rectangle()
                    .frame(width: 10, height: 10)
            case .diamond:
                Diamond()
                    .frame(width: 10, height: 10)
            case .circle:
                Circle()
                    .frame(width: 10, height: 10)
            }
        }
        .foregroundColor(type.shape == .dashedCircle ? .clear : type.color)
        .scaleEffect(isHovering ? 1.3 : 1.0)
        .overlay(
            // Value indicator
            currentValue != nil ? 
                Circle()
                    .frame(width: 4, height: 4)
                    .foregroundColor(.green)
                    .offset(x: isOutput ? 6 : -6) :
                nil
        )
        .overlay(
            // Default value indicator
            hasDefaultValue ? 
                Circle()
                    .frame(width: 3, height: 3)
                    .foregroundColor(.orange)
                    .offset(x: isOutput ? -6 : 6) :
                nil
        )
    }
}

// MARK: - Enhanced Port Tooltip

struct EnhancedPortTooltip: View {
    let port: NodePort
    let nodeId: UUID
    
    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            // Header
            HStack {
                Image(systemName: port.type.shape == .triangle ? "arrowtriangle.right.fill" : "circle.fill")
                    .foregroundColor(port.type.color)
                    .font(.caption)
                
                Text(port.name)
                    .font(.caption)
                    .fontWeight(.bold)
                
                Spacer()
            }
            
            Divider()
                .background(.white.opacity(0.2))
            
            // Type information
            VStack(alignment: .leading, spacing: 4) {
                HStack {
                    Text("Type:")
                        .font(.caption2)
                        .foregroundColor(.secondary)
                    Text(port.type.rawValue)
                        .font(.caption2)
                        .foregroundColor(.primary)
                }
                
                Text(port.type.description)
                    .font(.caption2)
                    .foregroundColor(.secondary)
                
                if port.type.isNullable {
                    HStack {
                        Image(systemName: "questionmark.circle")
                            .font(.caption2)
                        Text("Nullable type")
                            .font(.caption2)
                            .foregroundColor(.blue)
                    }
                }
                
                if port.type.isGeneric {
                    HStack {
                        Image(systemName: "gear")
                            .font(.caption2)
                        Text("Generic type")
                            .font(.caption2)
                            .foregroundColor(.purple)
                    }
                }
            }
            
            // Current value
            if let currentValue = port.currentValue {
                Divider()
                    .background(.white.opacity(0.2))
                
                VStack(alignment: .leading, spacing: 2) {
                    Text("Current Value:")
                        .font(.caption2)
                        .foregroundColor(.secondary)
                    
                    Text(formatValue(currentValue))
                        .font(.caption)
                        .foregroundColor(.green)
                        .padding(.horizontal, 4)
                        .padding(.vertical, 2)
                        .background(Color.green.opacity(0.1))
                        .cornerRadius(4)
                }
            }
            
            // Default value
            if let defaultValue = port.defaultValue {
                Divider()
                    .background(.white.opacity(0.2))
                
                VStack(alignment: .leading, spacing: 2) {
                    Text("Default Value:")
                        .font(.caption2)
                        .foregroundColor(.secondary)
                    
                    Text(formatValue(defaultValue))
                        .font(.caption)
                        .foregroundColor(.orange)
                        .padding(.horizontal, 4)
                        .padding(.vertical, 2)
                        .background(Color.orange.opacity(0.1))
                        .cornerRadius(4)
                }
            }
            
            // Error message
            if port.hasError, let errorMessage = port.errorMessage {
                Divider()
                    .background(.white.opacity(0.2))
                
                HStack {
                    Image(systemName: "exclamationmark.triangle.fill")
                        .foregroundColor(.red)
                        .font(.caption2)
                    Text(errorMessage)
                        .font(.caption2)
                        .foregroundColor(.red)
                }
            }
            
            // Node ID for debugging
            if DebugManager.shared.isDebugging {
                Divider()
                    .background(.white.opacity(0.2))
                
                Text("Node ID: \(nodeId.uuidString.prefix(8))")
                    .font(.caption2)
                    .foregroundColor(.gray)
            }
        }
        .padding(8)
        .background(Color(.controlBackgroundColor))
        .cornerRadius(8)
        .frame(maxWidth: 250)
    }
    
    private func formatValue(_ value: Any) -> String {
        if let stringValue = value as? String {
            return "\"\(stringValue)\""
        } else if let numericValue = value as? NSNumber {
            return numericValue.stringValue
        } else if let boolValue = value as? Bool {
            return boolValue.description
        } else if let vectorValue = value as? [Double] {
            return "(\(vectorValue.map { String(format: "%.2f", $0) }.joined(separator: ", ")))"
        } else {
            return String(describing: value)
        }
    }
}

// MARK: - Enhanced Port Default Value Editor

struct EnhancedPortDefaultValueEditor: View {
    let port: NodePort
    @State private var defaultValue: String = ""
    @State private var selectedEnumOption: String = ""
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            // Header
            HStack {
                Text("Default Value")
                    .font(.headline)
                
                Spacer()
                
                Button("Clear") {
                    // Clear default value
                    dismiss()
                }
                .foregroundColor(.red)
            }
            
            Text("Port: \(port.name)")
                .font(.caption)
                .foregroundColor(.secondary)
            
            Text("Type: \(port.type.rawValue)")
                .font(.caption)
                .foregroundColor(.secondary)
            
            Divider()
            
            // Type-specific editor
            typeSpecificEditor
            
            Spacer()
            
            // Actions
            HStack {
                Button("Cancel") { dismiss() }
                Spacer()
                Button("Apply") {
                    // Apply default value
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding()
        .frame(width: 300, height: 400)
        .onAppear {
            loadCurrentValue()
        }
    }
    
    @ViewBuilder
    private var typeSpecificEditor: some View {
        switch port.type {
        case .bool:
            Toggle("Default Value", isOn: Binding(
                get: { defaultValue.lowercased() == "true" },
                set: { defaultValue in $0 ? "true" : "false" }
            ))
            
        case .int, .float:
            VStack(alignment: .leading, spacing: 8) {
                Text("Value:")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                TextField("Enter value", text: $defaultValue)
                    .textFieldStyle(.roundedBorder)
                    .keyboardType(port.type == .int ? .numberPad : .decimalPad)
                
                // Quick value buttons
                HStack {
                    ForEach([0, 1, 2, 5, 10], id: \.self) { value in
                        Button("\(value)") {
                            defaultValue = "\(value)"
                        }
                        .buttonStyle(.bordered)
                        .controlSize(.small)
                    }
                }
            }
            
        case .string:
            VStack(alignment: .leading, spacing: 8) {
                Text("Text:")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                TextField("Enter text", text: $defaultValue)
                    .textFieldStyle(.roundedBorder)
                
                TextEditor(text: $defaultValue)
                    .frame(height: 60)
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(Color.gray, lineWidth: 1)
                    )
            }
            
        case .vector2, .vector3, .vector4:
            VStack(alignment: .leading, spacing: 8) {
                Text("Components:")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                ForEach(0..<port.type.componentCount, id: \.self) { index in
                    HStack {
                        Text(["X", "Y", "Z", "W"][index])
                            .frame(width: 20, alignment: .leading)
                        
                        TextField("0", text: Binding(
                            get: { defaultValue.components(separatedBy: ",")[safe: index] ?? "0" },
                            set: { newValue in
                                var components = defaultValue.components(separatedBy: ",")
                                while components.count <= index { components.append("0") }
                                components[index] = newValue
                                defaultValue = components.joined(separator: ",")
                            }
                        ))
                        .textFieldStyle(.roundedBorder)
                        .keyboardType(.decimalPad)
                    }
                }
            }
            
        case .enumType:
            VStack(alignment: .leading, spacing: 8) {
                Text("Enum Value:")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Picker("Enum", selection: $selectedEnumOption) {
                    // This would be populated with actual enum values
                    Text("Option 1").tag("Option1")
                    Text("Option 2").tag("Option2")
                    Text("Option 3").tag("Option3")
                }
                .pickerStyle(.menu)
            }
            
        default:
            VStack(alignment: .leading, spacing: 8) {
                Text("Value:")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                TextField("Enter value", text: $defaultValue)
                    .textFieldStyle(.roundedBorder)
                
                Text("Note: This type may require special formatting")
                    .font(.caption2)
                    .foregroundColor(.orange)
            }
        }
    }
    
    private func loadCurrentValue() {
        if let currentValue = port.defaultValue as? String {
            defaultValue = currentValue
        } else {
            defaultValue = ""
        }
    }
}

// MARK: - Node Tooltip

struct NodeTooltip: View {
    let node: GraphNode
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header
            HStack {
                Image(systemName: iconForType(node.type))
                    .foregroundColor(nodeColorForType(node.type))
                
                VStack(alignment: .leading, spacing: 2) {
                    Text(node.title)
                        .font(.caption)
                        .fontWeight(.bold)
                    
                    Text(String(describing: node.type))
                        .font(.caption2)
                        .foregroundColor(.secondary)
                }
                
                Spacer()
            }
            
            Divider()
                .background(.white.opacity(0.2))
            
            // Description
            if !node.description.isEmpty {
                Text(node.description)
                    .font(.caption)
                    .lineLimit(3)
            }
            
            // Ports info
            if !node.inputs.isEmpty || !node.outputs.isEmpty {
                VStack(alignment: .leading, spacing: 4) {
                    if !node.inputs.isEmpty {
                        Text("Inputs: \(node.inputs.count)")
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                    
                    if !node.outputs.isEmpty {
                        Text("Outputs: \(node.outputs.count)")
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                }
            }
            
            // Status
            if node.hasError || node.isDisabled || node.isBypassed {
                VStack(alignment: .leading, spacing: 2) {
                    if node.hasError {
                        HStack {
                            Image(systemName: "xmark.circle.fill")
                                .foregroundColor(.red)
                                .font(.caption2)
                            Text("Has errors")
                                .font(.caption2)
                                .foregroundColor(.red)
                        }
                    }
                    
                    if node.isDisabled {
                        HStack {
                            Image(systemName: "power.slash")
                                .foregroundColor(.gray)
                                .font(.caption2)
                            Text("Disabled")
                                .font(.caption2)
                                .foregroundColor(.gray)
                        }
                    }
                    
                    if node.isBypassed {
                        HStack {
                            Image(systemName: "arrow.trianglehead.2")
                                .foregroundColor(.orange)
                                .font(.caption2)
                            Text("Bypassed")
                                .font(.caption2)
                                .foregroundColor(.orange)
                        }
                    }
                }
            }
            
            // Debug info
            if DebugManager.shared.isDebugging {
                Divider()
                    .background(.white.opacity(0.2))
                
                Text("Node ID: \(node.id.uuidString.prefix(8))")
                    .font(.caption2)
                    .foregroundColor(.gray)
            }
        }
        .padding(8)
        .background(Color(.controlBackgroundColor))
        .cornerRadius(8)
        .frame(maxWidth: 250)
    }
    
    private func iconForType(_ type: NodeType) -> String {
        switch type {
        case .event: return "bolt.fill"
        case .flowBranch: return "arrow.branch"
        case .variable: return "v.square"
        case .constant: return "number"
        case .mathAdd: return "plus.forwardslash.minus"
        case .logicAnd: return "questionmark.diamond"
        case .compareEqual: return "equal"
        case .comment: return "text.bubble"
        default: return "square"
        }
    }
    
    private func nodeColorForType(_ type: NodeType) -> Color {
        switch type {
        case .event: return .red
        case .flowBranch: return .gray
        case .variable: return .orange
        case .constant: return .orange
        case .mathAdd: return .teal
        case .logicAnd: return .teal
        case .compareEqual: return .orange
        case .comment: return .yellow.opacity(0.3)
        default: return .secondary
        }
    }
}

// MARK: - Node Type Replacement View

struct NodeTypeReplacementView: View {
    @Binding var node: GraphNode
    @Environment(\.dismiss) private var dismiss
    
    @State private var selectedType: NodeType?
    @State private var searchText = ""
    
    var body: some View {
        VStack(spacing: 16) {
            // Header
            Text("Replace Node Type")
                .font(.headline)
            
            Text("Replace '\(node.title)' with a different node type")
                .font(.caption)
                .foregroundColor(.secondary)
            
            Divider()
            
            // Search
            HStack {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(.secondary)
                
                TextField("Search node types...", text: $searchText)
                    .textFieldStyle(.roundedBorder)
            }
            
            // Node type list
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 4) {
                    ForEach(filteredNodeTypes, id: \.self) { type in
                        Button(action: {
                            selectedType = type
                        }) {
                            HStack {
                                Image(systemName: iconForType(type))
                                    .foregroundColor(colorForType(type))
                                
                                VStack(alignment: .leading, spacing: 2) {
                                    Text(String(describing: type))
                                        .font(.caption)
                                        .foregroundColor(.primary)
                                    
                                    Text(descriptionForType(type))
                                        .font(.caption2)
                                        .foregroundColor(.secondary)
                                }
                                
                                Spacer()
                                
                                if selectedType == type {
                                    Image(systemName: "checkmark.circle.fill")
                                        .foregroundColor(.blue)
                                }
                            }
                            .padding(.horizontal, 8)
                            .padding(.vertical, 6)
                            .background(selectedType == type ? Color.blue.opacity(0.2) : Color.clear)
                            .cornerRadius(6)
                        }
                        .buttonStyle(.plain)
                    }
                }
            }
            .frame(maxHeight: 300)
            
            Divider()
            
            // Actions
            HStack {
                Button("Cancel") { dismiss() }
                Spacer()
                Button("Replace") {
                    if let newType = selectedType {
                        // Replace node type
                        node.type = newType
                        node.title = String(describing: newType)
                        dismiss()
                    }
                }
                .buttonStyle(.borderedProminent)
                .disabled(selectedType == nil)
            }
        }
        .padding()
        .frame(width: 400, height: 500)
    }
    
    private var filteredNodeTypes: [NodeType] {
        if searchText.isEmpty {
            return NodeType.allCases
        }
        
        return NodeType.allCases.filter { type in
            let typeName = String(describing: type).lowercased()
            let searchLower = searchText.lowercased()
            return typeName.contains(searchLower)
        }
    }
    
    private func iconForType(_ type: NodeType) -> String {
        switch type {
        case .event: return "bolt.fill"
        case .flowBranch: return "arrow.branch"
        case .variable: return "v.square"
        case .constant: return "number"
        case .mathAdd: return "plus.forwardslash.minus"
        case .logicAnd: return "questionmark.diamond"
        case .compareEqual: return "equal"
        case .comment: return "text.bubble"
        default: return "square"
        }
    }
    
    private func colorForType(_ type: NodeType) -> Color {
        switch type {
        case .event: return .red
        case .flowBranch: return .gray
        case .variable: return .orange
        case .constant: return .orange
        case .mathAdd: return .teal
        case .logicAnd: return .teal
        case .compareEqual: return .orange
        case .comment: return .yellow.opacity(0.3)
        default: return .secondary
        }
    }
    
    private func descriptionForType(_ type: NodeType) -> String {
        switch type {
        case .event: return "Event trigger"
        case .flowBranch: return "Conditional branch"
        case .variable: return "Variable storage"
        case .constant: return "Constant value"
        case .mathAdd: return "Addition operation"
        case .logicAnd: return "Logical AND"
        case .compareEqual: return "Equality comparison"
        case .comment: return "Documentation note"
        default: return "Node operation"
        }
    }
}
