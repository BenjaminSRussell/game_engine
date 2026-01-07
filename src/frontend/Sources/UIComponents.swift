import SwiftUI

// MARK: - Buttons

/// Primary action button with UE5-style appearance
struct EditorPrimaryButton: View {
    let title: String
    let icon: String?
    let action: () -> Void
    @State private var isHovering = false
    
    init(_ title: String, icon: String? = nil, action: @escaping () -> Void) {
        self.title = title
        self.icon = icon
        self.action = action
    }
    
    var body: some View {
        Button(action: action) {
            HStack(spacing: DesignSystem.Spacing.sm) {
                if let icon = icon {
                    Image(systemName: icon)
                }
                Text(title)
            }
            .font(DesignSystem.Typography.body)
            .foregroundColor(.white)
            .padding(.horizontal, DesignSystem.Spacing.lg)
            .padding(.vertical, DesignSystem.Spacing.sm)
            .background(isHovering ? DesignSystem.Colors.accentSecondary : DesignSystem.Colors.accentPrimary)
            .cornerRadius(4)
        }
        .buttonStyle(.plain)
        .onHover { hovering in
            withAnimation(DesignSystem.Animation.easeInOut) {
                isHovering = hovering
            }
        }
    }
}

/// Secondary button with border style
struct EditorSecondaryButton: View {
    let title: String
    let icon: String?
    let action: () -> Void
    @State private var isHovering = false
    
    init(_ title: String, icon: String? = nil, action: @escaping () -> Void) {
        self.title = title
        self.icon = icon
        self.action = action
    }
    
    var body: some View {
        Button(action: action) {
            HStack(spacing: DesignSystem.Spacing.sm) {
                if let icon = icon {
                    Image(systemName: icon)
                }
                Text(title)
            }
            .font(DesignSystem.Typography.body)
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .padding(.horizontal, DesignSystem.Spacing.lg)
            .padding(.vertical, DesignSystem.Spacing.sm)
            .background(isHovering ? DesignSystem.Colors.hover : Color.clear)
            .overlay(
                RoundedRectangle(cornerRadius: 4)
                    .stroke(DesignSystem.Colors.border, lineWidth: 1)
            )
        }
        .buttonStyle(.plain)
        .onHover { hovering in
            withAnimation(DesignSystem.Animation.easeInOut) {
                isHovering = hovering
            }
        }
    }
}

/// Icon-only button for toolbars
struct EditorIconButton: View {
    let icon: String
    let tooltip: String?
    let action: () -> Void
    @State private var isHovering = false
    
    init(icon: String, tooltip: String? = nil, action: @escaping () -> Void) {
        self.icon = icon
        self.tooltip = tooltip
        self.action = action
    }
    
    var body: some View {
        Button(action: action) {
            Image(systemName: icon)
                .font(.system(size: 16))
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .frame(width: 32, height: 32)
                .background(isHovering ? DesignSystem.Colors.hover : DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
        }
        .buttonStyle(.plain)
        .onHover { hovering in
            withAnimation(DesignSystem.Animation.easeInOut) {
                isHovering = hovering
            }
        }
        .help(tooltip ?? "")
    }
}

// MARK: - Input Fields

/// Styled text field matching editor theme
struct EditorTextField: View {
    @Binding var text: String
    let placeholder: String
    
    var body: some View {
        TextField(placeholder, text: $text)
            .textFieldStyle(.plain)
            .font(DesignSystem.Typography.body)
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(4)
            .overlay(
                RoundedRectangle(cornerRadius: 4)
                    .stroke(DesignSystem.Colors.border, lineWidth: 1)
            )
    }
}

/// Numeric input field with drag-to-change support
struct EditorNumericField: View {
    @Binding var value: Float
    let label: String
    let range: ClosedRange<Float>
    let step: Float
    @State private var isDragging = false
    @State private var dragStartValue: Float = 0
    @State private var dragStartLocation: CGPoint = .zero
    
    init(label: String, value: Binding<Float>, range: ClosedRange<Float> = -1000...1000, step: Float = 0.1) {
        self.label = label
        self._value = value
        self.range = range
        self.step = step
    }
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 60, alignment: .leading)
            
            TextField("", value: $value, format: .number)
                .textFieldStyle(.plain)
                .font(DesignSystem.Typography.mono)
                .multilineTextAlignment(.trailing)
                .frame(width: 80)
                .padding(DesignSystem.Spacing.xs)
                .background(isDragging ? DesignSystem.Colors.selection : DesignSystem.Colors.backgroundPrimary)
                .cornerRadius(4)
                .overlay(
                    RoundedRectangle(cornerRadius: 4)
                        .stroke(DesignSystem.Colors.border, lineWidth: 1)
                )
                .gesture(
                    DragGesture(minimumDistance: 2)
                        .onChanged { gesture in
                            if !isDragging {
                                isDragging = true
                                dragStartValue = value
                                dragStartLocation = gesture.startLocation
                            }
                            let delta = Float(gesture.translation.width) * step
                            value = min(max(dragStartValue + delta, range.lowerBound), range.upperBound)
                        }
                        .onEnded { _ in
                            isDragging = false
                        }
                )
        }
    }
}

/// Vector3 input field for positions, rotations, scales
struct EditorVector3Field: View {
    @Binding var x: Float
    @Binding var y: Float
    @Binding var z: Float
    let label: String
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack(spacing: DesignSystem.Spacing.xs) {
                VectorComponent(label: "X", value: $x, color: DesignSystem.Colors.accentError)
                VectorComponent(label: "Y", value: $y, color: DesignSystem.Colors.accentSuccess)
                VectorComponent(label: "Z", value: $z, color: DesignSystem.Colors.accentPrimary)
            }
        }
    }
}

private struct VectorComponent: View {
    let label: String
    @Binding var value: Float
    let color: Color
    
    var body: some View {
        HStack(spacing: 2) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(color)
                .frame(width: 12)
            
            TextField("", value: $value, format: .number)
                .textFieldStyle(.plain)
                .font(DesignSystem.Typography.mono)
                .multilineTextAlignment(.trailing)
                .padding(DesignSystem.Spacing.xs)
                .background(DesignSystem.Colors.backgroundPrimary)
                .cornerRadius(4)
                .overlay(
                    RoundedRectangle(cornerRadius: 4)
                        .stroke(DesignSystem.Colors.border, lineWidth: 1)
                )
        }
    }
}

// MARK: - Panels

/// Collapsible section for grouping properties
struct EditorCollapsibleSection<Content: View>: View {
    let title: String
    @State private var isExpanded: Bool
    let content: () -> Content
    
    init(_ title: String, isExpanded: Bool = true, @ViewBuilder content: @escaping () -> Content) {
        self.title = title
        self._isExpanded = State(initialValue: isExpanded)
        self.content = content
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Button(action: {
                withAnimation(DesignSystem.Animation.spring) {
                    isExpanded.toggle()
                }
            }) {
                HStack(spacing: DesignSystem.Spacing.sm) {
                    Image(systemName: isExpanded ? "chevron.down" : "chevron.right")
                        .font(.system(size: 12))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text(title)
                        .font(DesignSystem.Typography.bodyBold)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                }
                .padding(DesignSystem.Spacing.sm)
                .background(DesignSystem.Colors.backgroundTertiary)
            }
            .buttonStyle(.plain)
            
            if isExpanded {
                content()
                    .padding(DesignSystem.Spacing.sm)
                    .transition(.opacity.combined(with: .move(edge: .top)))
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(4)
    }
}

/// Panel with title bar and content
struct EditorPanel<Content: View>: View {
    let title: String
    let content: () -> Content
    
    init(_ title: String, @ViewBuilder content: @escaping () -> Content) {
        self.title = title
        self.content = content
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Title bar
            HStack {
                Text(title)
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            Divider()
                .background(DesignSystem.Colors.border)
            
            // Content
            content()
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(8)
        .shadow(
            color: DesignSystem.Shadows.medium.color,
            radius: DesignSystem.Shadows.medium.radius,
            x: DesignSystem.Shadows.medium.x,
            y: DesignSystem.Shadows.medium.y
        )
    }
}

// MARK: - Search Bar

struct EditorSearchBar: View {
    @Binding var text: String
    let placeholder: String
    
    init(text: Binding<String>, placeholder: String = "Search...") {
        self._text = text
        self.placeholder = placeholder
    }
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            Image(systemName: "magnifyingglass")
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .font(.system(size: 14))
            
            TextField(placeholder, text: $text)
                .textFieldStyle(.plain)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            if !text.isEmpty {
                Button(action: { text = "" }) {
                    Image(systemName: "xmark.circle.fill")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .font(.system(size: 14))
                }
                .buttonStyle(.plain)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(4)
        .overlay(
            RoundedRectangle(cornerRadius: 4)
                .stroke(DesignSystem.Colors.border, lineWidth: 1)
        )
    }
}

// MARK: - Divider

struct EditorDivider: View {
    var body: some View {
        Rectangle()
            .fill(DesignSystem.Colors.border)
            .frame(height: 1)
    }
}

// MARK: - Loading Indicator

struct EditorLoadingIndicator: View {
    let message: String?
    
    init(message: String? = nil) {
        self.message = message
    }
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            ProgressView()
                .progressViewStyle(.circular)
                .scaleEffect(1.2)
            
            if let message = message {
                Text(message)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
        }
        .padding(DesignSystem.Spacing.xl)
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(8)
        .shadow(
            color: DesignSystem.Shadows.large.color,
            radius: DesignSystem.Shadows.large.radius,
            x: DesignSystem.Shadows.large.x,
            y: DesignSystem.Shadows.large.y
        )
    }
}
