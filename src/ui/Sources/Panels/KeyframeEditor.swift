import SwiftUI

// MARK: - Keyframe Editor View
struct KeyframeEditorView: View {
    @Binding var keyframe: AnimationKeyframe
    let manager: AnimationEditorManager
    @Environment(\.dismiss) private var dismiss
    
    @State private var tempValue: String = ""
    @State private var tempInTangent: Float = 0
    @State private var tempOutTangent: Float = 0
    @State private var tempInterpolationType: AnimationKeyframe.InterpolationType = .linear
    @State private var tempPropertyType: AnimationKeyframe.PropertyType = .float
    @State private var tempEaseIn: Float = 0
    @State private var tempEaseOut: Float = 0
    @State private var tempIsBreakpoint: Bool = false
    @State private var tempTangentsUnified: Bool = true
    @State private var tempFreeTangents: Bool = false
    @State private var tempWeightedTangents: Bool = false
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            // Header
            HStack {
                Text("Keyframe Editor")
                    .font(DesignSystem.Typography.headline)
                Spacer()
                Button("Close") { dismiss() }
                    .buttonStyle(.bordered)
            }
            
            EditorDivider()
            
            // Value section
            GroupBox("Value") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("Property Type:")
                        Picker("Property Type", selection: $tempPropertyType) {
                            ForEach(AnimationKeyframe.PropertyType.allCases, id: \.self) { type in
                                Text(type.rawValue).tag(type)
                            }
                        }
                        .pickerStyle(.menu)
                    }
                    
                    HStack {
                        Text("Value:")
                        if tempPropertyType == .float || tempPropertyType == .integer {
                            TextField("Value", text: $tempValue)
                                .textFieldStyle(.roundedBorder)
                        } else if tempPropertyType == .bool {
                            Toggle("True/False", isOn: Binding(
                                get: { tempValue.lowercased() == "true" },
                                set: { tempValue = $0 ? "true" : "false" }
                            ))
                        } else {
                            TextField("Value", text: $tempValue)
                                .textFieldStyle(.roundedBorder)
                        }
                    }
                    
                    HStack {
                        Text("Time:")
                        TextField(String(format: "%.3f", keyframe.time), text: .constant(""))
                            .disabled(true)
                        Text("seconds")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                }
            }
            
            // Interpolation section
            GroupBox("Interpolation") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("Type:")
                        Picker("Interpolation", selection: $tempInterpolationType) {
                            ForEach(AnimationKeyframe.InterpolationType.allCases, id: \.self) { type in
                                Text(type.rawValue).tag(type)
                            }
                        }
                        .pickerStyle(.menu)
                    }
                    
                    HStack {
                        Text("Ease In:")
                        Slider(value: $tempEaseIn, in: 0...1)
                        Text(String(format: "%.2f", tempEaseIn))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Ease Out:")
                        Slider(value: $tempEaseOut, in: 0...1)
                        Text(String(format: "%.2f", tempEaseOut))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                }
            }
            
            // Tangents section
            GroupBox("Tangents") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Toggle("Unified Tangents", isOn: $tempTangentsUnified)
                    Toggle("Free Tangents", isOn: $tempFreeTangents)
                    Toggle("Weighted Tangents", isOn: $tempWeightedTangents)
                    
                    HStack {
                        VStack(alignment: .leading) {
                            Text("In Tangent:")
                            Slider(value: $tempInTangent, in: -1...1)
                            Text(String(format: "%.2f", tempInTangent))
                                .font(DesignSystem.Typography.smallMono)
                        }
                        
                        VStack(alignment: .leading) {
                            Text("Out Tangent:")
                            Slider(value: $tempOutTangent, in: -1...1)
                            Text(String(format: "%.2f", tempOutTangent))
                                .font(DesignSystem.Typography.smallMono)
                        }
                    }
                    
                    // Tangent presets
                    HStack {
                        Text("Presets:")
                        Button("Smooth") {
                            applyTangentPreset(.smooth)
                        }
                        .buttonStyle(.bordered)
                        
                        Button("Linear") {
                            applyTangentPreset(.linear)
                        }
                        .buttonStyle(.bordered)
                        
                        Button("Step") {
                            applyTangentPreset(.step)
                        }
                        .buttonStyle(.bordered)
                        
                        Button("Auto") {
                            calculateAutoTangents()
                        }
                        .buttonStyle(.borderedProminent)
                    }
                }
            }
            
            // Options section
            GroupBox("Options") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Toggle("Breakpoint", isOn: $tempIsBreakpoint)
                }
            }
            
            EditorDivider()
            
            // Action buttons
            HStack {
                Button("Reset") {
                    resetToOriginal()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Apply") {
                    applyChanges()
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .onAppear {
            loadKeyframeData()
        }
    }
    
    private func loadKeyframeData() {
        tempValue = "\(keyframe.value)"
        tempInTangent = keyframe.inTangent
        tempOutTangent = keyframe.outTangent
        tempInterpolationType = keyframe.interpolationType
        tempPropertyType = keyframe.propertyType
        tempEaseIn = keyframe.easeIn
        tempEaseOut = keyframe.easeOut
        tempIsBreakpoint = keyframe.isBreakpoint
        tempTangentsUnified = keyframe.tangentsUnified
        tempFreeTangents = keyframe.freeTangents
        tempWeightedTangents = keyframe.weightedTangents
    }
    
    private func applyChanges() {
        // Update keyframe with new values
        keyframe.inTangent = tempInTangent
        keyframe.outTangent = tempOutTangent
        keyframe.interpolationType = tempInterpolationType
        keyframe.propertyType = tempPropertyType
        keyframe.easeIn = tempEaseIn
        keyframe.easeOut = tempEaseOut
        keyframe.isBreakpoint = tempIsBreakpoint
        keyframe.tangentsUnified = tempTangentsUnified
        keyframe.freeTangents = tempFreeTangents
        keyframe.weightedTangents = tempWeightedTangents
        
        // Parse value based on type
        switch tempPropertyType {
        case .float:
            keyframe.value = Float(tempValue) ?? 0.0
        case .integer:
            keyframe.value = Int(tempValue) ?? 0
        case .bool:
            keyframe.value = tempValue.lowercased() == "true"
        default:
            keyframe.value = tempValue
        }
    }
    
    private func resetToOriginal() {
        loadKeyframeData()
    }
    
    private enum TangentPreset {
        case smooth, linear, step
    }
    
    private func applyTangentPreset(_ preset: TangentPreset) {
        switch preset {
        case .smooth:
            tempInTangent = 0.0
            tempOutTangent = 0.0
            tempEaseIn = 0.25
            tempEaseOut = 0.25
            tempInterpolationType = .cubic
        case .linear:
            tempInTangent = 0.0
            tempOutTangent = 0.0
            tempEaseIn = 0.0
            tempEaseOut = 0.0
            tempInterpolationType = .linear
        case .step:
            tempInTangent = 0.0
            tempOutTangent = 0.0
            tempEaseIn = 0.0
            tempEaseOut = 0.0
            tempInterpolationType = .step
        }
    }
    
    private func calculateAutoTangents() {
        // Auto-calculate tangents based on surrounding keyframes
        // This would need access to the track and neighboring keyframes
        // For now, set reasonable defaults
        tempInTangent = 0.0
        tempOutTangent = 0.0
        tempEaseIn = 0.25
        tempEaseOut = 0.25
        tempInterpolationType = .cubic
    }
}

// MARK: - Keyframe Context Menu
struct KeyframeContextMenu: View {
    let keyframe: AnimationKeyframe
    let manager: AnimationEditorManager
    @State private var showEditor = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Button("Edit Value...") {
                showEditor = true
            }
            
            Button("Set Tangents") {
                showEditor = true
            }
            
            Menu("Interpolation") {
                ForEach(AnimationKeyframe.InterpolationType.allCases, id: \.self) { type in
                    Button(type.rawValue) {
                        // Set interpolation type
                    }
                }
            }
            
            Divider()
            
            Button("Duplicate") {
                manager.duplicateSelectedKeyframes()
            }
            
            Button("Delete") {
                manager.deleteSelectedKeyframes()
            }
            
            Divider()
            
            Button("Toggle Breakpoint") {
                // Toggle breakpoint
            }
            
            Menu("Tangent Presets") {
                Button("Smooth") {
                    // Apply smooth tangents
                }
                Button("Linear") {
                    // Apply linear tangents
                }
                Button("Step") {
                    // Apply step tangents
                }
                Button("Auto Calculate") {
                    // Auto calculate tangents
                }
            }
            
            Divider()
            
            Menu("Scale") {
                Button("Scale Time 110%") {
                    // Scale time
                }
                Button("Scale Value 110%") {
                    // Scale value
                }
            }
            
            Menu("Transform") {
                Button("Mirror Horizontal") {
                    manager.mirrorSelectedKeyframesHorizontally()
                }
                Button("Offset Time +0.1s") {
                    manager.nudgeSelectedKeyframes(timeDelta: 0.1)
                }
                Button("Offset Value +0.1") {
                    manager.nudgeSelectedKeyframes(valueDelta: 0.1)
                }
            }
        }
        .sheet(isPresented: $showEditor) {
            KeyframeEditorView(keyframe: .constant(keyframe), manager: manager)
        }
    }
}
