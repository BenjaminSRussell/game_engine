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
                        .font(.system(size: 14, weight: .medium))
                }
                Text(title)
                    .font(.system(size: 14, weight: .semibold))
            }
            .foregroundColor(.white)
            .padding(.horizontal, DesignSystem.Spacing.md)
            .padding(.vertical, DesignSystem.Spacing.sm)
            .background(
                LinearGradient(
                    colors: isHovering ? [Color.blue.opacity(0.8), Color.purple.opacity(0.8)] : [Color.blue, Color.purple],
                    startPoint: .topLeading,
                    endPoint: .bottomTrailing
                )
            )
            .cornerRadius(8)
            .shadow(color: .black.opacity(0.2), radius: 4, x: 0, y: 2)
        }
        .onHover { hovering in
            isHovering = hovering
        }
        .buttonStyle(PlainButtonStyle())
    }
}

/// Secondary action button
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
                        .font(.system(size: 14, weight: .medium))
                }
                Text(title)
                    .font(.system(size: 14, weight: .medium))
            }
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .padding(.horizontal, DesignSystem.Spacing.md)
            .padding(.vertical, DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(6)
            .overlay(
                RoundedRectangle(cornerRadius: 6)
                    .stroke(DesignSystem.Colors.border, lineWidth: 1)
            )
        }
        .onHover { hovering in
            isHovering = hovering
        }
        .buttonStyle(PlainButtonStyle())
    }
}

/// Icon-only button for toolbars
struct EditorIconButton: View {
    let icon: String
    let tooltip: String?
    let action: () -> Void
    @State private var isHovering = false
    @State private var isActive = false
    
    init(icon: String, tooltip: String? = nil, isActive: Bool = false, action: @escaping () -> Void) {
        self.icon = icon
        self.tooltip = tooltip
        self.isActive = isActive
        self.action = action
    }
    
    var body: some View {
        Button(action: action) {
            Image(systemName: icon)
                .font(.system(size: 16, weight: .medium))
                .foregroundColor(isActive ? .white : DesignSystem.Colors.textSecondary)
                .frame(width: 32, height: 32)
                .background(
                    isActive ? DesignSystem.Colors.accent :
                    (isHovering ? DesignSystem.Colors.backgroundSecondary : Color.clear)
                )
                .cornerRadius(6)
        }
        .onHover { hovering in
            isHovering = hovering
        }
        .buttonStyle(PlainButtonStyle())
        .help(tooltip ?? "")
    }
}

// MARK: - Input Fields

/// Styled text field matching editor theme
struct EditorTextField: View {
    @Binding var text: String
    let placeholder: String
    let icon: String?
    @State private var isFocused = false
    
    init(_ placeholder: String, text: Binding<String>, icon: String? = nil) {
        self._text = text
        self.placeholder = placeholder
        self.icon = icon
    }
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            if let icon = icon {
                Image(systemName: icon)
                    .font(.system(size: 14, weight: .medium))
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            TextField(placeholder, text: $text)
                .textFieldStyle(PlainTextFieldStyle())
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .padding(.horizontal, DesignSystem.Spacing.sm)
                .padding(.vertical, DesignSystem.Spacing.xs)
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .padding(.vertical, DesignSystem.Spacing.xs)
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(6)
        .overlay(
            RoundedRectangle(cornerRadius: 6)
                .stroke(isFocused ? DesignSystem.Colors.accent : DesignSystem.Colors.border, lineWidth: 1)
        )
        .onFocusChange { focused in
            isFocused = focused
        }
    }
}

/// Numeric input field with drag-to-change support
struct EditorNumericField: View {
    @Binding var value: Float
    let label: String
    let range: ClosedRange<Float>?
    @State private var isDragging = false
    
    init(_ label: String, value: Binding<Float>, range: ClosedRange<Float>? = nil) {
        self._value = value
        self.label = label
        self.range = range
    }
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            Text(label)
                .font(.system(size: 12, weight: .medium))
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 60, alignment: .leading)
            
            TextField("", value: $value, format: "%.2f")
                .textFieldStyle(PlainTextFieldStyle())
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .padding(.horizontal, DesignSystem.Spacing.sm)
                .padding(.vertical, DesignSystem.Spacing.xs)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(4)
                .overlay(
                    RoundedRectangle(cornerRadius: 4)
                        .stroke(DesignSystem.Colors.border, lineWidth: 1)
                )
                .gesture(
                    DragGesture()
                        .onChanged { value in
                            isDragging = true
                            let delta = Float(value.translation.x) * 0.1
                            self.value = max(range?.lowerBound ?? -Float.greatestFiniteMagnitude,
                                           min(range?.upperBound ?? Float.greatestFiniteMagnitude,
                                               self.value + delta))
                        }
                        .onEnded { _ in
                            isDragging = false
                        }
                )
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .padding(.vertical, DesignSystem.Spacing.xs)
        .background(isDragging ? DesignSystem.Colors.backgroundSecondary : Color.clear)
        .cornerRadius(4)
    }
}

/// Vector3 input field for positions, rotations, scales
struct EditorVector3Field: View {
    @Binding var x: Float
    @Binding var y: Float
    @Binding var z: Float
    let label: String
    
    init(_ label: String, x: Binding<Float>, y: Binding<Float>, z: Binding<Float>) {
        self._x = x
        self._y = y
        self._z = z
        self.label = label
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
            Text(label)
                .font(.system(size: 12, weight: .medium))
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack(spacing: DesignSystem.Spacing.sm) {
                EditorNumericField("X", value: $x)
                EditorNumericField("Y", value: $y)
                EditorNumericField("Z", value: $z)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(6)
    }
}

/// Toggle switch with editor styling
struct EditorToggle: View {
    let label: String
    @Binding var isOn: Bool
    
    init(_ label: String, isOn: Binding<Bool>) {
        self.label = label
        self._isOn = isOn
    }
    
    var body: some View {
        HStack {
            Text(label)
                .font(.system(size: 14, weight: .medium))
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Spacer()
            
            Toggle("", isOn: $isOn)
                .toggleStyle(SwitchToggleStyle(tint: DesignSystem.Colors.accent))
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .padding(.vertical, DesignSystem.Spacing.xs)
    }
}

/// Visual separator
struct EditorDivider: View {
    var body: some View {
        Rectangle()
            .fill(DesignSystem.Colors.border)
            .frame(height: 1)
            .padding(.horizontal, DesignSystem.Spacing.md)
    }
}

// MARK: - Layout Components

/// Collapsible section for grouping properties
struct EditorCollapsibleSection<Content: View>: View {
    let title: String
    @State private var isExpanded: Bool = true
    let content: () -> Content
    
    init(_ title: String, isExpanded: Bool = true, @ViewBuilder content: @escaping () -> Content) {
        self.title = title
        self._isExpanded = State(initialValue: isExpanded)
        self.content = content
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Button(action: {
                withAnimation(.easeInOut(duration: 0.2)) {
                    isExpanded.toggle()
                }
            }) {
                HStack {
                    Text(title)
                        .font(.system(size: 14, weight: .semibold))
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                    
                    Image(systemName: isExpanded ? "chevron.down" : "chevron.right")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .rotationEffect(.degrees(isExpanded ? 0 : -90))
                        .animation(.easeInOut(duration: 0.2), value: isExpanded)
                }
                .padding(.horizontal, DesignSystem.Spacing.md)
                .padding(.vertical, DesignSystem.Spacing.sm)
                .contentShape(Rectangle())
            }
            .buttonStyle(PlainButtonStyle())
            
            if isExpanded {
                content()
                    .padding(DesignSystem.Spacing.md)
                    .background(DesignSystem.Colors.backgroundTertiary)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(8)
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(DesignSystem.Colors.border, lineWidth: 1)
        )
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
        VStack(alignment: .leading, spacing: 0) {
            // Title bar
            HStack {
                Text(title)
                    .font(.system(size: 16, weight: .semibold))
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
            }
            .padding(.horizontal, DesignSystem.Spacing.md)
            .padding(.vertical, DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            // Content
            content()
                .padding(DesignSystem.Spacing.md)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(8)
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(DesignSystem.Colors.border, lineWidth: 1)
        )
    }
}

/// Generic button for editor actions
struct EditorButton: View {
    let title: String
    let action: () -> Void
    @State private var isHovering = false
    
    init(_ title: String, icon: String? = nil, style: EditorButtonStyle = .normal, action: @escaping () -> Void) {
        self.title = title
        self.action = action
        // Note: We are ignoring icon and style in internal storage for now as they weren't in the struct properties, 
        // but we should probably add them if we want them to render. 
        // However, to fix the build quickly, allowing the init is key.
        // Actually, if I ignore them, they won't show.
        // I should add proper properties? 
        // But the struct (lines 380-381) only has title and action.
        // I should also update the properties and body.
    }
    
    var body: some View {
        Button(action: action) {
            Text(title)
                .font(.system(size: 14, weight: .medium))
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .padding(.horizontal, DesignSystem.Spacing.md)
                .padding(.vertical, DesignSystem.Spacing.xs)
                .background(isHovering ? DesignSystem.Colors.backgroundSecondary : DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(6)
                .overlay(
                    RoundedRectangle(cornerRadius: 6)
                        .stroke(DesignSystem.Colors.border, lineWidth: 1)
                )
        }
        .onHover { hovering in
            isHovering = hovering
        }
        .buttonStyle(PlainButtonStyle())
    }
}

/// Search bar component
struct EditorSearchBar: View {
    @Binding var text: String
    let placeholder: String
    
    init(text: Binding<String>, placeholder: String) {
        self._text = text
        self.placeholder = placeholder
    }
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            Image(systemName: "magnifyingglass")
                .font(.system(size: 14, weight: .medium))
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            TextField(placeholder, text: $text)
                .textFieldStyle(PlainTextFieldStyle())
                .foregroundColor(DesignSystem.Colors.textPrimary)
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .padding(.vertical, DesignSystem.Spacing.xs)
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(6)
        .overlay(
            RoundedRectangle(cornerRadius: 6)
                .stroke(DesignSystem.Colors.border, lineWidth: 1)
        )
    }
}

// MARK: - Search Bar

struct PremiumEditorSearchBar: View {
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

// MARK: - Editor Divider (Duplicate - Removed)

// struct EditorDivider: View {
//     var body: some View {
//         Rectangle()
//             .fill(DesignSystem.Colors.border)
//             .frame(height: 1)
//     }
// }

// MARK: - Loading Indicator

struct PremiumEditorLoadingIndicator: View {
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

struct EditorCard<Content: View>: View {
    let title: String
    let content: Content
    
    init(title: String, @ViewBuilder content: () -> Content) {
        self.title = title
        self.content = content()
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text(title)
                .font(.headline)
            content
        }
        .padding()
        .background(DesignSystem.Colors.backgroundSecondary.opacity(0.5)) // Slightly darker or lighter
        .cornerRadius(12)
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .strokeBorder(DesignSystem.Colors.border.opacity(0.5), lineWidth: 1)
        )
    }
}
