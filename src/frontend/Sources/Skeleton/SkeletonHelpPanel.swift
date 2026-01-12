import SwiftUI

// MARK: - Skeleton Help Panel
/// Comprehensive help and documentation for the skeleton editor

struct SkeletonHelpPanel: View {
    @State private var expandedSections: Set<String> = ["building"]
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.lg) {
                // Header
                HStack {
                    Image(systemName: "questionmark.circle.fill")
                        .font(.system(size: 32))
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                    VStack(alignment: .leading) {
                        Text("Skeleton Editor Help")
                            .font(DesignSystem.Typography.title2)
                        Text("Learn how to build and modify skeletons")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                }
                .padding(DesignSystem.Spacing.md)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(DesignSystem.CornerRadius.regular)
                
                // Building Guide
                HelpSection(
                    id: "building",
                    title: "Building Your Skeleton",
                    icon: "hammer.fill",
                    expandedSections: $expandedSections
                ) {
                    HelpStep(number: 1, text: "Click 'New Skeleton' button in toolbar")
                    HelpStep(number: 2, text: "Select a preset (Humanoid, Sailboat, Motorboat) or start Empty")
                    HelpStep(number: 3, text: "Use 'Add Bone' tool (plus icon) to create new bones")
                    HelpStep(number: 4, text: "Select bones in hierarchy or viewport to edit")
                    HelpStep(number: 5, text: "Adjust bone properties in right panel (length, position, color)")
                    HelpStep(number: 6, text: "Click 'Set Bind Pose' when skeleton is in T-pose/neutral position")
                }
                
                // Tools Guide
                HelpSection(
                    id: "tools",
                    title: "Tool Reference",
                    icon: "wrench.and.screwdriver.fill",
                    expandedSections: $expandedSections
                ) {
                    ToolHelp(
                        icon: "cursorarrow",
                        name: "Select Tool",
                        desc: "Click bones to select. Hold  (Shift) and click for multi-select. Use in hierarchy or viewport.",
                        color: DesignSystem.Colors.accentPrimary
                    )
                    
                    ToolHelp(
                        icon: "plus.circle",
                        name: "Add Bone",
                        desc: "Click parent bone in hierarchy first, then click in viewport to place new child bone. Drag to set initial length.",
                        color: .green
                    )
                    
                    ToolHelp(
                        icon: "link",
                        name: "Add Chain",
                        desc: "Click multiple times in viewport to create a chain of connected bones (spine, tail, tentacle).",
                        color: .blue
                    )
                    
                    ToolHelp(
                        icon: "arrow.left.and.right",
                        name: "Mirror Tool",
                        desc: "Mirrors selected bones across X-axis. Automatically renames .L to .R (Left to Right).",
                        color: .purple
                    )
                    
                    ToolHelp(
                        icon: "rotate.3d",
                        name: "Rotate",
                        desc: "Rotate selected bone. Click and drag in viewport. Use for joint rotations and sail positions.",
                        color: .orange
                    )
                    
                    ToolHelp(
                        icon: "arrow.up.left.and.arrow.down.right",
                        name: "Scale",
                        desc: "Scale bone length. Drag to resize. Useful for adjusting proportions after creation.",
                        color: .red
                    )
                }
                
                // Boat Skeletons
                HelpSection(
                    id: "boats",
                    title: "Boat Skeletons",
                    icon: "sailboat.fill",
                    expandedSections: $expandedSections
                ) {
                    Text("**Sailboat Components:**")
                        .font(DesignSystem.Typography.bodyBold)
                        .padding(.bottom, 4)
                    
                    BulletPoint(
                        icon: "cube.fill",
                        text: "**Hull** - Root bone, represents boat body (waterline level)",
                        color: .cyan
                    )
                    BulletPoint(
                        icon: "arrow.down.circle.fill",
                        text: "**Rudder** - Steerable, controls direction (rotate left/right)",
                        color: .blue
                    )
                    BulletPoint(
                        icon: "arrow.up.circle.fill",
                        text: "**Mast** - Vertical pole, holds sails (fixed position)",
                        color: .brown
                    )
                    BulletPoint(
                        icon: "minus.circle.fill",
                        text: "**Boom** - Horizontal arm, controls sail angle (rotates)",
                        color: .brown
                    )
                    BulletPoint(
                        icon: "rectangle.fill",
                        text: "**MainSail** - Large sail, catches wind (attached to boom)",
                        color: .white
                    )
                    BulletPoint(
                        icon: "rectangle.fill",
                        text: "**JibSail** - Front sail, additional wind power",
                        color: .white
                    )
                    
                    Divider()
                        .padding(.vertical, 8)
                    
                    Text("**Motorboat Components:**")
                        .font(DesignSystem.Typography.bodyBold)
                        .padding(.bottom, 4)
                    
                    BulletPoint(
                        icon: "cube.fill",
                        text: "**Hull** - Root bone, represents boat body",
                        color: .cyan
                    )
                    BulletPoint(
                        icon: "fan.fill",
                        text: "**Propeller** - Rotates continuously, generates thrust (animates)",
                        color: .orange
                    )
                    BulletPoint(
                        icon: "arrow.down.circle.fill",
                        text: "**Rudder** - Steerable, controls direction",
                        color: .blue
                    )
                    BulletPoint(
                        icon: "engine.combustion.fill",
                        text: "**OutboardMotor** - Tiltable motor mount",
                        color: .gray
                    )
                }
                
                // Keyboard Shortcuts
                HelpSection(
                    id: "shortcuts",
                    title: "Keyboard Shortcuts",
                    icon: "command",
                    expandedSections: $expandedSections
                ) {
                    ShortcutRow(key: "G", desc: "Move bone (Grab mode)")
                    ShortcutRow(key: "R", desc: "Rotate bone")
                    ShortcutRow(key: "S", desc: "Scale bone length")
                    ShortcutRow(key: "E", desc: "Extrude (create child bone)")
                    ShortcutRow(key: "X", desc: "Delete selected bones")
                    ShortcutRow(key: "Z", desc: "Undo last action")
                    ShortcutRow(key: "Z", desc: "Redo")
                    ShortcutRow(key: "D", desc: "Duplicate selected bones")
                    ShortcutRow(key: "Tab", desc: "Switch between tools")
                }
                
                // Properties Panel
                HelpSection(
                    id: "properties",
                    title: "Bone Properties",
                    icon: "slider.horizontal.3",
                    expandedSections: $expandedSections
                ) {
                    PropertyHelp(
                        name: "Name",
                        desc: "Bone identifier. Use .L/.R suffix for left/right pairs (Arm.L, Arm.R)"
                    )
                    PropertyHelp(
                        name: "Length",
                        desc: "Bone length in meters. Affects child bone positions. Typical human bone: 0.2-0.5m"
                    )
                    PropertyHelp(
                        name: "Position",
                        desc: "Local position offset from parent. X=Left/Right, Y=Up/Down, Z=Forward/Back"
                    )
                    PropertyHelp(
                        name: "Roll",
                        desc: "Twist rotation along bone axis. In degrees. Useful for limb orientation"
                    )
                    PropertyHelp(
                        name: "Color",
                        desc: "Visual bone color in editor. No effect on final render. Use for organization"
                    )
                    PropertyHelp(
                        name: "Visible",
                        desc: "Show/hide bone in viewport. Hidden bones still function in skeleton"
                    )
                    PropertyHelp(
                        name: "Locked",
                        desc: "Prevent bone from being modified. Protects important bones from accidental edits"
                    )
                }
                
                // Tips & Tricks
                HelpSection(
                    id: "tips",
                    title: "Tips & Best Practices",
                    icon: "lightbulb.fill",
                    expandedSections: $expandedSections
                ) {
                    TipCard(
                        icon: "figure.stand",
                        title: "T-Pose for Humanoids",
                        desc: "Always set bind pose with arms out horizontally (T-shape). Standard for character rigging."
                    )
                    TipCard(
                        icon: "arrow.triangle.swap",
                        title: "Use Bone Naming Conventions",
                        desc: "Name bones clearly: UpperArm.L, Forearm.R, Spine1, Spine2. Makes animation easier."
                    )
                    TipCard(
                        icon: "paintbrush.fill",
                        title: "Color Code Bone Groups",
                        desc: "Use colors to group bones: Blue for legs, Red for arms, Green for spine. Visual organization."
                    )
                    TipCard(
                        icon: "sailboat.fill",
                        title: "Test Boat Rotations",
                        desc: "For sails/rudders: rotate bones to test full range of motion before binding mesh."
                    )
                }
            }
            .padding(DesignSystem.Spacing.md)
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Help Components

struct HelpSection<Content: View>: View {
    let id: String
    let title: String
    let icon: String
    @Binding var expandedSections: Set<String>
    @ViewBuilder let content: Content
    
    private var isExpanded: Bool {
        expandedSections.contains(id)
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            Button {
                withAnimation(.spring(response: 0.3, dampingFraction: 0.7)) {
                    if isExpanded {
                        expandedSections.remove(id)
                    } else {
                        expandedSections.insert(id)
                    }
                }
            } label: {
                HStack {
                    Image(systemName: icon)
                        .font(.system(size: 18, weight: .semibold))
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                        .frame(width: 24)
                    
                    Text(title)
                        .font(DesignSystem.Typography.headline)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                    
                    Image(systemName: "chevron.right")
                        .font(.system(size: 12, weight: .semibold))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                        .rotationEffect(.degrees(isExpanded ? 90 : 0))
                }
                .padding(DesignSystem.Spacing.sm)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(DesignSystem.CornerRadius.regular)
            }
            .buttonStyle(.plain)
            
            if isExpanded {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    content
                }
                .padding(.leading, DesignSystem.Spacing.lg)
                .transition(.opacity.combined(with: .move(edge: .top)))
            }
        }
    }
}

struct HelpStep: View {
    let number: Int
    let text: String
    
    var body: some View {
        HStack(alignment: .top, spacing: DesignSystem.Spacing.sm) {
            Text("\(number)")
                .font(.system(size: 16, weight: .bold, design: .rounded))
                .foregroundColor(.white)
                .frame(width: 28, height: 28)
                .background(
                    Circle()
                        .fill(DesignSystem.Colors.accentPrimary)
                )
            
            Text(text)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
        }
    }
}

struct ToolHelp: View {
    let icon: String
    let name: String
    let desc: String
    var color: Color = DesignSystem.Colors.textSecondary
    
    var body: some View {
        HStack(alignment: .top, spacing: DesignSystem.Spacing.sm) {
            Image(systemName: icon)
                .font(.system(size: 20))
                .foregroundColor(color)
                .frame(width: 32, height: 32)
                .background(color.opacity(0.1))
                .cornerRadius(DesignSystem.CornerRadius.small)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(name)
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text(desc)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .fixedSize(horizontal: false, vertical: true)
            }
        }
        .padding(.vertical, 4)
    }
}

struct BulletPoint: View {
    var icon: String = "circle.fill"
    let text: String
    var color: Color = DesignSystem.Colors.textSecondary
    
    var body: some View {
        HStack(alignment: .top, spacing: DesignSystem.Spacing.sm) {
            Image(systemName: icon)
                .font(.system(size: 8))
                .foregroundColor(color)
                .frame(width: 16)
            
            Text(text)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
        }
    }
}

struct ShortcutRow: View {
    let key: String
    let desc: String
    
    var body: some View {
        HStack {
            Text(key)
                .font(.system(size: 14, weight: .semibold, design: .monospaced))
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
                .frame(width: 60, alignment: .center)
            
            Text(desc)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
        }
    }
}

struct PropertyHelp: View {
    let name: String
    let desc: String
    
    var body: some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(name)
                .font(DesignSystem.Typography.bodyBold)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Text(desc)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .padding(.vertical, 4)
    }
}

struct TipCard: View {
    let icon: String
    let title: String
    let desc: String
    
    var body: some View {
        HStack(alignment: .top, spacing: DesignSystem.Spacing.sm) {
            Image(systemName: icon)
                .font(.system(size: 24))
                .foregroundColor(DesignSystem.Colors.accentSuccess)
            
            VStack(alignment: .leading, spacing: 4) {
                Text(title)
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text(desc)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.accentSuccess.opacity(0.05))
        .borderWithBorder(color: DesignSystem.Colors.accentSuccess.opacity(0.2), width: 1, cornerRadius: DesignSystem.CornerRadius.regular)
    }
}

extension View {
    func borderWithBorder(color: Color, width: CGFloat, cornerRadius: CGFloat) -> some View {
        self
            .overlay(
                RoundedRectangle(cornerRadius: cornerRadius)
                    .stroke(color, lineWidth: width)
            )
    }
}
