import SwiftUI
import simd

// MARK: - Vector3 Property Editor

struct Vector3PropertyEditor: View {
    let label: String
    @Binding var value: SIMD3<Float>
    
    @State private var isDragging = false
    @State private var dragStartValue = SIMD3<Float>(0, 0, 0)
    @State private var dragStartPosition = CGPoint.zero
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack(spacing: DesignSystem.Spacing.xs) {
                // X Component
                ComponentField(
                    label: "X",
                    value: Binding(
                        get: { value.x },
                        set: { value.x = $0 }
                    ),
                    color: .red
                )
                
                // Y Component
                ComponentField(
                    label: "Y",
                    value: Binding(
                        get: { value.y },
                        set: { value.y = $0 }
                    ),
                    color: .green
                )
                
                // Z Component
                ComponentField(
                    label: "Z",
                    value: Binding(
                        get: { value.z },
                        set: { value.z = $0 }
                    ),
                    color: .blue
                )
            }
        }
    }
}

private struct ComponentField: View {
    let label: String
    @Binding var value: Float
    let color: Color
    
    @State private var textValue: String = ""
    @State private var isEditing = false
    @State private var isDragging = false
    @State private var dragStartValue: Float = 0
    @State private var dragStartPosition: CGFloat = 0
    
    var body: some View {
        HStack(spacing: 2) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(color.opacity(0.8))
                .frame(width: 12)
            
            TextField("", text: $textValue, onEditingChanged: { editing in
                if editing {
                    isEditing = true
                    textValue = String(format: "%.2f", value)
                } else {
                    isEditing = false
                    if let newValue = Float(textValue) {
                        value = newValue
                    } else {
                        textValue = String(format: "%.2f", value)
                    }
                }
            })
            .textFieldStyle(.plain)
            .multilineTextAlignment(.trailing)
            .font(DesignSystem.Typography.body)
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .frame(height: 20)
            .padding(.horizontal, 4)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(3)
            .gesture(
                DragGesture(minimumDistance: 2)
                    .onChanged { gesture in
                        if !isDragging {
                            isDragging = true
                            dragStartValue = value
                            dragStartPosition = gesture.location.x
                        }
                        let delta = (gesture.location.x - dragStartPosition) * 0.1
                        value = dragStartValue + Float(delta)
                        textValue = String(format: "%.2f", value)
                    }
                    .onEnded { _ in
                        isDragging = false
                    }
            )
        }
        .onAppear {
            textValue = String(format: "%.2f", value)
        }
        .onChange(of: value) { newValue in
            if !isEditing && !isDragging {
                textValue = String(format: "%.2f", newValue)
            }
        }
    }
}

// MARK: - Color Property Editor

struct ColorPropertyEditor: View {
    let label: String
    @Binding var value: Color
    @State private var showPicker = false
    
    var body: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            Button(action: { showPicker.toggle() }) {
                RoundedRectangle(cornerRadius: 4)
                    .fill(value)
                    .frame(width: 40, height: 20)
                    .overlay(
                        RoundedRectangle(cornerRadius: 4)
                            .stroke(DesignSystem.Colors.textSecondary.opacity(0.3), lineWidth: 1)
                    )
            }
            .buttonStyle(.plain)
            .popover(isPresented: $showPicker) {
                ColorPicker("", selection: $value)
                    .labelsHidden()
                    .padding()
            }
        }
    }
}

// MARK: - Float Property Editor

struct FloatPropertyEditor: View {
    let label: String
    @Binding var value: Float
    var range: ClosedRange<Float> = 0...100
    var step: Float = 0.1
    
    @State private var textValue: String = ""
    @State private var isEditing = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                TextField("", text: $textValue, onEditingChanged: { editing in
                    if editing {
                        isEditing = true
                        textValue = String(format: "%.2f", value)
                    } else {
                        isEditing = false
                        if let newValue = Float(textValue) {
                            value = min(max(newValue, range.lowerBound), range.upperBound)
                        }
                        textValue = String(format: "%.2f", value)
                    }
                })
                .textFieldStyle(.plain)
                .multilineTextAlignment(.trailing)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .frame(width: 60)
                .padding(.horizontal, 4)
                .padding(.vertical, 2)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(3)
            }
            
            Slider(value: $value, in: range, step: step)
                .accentColor(DesignSystem.Colors.accentPrimary)
        }
        .onAppear {
            textValue = String(format: "%.2f", value)
        }
        .onChange(of: value) { newValue in
            if !isEditing {
                textValue = String(format: "%.2f", newValue)
            }
        }
    }
}

// MARK: - Bool Property Editor

struct BoolPropertyEditor: View {
    let label: String
    @Binding var value: Bool
    
    var body: some View {
        Toggle(label, isOn: $value)
            .font(DesignSystem.Typography.small)
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .toggleStyle(.switch)
    }
}

// MARK: - Enum Property Editor

struct EnumPropertyEditor<T: RawRepresentable & CaseIterable & Hashable>: View where T.RawValue == String {
    let label: String
    @Binding var value: T
    
    var body: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            Picker("", selection: $value) {
                ForEach(Array(T.allCases), id: \.self) { item in
                    Text(item.rawValue).tag(item)
                }
            }
            .pickerStyle(.menu)
            .font(DesignSystem.Typography.body)
        }
    }
}

// MARK: - Asset Reference Editor

struct AssetReferenceEditor: View {
    let label: String
    @Binding var assetPath: String
    let assetType: AssetType
    
    @State private var isHovering = false
    @State private var isDragging = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack(spacing: 8) {
                // Asset Icon
                Image(systemName: assetType.icon)
                    .foregroundColor(assetType.color)
                    .frame(width: 24, height: 24)
                
                // Asset Name
                Text(assetPath.isEmpty ? "None" : assetPath.components(separatedBy: "/").last ?? "")
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(assetPath.isEmpty ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textPrimary)
                    .lineLimit(1)
                
                Spacer()
                
                // Clear Button
                if !assetPath.isEmpty {
                    Button(action: { assetPath = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(8)
            .background(isDragging ? DesignSystem.Colors.accentPrimary.opacity(0.2) : DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(4)
            .overlay(
                RoundedRectangle(cornerRadius: 4)
                    .stroke(
                        isDragging ? DesignSystem.Colors.accentPrimary : (isHovering ? DesignSystem.Colors.textSecondary.opacity(0.3) : Color.clear),
                        lineWidth: isDragging ? 2 : 1
                    )
            )
            .onHover { isHovering = $0 }
            .onDrop(of: [.fileURL], isTargeted: $isDragging) { providers in
                // Handle drop from ContentBrowser
                // TODO: Implement asset drag-drop
                return true
            }
        }
    }
}

// MARK: - String Property Editor

struct StringPropertyEditor: View {
    let label: String
    @Binding var value: String
    var placeholder: String = ""
    var multiline: Bool = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            if multiline {
                TextEditor(text: $value)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .frame(minHeight: 60, maxHeight: 120)
                    .padding(4)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
            } else {
                TextField(placeholder, text: $value)
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 6)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
            }
        }
    }
}

// MARK: - Int Property Editor

struct IntPropertyEditor: View {
    let label: String
    @Binding var value: Int
    var range: ClosedRange<Int> = 0...100
    
    @State private var textValue: String = ""
    @State private var isEditing = false
    
    var body: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            TextField("", text: $textValue, onEditingChanged: { editing in
                if editing {
                    isEditing = true
                    textValue = String(value)
                } else {
                    isEditing = false
                    if let newValue = Int(textValue) {
                        value = min(max(newValue, range.lowerBound), range.upperBound)
                    }
                    textValue = String(value)
                }
            })
            .textFieldStyle(.plain)
            .multilineTextAlignment(.trailing)
            .font(DesignSystem.Typography.body)
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .frame(width: 60)
            .padding(.horizontal, 4)
            .padding(.vertical, 2)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(3)
        }
        .onAppear {
            textValue = String(value)
        }
        .onChange(of: value) { newValue in
            if !isEditing {
                textValue = String(newValue)
            }
        }
    }
}
