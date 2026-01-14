import SwiftUI

// MARK: - Entity Type Icons (30+ types)

struct EntityTypeIcon: View {
    let type: EntityType
    var size: CGFloat = 16
    
    var body: some View {
        Image(systemName: type.iconName)
            .font(.system(size: size * 0.75, weight: .medium))
            .foregroundColor(type.color)
            .frame(width: size, height: size)
    }
}

enum EntityType: String, CaseIterable {
    // Core
    case empty = "Empty"
    case entity = "Entity"
    case group = "Group"
    
    // Rendering
    case mesh = "Mesh"
    case sprite = "Sprite"
    case billboard = "Billboard"
    case decal = "Decal"
    case trail = "Trail"
    case line = "Line"
    
    // Lighting
    case pointLight = "Point Light"
    case spotLight = "Spot Light"
    case directionalLight = "Directional Light"
    case areaLight = "Area Light"
    case reflectionProbe = "Reflection Probe"
    case lightProbe = "Light Probe"
    
    // Camera
    case camera = "Camera"
    case cinematicCamera = "Cinematic Camera"
    
    // Audio
    case audioSource = "Audio Source"
    case audioListener = "Audio Listener"
    case audioZone = "Audio Zone"
    
    // Physics
    case rigidbody = "Rigidbody"
    case collider = "Collider"
    case trigger = "Trigger"
    case joint = "Joint"
    case ragdoll = "Ragdoll"
    
    // Effects
    case particleSystem = "Particle System"
    case vfxGraph = "VFX Graph"
    case postProcess = "Post Process"
    
    // Scripting
    case script = "Script"
    case visualScript = "Visual Script"
    
    // UI
    case canvas = "Canvas"
    case uiElement = "UI Element"
    case text = "Text"
    case image = "Image"
    case button = "Button"
    
    // Navigation
    case navMesh = "NavMesh"
    case navAgent = "Nav Agent"
    case waypoint = "Waypoint"
    
    // Special
    case prefab = "Prefab"
    case terrain = "Terrain"
    case water = "Water"
    case wind = "Wind Zone"
    case volume = "Volume"
    
    var iconName: String {
        switch self {
        case .empty: return "circle"
        case .entity: return "cube"
        case .group: return "folder"
        case .mesh: return "cube.fill"
        case .sprite: return "photo"
        case .billboard: return "rectangle"
        case .decal: return "square.stack.3d.up"
        case .trail: return "line.diagonal"
        case .line: return "line.horizontal.3"
        case .pointLight: return "lightbulb.fill"
        case .spotLight: return "flashlight.on.fill"
        case .directionalLight: return "sun.max.fill"
        case .areaLight: return "rectangle.fill"
        case .reflectionProbe: return "circle.hexagongrid.fill"
        case .lightProbe: return "scope"
        case .camera: return "camera.fill"
        case .cinematicCamera: return "video.fill"
        case .audioSource: return "speaker.wave.3.fill"
        case .audioListener: return "ear.fill"
        case .audioZone: return "waveform"
        case .rigidbody: return "arrow.up.left.and.arrow.down.right"
        case .collider: return "square.dashed"
        case .trigger: return "bolt.fill"
        case .joint: return "link"
        case .ragdoll: return "figure.walk"
        case .particleSystem: return "sparkles"
        case .vfxGraph: return "wand.and.stars"
        case .postProcess: return "camera.filters"
        case .script: return "doc.text.fill"
        case .visualScript: return "flowchart.fill"
        case .canvas: return "rectangle.on.rectangle"
        case .uiElement: return "uiwindow.split.2x1"
        case .text: return "textformat"
        case .image: return "photo.fill"
        case .button: return "hand.tap.fill"
        case .navMesh: return "map.fill"
        case .navAgent: return "figure.walk.motion"
        case .waypoint: return "mappin"
        case .prefab: return "cube.transparent.fill"
        case .terrain: return "mountain.2.fill"
        case .water: return "drop.fill"
        case .wind: return "wind"
        case .volume: return "cube.transparent"
        }
    }
    
    var color: Color {
        switch self {
        case .empty, .entity, .group: return DesignSystem.Colors.textSecondary
        case .mesh, .sprite, .billboard, .decal, .trail, .line: return DesignSystem.Colors.entityMesh
        case .pointLight, .spotLight, .directionalLight, .areaLight, .reflectionProbe, .lightProbe: return DesignSystem.Colors.entityLight
        case .camera, .cinematicCamera: return DesignSystem.Colors.entityCamera
        case .audioSource, .audioListener, .audioZone: return DesignSystem.Colors.entityAudio
        case .rigidbody, .collider, .joint, .ragdoll: return DesignSystem.Colors.accentSuccess
        case .trigger: return DesignSystem.Colors.entityTrigger
        case .particleSystem, .vfxGraph, .postProcess: return DesignSystem.Colors.entityParticle
        case .script, .visualScript: return DesignSystem.Colors.accentInfo
        case .canvas, .uiElement, .text, .image, .button: return DesignSystem.Colors.accentWarning
        case .navMesh, .navAgent, .waypoint: return DesignSystem.Colors.accentTertiary
        case .prefab: return DesignSystem.Colors.entityPrefab
        case .terrain, .water, .wind, .volume: return DesignSystem.Colors.accentSuccess
        }
    }
}

// MARK: - Hierarchy Row

struct HierarchyRow: View {
    let name: String
    let type: EntityType
    let depth: Int
    let isSelected: Bool
    let isExpanded: Bool
    let hasChildren: Bool
    let isVisible: Bool
    let isLocked: Bool
    let isPrefab: Bool
    let isModified: Bool
    
    let onToggleExpand: () -> Void
    let onToggleVisibility: () -> Void
    let onToggleLock: () -> Void
    
    @State private var isHovered = false
    
    var body: some View {
        HStack(spacing: 4) {
            // Indent
            Spacer()
                .frame(width: CGFloat(depth) * 16)
            
            // Expand/collapse
            if hasChildren {
                Button(action: onToggleExpand) {
                    Image(systemName: "chevron.right")
                        .font(.system(size: 8, weight: .bold))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                        .rotationEffect(.degrees(isExpanded ? 90 : 0))
                        .animation(DesignSystem.Animation.fast, value: isExpanded)
                }
                .buttonStyle(.plain)
                .frame(width: 16)
            } else {
                Spacer().frame(width: 16)
            }
            
            // Entity icon
            EntityTypeIcon(type: type, size: 14)
            
            // Name
            Text(name)
                .font(DesignSystem.Typography.small)
                .foregroundColor(isVisible ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textDisabled)
                .lineLimit(1)
            
            // Prefab badge
            if isPrefab {
                EditorBadge(text: "P", style: .info)
            }
            
            // Modified indicator
            if isModified {
                Circle()
                    .fill(DesignSystem.Colors.accentWarning)
                    .frame(width: 6, height: 6)
            }
            
            Spacer()
            
            // Visibility toggle
            if isHovered || !isVisible {
                Button(action: onToggleVisibility) {
                    Image(systemName: isVisible ? "eye" : "eye.slash")
                        .font(.system(size: 10))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
            }
            
            // Lock toggle
            if isHovered || isLocked {
                Button(action: onToggleLock) {
                    Image(systemName: isLocked ? "lock.fill" : "lock.open")
                        .font(.system(size: 10))
                        .foregroundColor(isLocked ? DesignSystem.Colors.accentWarning : DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
            }
        }
        .padding(.horizontal, DesignSystem.Spacing.xs)
        .padding(.vertical, 4)
        .background(isSelected ? DesignSystem.Colors.selection : (isHovered ? DesignSystem.Colors.hover : Color.clear))
        .cornerRadius(DesignSystem.CornerRadius.regular)
        .onHover { isHovered = $0 }
    }
}

// MARK: - Property Row

struct PropertyRow<Content: View>: View {
    let label: String
    let content: Content
    var tooltip: String? = nil
    var isModified: Bool = false
    var canReset: Bool = false
    var onReset: (() -> Void)? = nil
    
    init(
        _ label: String,
        tooltip: String? = nil,
        isModified: Bool = false,
        canReset: Bool = false,
        onReset: (() -> Void)? = nil,
        @ViewBuilder content: () -> Content
    ) {
        self.label = label
        self.tooltip = tooltip
        self.isModified = isModified
        self.canReset = canReset
        self.onReset = onReset
        self.content = content()
    }
    
    @State private var isHovered = false
    @State private var showTooltip = false
    
    var body: some View {
        HStack(alignment: .center, spacing: DesignSystem.Spacing.sm) {
            HStack(spacing: 4) {
                if isModified {
                    Circle()
                        .fill(DesignSystem.Colors.accentPrimary)
                        .frame(width: 4, height: 4)
                }
                
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                if tooltip != nil {
                    Image(systemName: "questionmark.circle")
                        .font(.system(size: 10))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                        .onHover { showTooltip = $0 }
                }
            }
            .frame(width: 100, alignment: .trailing)
            
            content
                .frame(maxWidth: .infinity, alignment: .leading)
            
            if canReset && isHovered {
                Button(action: { onReset?() }) {
                    Image(systemName: "arrow.counterclockwise")
                        .font(.system(size: 10))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
            }
        }
        .padding(.vertical, 3)
        .onHover { isHovered = $0 }
    }
}

// MARK: - Component Header

struct ComponentHeader: View {
    let title: String
    let icon: String
    let isEnabled: Bool
    let isExpanded: Bool
    let onToggleEnabled: () -> Void
    let onToggleExpanded: () -> Void
    let onRemove: () -> Void
    let onCopy: () -> Void
    let onPaste: () -> Void
    let onReset: () -> Void
    
    @State private var isHovered = false
    @State private var showMenu = false
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Expand chevron
            Button(action: onToggleExpanded) {
                Image(systemName: "chevron.right")
                    .font(.system(size: 10, weight: .bold))
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .rotationEffect(.degrees(isExpanded ? 90 : 0))
            }
            .buttonStyle(.plain)
            
            // Enable toggle
            Button(action: onToggleEnabled) {
                Image(systemName: isEnabled ? "checkmark.square.fill" : "square")
                    .font(.system(size: 14))
                    .foregroundColor(isEnabled ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textTertiary)
            }
            .buttonStyle(.plain)
            
            // Icon
            Image(systemName: icon)
                .font(.system(size: 12))
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            // Title
            Text(title)
                .font(DesignSystem.Typography.bodyBold)
                .foregroundColor(isEnabled ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textDisabled)
            
            Spacer()
            
            // Menu
            if isHovered {
                Menu {
                    Button("Copy Component", action: onCopy)
                    Button("Paste Component Values", action: onPaste)
                    Divider()
                    Button("Reset", action: onReset)
                    Divider()
                    Button("Remove Component", role: .destructive, action: onRemove)
                } label: {
                    Image(systemName: "ellipsis")
                        .font(.system(size: 12))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 20, height: 20)
                }
                .menuStyle(.borderlessButton)
            }
        }
        .padding(DesignSystem.Spacing.xs)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(DesignSystem.CornerRadius.tight)
        .onHover { isHovered = $0 }
    }
}

// MARK: - Asset Thumbnail

struct AssetThumbnail: View {
    let name: String
    let type: AssetType
    let isSelected: Bool
    var thumbnail: Image? = nil
    
    @State private var isHovered = false
    
    var body: some View {
        VStack(spacing: 4) {
            // Thumbnail
            ZStack {
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular)
                    .fill(DesignSystem.Colors.backgroundTertiary)
                
                if let thumbnail = thumbnail {
                    thumbnail
                        .resizable()
                        .aspectRatio(contentMode: .fill)
                        .clipped()
                } else {
                    Image(systemName: type.iconName)
                        .font(.system(size: 24))
                        .foregroundColor(type.color)
                }
            }
            .frame(width: 64, height: 64)
            .overlay(
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular)
                    .stroke(isSelected ? DesignSystem.Colors.accentPrimary : (isHovered ? DesignSystem.Colors.borderStrong : DesignSystem.Colors.border), lineWidth: isSelected ? 2 : 1)
            )
            
            // Type badge
            EditorBadge(text: type.shortName, style: .default)
                .offset(x: 0, y: -8)
            
            // Name
            Text(name)
                .font(DesignSystem.Typography.caption)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .lineLimit(1)
                .frame(width: 64)
        }
        .onHover { isHovered = $0 }
    }
}

// Use AssetType from AssetDefinitions.swift
extension AssetType {
    var iconName: String { icon }  // Alias for icon
    
    var shortName: String {
        switch self {
        case .folder: return "DIR"
        case .texture: return "TEX"
        case .material: return "MAT"
        case .mesh: return "MSH"
        case .audio: return "AUD"
        case .animation: return "ANI"
        case .script: return "SCR"
        case .prefab: return "PRE"
        case .other: return "OTH"
        }
    }
}

// MARK: - Panel Toolbar

struct EditorPanelToolbar<Content: View>: View {
    let content: Content
    
    init(@ViewBuilder content: () -> Content) {
        self.content = content()
    }
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            content
        }
        .padding(.horizontal, DesignSystem.Spacing.xs)
        .padding(.vertical, 4)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
}

// MARK: - Split Handle

struct EditorSplitHandle: View {
    var axis: Axis = .horizontal
    @State private var isHovered = false
    
    var body: some View {
        Group {
            if axis == .horizontal {
                VStack(spacing: 2) {
                    ForEach(0..<3, id: \.self) { _ in
                        Circle()
                            .fill(isHovered ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textTertiary)
                            .frame(width: 3, height: 3)
                    }
                }
                .frame(width: 8)
            } else {
                HStack(spacing: 2) {
                    ForEach(0..<3, id: \.self) { _ in
                        Circle()
                            .fill(isHovered ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textTertiary)
                            .frame(width: 3, height: 3)
                    }
                }
                .frame(height: 8)
            }
        }
        .padding(4)
        .background(DesignSystem.Colors.backgroundSecondary)
        .onHover { hovering in
            isHovered = hovering
            if axis == .horizontal {
                if hovering { NSCursor.resizeLeftRight.push() }
                else { NSCursor.pop() }
            } else {
                if hovering { NSCursor.resizeUpDown.push() }
                else { NSCursor.pop() }
            }
        }
    }
}
