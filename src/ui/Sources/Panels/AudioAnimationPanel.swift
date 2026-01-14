import SwiftUI

struct AudioAnimationPanel: View {
    @ObservedObject var driver = AudioAnimationDriver.shared
    @ObservedObject var analyzer = AudioAnalyzer.shared
    @ObservedObject var skeletonManager = SkeletonManager.shared
    
    @State private var selectedBoneID: UUID?
    @State private var showBonePicker = false
    
    var body: some View {
        HSplitView {
            // Left: Visualization
            VStack(spacing: 0) {
                // Audio Spectrum
                AudioSpectrumView(analyzer: analyzer)
                    .frame(height: 100)
                    .background(DesignSystem.Colors.backgroundSecondary)
                
                Divider()
                
                // 3D Skeleton View
                if let skeleton = skeletonManager.activeSkeleton {
                    SkeletonVisualizer(skeleton: skeleton, audioDriver: driver)
                        .background(Color.black)
                } else {
                    VStack {
                        Image(systemName: "person.fill.questionmark")
                            .font(.system(size: 40))
                        Text("No Active Skeleton")
                            .foregroundColor(.secondary)
                    }
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .background(DesignSystem.Colors.backgroundPrimary)
                }
            }
            
            // Right: Mappings List
            VStack(alignment: .leading, spacing: 0) {
                HStack {
                    Text("Audio  Bone Mappings")
                        .font(DesignSystem.Typography.headline)
                    Spacer()
                    
                    Menu {
                        Button("Load: Dance") {
                            if skeletonManager.activeSkeleton != nil {
                                driver.loadPreset(.dance, skeletonManager: skeletonManager)
                            }
                        }
                        Button("Load: Idle Sway") {
                            if skeletonManager.activeSkeleton != nil {
                                driver.loadPreset(.idleSway, skeletonManager: skeletonManager)
                            }
                        }
                        Button("Load: Head Bob") {
                            if skeletonManager.activeSkeleton != nil {
                                driver.loadPreset(.headBob, skeletonManager: skeletonManager)
                            }
                        }
                        Divider()
                        Button("Clear All", role: .destructive) {
                            driver.clearMappings()
                        }
                    } label: {
                        Image(systemName: "list.bullet.circle")
                            .font(.system(size: 20))
                    }
                    .menuStyle(.borderlessButton)
                    
                    Button(action: { showBonePicker = true }) {
                        Image(systemName: "plus.circle.fill")
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                    }
                    .buttonStyle(.plain)
                }
                .padding()
                .background(DesignSystem.Colors.backgroundSecondary)
                
                Divider()
                
                if driver.mappings.isEmpty {
                    VStack(spacing: 12) {
                        Image(systemName: "waveform.path.ecg")
                            .font(.system(size: 40))
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        Text("No audio mappings")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        Button("Add Mapping") { showBonePicker = true }
                            .buttonStyle(.plain)
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                    }
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                } else {
                    List {
                        ForEach(driver.mappings) { mapping in
                            MappingRow(mapping: mapping, skeleton: skeletonManager.activeSkeleton)
                        }
                        .onDelete { indexSet in
                            indexSet.forEach { index in
                                let mapping = driver.mappings[index]
                                driver.removeMapping(id: mapping.id)
                            }
                        }
                    }
                    .listStyle(.plain)
                }
            }
            .frame(minWidth: 300)
            .background(DesignSystem.Colors.backgroundPrimary)
        }
        .sheet(isPresented: $showBonePicker) {
            BonePickerView(skeleton: skeletonManager.activeSkeleton) { boneID in
                // Add default mapping for selected bone
                driver.addMapping(BoneAudioMapping(
                    boneID: boneID,
                    audioChannel: .bass,
                    influenceType: .scale,
                    axis: nil,
                    multiplier: 1.0,
                    smoothing: 0.5
                ))
            }
        }
    }
}

// MARK: - Subviews

struct AudioSpectrumView: View {
    @ObservedObject var analyzer: AudioAnalyzer
    
    var body: some View {
        HStack(alignment: .bottom, spacing: 4) {
            SpectrumBar(value: analyzer.frequencyBands.bass, label: "Bass", color: .red)
            SpectrumBar(value: analyzer.frequencyBands.lowMid, label: "L-Mid", color: .orange)
            SpectrumBar(value: analyzer.frequencyBands.mid, label: "Mid", color: .yellow)
            SpectrumBar(value: analyzer.frequencyBands.highMid, label: "H-Mid", color: .green)
            SpectrumBar(value: analyzer.frequencyBands.treble, label: "Treb", color: .blue)
            
            Divider().padding(.horizontal, 4)
            
            SpectrumBar(value: analyzer.currentAmplitude * 2, label: "Amp", color: .white)
            SpectrumBar(value: analyzer.beatDetected ? 1.0 : 0.0, label: "Beat", color: .purple)
        }
        .padding()
    }
}

struct SpectrumBar: View {
    let value: Double
    let label: String
    let color: Color
    
    var body: some View {
        VStack {
            Spacer()
            RoundedRectangle(cornerRadius: 2)
                .fill(color)
                .frame(height: max(4, CGFloat(value * 80)))
            Text(label)
                .font(DesignSystem.Typography.micro)
                .foregroundColor(.secondary)
        }
    }
}

struct MappingRow: View {
    let mapping: BoneAudioMapping
    let skeleton: Skeleton?
    
    var boneName: String {
        skeleton?.bones[mapping.boneID]?.name ?? "Unknown Bone"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Image(systemName: "bone.fill")
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                Text(boneName)
                    .font(DesignSystem.Typography.bodyBold)
                Spacer()
                Text(mapping.influenceType.rawValue)
                    .font(DesignSystem.Typography.caption)
                    .padding(.horizontal, 6)
                    .padding(.vertical, 2)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
            }
            
            HStack {
                Text(mapping.audioChannel.displayName)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                
                Spacer()
                
                if let axis = mapping.axis {
                    Text("Axis: \(axis.rawValue)")
                        .font(DesignSystem.Typography.micro)
                }
            }
        }
        .padding(.vertical, 4)
    }
}

struct BonePickerView: View {
    let skeleton: Skeleton?
    let onSelect: (UUID) -> Void
    @Environment(\.dismiss) var dismiss
    
    var body: some View {
        VStack {
            Text("Select Bone")
                .font(DesignSystem.Typography.headline)
                .padding()
            
            List {
                if let skeleton = skeleton {
                    ForEach(skeleton.bones.sorted { $0.value.name < $1.value.name }, id: \.key) { id, bone in
                        Button(action: {
                            onSelect(id)
                            dismiss()
                        }) {
                            HStack {
                                Image(systemName: "bone")
                                Text(bone.name)
                            }
                        }
                        .buttonStyle(.plain)
                    }
                }
            }
        }
        .frame(width: 300, height: 400)
    }
}
