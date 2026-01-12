import SwiftUI

// MARK: - Premium Editor Button

enum EditorButtonStyle {
    case primary      // Vibrant gradient, main actions
    case secondary    // Border with transparency
    case tertiary     // Text only, minimal
    case icon         // Icon-only, compact
    case danger       // Red variant for destructive
}

struct EditorButton: View {
    let title: String
    let icon: String?
    let style: EditorButtonStyle
    let action: () -> Void
    
    @State private var isHovered = false
    @State private var isPressed = false
    
    init(_ title: String, icon: String? = nil, style: EditorButtonStyle = .secondary, action: @escaping () -> Void) {
        self.title = title
        self.icon = icon
        self.style = style
        self.action = action
    }
    
    var body: some View {
        Button(action: {
            withAnimation(.spring(response: 0.2, dampingFraction: 0.7)) {
                isPressed = true
            }
            action()
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.12) {
                withAnimation(.spring(response: 0.2, dampingFraction: 0.7)) {
                    isPressed = false
                }
            }
        }) {
            HStack(spacing: 6) {
                if let icon = icon {
                    Image(systemName: icon)
                        .font(.system(size: 14, weight: style == .primary ? .semibold : .medium))
                        .symbolRenderingMode(style == .primary ? .hierarchical : .monochrome)
                }
                
                if style != .icon {
                    Text(title)
                        .font(.system(size: 13, weight: style == .primary ? .semibold : .medium))
                }
            }
            .foregroundColor(foregroundColor)
            .padding(.horizontal, style == .icon ? 10 : 14)
            .padding(.vertical, style == .icon ? 10 : 8)
            .background(backgroundView)
            .clipShape(RoundedRectangle(cornerRadius: style == .icon ? 8 : 10, style: .continuous))
            .overlay(
                RoundedRectangle(cornerRadius: style == .icon ? 8 : 10, style: .continuous)
                    .strokeBorder(borderColor, lineWidth: borderWidth)
            )
            .shadow(
                color: shadowColor,
                radius: isHovered ? 6 : 2,
                x: 0,
                y: isHovered ? 3 : 1
            )
            .scaleEffect(isPressed ? 0.96 : (isHovered ? 1.02 : 1.0))
            .animation(.spring(response: 0.2, dampingFraction: 0.8), value: isPressed)
            .animation(.easeOut(duration: 0.15), value: isHovered)
        }
        .buttonStyle(.plain)
        .onHover { isHovered = $0 }
    }
    
    private var foregroundColor: Color {
        switch style {
        case .primary: return .white
        case .secondary: return isHovered ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary
        case .tertiary: return isHovered ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary
        case .icon: return isHovered ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary
        case .danger: return isHovered ? .white : DesignSystem.Colors.accentDanger
        }
    }
    
    @ViewBuilder
    private var backgroundView: some View {
        switch style {
        case .primary:
            LinearGradient(
                colors: isHovered 
                    ? [DesignSystem.Colors.accentPrimary.opacity(0.9), DesignSystem.Colors.accentSecondary.opacity(0.9)]
                    : [DesignSystem.Colors.accentPrimary, DesignSystem.Colors.accentSecondary],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
            
        case .secondary:
            DesignSystem.Colors.backgroundTertiary
                .opacity(isHovered ? 1.0 : 0.8)
            
        case .tertiary:
            isHovered ? DesignSystem.Colors.hoverStrong : Color.clear
            
        case .icon:
            isHovered ? DesignSystem.Colors.hoverStrong : Color.clear
            
        case .danger:
            isHovered ? DesignSystem.Colors.accentDanger : DesignSystem.Colors.accentDanger.opacity(0.1)
        }
    }
    
    private var borderColor: Color {
        switch style {
        case .primary: return Color.white.opacity(0.2)
        case .secondary: return isHovered ? DesignSystem.Colors.borderStrong : DesignSystem.Colors.border
        case .tertiary: return Color.clear
        case .icon: return Color.clear
        case .danger: return isHovered ? Color.clear : DesignSystem.Colors.accentDanger.opacity(0.3)
        }
    }
    
    private var borderWidth: CGFloat {
        switch style {
        case .primary: return 1
        case .secondary: return 1
        default: return 0
        }
    }
    
    private var shadowColor: Color {
        switch style {
        case .primary: return DesignSystem.Colors.accentPrimary.opacity(isHovered ? 0.4 : 0.2)
        case .danger: return isHovered ? DesignSystem.Colors.accentDanger.opacity(0.3) : .clear
        default: return .clear
        }
    }
}

// MARK: - Premium Card Component

struct EditorCard<Content: View>: View {
    let title: String?
    let content: Content
    let elevation: Int
    
    @State private var isHovered = false
    
    init(title: String? = nil, elevation: Int = 2, @ViewBuilder content: () -> Content) {
        self.title = title
        self.elevation = elevation
        self.content = content()
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            if let title = title {
                HStack {
                    Text(title)
                        .font(.system(size: 13, weight: .semibold))
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                    
                    // Expand/collapse indicator
                    Image(systemName: "chevron.down")
                        .font(.system(size: 10, weight: .semibold))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                        .rotationEffect(.degrees(isHovered ? 0 : -90))
                        .opacity(0.6)
                }
                .padding(.horizontal, DesignSystem.Spacing.md)
                .padding(.vertical, 12)
                .background(
                    DesignSystem.Colors.backgroundSecondary
                        .overlay(
                            LinearGradient(
                                colors: [Color.white.opacity(0.03), Color.clear],
                                startPoint: .top,
                                endPoint: .bottom
                            )
                        )
                )
                
                Rectangle()
                    .fill(DesignSystem.Colors.border)
                    .frame(height: 0.5)
            }
            
            content
                .padding(DesignSystem.Spacing.cardPadding)
        }
        .background(
            RoundedRectangle(cornerRadius: 12, style: .continuous)
                .fill(DesignSystem.Colors.backgroundSecondary)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 12, style: .continuous)
                .strokeBorder(
                    LinearGradient(
                        colors: isHovered 
                            ? [Color.white.opacity(0.1), Color.white.opacity(0.05)]
                            : [Color.white.opacity(0.05), Color.clear],
                        startPoint: .top,
                        endPoint: .bottom
                    ),
                    lineWidth: 1
                )
        )
        .shadow(
            color: Color.black.opacity(isHovered ? 0.12 : 0.06),
            radius: isHovered ? 8 : 4,
            x: 0,
            y: isHovered ? 4 : 2
        )
        .scaleEffect(isHovered ? 1.005 : 1.0)
        .animation(.easeOut(duration: 0.2), value: isHovered)
        .onHover { isHovered = $0 }
    }
}

// MARK: - Premium Text Field

struct EditorTextField: View {
    let placeholder: String
    @Binding var text: String
    let icon: String?
    
    @State private var isFocused = false
    @State private var isHovered = false
    
    init(_ placeholder: String, text: Binding<String>, icon: String? = nil) {
        self.placeholder = placeholder
        self._text = text
        self.icon = icon
    }
    
    var body: some View {
        HStack(spacing: 10) {
            if let icon = icon {
                Image(systemName: icon)
                    .font(.system(size: 14, weight: .medium))
                    .foregroundColor(isFocused ? DesignSystem.Colors.accentPrimary : (isHovered ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary))
                    .animation(.easeOut(duration: 0.15), value: isFocused)
            }
            
            TextField(placeholder, text: $text, onEditingChanged: { editing in
                withAnimation(.spring(response: 0.25, dampingFraction: 0.8)) {
                    isFocused = editing
                }
            })
            .font(.system(size: 13))
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .textFieldStyle(.plain)
            
            // Clear button when text is not empty
            if !text.isEmpty && isFocused {
                Button(action: { text = "" }) {
                    Image(systemName: "xmark.circle.fill")
                        .font(.system(size: 12))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                .transition(.scale.combined(with: .opacity))
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(
            RoundedRectangle(cornerRadius: 8, style: .continuous)
                .fill(DesignSystem.Colors.backgroundTertiary)
                .overlay(
                    // Inner shadow effect
                    RoundedRectangle(cornerRadius: 8, style: .continuous)
                        .stroke(Color.black.opacity(0.05), lineWidth: 1)
                        .blur(radius: 1)
                        .offset(x: 0, y: 1)
                        .mask(RoundedRectangle(cornerRadius: 8, style: .continuous))
                )
        )
        .overlay(
            RoundedRectangle(cornerRadius: 8, style: .continuous)
                .strokeBorder(
                    isFocused 
                        ? DesignSystem.Colors.accentPrimary.opacity(0.6)
                        : (isHovered ? DesignSystem.Colors.borderStrong : Color.clear),
                    lineWidth: isFocused ? 2 : 1
                )
        )
        .shadow(
            color: isFocused ? DesignSystem.Colors.accentPrimary.opacity(0.15) : .clear,
            radius: 6,
            x: 0,
            y: 0
        )
        .animation(.easeOut(duration: 0.15), value: isHovered)
        .onHover { isHovered = $0 }
    }
}

// MARK: - Premium Search Bar

struct EditorSearchBar: View {
    @Binding var text: String
    let placeholder: String
    
    @State private var isFocused = false
    @State private var isHovered = false
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            Image(systemName: "magnifyingglass")
                .font(.system(size: DesignSystem.IconSize.small, weight: .medium))
                .foregroundColor(isFocused ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
            
            TextField(placeholder, text: $text, onEditingChanged: { editing in
                withAnimation(DesignSystem.Animation.fast) {
                    isFocused = editing
                }
            })
            .font(DesignSystem.Typography.body)
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .textFieldStyle(.plain)
            
            if !text.isEmpty {
                Button(action: { text = "" }) {
                    Image(systemName: "xmark.circle.fill")
                        .font(.system(size: DesignSystem.IconSize.small))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .transition(.scale.combined(with: .opacity))
            }
        }

        .padding(.horizontal, 8)
        .padding(.vertical, 6)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(DesignSystem.CornerRadius.large) // Pill-ish
        .overlay(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.large)
                .stroke(isFocused ? DesignSystem.Colors.borderFocus : Color.clear, lineWidth: 2)
        )
        .onHover { isHovered = $0 }
    }
    
    private var borderColor: Color {
        return .clear
    }
}

// MARK: - Premium Divider

struct EditorDivider: View {
    var body: some View {
        Rectangle()
            .fill(DesignSystem.Colors.border)
            .frame(height: 1)
    }
}

// MARK: - Premium Icon Button

struct EditorIconButton: View {
    let icon: String
    let tooltip: String
    let action: () -> Void
    
    @State private var isHovered = false
    @State private var isPressed = false
    
    var body: some View {
        Button(action: {
            withAnimation(DesignSystem.Animation.spring) {
                isPressed = true
            }
            action()
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                isPressed = false
            }
        }) {
            Image(systemName: icon)
                .font(.system(size: DesignSystem.IconSize.regular, weight: .medium))
                .foregroundColor(isHovered ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                .frame(width: 32, height: 32)
                .background(isHovered ? DesignSystem.Colors.hover : Color.clear)
                .cornerRadius(DesignSystem.CornerRadius.tight)
                .scaleEffect(isPressed ? 0.9 : 1.0)
        }
        .buttonStyle(.plain)
        .help(tooltip)
        .onHover { hovering in
            withAnimation(DesignSystem.Animation.fast) {
                isHovered = hovering
            }
        }
    }
}

// MARK: - Premium Toggle

struct EditorToggle: View {
    let label: String
    @Binding var isOn: Bool
    
    @State private var isHovered = false
    @State private var isPressed = false
    
    var body: some View {
        Button(action: {
            withAnimation(.spring(response: 0.3, dampingFraction: 0.6)) {
                isOn.toggle()
            }
        }) {
            HStack {
                Text(label)
                    .font(.system(size: 13, weight: isOn ? .medium : .regular))
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                // Premium Toggle Switch
                ZStack {
                    // Track
                    Capsule()
                        .fill(
                            isOn 
                                ? LinearGradient(
                                    colors: [DesignSystem.Colors.accentPrimary, DesignSystem.Colors.accentSecondary],
                                    startPoint: .leading,
                                    endPoint: .trailing
                                )
                                : LinearGradient(
                                    colors: [Color.gray.opacity(0.25), Color.gray.opacity(0.2)],
                                    startPoint: .leading,
                                    endPoint: .trailing
                                )
                        )
                        .frame(width: 48, height: 28)
                        .overlay(
                            Capsule()
                                .strokeBorder(
                                    isOn ? Color.white.opacity(0.2) : Color.black.opacity(0.05),
                                    lineWidth: 1
                                )
                        )
                    
                    // Inner glow when on
                    if isOn {
                        Capsule()
                            .fill(DesignSystem.Colors.accentPrimary)
                            .frame(width: 48, height: 28)
                            .blur(radius: 8)
                            .opacity(0.3)
                    }
                    
                    // Knob
                    Circle()
                        .fill(
                            LinearGradient(
                                colors: [.white, Color(white: 0.95)],
                                startPoint: .top,
                                endPoint: .bottom
                            )
                        )
                        .frame(width: 24, height: 24)
                        .overlay(
                            Circle()
                                .strokeBorder(Color.black.opacity(0.1), lineWidth: 0.5)
                        )
                        .shadow(color: .black.opacity(0.15), radius: 3, x: 0, y: 2)
                        .shadow(color: .black.opacity(0.1), radius: 1, x: 0, y: 1)
                        .offset(x: isOn ? 10 : -10)
                        .scaleEffect(isPressed ? 0.9 : 1.0)
                }
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 10)
            .background(
                RoundedRectangle(cornerRadius: 8, style: .continuous)
                    .fill(isHovered ? DesignSystem.Colors.hoverStrong : Color.clear)
            )
        }
        .buttonStyle(.plain)
        .onHover { isHovered = $0 }
        .simultaneousGesture(
            DragGesture(minimumDistance: 0)
                .onChanged { _ in isPressed = true }
                .onEnded { _ in isPressed = false }
        )
    }
}

// MARK: - Premium Numeric Field

struct EditorNumericField: View {
    let label: String
    @Binding var value: Float
    let range: ClosedRange<Float>
    let step: Float
    
    @State private var isDragging = false
    @State private var dragStartValue: Float = 0
    @State private var isHovered = false
    
    init(label: String, value: Binding<Float>, range: ClosedRange<Float> = -1000...1000, step: Float = 0.1) {
        self.label = label
        self._value = value
        self.range = range
        self.step = step
    }
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            if !label.isEmpty {
                Text(label)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .frame(width: 40, alignment: .leading)
            }
            
            TextField("", value: $value, format: .number)
                .textFieldStyle(.plain)
                .font(DesignSystem.Typography.mono)
                .multilineTextAlignment(.trailing)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .padding(.horizontal, DesignSystem.Spacing.xs)
                .padding(.vertical, 4)
                .background(isDragging ? DesignSystem.Colors.selection : DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(DesignSystem.CornerRadius.tight)
                .overlay(
                    RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                        .stroke(isHovered ? DesignSystem.Colors.border.opacity(2) : DesignSystem.Colors.border, lineWidth: 1)
                )
                .gesture(
                    DragGesture(minimumDistance: 2)
                        .onChanged { gesture in
                            if !isDragging {
                                isDragging = true
                                dragStartValue = value
                                NSCursor.resizeLeftRight.push()
                            }
                            let delta = Float(gesture.translation.width) * step
                            value = min(max(dragStartValue + delta, range.lowerBound), range.upperBound)
                        }
                        .onEnded { _ in
                            isDragging = false
                            NSCursor.pop()
                        }
                )
                .onHover { hovering in
                    isHovered = hovering
                    if hovering { NSCursor.resizeLeftRight.push() }
                    else { NSCursor.pop() }
                }
        }
    }
}

// MARK: - Premium Vector3 Field

struct EditorVector3Field: View {
    let label: String
    @Binding var x: Float
    @Binding var y: Float
    @Binding var z: Float
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.caption)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack(spacing: 4) {
                VectorComponent(label: "X", value: $x, color: DesignSystem.Colors.accentDanger)
                VectorComponent(label: "Y", value: $y, color: DesignSystem.Colors.accentSuccess)
                VectorComponent(label: "Z", value: $z, color: DesignSystem.Colors.accentPrimary)
            }
        }
    }
    
    private struct VectorComponent: View {
        let label: String
        @Binding var value: Float
        let color: Color
        
        var body: some View {
            HStack(spacing: 0) {
                Text(label)
                    .font(DesignSystem.Typography.caption)
                    .fontWeight(.bold)
                    .foregroundColor(.white)
                    .frame(width: 20)
                    .background(color)
                    .cornerRadius(DesignSystem.CornerRadius.tight, corners: [.topLeft, .bottomLeft])
                
                TextField("", value: $value, format: .number)
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.mono)
                    .multilineTextAlignment(.leading)
                    .padding(.leading, 6)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .frame(height: 20)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(DesignSystem.CornerRadius.tight, corners: [.topRight, .bottomRight])
                    .overlay(
                        RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                            .stroke(DesignSystem.Colors.border, lineWidth: 1)
                    )
            }
        }
    }
}

// MARK: - Premium Collapsible Section

struct EditorCollapsibleSection<Content: View>: View {
    let title: String
    let content: Content
    @State private var isExpanded: Bool
    @State private var isHovered = false
    
    init(_ title: String, isExpanded: Bool = true, @ViewBuilder content: () -> Content) {
        self.title = title
        self.content = content()
        self._isExpanded = State(initialValue: isExpanded)
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Button(action: {
                withAnimation(DesignSystem.Animation.spring) {
                    isExpanded.toggle()
                }
            }) {
                HStack {
                    Image(systemName: "chevron.right")
                        .font(.system(size: 10, weight: .bold))
                        .rotationEffect(.degrees(isExpanded ? 90 : 0))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text(title)
                        .font(DesignSystem.Typography.headline)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                }
                .padding(DesignSystem.Spacing.sm)
                .background(isHovered ? DesignSystem.Colors.hover : DesignSystem.Colors.backgroundSecondary)
            }
            .buttonStyle(.plain)
            .onHover { isHovered = $0 }
            
            if isExpanded {
                content
                    .padding(DesignSystem.Spacing.sm)
                    .transition(.opacity.combined(with: .move(edge: .top)))
            }
        }
        .background(DesignSystem.Colors.backgroundTertiary.opacity(0.3))
        .cornerRadius(DesignSystem.CornerRadius.regular)
        .overlay(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular)
                .stroke(DesignSystem.Colors.border, lineWidth: 1)
        )
    }
}

// MARK: - Premium Panel

struct EditorPanel<Content: View>: View {
    let title: String
    let content: Content
    
    @State private var isHovered = false
    
    init(_ title: String, @ViewBuilder content: () -> Content) {
        self.title = title
        self.content = content()
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Premium Header
            HStack {
                Text(title)
                    .font(.system(size: 13, weight: .semibold))
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                // Menu Button with hover effect
                Button(action: {}) {
                    Image(systemName: "ellipsis")
                        .font(.system(size: 12, weight: .medium))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                        .frame(width: 24, height: 24)
                        .background(
                            Circle()
                                .fill(isHovered ? DesignSystem.Colors.hoverStrong : Color.clear)
                        )
                }
                .buttonStyle(.plain)
            }
            .padding(.horizontal, 16)
            .padding(.vertical, 12)
            .background(
                DesignSystem.Colors.backgroundSecondary
                    .overlay(
                        LinearGradient(
                            colors: [Color.white.opacity(0.03), Color.clear],
                            startPoint: .top,
                            endPoint: .bottom
                        )
                    )
            )
            
            // Refined Divider
            Rectangle()
                .fill(
                    LinearGradient(
                        colors: [Color.clear, DesignSystem.Colors.border, Color.clear],
                        startPoint: .leading,
                        endPoint: .trailing
                    )
                )
                .frame(height: 0.5)
            
            // Content
            content
                .background(DesignSystem.Colors.backgroundSecondary)
        }
        .clipShape(RoundedRectangle(cornerRadius: 12, style: .continuous))
        .overlay(
            RoundedRectangle(cornerRadius: 12, style: .continuous)
                .strokeBorder(
                    LinearGradient(
                        colors: [Color.white.opacity(0.08), Color.white.opacity(0.02)],
                        startPoint: .top,
                        endPoint: .bottom
                    ),
                    lineWidth: 1
                )
        )
        .shadow(color: .black.opacity(0.08), radius: 4, x: 0, y: 2)
        .onHover { isHovered = $0 }
    }
}

// MARK: - Premium Loading Indicator

struct EditorLoadingIndicator: View {
    let message: String?
    @State private var rotation: Double = 0
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            Circle()
                .trim(from: 0, to: 0.7)
                .stroke(
                    AngularGradient(
                        colors: [DesignSystem.Colors.accentPrimary, DesignSystem.Colors.accentSecondary],
                        center: .center
                    ),
                    style: StrokeStyle(lineWidth: 4, lineCap: .round)
                )
                .frame(width: 40, height: 40)
                .rotationEffect(.degrees(rotation))
                .onAppear {
                    withAnimation(.linear(duration: 1).repeatForever(autoreverses: false)) {
                        rotation = 360
                    }
                }
            
            if let message = message {
                Text(message)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
        }
        .padding(DesignSystem.Spacing.xl)
        .background(.ultraThinMaterial)
        .cornerRadius(DesignSystem.CornerRadius.large)
        .overlay(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.large)
                .stroke(DesignSystem.Colors.border, lineWidth: 1)
        )
    }
}

// MARK: - View Rounded Corner Extension
// MARK: - View Rounded Corner Extension

struct RectCorner: OptionSet {
    let rawValue: Int
    
    static let topLeft = RectCorner(rawValue: 1 << 0)
    static let topRight = RectCorner(rawValue: 1 << 1)
    static let bottomLeft = RectCorner(rawValue: 1 << 2)
    static let bottomRight = RectCorner(rawValue: 1 << 3)
    
    static let allCorners: RectCorner = [.topLeft, .topRight, .bottomLeft, .bottomRight]
}

extension View {
    func cornerRadius(_ radius: CGFloat, corners: RectCorner) -> some View {
        clipShape(RoundedCorner(radius: radius, corners: corners))
    }
}

struct RoundedCorner: Shape {
    var radius: CGFloat = .infinity
    var corners: RectCorner = .allCorners

    func path(in rect: CGRect) -> Path {
        var path = Path()

        let tl = corners.contains(.topLeft) ? radius : 0
        let tr = corners.contains(.topRight) ? radius : 0
        let br = corners.contains(.bottomRight) ? radius : 0
        let bl = corners.contains(.bottomLeft) ? radius : 0

        path.move(to: CGPoint(x: rect.minX + tl, y: rect.minY))
        path.addLine(to: CGPoint(x: rect.maxX - tr, y: rect.minY))
        if tr != 0 {
            path.addArc(center: CGPoint(x: rect.maxX - tr, y: rect.minY + tr), radius: tr,
                        startAngle: Angle(degrees: -90), endAngle: Angle(degrees: 0), clockwise: false)
        }
        path.addLine(to: CGPoint(x: rect.maxX, y: rect.maxY - br))
        if br != 0 {
            path.addArc(center: CGPoint(x: rect.maxX - br, y: rect.maxY - br), radius: br,
                        startAngle: Angle(degrees: 0), endAngle: Angle(degrees: 90), clockwise: false)
        }
        path.addLine(to: CGPoint(x: rect.minX + bl, y: rect.maxY))
        if bl != 0 {
            path.addArc(center: CGPoint(x: rect.minX + bl, y: rect.maxY - bl), radius: bl,
                        startAngle: Angle(degrees: 90), endAngle: Angle(degrees: 180), clockwise: false)
        }
        path.addLine(to: CGPoint(x: rect.minX, y: rect.minY + tl))
        if tl != 0 {
            path.addArc(center: CGPoint(x: rect.minX + tl, y: rect.minY + tl), radius: tl,
                        startAngle: Angle(degrees: 180), endAngle: Angle(degrees: 270), clockwise: false)
        }
        path.closeSubpath()

        return path
    }
}
// MARK: - Advanced Premium Components (100+ features)

// MARK: - Progress Bar

struct EditorProgressBar: View {
    let value: Double
    let total: Double
    var showLabel: Bool = true
    var accentColor: Color = DesignSystem.Colors.accentPrimary
    
    private var progress: Double { min(max(value / total, 0), 1) }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            GeometryReader { geo in
                ZStack(alignment: .leading) {
                    // Track
                    RoundedRectangle(cornerRadius: 3)
                        .fill(DesignSystem.Colors.backgroundPrimary)
                    
                    // Fill
                    RoundedRectangle(cornerRadius: 3)
                        .fill(
                            LinearGradient(
                                colors: [accentColor, accentColor.opacity(0.8)],
                                startPoint: .leading,
                                endPoint: .trailing
                            )
                        )
                        .frame(width: geo.size.width * progress)
                        .animation(DesignSystem.Animation.spring, value: progress)
                    
                    // Shine overlay
                    RoundedRectangle(cornerRadius: 3)
                        .fill(
                            LinearGradient(
                                colors: [.white.opacity(0.2), .clear],
                                startPoint: .top,
                                endPoint: .bottom
                            )
                        )
                        .frame(width: geo.size.width * progress)
                }
            }
            .frame(height: 6)
            
            if showLabel {
                Text("\(Int(progress * 100))%")
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
        }
    }
}

// MARK: - Indeterminate Progress

struct EditorIndeterminateProgress: View {
    @State private var isAnimating = false
    var color: Color = DesignSystem.Colors.accentPrimary
    
    var body: some View {
        GeometryReader { geo in
            RoundedRectangle(cornerRadius: 3)
                .fill(DesignSystem.Colors.backgroundPrimary)
                .overlay(
                    RoundedRectangle(cornerRadius: 3)
                        .fill(
                            LinearGradient(
                                colors: [.clear, color, .clear],
                                startPoint: .leading,
                                endPoint: .trailing
                            )
                        )
                        .frame(width: geo.size.width * 0.4)
                        .offset(x: isAnimating ? geo.size.width * 0.8 : -geo.size.width * 0.2)
                )
                .clipped()
        }
        .frame(height: 4)
        .onAppear {
            withAnimation(.linear(duration: 1.2).repeatForever(autoreverses: false)) {
                isAnimating = true
            }
        }
    }
}

// MARK: - Badge

struct EditorBadge: View {
    let text: String
    var style: BadgeStyle = .default
    
    enum BadgeStyle {
        case `default`, success, warning, danger, info
        
        var backgroundColor: Color {
            switch self {
            case .default: return DesignSystem.Colors.backgroundQuaternary
            case .success: return DesignSystem.Colors.accentSuccess.opacity(0.2)
            case .warning: return DesignSystem.Colors.accentWarning.opacity(0.2)
            case .danger: return DesignSystem.Colors.accentDanger.opacity(0.2)
            case .info: return DesignSystem.Colors.accentInfo.opacity(0.2)
            }
        }
        
        var foregroundColor: Color {
            switch self {
            case .default: return DesignSystem.Colors.textSecondary
            case .success: return DesignSystem.Colors.accentSuccess
            case .warning: return DesignSystem.Colors.accentWarning
            case .danger: return DesignSystem.Colors.accentDanger
            case .info: return DesignSystem.Colors.accentInfo
            }
        }
    }
    
    var body: some View {
        Text(text)
            .font(DesignSystem.Typography.micro)
            .fontWeight(.semibold)
            .foregroundColor(style.foregroundColor)
            .padding(.horizontal, 6)
            .padding(.vertical, 2)
            .background(style.backgroundColor)
            .cornerRadius(DesignSystem.CornerRadius.tight)
    }
}

// MARK: - Toast Notification

struct EditorToast: View {
    let message: String
    let type: ToastType
    var action: (() -> Void)? = nil
    var actionLabel: String = "Undo"
    
    enum ToastType {
        case info, success, warning, error
        
        var icon: String {
            switch self {
            case .info: return "info.circle.fill"
            case .success: return "checkmark.circle.fill"
            case .warning: return "exclamationmark.triangle.fill"
            case .error: return "xmark.circle.fill"
            }
        }
        
        var color: Color {
            switch self {
            case .info: return DesignSystem.Colors.accentInfo
            case .success: return DesignSystem.Colors.accentSuccess
            case .warning: return DesignSystem.Colors.accentWarning
            case .error: return DesignSystem.Colors.accentDanger
            }
        }
    }
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            Image(systemName: type.icon)
                .font(.system(size: 18))
                .foregroundColor(type.color)
            
            Text(message)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Spacer()
            
            if let action = action {
                Button(actionLabel) {
                    action()
                }
                .font(DesignSystem.Typography.bodyBold)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
        }
        .padding(DesignSystem.Spacing.md)
        .background(DesignSystem.Colors.backgroundFloating)
        .cornerRadius(DesignSystem.CornerRadius.medium)
        .overlay(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.medium)
                .stroke(DesignSystem.Colors.border, lineWidth: 1)
        )
        .shadow(color: .black.opacity(0.2), radius: 16, x: 0, y: 8)
    }
}

// MARK: - Tooltip

struct EditorTooltip: View {
    let text: String
    
    var body: some View {
        Text(text)
            .font(DesignSystem.Typography.caption)
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .padding(.horizontal, DesignSystem.Spacing.xs)
            .padding(.vertical, DesignSystem.Spacing.xxs)
            .background(DesignSystem.Colors.backgroundFloating)
            .cornerRadius(DesignSystem.CornerRadius.tight)
            .shadow(color: .black.opacity(0.15), radius: 8, x: 0, y: 4)
    }
}

// MARK: - Color Picker Swatch

struct EditorColorPicker: View {
    @Binding var color: Color
    @State private var showPicker = false
    @State private var isHovered = false
    
    var body: some View {
        Button(action: { showPicker.toggle() }) {
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                .fill(color)
                .frame(width: 32, height: 24)
                .overlay(
                    RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                        .stroke(DesignSystem.Colors.border, lineWidth: 1)
                )
                .overlay(
                    RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                        .stroke(isHovered ? DesignSystem.Colors.borderStrong : .clear, lineWidth: 2)
                )
        }
        .buttonStyle(.plain)
        .onHover { isHovered = $0 }
        .popover(isPresented: $showPicker) {
            ColorPicker("", selection: $color)
                .labelsHidden()
                .padding()
        }
    }
}

// MARK: - Slider

struct EditorSlider: View {
    let label: String
    @Binding var value: Double
    let range: ClosedRange<Double>
    var step: Double = 0.01
    var showValue: Bool = true
    
    @State private var isDragging = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                if showValue {
                    Text(String(format: "%.2f", value))
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            
            GeometryReader { geo in
                ZStack(alignment: .leading) {
                    // Track
                    RoundedRectangle(cornerRadius: 2)
                        .fill(DesignSystem.Colors.backgroundPrimary)
                        .frame(height: 4)
                    
                    // Fill
                    RoundedRectangle(cornerRadius: 2)
                        .fill(DesignSystem.Colors.accentPrimary)
                        .frame(width: geo.size.width * normalizedValue, height: 4)
                    
                    // Thumb
                    Circle()
                        .fill(Color.white)
                        .frame(width: 16, height: 16)
                        .shadow(color: .black.opacity(0.2), radius: 2, x: 0, y: 1)
                        .offset(x: thumbOffset(in: geo.size.width))
                        .scaleEffect(isDragging ? 1.2 : 1.0)
                }
                .gesture(
                    DragGesture(minimumDistance: 0)
                        .onChanged { gesture in
                            isDragging = true
                            let newValue = gesture.location.x / geo.size.width
                            value = range.lowerBound + (range.upperBound - range.lowerBound) * max(0, min(1, newValue))
                        }
                        .onEnded { _ in
                            isDragging = false
                        }
                )
            }
            .frame(height: 20)
        }
    }
    
    private var normalizedValue: Double {
        (value - range.lowerBound) / (range.upperBound - range.lowerBound)
    }
    
    private func thumbOffset(in width: CGFloat) -> CGFloat {
        width * normalizedValue - 8
    }
}

// MARK: - Segmented Control

struct EditorSegmentedControl<T: Hashable>: View {
    let options: [T]
    @Binding var selected: T
    let label: (T) -> String
    
    var body: some View {
        HStack(spacing: 1) {
            ForEach(options, id: \.self) { option in
                SegmentButton(
                    label: label(option),
                    isSelected: selected == option
                ) {
                    withAnimation(DesignSystem.Animation.fast) {
                        selected = option
                    }
                }
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(DesignSystem.CornerRadius.regular)
        .overlay(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular)
                .stroke(DesignSystem.Colors.border, lineWidth: 1)
        )
    }
    
    private struct SegmentButton: View {
        let label: String
        let isSelected: Bool
        let action: () -> Void
        
        @State private var isHovered = false
        
        var body: some View {
            Button(action: action) {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .fontWeight(isSelected ? .semibold : .regular)
                    .foregroundColor(isSelected ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                    .padding(.horizontal, DesignSystem.Spacing.sm)
                    .padding(.vertical, DesignSystem.Spacing.xs)
                    .background(isSelected ? DesignSystem.Colors.backgroundQuaternary : (isHovered ? DesignSystem.Colors.hover : Color.clear))
            }
            .buttonStyle(.plain)
            .onHover { isHovered = $0 }
        }
    }
}

// MARK: - Tab Bar

struct EditorTabBar<T: Hashable>: View {
    let tabs: [T]
    @Binding var selected: T
    let label: (T) -> String
    let icon: (T) -> String
    
    var body: some View {
        HStack(spacing: 0) {
            ForEach(tabs, id: \.self) { tab in
                TabItem(
                    label: label(tab),
                    icon: icon(tab),
                    isSelected: selected == tab
                ) {
                    withAnimation(DesignSystem.Animation.spring) {
                        selected = tab
                    }
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
    
    private struct TabItem: View {
        let label: String
        let icon: String
        let isSelected: Bool
        let action: () -> Void
        
        @State private var isHovered = false
        
        var body: some View {
            Button(action: action) {
                VStack(spacing: 4) {
                    Image(systemName: icon)
                        .font(.system(size: 16))
                    Text(label)
                        .font(DesignSystem.Typography.caption)
                }
                .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                .frame(maxWidth: .infinity)
                .padding(.vertical, DesignSystem.Spacing.xs)
                .background(isHovered ? DesignSystem.Colors.hover : Color.clear)
                .overlay(
                    Rectangle()
                        .fill(isSelected ? DesignSystem.Colors.accentPrimary : Color.clear)
                        .frame(height: 2),
                    alignment: .bottom
                )
            }
            .buttonStyle(.plain)
            .onHover { isHovered = $0 }
        }
    }
}

// MARK: - Stepper

struct EditorStepper: View {
    let label: String
    @Binding var value: Int
    let range: ClosedRange<Int>
    
    var body: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.caption)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            HStack(spacing: 0) {
                StepperButton(icon: "minus") {
                    if value > range.lowerBound {
                        value -= 1
                    }
                }
                
                Text("\(value)")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .frame(minWidth: 32)
                
                StepperButton(icon: "plus") {
                    if value < range.upperBound {
                        value += 1
                    }
                }
            }
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(DesignSystem.CornerRadius.tight)
            .overlay(
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                    .stroke(DesignSystem.Colors.border, lineWidth: 1)
            )
        }
    }
    
    private struct StepperButton: View {
        let icon: String
        let action: () -> Void
        
        @State private var isHovered = false
        
        var body: some View {
            Button(action: action) {
                Image(systemName: icon)
                    .font(.system(size: 10, weight: .bold))
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .frame(width: 24, height: 24)
                    .background(isHovered ? DesignSystem.Colors.hover : Color.clear)
            }
            .buttonStyle(.plain)
            .onHover { isHovered = $0 }
        }
    }
}

// MARK: - Context Menu Item

struct EditorContextMenuItem: View {
    let title: String
    let icon: String?
    let shortcut: String?
    let isDestructive: Bool
    let action: () -> Void
    
    init(_ title: String, icon: String? = nil, shortcut: String? = nil, isDestructive: Bool = false, action: @escaping () -> Void) {
        self.title = title
        self.icon = icon
        self.shortcut = shortcut
        self.isDestructive = isDestructive
        self.action = action
    }
    
    @State private var isHovered = false
    
    var body: some View {
        Button(action: action) {
            HStack(spacing: DesignSystem.Spacing.sm) {
                if let icon = icon {
                    Image(systemName: icon)
                        .font(.system(size: 14))
                        .foregroundColor(iconColor)
                        .frame(width: 20)
                }
                
                Text(title)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(textColor)
                
                Spacer()
                
                if let shortcut = shortcut {
                    Text(shortcut)
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
            }
            .padding(.horizontal, DesignSystem.Spacing.sm)
            .padding(.vertical, DesignSystem.Spacing.xs)
            .background(isHovered ? DesignSystem.Colors.accentPrimary : Color.clear)
            .cornerRadius(DesignSystem.CornerRadius.tight)
        }
        .buttonStyle(.plain)
        .onHover { isHovered = $0 }
    }
    
    private var textColor: Color {
        if isHovered { return .white }
        return isDestructive ? DesignSystem.Colors.accentDanger : DesignSystem.Colors.textPrimary
    }
    
    private var iconColor: Color {
        if isHovered { return .white }
        return isDestructive ? DesignSystem.Colors.accentDanger : DesignSystem.Colors.textSecondary
    }
}

// MARK: - Breadcrumb

struct EditorBreadcrumb: View {
    let items: [String]
    let onTap: (Int) -> Void
    
    var body: some View {
        HStack(spacing: 4) {
            ForEach(Array(items.enumerated()), id: \.offset) { index, item in
                if index > 0 {
                    Image(systemName: "chevron.right")
                        .font(.system(size: 8, weight: .bold))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                BreadcrumbItem(
                    text: item,
                    isLast: index == items.count - 1
                ) {
                    onTap(index)
                }
            }
        }
    }
    
    private struct BreadcrumbItem: View {
        let text: String
        let isLast: Bool
        let action: () -> Void
        
        @State private var isHovered = false
        
        var body: some View {
            Button(action: action) {
                Text(text)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(isLast ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                    .underline(isHovered && !isLast)
            }
            .buttonStyle(.plain)
            .onHover { isHovered = $0 }
            .disabled(isLast)
        }
    }
}

// MARK: - Empty State

struct EditorEmptyState: View {
    let icon: String
    let title: String
    let message: String
    var actionLabel: String? = nil
    var action: (() -> Void)? = nil
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Image(systemName: icon)
                .font(.system(size: 48, weight: .light))
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            VStack(spacing: DesignSystem.Spacing.xs) {
                Text(title)
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text(message)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .multilineTextAlignment(.center)
            }
            
            if let actionLabel = actionLabel, let action = action {
                EditorButton(actionLabel, icon: "plus", style: .primary, action: action)
            }
        }
        .padding(DesignSystem.Spacing.xl)
    }
}

// MARK: - Keyboard Shortcut Display

struct EditorKeyboardShortcut: View {
    let keys: [String]
    
    var body: some View {
        HStack(spacing: 2) {
            ForEach(Array(keys.enumerated()), id: \.offset) { _, key in
                Text(key)
                    .font(DesignSystem.Typography.micro)
                    .fontWeight(.medium)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .padding(.horizontal, 4)
                    .padding(.vertical, 2)
                    .background(DesignSystem.Colors.backgroundQuaternary)
                    .cornerRadius(3)
            }
        }
    }
}

// MARK: - Avatar / Icon Circle

struct EditorAvatar: View {
    let initials: String
    var color: Color = DesignSystem.Colors.accentPrimary
    var size: CGFloat = 32
    
    var body: some View {
        Text(initials.prefix(2).uppercased())
            .font(.system(size: size * 0.4, weight: .semibold))
            .foregroundColor(.white)
            .frame(width: size, height: size)
            .background(color)
            .clipShape(Circle())
    }
}

// MARK: - Status Indicator

struct EditorStatusIndicator: View {
    let status: Status
    
    enum Status {
        case online, offline, busy, away
        
        var color: Color {
            switch self {
            case .online: return DesignSystem.Colors.accentSuccess
            case .offline: return DesignSystem.Colors.textTertiary
            case .busy: return DesignSystem.Colors.accentDanger
            case .away: return DesignSystem.Colors.accentWarning
            }
        }
    }
    
    var body: some View {
        Circle()
            .fill(status.color)
            .frame(width: 8, height: 8)
            .overlay(
                Circle()
                    .stroke(DesignSystem.Colors.backgroundSecondary, lineWidth: 2)
            )
    }
}

// MARK: - Resizable Handle

struct EditorResizeHandle: View {
    var axis: Axis = .horizontal
    @State private var isHovered = false
    
    var body: some View {
        Group {
            if axis == .horizontal {
                Rectangle()
                    .fill(isHovered ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border)
                    .frame(width: 4)
            } else {
                Rectangle()
                    .fill(isHovered ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border)
                    .frame(height: 4)
            }
        }
        .contentShape(Rectangle())
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

// MARK: - Spinner

struct EditorSpinner: View {
    var size: CGFloat = 20
    var color: Color = DesignSystem.Colors.accentPrimary
    @State private var rotation: Double = 0
    
    var body: some View {
        Circle()
            .trim(from: 0, to: 0.7)
            .stroke(color, style: StrokeStyle(lineWidth: size / 8, lineCap: .round))
            .frame(width: size, height: size)
            .rotationEffect(.degrees(rotation))
            .onAppear {
                withAnimation(.linear(duration: 0.8).repeatForever(autoreverses: false)) {
                    rotation = 360
                }
            }
    }
}
