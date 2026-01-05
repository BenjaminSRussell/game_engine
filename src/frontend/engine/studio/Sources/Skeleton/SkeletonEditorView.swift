import SwiftUI
import simd

// MARK: - Skeleton Editor View
/// Main skeleton editing interface for character rigging

struct SkeletonEditorView: View {
    @StateObject private var skeletonManager = SkeletonManager.shared
    @State private var selectedTool: SkeletonTool = .select
   @State private var showHierarchy = true
    @State private var showProperties = true
    @State private var showHelp = false  // Help panel toggle
    @State private var viewMode: ViewMode = .bones
    
    enum SkeletonTool: String, CaseIterable {
        case select = "Select"
        case addBone = "Add Bone"
        case addChain = "Add Chain"
        case mirror = "Mirror"
        case rotate = "Rotate"
        case scale = "Scale"
    }
    
    enum ViewMode: String, CaseIterable {
        case bones = "Bones"
        case mesh = "Mesh"
        case both = "Both"
        case xray = "X-Ray"
    }
    
    var body: some View {
        ZStack(alignment: .trailing) {
            // Main Editor
            HSplitView {
                // Left: Bone Hierarchy
                if showHierarchy {
                    BoneHierarchyPanel(skeleton: skeletonManager.activeSkeleton)
                        .frame(minWidth: 200, maxWidth: 280)
                }
                
                // Center: Viewport
                VStack(spacing: 0) {
                    SkeletonToolbar(
                        selectedTool: $selectedTool,
                        viewMode: $viewMode,
                        showHierarchy: $showHierarchy,
                        showProperties: $showProperties,
                        showHelp: $showHelp
                    )
                    
                    EditorDivider()
                    
                    SkeletonViewport(
                        skeleton: skeletonManager.activeSkeleton,
                        selectedTool: selectedTool,
                        viewMode: viewMode
                    )
                }
                
                // Right: Properties
                if showProperties {
                    BonePropertiesPanel(skeleton: skeletonManager.activeSkeleton)
                        .frame(minWidth: 240, maxWidth: 320)
                }
            }
            .background(DesignSystem.Colors.backgroundPrimary)
            .disabled(showHelp)  // Disable interaction when help is open
            .blur(radius: showHelp ? 3 : 0)
            .animation(.easeInOut(duration: 0.2), value: showHelp)
            
            // Help Panel Overlay
            if showHelp {
                SkeletonHelpPanel()
                    .frame(maxWidth: 600)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .shadow(color: Color.black.opacity(0.3), radius: 20, x: -5, y: 0)
                    .transition(.move(edge: .trailing))
            }
        }
        .onAppear {
            if skeletonManager.activeSkeleton == nil {
                createDefaultSkeleton()
            }
        }
    }
    
    private func createDefaultSkeleton() {
        // Use humanoid preset for default skeleton
        _ = skeletonManager.createSkeleton(preset: .humanoid)
    }
}

// MARK: - Skeleton Toolbar

struct SkeletonToolbar: View {
    @Binding var selectedTool: SkeletonEditorView.SkeletonTool
    @Binding var viewMode: SkeletonEditorView.ViewMode
    @Binding var showHierarchy: Bool
    @Binding var showProperties: Bool
    @Binding var showHelp: Bool
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            // Tool buttons
            ForEach(SkeletonEditorView.SkeletonTool.allCases, id: \.self) { tool in
                ToolbarButton(
                    icon: iconForTool(tool),
                    isSelected: selectedTool == tool,
                    tooltip: tool.rawValue
                ) {
                    selectedTool = tool
                }
            }
            
            EditorDivider()
                .frame(height: 20)
            
            // View mode picker
            Picker("View", selection: $viewMode) {
                ForEach(SkeletonEditorView.ViewMode.allCases, id: \.self) { mode in
                    Text(mode.rawValue).tag(mode)
                }
            }
            .pickerStyle(.segmented)
            .frame(width: 200)
            
            Spacer()
            
            // Panel toggles
            Toggle(isOn: $showHierarchy) {
                Image(systemName: "list.bullet.indent")
            }
            .toggleStyle(.button)
            .help("Toggle Hierarchy Panel - Show/hide bone tree view")
            
            Toggle(isOn: $showProperties) {
                Image(systemName: "slider.horizontal.3")
            }
            .toggleStyle(.button)
            .help("Toggle Properties Panel - Show/hide bone property editor")
            
            // Help toggle button - CLEARLY LABELED
            Toggle(isOn: $showHelp) {
                Label("Help & Guide", systemImage: "questionmark.circle.fill")
                    .font(DesignSystem.Typography.body)
            }
            .toggleStyle(.button)
            .help("""
            Help & Guide
            
            Opens comprehensive documentation including:
            • Step-by-step building instructions
            • Tool reference with usage examples
            • Boat skeleton component details
            • Keyboard shortcuts
            • Best practices and tips
            """)
            
            EditorDivider()
                .frame(height: 20)
            
            // Preset Selector - CLEARLY LABELED
            Menu {
                Text("Create New Skeleton")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Divider()
                
                // Characters Category
                ForEach(SkeletonPreset.allCases.filter { $0.category == .character }) { preset in
                    Button {
                        createSkeletonFromPreset(preset)
                    } label: {
                        Label {
                            VStack(alignment: .leading, spacing: 2) {
                                Text(preset.rawValue)
                                    .font(DesignSystem.Typography.body)
                                Text(preset.description)
                                    .font(DesignSystem.Typography.caption)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                Text("\(preset.boneCount) bones")
                                    .font(DesignSystem.Typography.micro)
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                            }
                        } icon: {
                            Image(systemName: preset.icon)
                        }
                    }
                }
                
                Divider()
                
                // Vehicles Category
                Text("Boats & Vehicles")
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                ForEach(SkeletonPreset.allCases.filter { $0.category == .vehicle }) { preset in
                    Button {
                        createSkeletonFromPreset(preset)
                    } label: {
                        Label {
                            VStack(alignment: .leading, spacing: 2) {
                                Text(preset.rawValue)
                                    .font(DesignSystem.Typography.body)
                                Text(preset.description)
                                    .font(DesignSystem.Typography.caption)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                Text("\(preset.boneCount) bones")
                                    .font(DesignSystem.Typography.micro)
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                            }
                        } icon: {
                            Image(systemName: preset.icon)
                        }
                    }
                }
                
                Divider()
                
                // Custom/Empty
                ForEach(SkeletonPreset.allCases.filter { $0.category == .custom }) { preset in
                    Button {
                        createSkeletonFromPreset(preset)
                    } label: {
                        Label {
                            VStack(alignment: .leading, spacing: 2) {
                                Text(preset.rawValue)
                                    .font(DesignSystem.Typography.body)
                                Text(preset.description)
                                    .font(DesignSystem.Typography.caption)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                            }
                        } icon: {
                            Image(systemName: preset.icon)
                        }
                    }
                }
            } label: {
                Label("New Skeleton", systemImage: "plus.square.on.square")
                    .font(DesignSystem.Typography.body)
            }
            .buttonStyle(.bordered)
            .help("""
            Create New Skeleton
            
            Choose from presets:
            • Humanoid Character (19 bones)
            • Sailboat (6 bones) - Hull, Mast, Boom, Sails, Rudder
            • Motorboat (4 bones) - Hull, Propeller, Rudder, Motor
            • Empty Skeleton (build from scratch)
            """)
            
            // Actions
            Button {
                SkeletonManager.shared.activeSkeleton?.captureBindPose()
            } label: {
                Label("Set Bind Pose", systemImage: "figure.stand")
            }
            .buttonStyle(.bordered)
            .help("""
            Set Bind Pose
            
            Captures the current skeleton pose as the reference pose for mesh skinning.
            This should be done after positioning all bones in T-pose or neutral stance.
            """)
        }
        .padding(.horizontal, DesignSystem.Spacing.md)
        .padding(.vertical, DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundSecondary)
    }
    
    private func createSkeletonFromPreset(_ preset: SkeletonPreset) {
        _ = SkeletonManager.shared.createSkeleton(preset: preset)
    }
    
    private func iconForTool(_ tool: SkeletonEditorView.SkeletonTool) -> String {
        switch tool {
        case .select: return "cursorarrow"
        case .addBone: return "plus.circle"
        case .addChain: return "link"
        case .mirror: return "arrow.left.and.right"
        case .rotate: return "rotate.3d"
        case .scale: return "arrow.up.left.and.arrow.down.right"
        }
    }
}

// MARK: - Toolbar Button

struct ToolbarButton: View {
    let icon: String
    var isSelected: Bool = false
    var tooltip: String = ""
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            Image(systemName: icon)
                .font(.system(size: 14))
                .foregroundColor(isSelected ? .white : DesignSystem.Colors.textSecondary)
                .frame(width: 28, height: 28)
                .background(isSelected ? DesignSystem.Colors.accentPrimary : Color.clear)
                .cornerRadius(DesignSystem.CornerRadius.small)
        }
        .buttonStyle(.plain)
        .help(tooltip)
    }
}

// MARK: - Skeleton Viewport

struct SkeletonViewport: View {
    var skeleton: Skeleton?
    let selectedTool: SkeletonEditorView.SkeletonTool
    let viewMode: SkeletonEditorView.ViewMode
    
    @State private var cameraRotation: SIMD2<Float> = .zero
    @State private var cameraZoom: Float = 5.0
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Background grid
                SkeletonGridBackground()
                
                // Bone visualization
                if let skeleton = skeleton {
                    SkeletonBoneRenderer(
                        skeleton: skeleton,
                        viewMode: viewMode,
                        size: geometry.size
                    )
                }
                
                // Overlay info
                VStack {
                    HStack {
                        Spacer()
                        VStack(alignment: .trailing, spacing: 4) {
                            Text("Bones: \(skeleton?.bones.count ?? 0)")
                            Text("Selected: \(skeleton?.selectedBoneIDs.count ?? 0)")
                        }
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                        .padding(DesignSystem.Spacing.sm)
                    }
                    Spacer()
                }
                
                // Empty state
                if skeleton == nil || skeleton?.bones.isEmpty == true {
                    VStack(spacing: DesignSystem.Spacing.md) {
                        Image(systemName: "figure.stand")
                            .font(.system(size: 48))
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        Text("No skeleton loaded")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        Text("Create a new skeleton or import one")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                }
            }
            .background(Color.black.opacity(0.3))
            .cornerRadius(DesignSystem.CornerRadius.regular)
            .padding(DesignSystem.Spacing.sm)
        }
    }
}

// MARK: - Skeleton Grid Background

struct SkeletonGridBackground: View {
    var body: some View {
        Canvas { context, size in
            let gridSize: CGFloat = 40
            let centerX = size.width / 2
            let centerY = size.height / 2
            
            // Minor grid lines
            context.stroke(
                Path { path in
                    for x in stride(from: centerX.truncatingRemainder(dividingBy: gridSize), to: size.width, by: gridSize) {
                        path.move(to: CGPoint(x: x, y: 0))
                        path.addLine(to: CGPoint(x: x, y: size.height))
                    }
                    for y in stride(from: centerY.truncatingRemainder(dividingBy: gridSize), to: size.height, by: gridSize) {
                        path.move(to: CGPoint(x: 0, y: y))
                        path.addLine(to: CGPoint(x: size.width, y: y))
                    }
                },
                with: .color(Color.white.opacity(0.05)),
                lineWidth: 1
            )
            
            // Center lines
            context.stroke(
                Path { path in
                    path.move(to: CGPoint(x: centerX, y: 0))
                    path.addLine(to: CGPoint(x: centerX, y: size.height))
                    path.move(to: CGPoint(x: 0, y: centerY))
                    path.addLine(to: CGPoint(x: size.width, y: centerY))
                },
                with: .color(Color.white.opacity(0.15)),
                lineWidth: 1
            )
        }
    }
}

// MARK: - Skeleton Bone Renderer

struct SkeletonBoneRenderer: View {
    @ObservedObject var skeleton: Skeleton
    let viewMode: SkeletonEditorView.ViewMode
    let size: CGSize
    
    var body: some View {
        Canvas { context, canvasSize in
            let center = CGPoint(x: canvasSize.width / 2, y: canvasSize.height / 2)
            let scale: CGFloat = 100  // Pixels per unit
            
            // Draw bones depth-first
            for boneID in skeleton.allBoneIDsDepthFirst() {
                guard let bone = skeleton.bones[boneID] else { continue }
                
                let headPos = skeleton.boneWorldPosition(boneID)
                let tailPos = skeleton.boneTailWorldPosition(boneID)
                
                // Simple orthographic projection (front view)
                let headScreen = CGPoint(
                    x: center.x + CGFloat(headPos.x) * scale,
                    y: center.y - CGFloat(headPos.y) * scale
                )
                let tailScreen = CGPoint(
                    x: center.x + CGFloat(tailPos.x) * scale,
                    y: center.y - CGFloat(tailPos.y) * scale
                )
                
                let isSelected = skeleton.selectedBoneIDs.contains(boneID)
                let boneColor = isSelected ? Color.yellow : bone.color
                
                // Draw bone shape (octahedron-style)
                drawBoneShape(context: context, from: headScreen, to: tailScreen, color: boneColor, isSelected: isSelected)
                
                // Draw joint
                let jointPath = Path(ellipseIn: CGRect(
                    x: headScreen.x - 5,
                    y: headScreen.y - 5,
                    width: 10,
                    height: 10
                ))
                context.fill(jointPath, with: .color(boneColor))
                
                if isSelected {
                    context.stroke(jointPath, with: .color(.white), lineWidth: 2)
                }
            }
        }
        .gesture(
            TapGesture()
                .onEnded { _ in
                    // TODO: Implement bone picking
                }
        )
    }
    
    private func drawBoneShape(context: GraphicsContext, from: CGPoint, to: CGPoint, color: Color, isSelected: Bool) {
        let dx = to.x - from.x
        let dy = to.y - from.y
        let length = sqrt(dx * dx + dy * dy)
        
        guard length > 0 else { return }
        
        // Perpendicular vector
        let perpX = -dy / length * 4
        let perpY = dx / length * 4
        
        // Bone midpoint (slightly closer to head for octahedron look)
        let midX = from.x + dx * 0.15
        let midY = from.y + dy * 0.15
        
        let bonePath = Path { path in
            path.move(to: from)
            path.addLine(to: CGPoint(x: midX + perpX, y: midY + perpY))
            path.addLine(to: to)
            path.addLine(to: CGPoint(x: midX - perpX, y: midY - perpY))
            path.closeSubpath()
        }
        
        context.fill(bonePath, with: .color(color.opacity(0.6)))
        context.stroke(bonePath, with: .color(isSelected ? .white : color), lineWidth: isSelected ? 2 : 1)
    }
}

// MARK: - Bone Hierarchy Panel

struct BoneHierarchyPanel: View {
    var skeleton: Skeleton?
    @State private var searchText = ""
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Bone Hierarchy")
                    .font(DesignSystem.Typography.headline)
                Spacer()
                Button {
                    // Add bone at root
                    skeleton?.addBone(name: "New Bone", length: 0.5)
                } label: {
                    Image(systemName: "plus")
                }
                .buttonStyle(.plain)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Search
            TextField("Search bones...", text: $searchText)
                .textFieldStyle(.roundedBorder)
                .padding(DesignSystem.Spacing.sm)
            
            // Tree view
            if let skeleton = skeleton {
                ScrollView {
                    LazyVStack(spacing: 0) {
                        if let rootID = skeleton.rootBoneID {
                            BoneTreeItem(skeleton: skeleton, boneID: rootID, depth: 0, searchText: searchText)
                        }
                    }
                    .padding(.horizontal, DesignSystem.Spacing.xs)
                }
            } else {
                Spacer()
                Text("No skeleton")
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                Spacer()
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Bone Tree Item

struct BoneTreeItem: View {
    @ObservedObject var skeleton: Skeleton
    let boneID: UUID
    let depth: Int
    let searchText: String
    
    @State private var isExpanded = true
    
    var bone: BoneNode? {
        skeleton.bones[boneID]
    }
    
    var matchesSearch: Bool {
        searchText.isEmpty || (bone?.name.localizedCaseInsensitiveContains(searchText) ?? false)
    }
    
    var body: some View {
        if let bone = bone, matchesSearch {
            VStack(spacing: 0) {
                HStack(spacing: DesignSystem.Spacing.xs) {
                    // Indent
                    Spacer()
                        .frame(width: CGFloat(depth) * 16)
                    
                    // Expand/collapse button
                    if !bone.childIDs.isEmpty {
                        Button {
                            withAnimation(DesignSystem.Animation.fast) {
                                isExpanded.toggle()
                            }
                        } label: {
                            Image(systemName: isExpanded ? "chevron.down" : "chevron.right")
                                .font(.system(size: 10))
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                        }
                        .buttonStyle(.plain)
                        .frame(width: 16)
                    } else {
                        Spacer().frame(width: 16)
                    }
                    
                    // Bone icon
                    Circle()
                        .fill(bone.color)
                        .frame(width: 8, height: 8)
                    
                    // Name
                    Text(bone.name)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(skeleton.selectedBoneIDs.contains(boneID) ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                    
                    // Visibility toggle
                    Button {
                        var updatedBone = bone
                        updatedBone.isVisible.toggle()
                        skeleton.bones[boneID] = updatedBone
                    } label: {
                        Image(systemName: bone.isVisible ? "eye" : "eye.slash")
                            .font(.system(size: 11))
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                }
                .padding(.vertical, DesignSystem.Spacing.xs)
                .padding(.horizontal, DesignSystem.Spacing.sm)
                .background(skeleton.selectedBoneIDs.contains(boneID) ? DesignSystem.Colors.selection : Color.clear)
                .cornerRadius(DesignSystem.CornerRadius.small)
                .contentShape(Rectangle())
                .onTapGesture {
                    skeleton.selectBone(boneID, additive: NSEvent.modifierFlags.contains(.shift))
                }
                
                // Children
                if isExpanded {
                    ForEach(bone.childIDs, id: \.self) { childID in
                        BoneTreeItem(skeleton: skeleton, boneID: childID, depth: depth + 1, searchText: searchText)
                    }
                }
            }
        }
    }
}

// MARK: - Bone Properties Panel

struct BonePropertiesPanel: View {
    var skeleton: Skeleton?
    
    var selectedBone: BoneNode? {
        guard let skeleton = skeleton,
              let boneID = skeleton.selectedBoneIDs.first else {
            return nil
        }
        return skeleton.bones[boneID]
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Bone Properties")
                    .font(DesignSystem.Typography.headline)
                Spacer()
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            if let bone = selectedBone, let skeleton = skeleton {
                ScrollView {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                        // Name
                        SkeletonPropertySection(title: "General") {
                            SkeletonPropertyRow(label: "Name") {
                                TextField("Name", text: Binding(
                                    get: { bone.name },
                                    set: { newValue in
                                        var updated = bone
                                        updated.name = newValue
                                        skeleton.bones[bone.id] = updated
                                    }
                                ))
                                .textFieldStyle(.roundedBorder)
                            }
                            
                            SkeletonPropertyRow(label: "Length") {
                                HStack {
                                    Slider(value: Binding(
                                        get: { Double(bone.length) },
                                        set: { newValue in
                                            var updated = bone
                                            updated.length = Float(newValue)
                                            skeleton.bones[bone.id] = updated
                                            skeleton.updateWorldTransforms()
                                        }
                                    ), in: 0.01...2.0)
                                    Text(String(format: "%.2f", bone.length))
                                        .font(DesignSystem.Typography.caption)
                                        .frame(width: 40)
                                }
                            }
                        }
                        
                        // Transform
                        SkeletonPropertySection(title: "Local Transform") {
                            SkeletonVector3Field(
                                label: "Position",
                                value: Binding(
                                    get: { bone.localPosition },
                                    set: { newValue in
                                        var updated = bone
                                        updated.localPosition = newValue
                                        skeleton.bones[bone.id] = updated
                                        skeleton.updateWorldTransforms()
                                    }
                                )
                            )
                            
                            SkeletonPropertyRow(label: "Roll") {
                                HStack {
                                    Slider(value: Binding(
                                        get: { Double(bone.roll) },
                                        set: { newValue in
                                            var updated = bone
                                            updated.roll = Float(newValue)
                                            skeleton.bones[bone.id] = updated
                                        }
                                    ), in: -Double.pi...Double.pi)
                                    Text(String(format: "%.1f°", bone.roll * 180 / Float.pi))
                                        .font(DesignSystem.Typography.caption)
                                        .frame(width: 50)
                                }
                            }
                        }
                        
                        // Display
                        SkeletonPropertySection(title: "Display") {
                            SkeletonPropertyRow(label: "Color") {
                                ColorPicker("", selection: Binding(
                                    get: { bone.color },
                                    set: { newValue in
                                        var updated = bone
                                        updated.color = newValue
                                        skeleton.bones[bone.id] = updated
                                    }
                                ))
                            }
                            
                            SkeletonPropertyRow(label: "Visible") {
                                Toggle("", isOn: Binding(
                                    get: { bone.isVisible },
                                    set: { newValue in
                                        var updated = bone
                                        updated.isVisible = newValue
                                        skeleton.bones[bone.id] = updated
                                    }
                                ))
                            }
                            
                            SkeletonPropertyRow(label: "Locked") {
                                Toggle("", isOn: Binding(
                                    get: { bone.isLocked },
                                    set: { newValue in
                                        var updated = bone
                                        updated.isLocked = newValue
                                        skeleton.bones[bone.id] = updated
                                    }
                                ))
                            }
                        }
                        
                        Spacer()
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
            } else {
                Spacer()
                VStack(spacing: DesignSystem.Spacing.sm) {
                    Image(systemName: "hand.point.up.left")
                        .font(.system(size: 32))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    Text("Select a bone")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                Spacer()
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Skeleton Property Section

struct SkeletonPropertySection<Content: View>: View {
    let title: String
    @ViewBuilder let content: Content
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            Text(title)
                .font(DesignSystem.Typography.caption)
                .foregroundColor(DesignSystem.Colors.textTertiary)
                .textCase(.uppercase)
            
            content
        }
    }
}

// MARK: - Skeleton Property Row

struct SkeletonPropertyRow<Content: View>: View {
    let label: String
    @ViewBuilder let content: Content
    
    var body: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 70, alignment: .leading)
            
            content
        }
    }
}

// MARK: - Vector3 Field

struct SkeletonVector3Field: View {
    let label: String
    @Binding var value: SIMD3<Float>
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
            Text(label)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack(spacing: DesignSystem.Spacing.xs) {
                SkeletonFloatField(label: "X", value: Binding(
                    get: { value.x },
                    set: { value.x = $0 }
                ), color: .red)
                
                SkeletonFloatField(label: "Y", value: Binding(
                    get: { value.y },
                    set: { value.y = $0 }
                ), color: .green)
                
                SkeletonFloatField(label: "Z", value: Binding(
                    get: { value.z },
                    set: { value.z = $0 }
                ), color: .blue)
            }
        }
    }
}

// MARK: - Float Field

struct SkeletonFloatField: View {
    let label: String
    @Binding var value: Float
    var color: Color = .gray
    
    var body: some View {
        HStack(spacing: 2) {
            Text(label)
                .font(DesignSystem.Typography.micro)
                .foregroundColor(color)
                .frame(width: 12)
            
            TextField("", value: $value, format: .number.precision(.fractionLength(2)))
                .textFieldStyle(.roundedBorder)
                .font(DesignSystem.Typography.caption)
        }
    }
}
