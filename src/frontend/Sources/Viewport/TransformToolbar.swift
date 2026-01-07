import SwiftUI

struct TransformToolbar: View {
    @ObservedObject var controller: TransformController
    
    var body: some View {
        HStack(spacing: 8) {
            // Transform mode indicators
            ModeButton(
                icon: "arrow.up.and.down.and.arrow.left.and.right",
                isActive: {
                    if case .translating = controller.mode { return true }
                    return false
                }(),
                tooltip: "Move (G)"
            ) {
                controller.beginTranslate()
            }
            
            ModeButton(
                icon: "arrow.triangle.2.circlepath",
                isActive: {
                    if case .rotating = controller.mode { return true }
                    return false
                }(),
                tooltip: "Rotate (R)"
            ) {
                controller.beginRotate()
            }
            
            ModeButton(
                icon: "arrow.up.left.and.arrow.down.right",
                isActive: {
                    if case .scaling = controller.mode { return true }
                    return false
                }(),
                tooltip: "Scale (S)"
            ) {
                controller.beginScale()
            }
            
            EditorDivider()
                .frame(height: 20)
            
            // Transform space
            Menu {
                ForEach(TransformSpace.allCases, id: \.self) { space in
                    Button(action: {
                        controller.transformSpace = space
                    }) {
                        HStack {
                            Image(systemName: space.icon)
                            Text(space.rawValue)
                            if controller.transformSpace == space {
                                Spacer()
                                Image(systemName: "checkmark")
                            }
                        }
                    }
                }
            } label: {
                HStack(spacing: 4) {
                    Image(systemName: controller.transformSpace.icon)
                    Text(controller.transformSpace.rawValue)
                    Image(systemName: "chevron.down")
                        .font(.system(size: 8))
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
            .menuStyle(.borderlessButton)
            
            EditorDivider()
                .frame(height: 20)
            
            // Pivot point
            Menu {
                ForEach(PivotPoint.allCases, id: \.self) { pivot in
                    Button(action: {
                        controller.pivotPoint = pivot
                    }) {
                        HStack {
                            Image(systemName: pivot.icon)
                            Text(pivot.rawValue)
                            if controller.pivotPoint == pivot {
                                Spacer()
                                Image(systemName: "checkmark")
                            }
                        }
                    }
                }
            } label: {
                HStack(spacing: 4) {
                    Image(systemName: controller.pivotPoint.icon)
                    Image(systemName: "chevron.down")
                        .font(.system(size: 8))
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
            .menuStyle(.borderlessButton)
            .help("Pivot Point")
            
            EditorDivider()
                .frame(height: 20)
            
            // Snap mode
            Menu {
                ForEach(SnapMode.allCases, id: \.self) { mode in
                    Button(action: {
                        controller.snapMode = mode
                    }) {
                        HStack {
                            Image(systemName: mode.icon)
                            Text(mode.rawValue)
                            if controller.snapMode == mode {
                                Spacer()
                                Image(systemName: "checkmark")
                            }
                        }
                    }
                }
            } label: {
                HStack(spacing: 4) {
                    Image(systemName: controller.snapMode.icon)
                    Text(controller.snapMode.rawValue)
                    Image(systemName: "chevron.down")
                        .font(.system(size: 8))
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(controller.snapMode == .none ? DesignSystem.Colors.textSecondary : DesignSystem.Colors.accentPrimary)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
            .menuStyle(.borderlessButton)
            
            // Snap increment
            if controller.snapMode == .increment || controller.snapMode == .grid {
                HStack(spacing: 2) {
                    TextField("", value: $controller.snapIncrement, format: .number)
                        .textFieldStyle(.roundedBorder)
                        .frame(width: 50)
                        .font(DesignSystem.Typography.small)
                }
            }
        }
    }
}

private struct ModeButton: View {
    let icon: String
    let isActive: Bool
    let tooltip: String
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            Image(systemName: icon)
                .font(.system(size: 16))
                .foregroundColor(isActive ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
                .frame(width: 32, height: 32)
                .background(isActive ? DesignSystem.Colors.selection : Color.clear)
                .cornerRadius(4)
        }
        .buttonStyle(.plain)
        .help(tooltip)
    }
}
