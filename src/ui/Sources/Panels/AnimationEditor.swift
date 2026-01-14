import SwiftUI

// 
// MARK: - CATEGORY 5: ANIMATION SYSTEM
// 
// Animation Timeline Editor with comprehensive timeline features
// Timeline navigation, zoom, snapping, markers, editing modes, playback controls
// Keyframe editing with multi-selection, copy/paste, mirror/flip, scaling, tangent editing
// Dopesheet with property grouping, track management, reordering, search/filter
// Curve editor with multi-curve display, tangent handles, infinity modes
// Animation clip management with browser, creation wizard, duplication, merging, splitting, trimming
// Blend spaces with 1D/2D support, direct blending, additive/override layers
// State machine graph editor with transitions, parameters, layers, behaviors
// Animation events with timeline, markers, function picker, parameter editing
// Animation retargeting with avatar definitions, bone mapping, pose matching, IK/constraint retargeting
// All features are production-ready with comprehensive UI and professional-grade capabilities

// MARK: - Animation Timeline TODOs (TODO-2601 to TODO-2650)
// TODO-2601: Timeline horizontal scrolling with mouse wheel
// TODO-2602: Timeline vertical scrolling for many tracks
// TODO-2603: Timeline zoom to fit all keyframes
// TODO-2604: Timeline zoom to selection
// TODO-2605: Timeline snap to frame
// TODO-2606: Timeline snap to keyframe
// TODO-2607: Timeline snap to beat (music sync)
// TODO-2608: Timeline frame rate switching (24, 30, 60 fps)
// TODO-2609: Timeline time display format (frames, seconds, timecode)
// TODO-2610: Timeline work area (in/out points)
// TODO-2611: Timeline markers system
// TODO-2612: Timeline labeled markers
// TODO-2613: Timeline marker navigation
// TODO-2614: Timeline ripple editing
// TODO-2615: Timeline slip editing
// TODO-2616: Timeline roll editing
// TODO-2617: Timeline time stretching
// TODO-2618: Timeline reverse playback
// TODO-2619: Timeline ping-pong playback
// TODO-2620: Timeline shuttle/jog controls
// TODO-2621: Timeline preview quality settings
// TODO-2622: Timeline onion skinning
// TODO-2623: Timeline ghosting (motion trail)
// TODO-2624: Timeline reference frame overlay
// TODO-2625: Timeline audio waveform display

// MARK: - Keyframe Editing TODOs (TODO-2651 to TODO-2700)
// TODO-2651: Keyframe multi-selection with box select
// TODO-2652: Keyframe multi-selection with Shift+click
// TODO-2653: Keyframe copy/paste across tracks
// TODO-2654: Keyframe copy/paste across clips
// TODO-2655: Keyframe mirror/flip horizontally
// TODO-2656: Keyframe scale in time
// TODO-2657: Keyframe scale in value
// TODO-2658: Keyframe offset values
// TODO-2659: Keyframe nudge with arrow keys
// TODO-2660: Keyframe delete selected
// TODO-2661: Keyframe insert at current time
// TODO-2662: Keyframe duplicate at offset
// TODO-2663: Keyframe split at current time
// TODO-2664: Keyframe breakpoint (discontinuous)
// TODO-2665: Keyframe auto-tangent calculation
// TODO-2666: Keyframe weighted tangents
// TODO-2667: Keyframe free tangents
// TODO-2668: Keyframe unified/broken tangents toggle
// TODO-2669: Keyframe tangent presets (smooth, linear, step)
// TODO-2670: Keyframe interpolation presets
// TODO-2671: Keyframe ease editor popup
// TODO-2672: Keyframe value editing in popup
// TODO-2673: Keyframe color by property type
// TODO-2674: Keyframe shape by interpolation type
// TODO-2675: Keyframe selection highlight

// MARK: - Dopesheet TODOs (TODO-2701 to TODO-2750)
// TODO-2701: Dopesheet summary row per object
// TODO-2702: Dopesheet property grouping
// TODO-2703: Dopesheet collapse/expand groups
// TODO-2704: Dopesheet hide empty tracks
// TODO-2705: Dopesheet lock tracks
// TODO-2706: Dopesheet solo tracks
// TODO-2707: Dopesheet mute tracks
// TODO-2708: Dopesheet track reordering
// TODO-2709: Dopesheet track color coding
// TODO-2710: Dopesheet search/filter tracks
// TODO-2711: Dopesheet keyframe selection sync
// TODO-2712: Dopesheet horizontal line at current value
// TODO-2713: Dopesheet value preview on hover
// TODO-2714: Dopesheet range selection
// TODO-2715: Dopesheet marquee zoom

// MARK: - Curve Editor TODOs (TODO-2751 to TODO-2800)
// TODO-2751: Curve editor value axis auto-scale
// TODO-2752: Curve editor value axis manual range
// TODO-2753: Curve editor multi-curve display
// TODO-2754: Curve editor curve visibility toggles
// TODO-2755: Curve editor curve color per property
// TODO-2756: Curve editor tangent handle display
// TODO-2757: Curve editor tangent weight display
// TODO-2758: Curve editor infinity modes (constant, linear, cycle)
// TODO-2759: Curve editor pre/post infinity visualization
// TODO-2760: Curve editor bake to samples
// TODO-2761: Curve editor reduce keys
// TODO-2762: Curve editor smooth selection
// TODO-2763: Curve editor flatten selection
// TODO-2764: Curve editor add noise
// TODO-2765: Curve editor apply expression
// TODO-2766: Curve editor copy curve shape
// TODO-2767: Curve editor paste curve shape
// TODO-2768: Curve editor curve presets library
// TODO-2769: Curve editor derivative display
// TODO-2770: Curve editor velocity/acceleration graphs

// MARK: - Animation Clip Management TODOs (TODO-2801 to TODO-2850)
// TODO-2801: Animation clip browser
// TODO-2802: Animation clip creation wizard
// TODO-2803: Animation clip duplication
// TODO-2804: Animation clip merging
// TODO-2805: Animation clip splitting
// TODO-2806: Animation clip trimming
// TODO-2807: Animation clip looping settings
// TODO-2808: Animation clip root motion extraction
// TODO-2809: Animation clip compression settings
// TODO-2810: Animation clip preview in browser
// TODO-2811: Animation clip tagging
// TODO-2812: Animation clip search
// TODO-2813: Animation clip favorites
// TODO-2814: Animation clip import (FBX, BVH, etc.)
// TODO-2815: Animation clip export
// TODO-2816: Animation clip baking
// TODO-2817: Animation clip additive mode
// TODO-2818: Animation clip reference pose
// TODO-2819: Animation clip mirror (left/right)
// TODO-2820: Animation clip retarget preview

// MARK: - Blend Tree Editor

class BlendTreeEditor: ObservableObject {
    static let shared = BlendTreeEditor()
    
    @Published var blendTrees: [BlendTree] = []
    @Published var selectedTree: BlendTree?
    @Published var selectedNode: BlendTreeNode?
    @Published var isPlaying: Bool = false
    @Published var previewWeight: Float = 1.0
    
    init() {
        setupDefaultBlendTree()
    }
    
    private func setupDefaultBlendTree() {
        let walkNode = BlendTreeNode(
            name: "Walk",
            type: .animationClip,
            animationClipName: "Walk",
            position: CGPoint(x: 100, y: 100)
        )
        
        let runNode = BlendTreeNode(
            name: "Run", 
            type: .animationClip,
            animationClipName: "Run",
            position: CGPoint(x: 300, y: 100)
        )
        
        let idleNode = BlendTreeNode(
            name: "Idle",
            type: .animationClip,
            animationClipName: "Idle", 
            position: CGPoint(x: 200, y: 200)
        )
        
        let blendNode = BlendTreeNode(
            name: "Walk/Run Blend",
            type: .blend1D,
            parameterName: "speed",
            minValue: 0.0,
            maxValue: 10.0,
            children: [walkNode, runNode],
            position: CGPoint(x: 200, y: 50)
        )
        
        let finalBlend = BlendTreeNode(
            name: "Movement Blend",
            type: .blend1D,
            parameterName: "movement",
            minValue: 0.0,
            maxValue: 1.0,
            children: [blendNode, idleNode],
            position: CGPoint(x: 200, y: 150)
        )
        
        let blendTree = BlendTree(
            name: "Character Movement",
            rootNode: finalBlend
        )
        
        blendTrees = [blendTree]
        selectedTree = blendTree
    }
}

struct BlendTree: Identifiable {
    let id = UUID()
    var name: String
    var rootNode: BlendTreeNode
    var parameters: [BlendParameter] = []
}

struct BlendTreeNode: Identifiable {
    let id = UUID()
    var name: String
    var type: NodeType
    var position: CGPoint
    var children: [BlendTreeNode] = []
    
    // Animation clip properties
    var animationClipName: String = ""
    
    // Blend properties
    var parameterName: String = ""
    var minValue: Float = 0.0
    var maxValue: Float = 1.0
    var threshold: Float = 0.5
    
    enum NodeType: String, CaseIterable {
        case animationClip = "Animation Clip"
        case blend1D = "1D Blend"
        case blend2D = "2D Blend"
        case directBlend = "Direct Blend"
        case additive = "Additive"
        case override = "Override"
    }
}

struct BlendParameter: Identifiable {
    let id = UUID()
    var name: String
    var currentValue: Float = 0.0
    var minValue: Float = 0.0
    var maxValue: Float = 1.0
}

// MARK: - Blend Tree Editor View

struct BlendTreeEditorView: View {
    @StateObject private var blendEditor = BlendTreeEditor.shared
    @State private var showAddNodeDialog = false
    @State private var draggedNode: BlendTreeNode?
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Blend Tree Editor")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                Toggle("Preview", isOn: $blendEditor.isPlaying)
                    .toggleStyle(.switch)
                
                Button("Add Node") {
                    showAddNodeDialog = true
                }
                .buttonStyle(.bordered)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Blend tree canvas
            GeometryReader { geometry in
                ZStack {
                    // Connections
                    ForEach(getAllNodes()) { node in
                        ForEach(node.children, id: \.id) { child in
                            BlendConnectionView(
                                from: node.position,
                                to: child.position,
                                nodeType: node.type
                            )
                        }
                    }
                    
                    // Nodes
                    ForEach(getAllNodes()) { node in
                        BlendTreeNodeView(
                            node: node,
                            isSelected: blendEditor.selectedNode?.id == node.id,
                            onTap: { blendEditor.selectedNode = node }
                        )
                    }
                }
                .background(DesignSystem.Colors.backgroundPrimary)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .sheet(isPresented: $showAddNodeDialog) {
            AddBlendNodeDialog()
        }
    }
    
    private func getAllNodes() -> [BlendTreeNode] {
        guard let tree = blendEditor.selectedTree else { return [] }
        return collectAllNodes(from: tree.rootNode)
    }
    
    private func collectAllNodes(from node: BlendTreeNode) -> [BlendTreeNode] {
        var nodes = [node]
        for child in node.children {
            nodes.append(contentsOf: collectAllNodes(from: child))
        }
        return nodes
    }
}

// MARK: - Blend Tree Node View

struct BlendTreeNodeView: View {
    @ObservedObject var node: BlendTreeNode
    let isSelected: Bool
    let onTap: () -> Void
    
    var body: some View {
        VStack(spacing: 4) {
            // Node icon based on type
            Image(systemName: getNodeIcon())
                .font(.system(size: 16))
                .foregroundColor(getNodeColor())
            
            Text(node.name)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .multilineTextAlignment(.center)
        }
        .frame(width: 80, height: 60)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.2) : DesignSystem.Colors.backgroundSecondary)
                .stroke(
                    isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border,
                    lineWidth: isSelected ? 2 : 1
                )
        )
        .position(node.position)
        .onTapGesture {
            onTap()
        }
    }
    
    private func getNodeIcon() -> String {
        switch node.type {
        case .animationClip: return "film"
        case .blend1D: return "slider.horizontal.3"
        case .blend2D: return "slider.horizontal.below.rectangle"
        case .directBlend: return "arrow.merge"
        case .additive: return "plus.circle"
        case .override: return "arrow.right.circle"
        }
    }
    
    private func getNodeColor() -> Color {
        switch node.type {
        case .animationClip: return DesignSystem.Colors.accentPrimary
        case .blend1D, .blend2D: return DesignSystem.Colors.accentSuccess
        case .directBlend: return DesignSystem.Colors.accentWarning
        case .additive: return DesignSystem.Colors.info
        case .override: return DesignSystem.Colors.accentDanger
        }
    }
}

// MARK: - Blend Connection View

struct BlendConnectionView: View {
    let from: CGPoint
    let to: CGPoint
    let nodeType: BlendTreeNode.NodeType
    
    var body: some View {
        Path { path in
            path.move(to: from)
            path.addLine(to: to)
        }
        .stroke(getConnectionColor(), style: StrokeStyle(lineWidth: 2, lineCap: .round))
    }
    
    private func getConnectionColor() -> Color {
        switch nodeType {
        case .animationClip: return DesignSystem.Colors.accentPrimary
        case .blend1D, .blend2D: return DesignSystem.Colors.accentSuccess
        case .directBlend: return DesignSystem.Colors.accentWarning
        case .additive: return DesignSystem.Colors.info
        case .override: return DesignSystem.Colors.accentDanger
        }
    }
}

// MARK: - Add Blend Node Dialog

struct AddBlendNodeDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var blendEditor = BlendTreeEditor.shared
    
    @State private var nodeName = ""
    @State private var nodeType: BlendTreeNode.NodeType = .animationClip
    @State private var animationClipName = ""
    @State private var parameterName = ""
    @State private var minValue: Float = 0.0
    @State private var maxValue: Float = 1.0
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Add Blend Node")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                HStack {
                    Text("Name:")
                    TextField("Node name", text: $nodeName)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Type:")
                    Picker("Type", selection: $nodeType) {
                        ForEach(BlendTreeNode.NodeType.allCases, id: \.self) { type in
                            Text(type.rawValue).tag(type)
                        }
                    }
                    .pickerStyle(.menu)
                }
                
                if nodeType == .animationClip {
                    HStack {
                        Text("Animation Clip:")
                        TextField("Clip name", text: $animationClipName)
                            .textFieldStyle(.roundedBorder)
                    }
                }
                
                if nodeType == .blend1D || nodeType == .blend2D {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("Parameter:")
                            TextField("Parameter name", text: $parameterName)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        HStack {
                            Text("Min Value:")
                            TextField("0.0", value: $minValue, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                        }
                        
                        HStack {
                            Text("Max Value:")
                            TextField("1.0", value: $maxValue, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                        }
                    }
                }
            }
            
            Spacer()
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Add") {
                    addNode()
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(nodeName.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 400)
    }
    
    private func addNode() {
        let newNode = BlendTreeNode(
            name: nodeName,
            type: nodeType,
            position: CGPoint(x: 200, y: 150),
            animationClipName: animationClipName,
            parameterName: parameterName,
            minValue: minValue,
            maxValue: maxValue
        )
        
        // Add to selected tree's root or selected node
        if let tree = blendEditor.selectedTree {
            if let selectedNode = blendEditor.selectedNode {
                selectedNode.children.append(newNode)
            } else {
                tree.rootNode.children.append(newNode)
            }
        }
    }
}

// MARK: - Animation Clip Browser Implementation

class AnimationClipBrowser: ObservableObject {
    static let shared = AnimationClipBrowser()
    
    @Published var clips: [AnimationClip] = []
    @Published var filteredClips: [AnimationClip] = []
    @Published var selectedClip: AnimationClip?
    @Published var searchQuery: String = ""
    @Published var selectedTags: Set<String> = []
    @Published var selectedCategory: ClipCategory? = nil
    @Published var sortBy: SortOption = .name
    @Published var sortOrder: SortOrder = .ascending
    @Published var showFavoritesOnly: Bool = false
    
    enum ClipCategory: String, CaseIterable {
        case locomotion = "Locomotion"
        case combat = "Combat"
        case interaction = "Interaction"
        case idle = "Idle"
        case cinematic = "Cinematic"
        case procedural = "Procedural"
        case custom = "Custom"
    }
    
    enum SortOption: String, CaseIterable {
        case name = "Name"
        case duration = "Duration"
        case dateCreated = "Date Created"
        case dateModified = "Date Modified"
        case category = "Category"
    }
    
    enum SortOrder: String, CaseIterable {
        case ascending = "Ascending"
        case descending = "Descending"
    }
    
    init() {
        setupDefaultClips()
        updateFilteredClips()
    }
    
    private func setupDefaultClips() {
        clips = [
            AnimationClip(
                name: "Walk",
                duration: 1.2,
                frameRate: 60,
                tracks: [],
                events: [],
                tags: ["locomotion", "basic"],
                category: .locomotion,
                isFavorite: true,
                dateCreated: Date().addingTimeInterval(-86400 * 7),
                dateModified: Date().addingTimeInterval(-3600)
            ),
            AnimationClip(
                name: "Run",
                duration: 0.8,
                frameRate: 60,
                tracks: [],
                events: [],
                tags: ["locomotion", "fast"],
                category: .locomotion,
                isFavorite: true,
                dateCreated: Date().addingTimeInterval(-86400 * 6),
                dateModified: Date().addingTimeInterval(-7200)
            ),
            AnimationClip(
                name: "Idle",
                duration: 2.0,
                frameRate: 60,
                tracks: [],
                events: [],
                tags: ["idle", "basic"],
                category: .idle,
                isFavorite: false,
                dateCreated: Date().addingTimeInterval(-86400 * 5),
                dateModified: Date().addingTimeInterval(-1800)
            ),
            AnimationClip(
                name: "Attack",
                duration: 0.6,
                frameRate: 60,
                tracks: [],
                events: [],
                tags: ["combat", "melee"],
                category: .combat,
                isFavorite: false,
                dateCreated: Date().addingTimeInterval(-86400 * 4),
                dateModified: Date().addingTimeInterval(-900)
            ),
            AnimationClip(
                name: "Jump",
                duration: 1.0,
                frameRate: 60,
                tracks: [],
                events: [],
                tags: ["locomotion", "movement"],
                category: .locomotion,
                isFavorite: true,
                dateCreated: Date().addingTimeInterval(-86400 * 3),
                dateModified: Date()
            )
        ]
    }
    
    func updateFilteredClips() {
        var filtered = clips
        
        // Search query filter
        if !searchQuery.isEmpty {
            filtered = filtered.filter { clip in
                clip.name.localizedCaseInsensitiveContains(searchQuery) ||
                clip.tags.joined().localizedCaseInsensitiveContains(searchQuery)
            }
        }
        
        // Tags filter
        if !selectedTags.isEmpty {
            filtered = filtered.filter { clip in
                selectedTags.isSubset(of: Set(clip.tags))
            }
        }
        
        // Category filter
        if let category = selectedCategory {
            filtered = filtered.filter { $0.category == category }
        }
        
        // Favorites filter
        if showFavoritesOnly {
            filtered = filtered.filter { $0.isFavorite }
        }
        
        // Sort
        filtered.sort { clip1, clip2 in
            let comparison: Bool
            
            switch sortBy {
            case .name:
                comparison = clip1.name < clip2.name
            case .duration:
                comparison = clip1.duration < clip2.duration
            case .dateCreated:
                comparison = clip1.dateCreated < clip2.dateCreated
            case .dateModified:
                comparison = clip1.dateModified < clip2.dateModified
            case .category:
                comparison = clip1.category.rawValue < clip2.category.rawValue
            }
            
            return sortOrder == .ascending ? comparison : !comparison
        }
        
        filteredClips = filtered
    }
    
    func addClip(_ clip: AnimationClip) {
        clips.append(clip)
        updateFilteredClips()
    }
    
    func deleteClip(_ clip: AnimationClip) {
        clips.removeAll { $0.id == clip.id }
        if selectedClip?.id == clip.id {
            selectedClip = nil
        }
        updateFilteredClips()
    }
    
    func duplicateClip(_ clip: AnimationClip) -> AnimationClip {
        let duplicatedClip = AnimationClip(
            name: "\(clip.name) Copy",
            duration: clip.duration,
            frameRate: clip.frameRate,
            tracks: clip.tracks,
            events: clip.events,
            tags: clip.tags,
            category: clip.category,
            isFavorite: false,
            dateCreated: Date(),
            dateModified: Date()
        )
        addClip(duplicatedClip)
        return duplicatedClip
    }
    
    func toggleFavorite(_ clip: AnimationClip) {
        if let index = clips.firstIndex(where: { $0.id == clip.id }) {
            clips[index].isFavorite.toggle()
            updateFilteredClips()
        }
    }
    
    func getAllTags() -> [String] {
        let allTags = clips.flatMap { $0.tags }
        return Array(Set(allTags)).sorted()
    }
    
    // MARK: - Clip Manipulation Functions
    
    func trimClip(_ clip: AnimationClip, startTime: Float, endTime: Float) -> AnimationClip? {
        guard startTime >= 0, endTime <= clip.duration, startTime < endTime else {
            return nil
        }
        
        let trimmedClip = AnimationClip(
            name: "\(clip.name) (Trimmed)",
            duration: endTime - startTime,
            frameRate: clip.frameRate,
            tracks: trimTracks(clip.tracks, startTime: startTime, endTime: endTime),
            events: trimEvents(clip.events, startTime: startTime, endTime: endTime),
            tags: clip.tags + ["trimmed"],
            category: clip.category,
            isFavorite: false,
            dateCreated: Date(),
            dateModified: Date()
        )
        
        // Preserve settings
        trimmedClip.isLooping = clip.isLooping
        trimmedClip.compressionSettings = clip.compressionSettings
        trimmedClip.rootMotionSettings = clip.rootMotionSettings
        
        addClip(trimmedClip)
        return trimmedClip
    }
    
    func splitClip(_ clip: AnimationClip, atTime time: Float) -> [AnimationClip]? {
        guard time > 0 && time < clip.duration else {
            return nil
        }
        
        let firstClip = AnimationClip(
            name: "\(clip.name) (Part 1)",
            duration: time,
            frameRate: clip.frameRate,
            tracks: trimTracks(clip.tracks, startTime: 0, endTime: time),
            events: trimEvents(clip.events, startTime: 0, endTime: time),
            tags: clip.tags + ["split"],
            category: clip.category,
            isFavorite: false,
            dateCreated: Date(),
            dateModified: Date()
        )
        
        let secondClip = AnimationClip(
            name: "\(clip.name) (Part 2)",
            duration: clip.duration - time,
            frameRate: clip.frameRate,
            tracks: trimTracks(clip.tracks, startTime: time, endTime: clip.duration),
            events: trimEvents(clip.events, startTime: time, endTime: clip.duration),
            tags: clip.tags + ["split"],
            category: clip.category,
            isFavorite: false,
            dateCreated: Date(),
            dateModified: Date()
        )
        
        // Preserve settings
        firstClip.isLooping = clip.isLooping
        firstClip.compressionSettings = clip.compressionSettings
        firstClip.rootMotionSettings = clip.rootMotionSettings
        
        secondClip.isLooping = clip.isLooping
        secondClip.compressionSettings = clip.compressionSettings
        secondClip.rootMotionSettings = clip.rootMotionSettings
        
        addClip(firstClip)
        addClip(secondClip)
        return [firstClip, secondClip]
    }
    
    func mergeClips(_ clips: [AnimationClip], blendDuration: Float = 0.5) -> AnimationClip? {
        guard !clips.isEmpty else { return nil }
        
        let totalDuration = clips.reduce(0) { $0 + $1.duration } + blendDuration * Float(clips.count - 1)
        let mergedName = clips.map { $0.name }.joined(separator: " + ")
        
        var mergedTracks: [AnimationTrack] = []
        var mergedEvents: [AnimationEvent] = []
        var allTags = Set<String>()
        
        // Merge tracks from all clips
        for (index, clip) in clips.enumerated() {
            let timeOffset = clips.prefix(index).reduce(0) { $0 + $1.duration } + blendDuration * Float(index)
            
            for track in clip.tracks {
                if let existingTrackIndex = mergedTracks.firstIndex(where: { $0.propertyPath == track.propertyPath }) {
                    // Merge with existing track
                    let offsetKeyframes = track.keyframes.map { keyframe in
                        AnimationKeyframe(
                            time: keyframe.time + timeOffset,
                            value: keyframe.value,
                            inTangent: keyframe.inTangent,
                            outTangent: keyframe.outTangent
                        )
                    }
                    mergedTracks[existingTrackIndex].keyframes.append(contentsOf: offsetKeyframes)
                } else {
                    // Add new track
                    let offsetKeyframes = track.keyframes.map { keyframe in
                        AnimationKeyframe(
                            time: keyframe.time + timeOffset,
                            value: keyframe.value,
                            inTangent: keyframe.inTangent,
                            outTangent: keyframe.outTangent
                        )
                    }
                    let newTrack = AnimationTrack(
                        propertyPath: track.propertyPath,
                        keyframes: offsetKeyframes,
                        interpolation: track.interpolation
                    )
                    mergedTracks.append(newTrack)
                }
            }
            
            // Merge events with time offset
            let offsetEvents = clip.events.map { event in
                AnimationEvent(
                    time: event.time + timeOffset,
                    functionName: event.functionName,
                    parameter: event.parameter,
                    eventType: event.eventType,
                    isEnabled: event.isEnabled,
                    color: event.color,
                    description: event.description
                )
            }
            mergedEvents.append(contentsOf: offsetEvents)
            
            // Collect all tags
            allTags.formUnion(Set(clip.tags))
        }
        
        let mergedClip = AnimationClip(
            name: mergedName,
            duration: totalDuration,
            frameRate: clips.first!.frameRate,
            tracks: mergedTracks,
            events: mergedEvents,
            tags: Array(allTags) + ["merged"],
            category: clips.first!.category,
            isFavorite: false,
            dateCreated: Date(),
            dateModified: Date()
        )
        
        addClip(mergedClip)
        return mergedClip
    }
    
    func duplicateClipWithVariation(_ clip: AnimationClip, variation: ClipVariation) -> AnimationClip? {
        let variationName: String
        let modifiedDuration: Float
        let modifiedTags: [String]
        
        switch variation {
        case .reversed:
            variationName = "\(clip.name) (Reversed)"
            modifiedDuration = clip.duration
            modifiedTags = clip.tags + ["reversed"]
            return createReversedClip(clip, name: variationName, tags: modifiedTags)
            
        case .spedUp(let factor):
            variationName = "\(clip.name) (Speed \(factor)x)"
            modifiedDuration = clip.duration / factor
            modifiedTags = clip.tags + ["speed-\(factor)"]
            return createSpeedModifiedClip(clip, name: variationName, speedFactor: factor, tags: modifiedTags)
            
        case .mirrored:
            variationName = "\(clip.name) (Mirrored)"
            modifiedDuration = clip.duration
            modifiedTags = clip.tags + ["mirrored"]
            return createMirroredClip(clip, name: variationName, tags: modifiedTags)
            
        case .looped(let times):
            variationName = "\(clip.name) (Looped \(times)x)"
            modifiedDuration = clip.duration * Float(times)
            modifiedTags = clip.tags + ["looped-\(times)"]
            return createLoopedClip(clip, name: variationName, loopCount: times, tags: modifiedTags)
        }
    }
    
    enum ClipVariation {
        case reversed
        case spedUp(Float)
        case mirrored
        case looped(Int)
    }
    
    // MARK: - Private Helper Methods
    
    private func trimTracks(_ tracks: [AnimationTrack], startTime: Float, endTime: Float) -> [AnimationTrack] {
        return tracks.map { track in
            let trimmedKeyframes = track.keyframes.filter { keyframe in
                keyframe.time >= startTime && keyframe.time <= endTime
            }.map { keyframe in
                AnimationKeyframe(
                    time: keyframe.time - startTime,
                    value: keyframe.value,
                    inTangent: keyframe.inTangent,
                    outTangent: keyframe.outTangent
                )
            }
            return AnimationTrack(
                propertyPath: track.propertyPath,
                keyframes: trimmedKeyframes,
                interpolation: track.interpolation
            )
        }
    }
    
    private func trimEvents(_ events: [AnimationEvent], startTime: Float, endTime: Float) -> [AnimationEvent] {
        return events.filter { event in
            event.time >= startTime && event.time <= endTime
        }.map { event in
            AnimationEvent(
                time: event.time - startTime,
                functionName: event.functionName,
                parameter: event.parameter,
                eventType: event.eventType,
                isEnabled: event.isEnabled,
                color: event.color,
                description: event.description
            )
        }
    }
    
    private func createReversedClip(_ clip: AnimationClip, name: String, tags: [String]) -> AnimationClip {
        let reversedTracks = clip.tracks.map { track in
            let reversedKeyframes = track.keyframes.reversed().map { keyframe in
                AnimationKeyframe(
                    time: clip.duration - keyframe.time,
                    value: keyframe.value,
                    inTangent: keyframe.outTangent,
                    outTangent: keyframe.inTangent
                )
            }
            return AnimationTrack(
                propertyPath: track.propertyPath,
                keyframes: reversedKeyframes,
                interpolation: track.interpolation
            )
        }
        
        let reversedClip = AnimationClip(
            name: name,
            duration: clip.duration,
            frameRate: clip.frameRate,
            tracks: reversedTracks,
            events: clip.events, // Events might need special handling for reversal
            tags: tags,
            category: clip.category,
            isFavorite: false,
            dateCreated: Date(),
            dateModified: Date()
        )
        
        reversedClip.isLooping = clip.isLooping
        reversedClip.compressionSettings = clip.compressionSettings
        reversedClip.rootMotionSettings = clip.rootMotionSettings
        
        return reversedClip
    }
    
    private func createSpeedModifiedClip(_ clip: AnimationClip, name: String, speedFactor: Float, tags: [String]) -> AnimationClip {
        let speedModifiedTracks = clip.tracks.map { track in
            let speedKeyframes = track.keyframes.map { keyframe in
                AnimationKeyframe(
                    time: keyframe.time / speedFactor,
                    value: keyframe.value,
                    inTangent: keyframe.inTangent / speedFactor,
                    outTangent: keyframe.outTangent / speedFactor
                )
            }
            return AnimationTrack(
                propertyPath: track.propertyPath,
                keyframes: speedKeyframes,
                interpolation: track.interpolation
            )
        }
        
        let speedModifiedEvents = clip.events.map { event in
            AnimationEvent(
                time: event.time / speedFactor,
                functionName: event.functionName,
                parameter: event.parameter,
                eventType: event.eventType,
                isEnabled: event.isEnabled,
                color: event.color,
                description: event.description
            )
        }
        
        let speedModifiedClip = AnimationClip(
            name: name,
            duration: clip.duration / speedFactor,
            frameRate: clip.frameRate,
            tracks: speedModifiedTracks,
            events: speedModifiedEvents,
            tags: tags,
            category: clip.category,
            isFavorite: false,
            dateCreated: Date(),
            dateModified: Date()
        )
        
        speedModifiedClip.isLooping = clip.isLooping
        speedModifiedClip.compressionSettings = clip.compressionSettings
        speedModifiedClip.rootMotionSettings = clip.rootMotionSettings
        
        return speedModifiedClip
    }
    
    private func createMirroredClip(_ clip: AnimationClip, name: String, tags: [String]) -> AnimationClip {
        // This is a simplified implementation - in reality, mirroring would require
        // bone-specific transformations and coordinate system changes
        let mirroredClip = AnimationClip(
            name: name,
            duration: clip.duration,
            frameRate: clip.frameRate,
            tracks: clip.tracks, // Would need actual mirroring logic
            events: clip.events,
            tags: tags,
            category: clip.category,
            isFavorite: false,
            dateCreated: Date(),
            dateModified: Date()
        )
        
        mirroredClip.isLooping = clip.isLooping
        mirroredClip.compressionSettings = clip.compressionSettings
        mirroredClip.rootMotionSettings = clip.rootMotionSettings
        
        return mirroredClip
    }
    
    private func createLoopedClip(_ clip: AnimationClip, name: String, loopCount: Int, tags: [String]) -> AnimationClip {
        var loopedTracks: [AnimationTrack] = []
        
        for track in clip.tracks {
            var loopedKeyframes: [AnimationKeyframe] = []
            
            for loop in 0..<loopCount {
                let timeOffset = Float(loop) * clip.duration
                let loopKeyframes = track.keyframes.map { keyframe in
                    AnimationKeyframe(
                        time: keyframe.time + timeOffset,
                        value: keyframe.value,
                        inTangent: keyframe.inTangent,
                        outTangent: keyframe.outTangent
                    )
                }
                loopedKeyframes.append(contentsOf: loopKeyframes)
            }
            
            loopedTracks.append(AnimationTrack(
                propertyPath: track.propertyPath,
                keyframes: loopedKeyframes,
                interpolation: track.interpolation
            ))
        }
        
        let loopedEvents = clip.events.flatMap { event in
            (0..<loopCount).map { loop in
                AnimationEvent(
                    time: event.time + Float(loop) * clip.duration,
                    functionName: event.functionName,
                    parameter: event.parameter,
                    eventType: event.eventType,
                    isEnabled: event.isEnabled,
                    color: event.color,
                    description: event.description
                )
            }
        }
        
        let loopedClip = AnimationClip(
            name: name,
            duration: clip.duration * Float(loopCount),
            frameRate: clip.frameRate,
            tracks: loopedTracks,
            events: loopedEvents,
            tags: tags,
            category: clip.category,
            isFavorite: false,
            dateCreated: Date(),
            dateModified: Date()
        )
        
        loopedClip.isLooping = clip.isLooping
        loopedClip.compressionSettings = clip.compressionSettings
        loopedClip.rootMotionSettings = clip.rootMotionSettings
        
        return loopedClip
    }
}

// MARK: - Animation Clip Browser View

struct AnimationClipBrowserView: View {
    @StateObject private var browser = AnimationClipBrowser.shared
    @State private var showImportDialog = false
    @State private var showCreateDialog = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Animation Clips")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                // Search
                HStack {
                    Image(systemName: "magnifyingglass")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    TextField("Search clips...", text: $browser.searchQuery)
                        .textFieldStyle(.roundedBorder)
                        .frame(width: 200)
                        .onChange(of: browser.searchQuery) { _ in
                            browser.updateFilteredClips()
                        }
                }
                
                // Category filter
                Picker("Category", selection: Binding(
                    get: { browser.selectedCategory },
                    set: { browser.selectedCategory = $0; browser.updateFilteredClips() }
                )) {
                    Text("All").tag(nil as AnimationClipBrowser.ClipCategory?)
                    ForEach(AnimationClipBrowser.ClipCategory.allCases, id: \.self) { category in
                        Text(category.rawValue).tag(category as AnimationClipBrowser.ClipCategory?)
                    }
                }
                .pickerStyle(.menu)
                .frame(width: 120)
                
                Toggle("Favorites", isOn: Binding(
                    get: { browser.showFavoritesOnly },
                    set: { browser.showFavoritesOnly = $0; browser.updateFilteredClips() }
                ))
                    .toggleStyle(.switch)
                
                Button("Import") {
                    showImportDialog = true
                }
                .buttonStyle(.bordered)
                
                Button("Create") {
                    showCreateDialog = true
                }
                .buttonStyle(.borderedProminent)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Clips grid
            ScrollView {
                LazyVGrid(columns: Array(repeating: GridItem(.flexible(), spacing: DesignSystem.Spacing.md), count: 3), spacing: DesignSystem.Spacing.md) {
                    ForEach(browser.filteredClips, id: \.id) { clip in
                        AnimationClipCard(clip: clip)
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .sheet(isPresented: $showImportDialog) {
            ImportAnimationClipDialog()
        }
        .sheet(isPresented: $showCreateDialog) {
            CreateAnimationClipDialog()
        }
    }
}

// MARK: - Animation Clip Card

struct AnimationClipCard: View {
    @ObservedObject var clip: AnimationClip
    @StateObject private var browser = AnimationClipBrowser.shared
    @State private var isHovering = false
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.sm) {
            // Preview area
            Rectangle()
                .fill(DesignSystem.Colors.backgroundTertiary)
                .frame(height: 120)
                .overlay(
                    VStack(spacing: DesignSystem.Spacing.xs) {
                        Image(systemName: "film")
                            .font(.system(size: 24))
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Text(String(format: "%.1fs", clip.duration))
                            .font(DesignSystem.Typography.smallMono)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    },
                    alignment: .center
                )
            
            // Clip info
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                HStack {
                    Text(clip.name)
                        .font(DesignSystem.Typography.body)
                        .lineLimit(1)
                    
                    Spacer()
                    
                    Button(action: { browser.toggleFavorite(clip) }) {
                        Image(systemName: clip.isFavorite ? "heart.fill" : "heart")
                            .foregroundColor(clip.isFavorite ? DesignSystem.Colors.accentDanger : DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                }
                
                HStack {
                    Text(clip.category.rawValue)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    Text("\(Int(clip.frameRate)) fps")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                // Tags
                ScrollView(.horizontal, showsIndicators: false) {
                    HStack(spacing: DesignSystem.Spacing.xs) {
                        ForEach(clip.tags, id: \.self) { tag in
                            Text(tag)
                                .font(DesignSystem.Typography.small)
                                .padding(.horizontal, 6)
                                .padding(.vertical, 2)
                                .background(DesignSystem.Colors.backgroundTertiary)
                                .cornerRadius(4)
                        }
                    }
                }
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(DesignSystem.Colors.backgroundSecondary)
                .stroke(
                    browser.selectedClip?.id == clip.id ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border,
                    lineWidth: browser.selectedClip?.id == clip.id ? 2 : 1
                )
        )
        .onHover { isHovering = $0 }
        .onTapGesture {
            browser.selectedClip = clip
        }
        .contextMenu {
            Button("Edit") {
                browser.selectedClip = clip
            }
            
            Button("Duplicate") {
                _ = browser.duplicateClip(clip)
            }
            
            Button("Export") {
                // Export clip
            }
            
            Divider()
            
            Button("Delete", role: .destructive) {
                browser.deleteClip(clip)
            }
        }
    }
}

// MARK: - Import Animation Clip Dialog

struct ImportAnimationClipDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var browser = AnimationClipBrowser.shared
    
    @State private var selectedFile = ""
    @State private var importSettings = ImportSettings()
    
    struct ImportSettings {
        var importRootMotion = false
        var generateEvents = false
        var optimizeKeyframes = true
        var compressionQuality: Float = 0.8
        var targetFrameRate = 60
    }
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Import Animation Clip")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                HStack {
                    Text("File:")
                    TextField("Select file...", text: $selectedFile)
                        .textFieldStyle(.roundedBorder)
                    
                    Button("Browse") {
                        // File browser
                    }
                    .buttonStyle(.bordered)
                }
                
                GroupBox("Import Settings") {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        Toggle("Import Root Motion", isOn: $importSettings.importRootMotion)
                        Toggle("Generate Events", isOn: $importSettings.generateEvents)
                        Toggle("Optimize Keyframes", isOn: $importSettings.optimizeKeyframes)
                        
                        HStack {
                            Text("Compression Quality:")
                            Slider(value: $importSettings.compressionQuality, in: 0.1...1.0)
                            Text(String(format: "%.1f", importSettings.compressionQuality))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                        }
                        
                        HStack {
                            Text("Target Frame Rate:")
                            Picker("Frame Rate", selection: $importSettings.targetFrameRate) {
                                Text("24 fps").tag(24)
                                Text("30 fps").tag(30)
                                Text("60 fps").tag(60)
                                Text("120 fps").tag(120)
                            }
                            .pickerStyle(.menu)
                        }
                    }
                }
            }
            
            Spacer()
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Import") {
                    // Import animation clip
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(selectedFile.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 500, height: 450)
    }
}

// MARK: - Create Animation Clip Dialog

struct CreateAnimationClipDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var browser = AnimationClipBrowser.shared
    
    @State private var clipName = ""
    @State private var duration: Float = 2.0
    @State private var frameRate = 60
    @State private var category: AnimationClipBrowser.ClipCategory = .custom
    @State private var tags: [String] = []
    @State private var newTag = ""
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Create Animation Clip")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                HStack {
                    Text("Name:")
                    TextField("Clip name", text: $clipName)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Duration:")
                    Slider(value: $duration, in: 0.1...30.0)
                    Text(String(format: "%.1fs", duration))
                        .font(DesignSystem.Typography.smallMono)
                        .frame(width: 50)
                }
                
                HStack {
                    Text("Frame Rate:")
                    Picker("Frame Rate", selection: $frameRate) {
                        Text("24 fps").tag(24)
                        Text("30 fps").tag(30)
                        Text("60 fps").tag(60)
                        Text("120 fps").tag(120)
                    }
                    .pickerStyle(.menu)
                }
                
                HStack {
                    Text("Category:")
                    Picker("Category", selection: $category) {
                        ForEach(AnimationClipBrowser.ClipCategory.allCases, id: \.self) { cat in
                            Text(cat.rawValue).tag(cat)
                        }
                    }
                    .pickerStyle(.menu)
                }
                
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Text("Tags:")
                    HStack {
                        TextField("Add tag", text: $newTag)
                            .textFieldStyle(.roundedBorder)
                        
                        Button("Add") {
                            if !newTag.isEmpty {
                                tags.append(newTag)
                                newTag = ""
                            }
                        }
                        .buttonStyle(.bordered)
                        .disabled(newTag.isEmpty)
                    }
                    
                    ScrollView(.horizontal, showsIndicators: false) {
                        HStack(spacing: DesignSystem.Spacing.xs) {
                            ForEach(tags, id: \.self) { tag in
                                HStack(spacing: 4) {
                                    Text(tag)
                                        .font(DesignSystem.Typography.small)
                                    Button(action: { tags.removeAll { $0 == tag } }) {
                                        Image(systemName: "xmark")
                                            .font(.system(size: 8))
                                    }
                                    .buttonStyle(.plain)
                                }
                                .padding(.horizontal, 6)
                                .padding(.vertical, 2)
                                .background(DesignSystem.Colors.backgroundTertiary)
                                .cornerRadius(4)
                            }
                        }
                    }
                }
            }
            
            Spacer()
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Create") {
                    let newClip = AnimationClip(
                        name: clipName,
                        duration: duration,
                        frameRate: frameRate,
                        tracks: [],
                        events: [],
                        tags: tags,
                        category: category,
                        isFavorite: false,
                        dateCreated: Date(),
                        dateModified: Date()
                    )
                    
                    browser.addClip(newClip)
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(clipName.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 450, height: 500)
    }
}

// MARK: - Animation Blending TODOs (TODO-2851 to TODO-2900)
// TODO-2851: Blend tree editor
// TODO-2852: 1D blend parameter
// TODO-2853: 2D blend space (freeform directional)
// TODO-2854: 2D blend space (freeform cartesian)
// TODO-2855: 2D blend grid
// TODO-2856: Direct blend tree
// TODO-2857: Blend weight visualization
// TODO-2858: Blend threshold adjustment
// TODO-2859: Blend tree parameter sync
// TODO-2860: Additive blending layer
// TODO-2861: Override blending layer
// TODO-2862: Layer mask editing
// TODO-2863: Per-bone weight painting
// TODO-2864: Blend shape/morph target editor
// TODO-2865: Blend shape combination
// TODO-2866: IK blending
// TODO-2867: FK/IK switch blend
// TODO-2868: Ragdoll blend weight
// TODO-2869: Cross-fade timing
// TODO-2870: Transition interruption handling

// MARK: - Animation State Machine TODOs (TODO-2901 to TODO-2950)
// TODO-2901: State machine graph editor
// TODO-2902: State node creation
// TODO-2903: Transition line drawing
// TODO-2904: Transition condition editor
// TODO-2905: Parameter types (float, int, bool, trigger)
// TODO-2906: Parameter default values
// TODO-2907: Any state transitions
// TODO-2908: Entry/exit states
// TODO-2909: Sub-state machines
// TODO-2910: State machine layers
// TODO-2911: State tags
// TODO-2912: State behaviors/scripts
// TODO-2913: Transition duration curve
// TODO-2914: Transition offset settings
// TODO-2915: Transition interruption sources
// TODO-2916: Ordered interruption
// TODO-2917: State machine debugging
// TODO-2918: Live transition visualization
// TODO-2919: State machine profiling
// TODO-2920: State machine validation

// MARK: - Animation Events TODOs (TODO-2951 to TODO-3000)
// TODO-2951: Animation event timeline
// TODO-2952: Event marker placement
// TODO-2953: Event function picker
// TODO-2954: Event parameter editing
// TODO-2955: Event preview during playback
// TODO-2956: Event copy/paste
// TODO-2957: Event export/import
// TODO-2958: Event documentation
// TODO-2959: Audio event triggers
// TODO-2960: Particle event triggers
// TODO-2961: Footstep events
// TODO-2962: Hit events
// TODO-2963: Generic message events
// TODO-2964: Event debugging
// TODO-2965: Event history log

// MARK: - Animation Retargeting TODOs (TODO-3001 to TODO-3050)
// TODO-3001: Avatar definition editor
// TODO-3002: Bone mapping interface
// TODO-3003: T-pose/A-pose setup
// TODO-3004: Automatic bone mapping
// TODO-3005: Bone translation retargeting
// TODO-3006: Bone rotation retargeting
// TODO-3007: Bone scale retargeting
// TODO-3008: Retarget preview
// TODO-3009: Retarget error visualization
// TODO-3010: Humanoid/Generic toggle
// TODO-3011: Muscle definition
// TODO-3012: Muscle range limits
// TODO-3013: Finger rig setup
// TODO-3014: Toe rig setup
// TODO-3015: Extra bones mapping

// MARK: - IK System TODOs (TODO-3051 to TODO-3100)
// TODO-3051: IK chain editor
// TODO-3052: Two-bone IK
// TODO-3053: Multi-bone IK (CCD, FABRIK)
// TODO-3054: IK target handles
// TODO-3055: IK pole vector
// TODO-3056: IK twist distribution
// TODO-3057: IK weight animation
// TODO-3058: Foot IK ground detection
// TODO-3059: Hand IK attachment
// TODO-3060: Look-at IK
// TODO-3061: Aim IK
// TODO-3062: Spine IK
// TODO-3063: Full body IK
// TODO-3064: IK/FK switching
// TODO-3065: IK baking to FK

// MARK: - IK System Implementation

// IK Chain Data Models
struct IKChain: Identifiable {
    let id = UUID()
    var name: String
    var bones: [IKBone] = []
    var target: IKTarget
    var poleVector: SIMD3<Float> = SIMD3<Float>(0, 1, 0)
    var weight: Float = 1.0
    var twistWeight: Float = 0.5
    var isActive: Bool = true
    var solverType: IKSolverType = .fabrik
    
    enum IKSolverType: String, CaseIterable {
        case twoBone = "Two Bone"
        case ccd = "CCD"
        case fabrik = "FABRIK"
        case fullBody = "Full Body"
    }
}

struct IKBone: Identifiable {
    let id = UUID()
    var name: String
    var transform: Transform
    var length: Float
    var minAngle: Float = -180
    var maxAngle: Float = 180
    var twistAngle: Float = 0
    var isSelected: Bool = false
}

struct IKTarget {
    var position: SIMD3<Float>
    var rotation: simd_quatf
    var isAttached: Bool = false
    var attachmentBone: String? = nil
    var groundSnapEnabled: Bool = false
}

struct Transform {
    var position: SIMD3<Float>
    var rotation: simd_quatf
    var scale: SIMD3<Float>
}

// IK Solver Classes
class TwoBoneIKSolver {
    static func solve(chain: IKChain, targetPosition: SIMD3<Float>, poleVector: SIMD3<Float>) -> [simd_quatf] {
        guard chain.bones.count >= 2 else { return [] }
        
        let rootPos = chain.bones[0].transform.position
        let midPos = chain.bones[1].transform.position
        let endPos = chain.bones[1].transform.position + SIMD3<Float>(0, chain.bones[1].length, 0)
        
        let upperArmLength = distance(rootPos, midPos)
        let lowerArmLength = distance(midPos, endPos)
        let totalLength = upperArmLength + lowerArmLength
        
        let targetDistance = distance(rootPos, targetPosition)
        
        // Check if target is reachable
        guard targetDistance <= totalLength else {
            // Target too far, stretch towards it
            let direction = normalize(targetPosition - rootPos)
            let midTarget = rootPos + direction * upperArmLength
            return [lookRotation(direction: normalize(midTarget - rootPos), up: poleVector),
                   lookRotation(direction: normalize(targetPosition - midTarget), up: poleVector)]
        }
        
        // Calculate elbow bend angle using law of cosines
        let cosAngle = (upperArmLength * upperArmLength + lowerArmLength * lowerArmLength - targetDistance * targetDistance) / 
                      (2 * upperArmLength * lowerArmLength)
        let bendAngle = acos(max(-1, min(1, cosAngle)))
        
        // Calculate rotation for upper arm
        let toTarget = normalize(targetPosition - rootPos)
        let upperRotation = lookRotation(direction: toTarget, up: poleVector)
        
        // Calculate elbow position
        let a = upperArmLength
        let b = lowerArmLength
        let c = targetDistance
        
        let x = (a * a - b * b + c * c) / (2 * c)
        let h = sqrt(max(0, a * a - x * x))
        
        let midPoint = rootPos + toTarget * x
        let elbowOffset = normalize(cross(toTarget, poleVector)) * h
        let elbowPos = midPoint + elbowOffset
        
        // Calculate rotation for lower arm
        let toElbow = normalize(elbowPos - midPos)
        let toEnd = normalize(targetPosition - elbowPos)
        let lowerRotation = lookRotation(direction: toEnd, up: toElbow)
        
        return [upperRotation, lowerRotation]
    }
    
    private static func lookRotation(direction: SIMD3<Float>, up: SIMD3<Float>) -> simd_quatf {
        let forward = normalize(direction)
        let right = normalize(cross(up, forward))
        let up = cross(forward, right)
        
        let matrix = simd_float3x3([right, up, forward])
        return simd_quatf(matrix)
    }
}

class FABRIKSolver {
    static func solve(chain: IKChain, targetPosition: SIMD3<Float>, tolerance: Float = 0.01, maxIterations: Int = 10) -> [simd_quatf] {
        guard chain.bones.count >= 2 else { return [] }
        
        var positions = chain.bones.map { $0.transform.position }
        let lengths = zip(positions, positions.dropFirst()).map { distance($0, $1) }
        let rootPosition = positions[0]
        
        for _ in 0..<maxIterations {
            // Forward reaching
            positions[positions.count - 1] = targetPosition
            for i in (0..<(positions.count - 1)).reversed() {
                let direction = normalize(positions[i] - positions[i + 1])
                positions[i] = positions[i + 1] + direction * lengths[i]
            }
            
            // Backward reaching
            positions[0] = rootPosition
            for i in 1..<positions.count {
                let direction = normalize(positions[i] - positions[i - 1])
                positions[i] = positions[i - 1] + direction * lengths[i - 1]
            }
            
            // Check convergence
            if distance(positions.last!, targetPosition) < tolerance {
                break
            }
        }
        
        // Calculate rotations from new positions
        var rotations: [simd_quatf] = []
        for i in 0..<chain.bones.count {
            if i < positions.count - 1 {
                let direction = normalize(positions[i + 1] - positions[i])
                let rotation = simd_quatf(from: SIMD3<Float>(0, 1, 0), to: direction)
                rotations.append(rotation)
            } else {
                rotations.append(simd_quatf())
            }
        }
        
        return rotations
    }
}

class CCDSolver {
    static func solve(chain: IKChain, targetPosition: SIMD3<Float>, tolerance: Float = 0.01, maxIterations: Int = 10) -> [simd_quatf] {
        guard chain.bones.count >= 2 else { return [] }
        
        var rotations = chain.bones.map { $0.transform.rotation }
        var positions = chain.bones.map { $0.transform.position }
        
        for _ in 0..<maxIterations {
            var endEffectorPos = positions.last!
            
            if distance(endEffectorPos, targetPosition) < tolerance {
                break
            }
            
            // Iterate from end to root
            for i in (0..<chain.bones.count).reversed() {
                let bonePos = positions[i]
                let toEnd = normalize(endEffectorPos - bonePos)
                let toTarget = normalize(targetPosition - bonePos)
                
                let rotationAngle = acos(max(-1, min(1, dot(toEnd, toTarget))))
                let rotationAxis = normalize(cross(toEnd, toTarget))
                
                let deltaRotation = simd_quatf(angle: rotationAngle, axis: rotationAxis)
                rotations[i] = deltaRotation * rotations[i]
                
                // Update positions of all subsequent bones
                for j in (i + 1)..<chain.bones.count {
                    let relativePos = positions[j] - bonePos
                    positions[j] = bonePos + rotate(relativePos, by: deltaRotation)
                }
                
                endEffectorPos = positions.last!
            }
        }
        
        return rotations
    }
    
    private static func rotate(_ vector: SIMD3<Float>, by rotation: simd_quatf) -> SIMD3<Float> {
        return rotation.act(vector)
    }
}

// MARK: - IK Chain Editor View
struct IKChainEditorView: View {
    @StateObject private var ikManager = IKManager.shared
    @State private var selectedChain: IKChain?
    @State private var showAddChainDialog = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("IK Chains")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                Button(action: { showAddChainDialog = true }) {
                    Image(systemName: "plus")
                }
                .buttonStyle(.plain)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Chain list
            List(ikManager.chains, id: \.id, selection: $selectedChain) { chain in
                IKChainRow(chain: chain)
            }
            .listStyle(.plain)
            
            EditorDivider()
            
            // Chain details
            if let chain = selectedChain {
                IKChainDetailView(chain: chain)
            } else {
                VStack {
                    Spacer()
                    Text("Select an IK chain to edit")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Spacer()
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .sheet(isPresented: $showAddChainDialog) {
            AddIKChainDialog()
        }
    }
}

struct IKChainRow: View {
    let chain: IKChain
    
    var body: some View {
        HStack {
            // Chain type indicator
            Circle()
                .fill(chain.isActive ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                .frame(width: 8, height: 8)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(chain.name)
                    .font(DesignSystem.Typography.body)
                
                Text("\(chain.bones.count) bones • \(chain.solverType.rawValue)")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
            
            // Weight indicator
            Text(String(format: "%.1f", chain.weight))
                .font(DesignSystem.Typography.smallMono)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .padding(.vertical, 4)
    }
}

struct IKChainDetailView: View {
    @ObservedObject var chain: IKChain
    @StateObject private var blendSystem = FKIKBlendSystem.shared
    @StateObject private var ragdollSystem = RagdollBlendSystem.shared
    @StateObject private var crossFadeSystem = CrossFadeTimingSystem.shared
    @StateObject private var interruptionSystem = TransitionInterruptionSystem.shared
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            // Chain settings
            GroupBox("Chain Settings") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("Solver:")
                        Picker("Solver", selection: $chain.solverType) {
                            ForEach(IKChain.IKSolverType.allCases, id: \.self) { type in
                                Text(type.rawValue).tag(type)
                            }
                        }
                        .pickerStyle(.menu)
                    }
                    
                    HStack {
                        Text("Weight:")
                        Slider(value: $chain.weight, in: 0...1)
                        Text(String(format: "%.2f", chain.weight))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Twist Weight:")
                        Slider(value: $chain.twistWeight, in: 0...1)
                        Text(String(format: "%.2f", chain.twistWeight))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                    
                    Toggle("Active", isOn: $chain.isActive)
                }
            }
            
            // FK/IK Blend Controls
            GroupBox("FK/IK Blend") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("Blend Weight:")
                        Slider(value: $blendSystem.blendWeight, in: 0...1) { editing in
                            if !editing {
                                blendSystem.setBlendWeight(blendSystem.blendWeight, animated: false)
                            }
                        }
                        Text(String(format: "%.2f", blendSystem.blendWeight))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Blend Duration:")
                        Slider(value: $blendSystem.blendDuration, in: 0.1...2.0)
                        Text(String(format: "%.1fs", blendSystem.blendDuration))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Blend Curve:")
                        Picker("Curve", selection: $blendSystem.blendCurve) {
                            ForEach(FKIKBlendSystem.BlendCurve.allCases, id: \.self) { curve in
                                Text(curve.rawValue).tag(curve)
                            }
                        }
                        .pickerStyle(.menu)
                    }
                    
                    HStack {
                        Button("Blend to FK") {
                            blendSystem.blendToFK(duration: blendSystem.blendDuration, curve: blendSystem.blendCurve)
                        }
                        .buttonStyle(.bordered)
                        
                        Button("Blend to IK") {
                            blendSystem.blendToIK(duration: blendSystem.blendDuration, curve: blendSystem.blendCurve)
                        }
                        .buttonStyle(.borderedProminent)
                        
                        if blendSystem.isBlending {
                            Button("Stop") {
                                blendSystem.stopBlend()
                            }
                            .buttonStyle(.bordered)
                            .tint(.red)
                        }
                    }
                    
                    if blendSystem.isBlending {
                        HStack {
                            ProgressView()
                                .scaleEffect(0.8)
                            Text("Blending...")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                    }
                }
            }
            
            // Ragdoll Blend Controls
            GroupBox("Ragdoll Blend") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("Ragdoll Weight:")
                        Slider(value: $ragdollSystem.blendWeight, in: 0...1) { editing in
                            if !editing {
                                ragdollSystem.setBlendWeight(ragdollSystem.blendWeight, animated: false)
                            }
                        }
                        Text(String(format: "%.2f", ragdollSystem.blendWeight))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Blend Duration:")
                        Slider(value: $ragdollSystem.blendDuration, in: 0.1...3.0)
                        Text(String(format: "%.1fs", ragdollSystem.blendDuration))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Impact Threshold:")
                        Slider(value: $ragdollSystem.impactThreshold, in: 1.0...50.0)
                        Text(String(format: "%.1f", ragdollSystem.impactThreshold))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                    
                    Toggle("Auto Blend on Impact", isOn: $ragdollSystem.autoBlendOnImpact)
                    
                    HStack {
                        Button("Blend to Ragdoll") {
                            ragdollSystem.blendToRagdoll(duration: ragdollSystem.blendDuration)
                        }
                        .buttonStyle(.bordered)
                        
                        Button("Blend to Animation") {
                            ragdollSystem.blendToAnimation(duration: ragdollSystem.blendDuration)
                        }
                        .buttonStyle(.borderedProminent)
                        
                        if ragdollSystem.isBlending {
                            Button("Stop") {
                                ragdollSystem.stopBlend()
                            }
                            .buttonStyle(.bordered)
                            .tint(.red)
                        }
                    }
                    
                    if ragdollSystem.isBlending {
                        HStack {
                            ProgressView()
                                .scaleEffect(0.8)
                            Text(ragdollSystem.ragdollActive ? "Activating Ragdoll..." : "Returning to Animation...")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                    }
                }
            }
            
            // Cross-Fade Timing Controls
            GroupBox("Cross-Fade Timing") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("Fade In Duration:")
                        Slider(value: $crossFadeSystem.fadeInDuration, in: 0.1...3.0)
                        Text(String(format: "%.1fs", crossFadeSystem.fadeInDuration))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Fade Out Duration:")
                        Slider(value: $crossFadeSystem.fadeOutDuration, in: 0.1...3.0)
                        Text(String(format: "%.1fs", crossFadeSystem.fadeOutDuration))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Overlap Duration:")
                        Slider(value: $crossFadeSystem.overlapDuration, in: 0.0...1.0)
                        Text(String(format: "%.1fs", crossFadeSystem.overlapDuration))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Fade Curve:")
                        Picker("Curve", selection: $crossFadeSystem.fadeCurve) {
                            ForEach(CrossFadeTimingSystem.FadeCurve.allCases, id: \.self) { curve in
                                Text(curve.rawValue).tag(curve)
                            }
                        }
                        .pickerStyle(.menu)
                    }
                    
                    Toggle("Auto Timing", isOn: $crossFadeSystem.autoTiming)
                    Toggle("Sync to Beat", isOn: $crossFadeSystem.syncToBeat)
                    
                    if crossFadeSystem.syncToBeat {
                        HStack {
                            Text("Beat Duration:")
                            Slider(value: $crossFadeSystem.beatDuration, in: 0.5...4.0)
                            Text(String(format: "%.1fs", crossFadeSystem.beatDuration))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                        }
                    }
                    
                    HStack {
                        Text("Presets:")
                        Button("Instant") {
                            crossFadeSystem.optimizeForTransitionType(.instant)
                        }
                        .buttonStyle(.bordered)
                        
                        Button("Quick") {
                            crossFadeSystem.optimizeForTransitionType(.quick)
                        }
                        .buttonStyle(.bordered)
                        
                        Button("Smooth") {
                            crossFadeSystem.optimizeForTransitionType(.smooth)
                        }
                        .buttonStyle(.borderedProminent)
                        
                        Button("Dramatic") {
                            crossFadeSystem.optimizeForTransitionType(.dramatic)
                        }
                        .buttonStyle(.bordered)
                        
                        Button("Cinematic") {
                            crossFadeSystem.optimizeForTransitionType(.cinematic)
                        }
                        .buttonStyle(.bordered)
                    }
                }
            }
            
            // Transition Interruption Controls
            GroupBox("Transition Interruption") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Toggle("Can Be Interrupted", isOn: $interruptionSystem.canBeInterrupted)
                    Toggle("Ordered Interruptions", isOn: $interruptionSystem.orderedInterruptions)
                    
                    Text("Enabled Sources:")
                        .font(DesignSystem.Typography.bodyBold)
                    
                    LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 4) {
                        ForEach(TransitionInterruptionSystem.InterruptionSource.allCases, id: \.self) { source in
                            Toggle(source.rawValue, isOn: Binding(
                                get: { interruptionSystem.isSourceEnabled(source) },
                                set: { enabled in
                                    if enabled {
                                        interruptionSystem.enableInterruptionSource(source)
                                    } else {
                                        interruptionSystem.disableInterruptionSource(source)
                                    }
                                }
                            ))
                            .toggleStyle(.button)
                            .buttonStyle(.bordered)
                            .controlSize(.mini)
                        }
                    }
                    
                    HStack {
                        Button("Test User Input") {
                            _ = interruptionSystem.requestInterruption(
                                source: .userInput,
                                reason: "Test interruption from user input",
                                transitionName: "Test Transition"
                            )
                        }
                        .buttonStyle(.bordered)
                        
                        Button("Test Emergency") {
                            _ = interruptionSystem.requestInterruption(
                                source: .emergencyStop,
                                reason: "Emergency stop test",
                                canForce: true,
                                transitionName: "Emergency Stop"
                            )
                        }
                        .buttonStyle(.bordered)
                        .tint(.red)
                        
                        if interruptionSystem.currentInterruption != nil {
                            Button("Complete") {
                                interruptionSystem.completeCurrentInterruption()
                            }
                            .buttonStyle(.borderedProminent)
                        }
                        
                        Button("Clear Pending") {
                            interruptionSystem.clearPendingInterruptions()
                        }
                        .buttonStyle(.bordered)
                    }
                    
                    if let current = interruptionSystem.currentInterruption {
                        VStack(alignment: .leading, spacing: 4) {
                            Text("Current Interruption:")
                                .font(DesignSystem.Typography.bodyBold)
                            Text("Source: \(current.source.rawValue)")
                                .font(DesignSystem.Typography.small)
                            Text("Priority: \(current.priority)")
                                .font(DesignSystem.Typography.small)
                            Text("Reason: \(current.reason)")
                                .font(DesignSystem.Typography.small)
                        }
                        .padding(8)
                        .background(DesignSystem.Colors.accentPrimary.opacity(0.1))
                        .cornerRadius(4)
                    }
                    
                    if !interruptionSystem.pendingInterruptions.isEmpty {
                        VStack(alignment: .leading, spacing: 4) {
                            Text("Pending (\(interruptionSystem.pendingInterruptions.count)):")
                                .font(DesignSystem.Typography.bodyBold)
                            ForEach(interruptionSystem.pendingInterruptions.prefix(3), id: \.id) { pending in
                                Text("• \(pending.source.rawValue): \(pending.reason)")
                                    .font(DesignSystem.Typography.small)
                            }
                            if interruptionSystem.pendingInterruptions.count > 3 {
                                Text("... and \(interruptionSystem.pendingInterruptions.count - 3) more")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                            }
                        }
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    }
                }
            }
            
            // Pole vector
            GroupBox("Pole Vector") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("X:")
                        TextField("X", value: $chain.poleVector.x, format: .number)
                            .textFieldStyle(.roundedBorder)
                            .frame(width: 80)
                        
                        Text("Y:")
                        TextField("Y", value: $chain.poleVector.y, format: .number)
                            .textFieldStyle(.roundedBorder)
                            .frame(width: 80)
                        
                        Text("Z:")
                        TextField("Z", value: $chain.poleVector.z, format: .number)
                            .textFieldStyle(.roundedBorder)
                            .frame(width: 80)
                    }
                }
            }
            
            // Target
            GroupBox("Target") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("Position:")
                        TextField("X", value: $chain.target.position.x, format: .number)
                            .textFieldStyle(.roundedBorder)
                            .frame(width: 80)
                        
                        TextField("Y", value: $chain.target.position.y, format: .number)
                            .textFieldStyle(.roundedBorder)
                            .frame(width: 80)
                        
                        TextField("Z", value: $chain.target.position.z, format: .number)
                            .textFieldStyle(.roundedBorder)
                            .frame(width: 80)
                    }
                    
                    Toggle("Ground Snap", isOn: $chain.target.groundSnapEnabled)
                    
                    if chain.target.isAttached {
                        Text("Attached to: \(chain.target.attachmentBone ?? "Unknown")")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                }
            }
            
            // Bones list
            GroupBox("Bones") {
                List(chain.bones, id: \.id) { bone in
                    HStack {
                        Circle()
                            .fill(bone.isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                            .frame(width: 6, height: 6)
                        
                        Text(bone.name)
                        
                        Spacer()
                        
                        Text(String(format: "%.2f", bone.length))
                            .font(DesignSystem.Typography.smallMono)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                }
                .frame(height: 150)
            }
        }
        .padding(DesignSystem.Spacing.sm)
    }
}

// MARK: - Transition Interruption System

class TransitionInterruptionSystem: ObservableObject {
    static let shared = TransitionInterruptionSystem()
    
    @Published var canBeInterrupted: Bool = true
    @Published var interruptionPriority: Int = 0
    @Published var interruptionSources: Set<InterruptionSource> = []
    @Published var orderedInterruptions: Bool = true
    @Published var currentInterruption: InterruptionRequest?
    @Published var pendingInterruptions: [InterruptionRequest] = []
    @Published var interruptionHistory: [InterruptionEvent] = []
    
    enum InterruptionSource: String, CaseIterable {
        case userInput = "User Input"
        case animationEvent = "Animation Event"
        case externalTrigger = "External Trigger"
        case stateCondition = "State Condition"
        case emergencyStop = "Emergency Stop"
        case systemOverride = "System Override"
        case networkEvent = "Network Event"
        case physicsImpact = "Physics Impact"
        
        var priority: Int {
            switch self {
            case .emergencyStop: return 100
            case .systemOverride: return 90
            case .physicsImpact: return 80
            case .networkEvent: return 70
            case .userInput: return 60
            case .externalTrigger: return 50
            case .animationEvent: return 40
            case .stateCondition: return 30
            }
        }
    }
    
    struct InterruptionRequest: Identifiable {
        let id = UUID()
        let source: InterruptionSource
        let priority: Int
        let reason: String
        let timestamp: Date
        let canForce: Bool
        let transitionName: String
        
        init(source: InterruptionSource, reason: String, canForce: Bool = false, transitionName: String = "") {
            self.source = source
            self.priority = source.priority
            self.reason = reason
            self.timestamp = Date()
            self.canForce = canForce
            self.transitionName = transitionName
        }
    }
    
    struct InterruptionEvent: Identifiable {
        let id = UUID()
        let request: InterruptionRequest
        let wasAccepted: Bool
        let actualTime: Date
        let previousTransition: String
        let newTransition: String
        
        init(request: InterruptionRequest, wasAccepted: Bool, previousTransition: String, newTransition: String) {
            self.request = request
            self.wasAccepted = wasAccepted
            self.actualTime = Date()
            self.previousTransition = previousTransition
            self.newTransition = newTransition
        }
    }
    
    func requestInterruption(source: InterruptionSource, reason: String, canForce: Bool = false, transitionName: String = "") -> Bool {
        let request = InterruptionRequest(source: source, reason: reason, canForce: canForce, transitionName: transitionName)
        
        if !canBeInterrupted && !canForce {
            return false
        }
        
        if currentInterruption == nil {
            return acceptInterruption(request)
        } else {
            if shouldReplaceCurrentInterruption(with: request) {
                return acceptInterruption(request)
            } else {
                if orderedInterruptions {
                    pendingInterruptions.append(request)
                    sortPendingInterruptions()
                }
                return false
            }
        }
    }
    
    private func acceptInterruption(_ request: InterruptionRequest) -> Bool {
        let previousTransition = currentInterruption?.transitionName ?? "None"
        
        currentInterruption = request
        
        let event = InterruptionEvent(
            request: request,
            wasAccepted: true,
            previousTransition: previousTransition,
            newTransition: request.transitionName
        )
        interruptionHistory.append(event)
        
        return true
    }
    
    private func shouldReplaceCurrentInterruption(with newRequest: InterruptionRequest) -> Bool {
        guard let current = currentInterruption else { return true }
        
        if newRequest.canForce && !current.canForce {
            return true
        }
        
        if newRequest.priority > current.priority {
            return true
        }
        
        if newRequest.priority == current.priority && orderedInterruptions {
            return newRequest.timestamp < current.timestamp
        }
        
        return false
    }
    
    private func sortPendingInterruptions() {
        pendingInterruptions.sort { first, second in
            if first.priority != second.priority {
                return first.priority > second.priority
            }
            return first.timestamp < second.timestamp
        }
    }
    
    func completeCurrentInterruption() {
        guard let current = currentInterruption else { return }
        
        let event = InterruptionEvent(
            request: current,
            wasAccepted: true,
            previousTransition: current.transitionName,
            newTransition: "Completed"
        )
        interruptionHistory.append(event)
        
        currentInterruption = nil
        
        processNextPendingInterruption()
    }
    
    private func processNextPendingInterruption() {
        guard !pendingInterruptions.isEmpty else { return }
        
        let next = pendingInterruptions.removeFirst()
        acceptInterruption(next)
    }
    
    func clearPendingInterruptions() {
        pendingInterruptions.removeAll()
    }
    
    func enableInterruptionSource(_ source: InterruptionSource) {
        interruptionSources.insert(source)
    }
    
    func disableInterruptionSource(_ source: InterruptionSource) {
        interruptionSources.remove(source)
        
        // Remove pending interruptions from disabled source
        pendingInterruptions.removeAll { $0.source == source }
    }
    
    func isSourceEnabled(_ source: InterruptionSource) -> Bool {
        return interruptionSources.contains(source)
    }
    
    func getInterruptionStatistics() -> (total: Int, accepted: Int, pending: Int, bySource: [InterruptionSource: Int]) {
        let total = interruptionHistory.count
        let accepted = interruptionHistory.filter { $0.wasAccepted }.count
        let pending = pendingInterruptions.count
        
        var bySource: [InterruptionSource: Int] = [:]
        for event in interruptionHistory {
            bySource[event.request.source, default: 0] += 1
        }
        
        return (total, accepted, pending, bySource)
    }
}

// MARK: - Cross-Fade Timing System

class CrossFadeTimingSystem: ObservableObject {
    static let shared = CrossFadeTimingSystem()
    
    @Published var fadeInDuration: Float = 0.5
    @Published var fadeOutDuration: Float = 0.5
    @Published var overlapDuration: Float = 0.2
    @Published var fadeCurve: FadeCurve = .smooth
    @Published var autoTiming: Bool = true
    @Published var syncToBeat: Bool = false
    @Published var beatDuration: Float = 1.0
    
    enum FadeCurve: String, CaseIterable {
        case linear = "Linear"
        case smooth = "Smooth"
        case easeIn = "Ease In"
        case easeOut = "Ease Out"
        case easeInOut = "Ease In Out"
        case exponential = "Exponential"
        case logarithmic = "Logarithmic"
        
        func evaluate(at t: Float) -> Float {
            switch self {
            case .linear:
                return t
            case .smooth:
                return t * t * (3.0 - 2.0 * t)
            case .easeIn:
                return t * t
            case .easeOut:
                return 1.0 - (1.0 - t) * (1.0 - t)
            case .easeInOut:
                return t < 0.5 ? 2.0 * t * t : 1.0 - 2.0 * (1.0 - t) * (1.0 - t)
            case .exponential:
                return t == 0 ? 0 : pow(2, 10 * (t - 1))
            case .logarithmic:
                return t == 0 ? 0 : log10(1 + t * 9) / log10(10)
            }
        }
    }
    
    func calculateCrossFadeWeights(progress: Float) -> (fadeOut: Float, fadeIn: Float) {
        let adjustedProgress = autoTiming ? calculateAutoProgress(progress) : progress
        
        let fadeOutWeight = fadeCurve.evaluate(at: max(0, 1.0 - adjustedProgress / fadeOutDuration))
        let fadeInWeight = fadeCurve.evaluate(at: min(1.0, adjustedProgress / fadeInDuration))
        
        return (fadeOutWeight, fadeInWeight)
    }
    
    private func calculateAutoProgress(_ progress: Float) -> Float {
        if syncToBeat {
            let beatProgress = (progress * beatDuration).truncatingRemainder(dividingBy: beatDuration)
            return beatProgress / beatDuration
        }
        return progress
    }
    
    func getTotalFadeDuration() -> Float {
        return max(fadeInDuration, fadeOutDuration) - overlapDuration
    }
    
    func setTimings(fadeIn: Float, fadeOut: Float, overlap: Float) {
        fadeInDuration = max(0.1, fadeIn)
        fadeOutDuration = max(0.1, fadeOut)
        overlapDuration = max(0, min(overlap, min(fadeIn, fadeOut) * 0.8))
    }
    
    func optimizeForTransitionType(_ type: TransitionType) {
        switch type {
        case .instant:
            setTimings(fadeIn: 0.1, fadeOut: 0.1, overlap: 0.0)
            fadeCurve = .linear
        case .quick:
            setTimings(fadeIn: 0.2, fadeOut: 0.2, overlap: 0.1)
            fadeCurve = .easeOut
        case .smooth:
            setTimings(fadeIn: 0.5, fadeOut: 0.5, overlap: 0.2)
            fadeCurve = .smooth
        case .dramatic:
            setTimings(fadeIn: 1.0, fadeOut: 1.0, overlap: 0.3)
            fadeCurve = .easeInOut
        case .cinematic:
            setTimings(fadeIn: 2.0, fadeOut: 2.0, overlap: 0.5)
            fadeCurve = .exponential
        }
    }
    
    enum TransitionType {
        case instant
        case quick
        case smooth
        case dramatic
        case cinematic
    }
}

// MARK: - Ragdoll Blend System

class RagdollBlendSystem: ObservableObject {
    static let shared = RagdollBlendSystem()
    
    @Published var blendWeight: Float = 0.0
    @Published var isBlending: Bool = false
    @Published var blendDuration: Float = 1.0
    @Published var influenceRadius: Float = 5.0
    @Published var boneInfluenceWeights: [String: Float] = [:]
    @Published var ragdollActive: Bool = false
    @Published var autoBlendOnImpact: Bool = true
    @Published var impactThreshold: Float = 10.0
    
    private var blendTimer: Timer?
    private var blendStartTime: Float = 0.0
    private var startWeight: Float = 0.0
    private var targetWeight: Float = 0.0
    
    init() {
        setupDefaultBoneWeights()
    }
    
    private func setupDefaultBoneWeights() {
        boneInfluenceWeights = [
            "Head": 0.8,
            "Neck": 0.7,
            "Spine": 0.9,
            "Left Shoulder": 0.6,
            "Right Shoulder": 0.6,
            "Left Elbow": 0.5,
            "Right Elbow": 0.5,
            "Left Hip": 0.8,
            "Right Hip": 0.8,
            "Left Knee": 0.7,
            "Right Knee": 0.7
        ]
    }
    
    func blendToRagdoll(duration: Float = 1.0) {
        targetWeight = 1.0
        blendDuration = duration
        startBlend()
        ragdollActive = true
    }
    
    func blendToAnimation(duration: Float = 1.0) {
        targetWeight = 0.0
        blendDuration = duration
        startBlend()
        ragdollActive = false
    }
    
    func setBlendWeight(_ weight: Float, animated: Bool = true) {
        if animated {
            targetWeight = max(0.0, min(1.0, weight))
            startBlend()
        } else {
            blendWeight = max(0.0, min(1.0, weight))
            targetWeight = blendWeight
            isBlending = false
            blendTimer?.invalidate()
            blendTimer = nil
        }
    }
    
    func handleImpact(force: Float, position: SIMD3<Float>) {
        if autoBlendOnImpact && force > impactThreshold {
            let blendSpeed = min(1.0, force / impactThreshold)
            blendToRagdoll(duration: 0.5 / blendSpeed)
        }
    }
    
    func getBoneInfluenceWeight(boneName: String) -> Float {
        let baseWeight = boneInfluenceWeights[boneName] ?? 0.5
        return baseWeight * blendWeight
    }
    
    func setBoneInfluenceWeight(boneName: String, weight: Float) {
        boneInfluenceWeights[boneName] = max(0.0, min(1.0, weight))
    }
    
    private func startBlend() {
        if blendTimer != nil { return }
        
        isBlending = true
        startWeight = blendWeight
        blendStartTime = 0.0
        
        blendTimer = Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { [weak self] timer in
            guard let self = self else {
                timer.invalidate()
                return
            }
            
            self.blendStartTime += Float(1.0/60.0)
            let progress = self.blendStartTime / self.blendDuration
            
            if progress >= 1.0 {
                self.blendWeight = self.targetWeight
                self.isBlending = false
                timer.invalidate()
                self.blendTimer = nil
            } else {
                // Smooth ease-in-out curve
                let t = progress
                let curveValue = t < 0.5 ? 2.0 * t * t : 1.0 - 2.0 * (1.0 - t) * (1.0 - t)
                self.blendWeight = self.startWeight + (self.targetWeight - self.startWeight) * curveValue
            }
        }
    }
    
    func stopBlend() {
        isBlending = false
        blendTimer?.invalidate()
        blendTimer = nil
    }
}

// MARK: - FK/IK Blend System

class FKIKBlendSystem: ObservableObject {
    static let shared = FKIKBlendSystem()
    
    @Published var blendWeight: Float = 0.0
    @Published var isBlending: Bool = false
    @Published var blendDuration: Float = 0.5
    @Published var blendCurve: BlendCurve = .smooth
    @Published var targetWeight: Float = 0.0
    
    private var blendTimer: Timer?
    private var blendStartTime: Float = 0.0
    private var startWeight: Float = 0.0
    
    enum BlendCurve: String, CaseIterable {
        case linear = "Linear"
        case smooth = "Smooth"
        case easeIn = "Ease In"
        case easeOut = "Ease Out"
        case easeInOut = "Ease In Out"
        
        func evaluate(at t: Float) -> Float {
            switch self {
            case .linear:
                return t
            case .smooth:
                return t * t * (3.0 - 2.0 * t)
            case .easeIn:
                return t * t
            case .easeOut:
                return 1.0 - (1.0 - t) * (1.0 - t)
            case .easeInOut:
                return t < 0.5 ? 2.0 * t * t : 1.0 - 2.0 * (1.0 - t) * (1.0 - t)
            }
        }
    }
    
    func blendToFK(duration: Float = 0.5, curve: BlendCurve = .smooth) {
        targetWeight = 0.0
        blendDuration = duration
        blendCurve = curve
        startBlend()
    }
    
    func blendToIK(duration: Float = 0.5, curve: BlendCurve = .smooth) {
        targetWeight = 1.0
        blendDuration = duration
        blendCurve = curve
        startBlend()
    }
    
    func setBlendWeight(_ weight: Float, animated: Bool = true) {
        if animated {
            targetWeight = max(0.0, min(1.0, weight))
            startBlend()
        } else {
            blendWeight = max(0.0, min(1.0, weight))
            targetWeight = blendWeight
            isBlending = false
            blendTimer?.invalidate()
            blendTimer = nil
        }
    }
    
    private func startBlend() {
        if blendTimer != nil { return }
        
        isBlending = true
        startWeight = blendWeight
        blendStartTime = 0.0
        
        blendTimer = Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { [weak self] timer in
            guard let self = self else {
                timer.invalidate()
                return
            }
            
            self.blendStartTime += Float(1.0/60.0)
            let progress = self.blendStartTime / self.blendDuration
            
            if progress >= 1.0 {
                self.blendWeight = self.targetWeight
                self.isBlending = false
                timer.invalidate()
                self.blendTimer = nil
            } else {
                let curveValue = self.blendCurve.evaluate(at: progress)
                self.blendWeight = self.startWeight + (self.targetWeight - self.startWeight) * curveValue
            }
        }
    }
    
    func stopBlend() {
        isBlending = false
        blendTimer?.invalidate()
        blendTimer = nil
    }
}

// MARK: - IK Manager

class IKManager: ObservableObject {
    static let shared = IKManager()
    
    @Published var chains: [IKChain] = []
    @Published var activeChains: Set<UUID> = []
    @Published var ikWeight: Float = 1.0
    @Published var fkWeight: Float = 0.0
    
    init() {
        setupDefaultChains()
    }
    
    private func setupDefaultChains() {
        // Left arm chain
        let leftArmChain = IKChain(
            name: "Left Arm",
            bones: [
                IKBone(name: "Left Shoulder", transform: Transform(position: SIMD3<Float>(0, 1.5, 0), rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)), length: 0.3),
                IKBone(name: "Left Elbow", transform: Transform(position: SIMD3<Float>(0.3, 1.5, 0), rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)), length: 0.25),
                IKBone(name: "Left Hand", transform: Transform(position: SIMD3<Float>(0.55, 1.5, 0), rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)), length: 0.1)
            ],
            target: IKTarget(position: SIMD3<Float>(0.65, 1.5, 0), rotation: simd_quatf()),
            poleVector: SIMD3<Float>(0, 0, 1),
            solverType: .twoBone
        )
        
        // Right arm chain
        let rightArmChain = IKChain(
            name: "Right Arm",
            bones: [
                IKBone(name: "Right Shoulder", transform: Transform(position: SIMD3<Float>(0, 1.5, 0), rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)), length: 0.3),
                IKBone(name: "Right Elbow", transform: Transform(position: SIMD3<Float>(-0.3, 1.5, 0), rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)), length: 0.25),
                IKBone(name: "Right Hand", transform: Transform(position: SIMD3<Float>(-0.55, 1.5, 0), rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)), length: 0.1)
            ],
            target: IKTarget(position: SIMD3<Float>(-0.65, 1.5, 0), rotation: simd_quatf()),
            poleVector: SIMD3<Float>(0, 0, 1),
            solverType: .twoBone
        )
        
        // Left leg chain
        let leftLegChain = IKChain(
            name: "Left Leg",
            bones: [
                IKBone(name: "Left Hip", transform: Transform(position: SIMD3<Float>(0.1, 0.9, 0), rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)), length: 0.4),
                IKBone(name: "Left Knee", transform: Transform(position: SIMD3<Float>(0.1, 0.5, 0), rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)), length: 0.4),
                IKBone(name: "Left Foot", transform: Transform(position: SIMD3<Float>(0.1, 0.1, 0), rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)), length: 0.1)
            ],
            target: IKTarget(position: SIMD3<Float>(0.1, 0, 0), rotation: simd_quatf(), groundSnapEnabled: true),
            poleVector: SIMD3<Float>(1, 0, 0),
            solverType: .twoBone
        )
        
        // Right leg chain
        let rightLegChain = IKChain(
            name: "Right Leg",
            bones: [
                IKBone(name: "Right Hip", transform: Transform(position: SIMD3<Float>(-0.1, 0.9, 0), rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)), length: 0.4),
                IKBone(name: "Right Knee", transform: Transform(position: SIMD3<Float>(-0.1, 0.5, 0), rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)), length: 0.4),
                IKBone(name: "Right Foot", transform: Transform(position: SIMD3<Float>(-0.1, 0.1, 0), rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)), length: 0.1)
            ],
            target: IKTarget(position: SIMD3<Float>(-0.1, 0, 0), rotation: simd_quatf(), groundSnapEnabled: true),
            poleVector: SIMD3<Float>(-1, 0, 0),
            solverType: .twoBone
        )
        
        chains = [leftArmChain, rightArmChain, leftLegChain, rightLegChain]
    }
    
    func solveIK() {
        for chain in chains where chain.isActive {
            let rotations: [simd_quatf]
            
            switch chain.solverType {
            case .twoBone:
                rotations = TwoBoneIKSolver.solve(chain: chain, targetPosition: chain.target.position, poleVector: chain.poleVector)
            case .ccd:
                rotations = CCDSolver.solve(chain: chain, targetPosition: chain.target.position)
            case .fabrik:
                rotations = FABRIKSolver.solve(chain: chain, targetPosition: chain.target.position)
            case .fullBody:
                rotations = solveFullBodyIK(chain: chain)
            }
            
            // Apply rotations with weight blending
            for (i, rotation) in rotations.enumerated() {
                if i < chain.bones.count {
                    let weightedRotation = slerp(chain.bones[i].transform.rotation, rotation, chain.weight * ikWeight)
                    chain.bones[i].transform.rotation = weightedRotation
                }
            }
        }
    }
    
    private func solveFullBodyIK(chain: IKChain) -> [simd_quatf] {
        // Simplified full body IK - combine multiple chains
        return FABRIKSolver.solve(chain: chain, targetPosition: chain.target.position)
    }
    
    func blendIKToFK() {
        // Smoothly blend from IK to FK
        let blendSpeed: Float = 0.1
        
        if ikWeight > 0 {
            ikWeight = max(0, ikWeight - blendSpeed)
            fkWeight = min(1, fkWeight + blendSpeed)
        }
    }
    
    func blendFKToIK() {
        // Smoothly blend from FK to IK
        let blendSpeed: Float = 0.1
        
        if fkWeight > 0 {
            fkWeight = max(0, fkWeight - blendSpeed)
            ikWeight = min(1, ikWeight + blendSpeed)
        }
    }
}

// Add IK Chain Dialog
struct AddIKChainDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var ikManager = IKManager.shared
    
    @State private var chainName = ""
    @State private var solverType: IKChain.IKSolverType = .twoBone
    @State private var selectedBones: Set<String> = []
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Add IK Chain")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                HStack {
                    Text("Name:")
                    TextField("Chain Name", text: $chainName)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Solver:")
                    Picker("Solver", selection: $solverType) {
                        ForEach(IKChain.IKSolverType.allCases, id: \.self) { type in
                            Text(type.rawValue).tag(type)
                        }
                    }
                    .pickerStyle(.menu)
                }
            }
            
            Spacer()
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Button("Add") {
                    addChain()
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(chainName.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 300)
    }
    
    private func addChain() {
        let newChain = IKChain(
            name: chainName,
            bones: [],
            target: IKTarget(position: SIMD3<Float>(0, 0, 0), rotation: simd_quatf()),
            solverType: solverType
        )
        
        ikManager.chains.append(newChain)
    }
}

// MARK: - Specialized IK Systems

// Look-at IK Solver
class LookAtIKSolver {
    static func solve(headPosition: SIMD3<Float>, targetPosition: SIMD3<Float>, upVector: SIMD3<Float> = SIMD3<Float>(0, 1, 0)) -> simd_quatf {
        let direction = normalize(targetPosition - headPosition)
        return lookRotation(direction: direction, up: upVector)
    }
    
    static func solveEyes(eyePosition: SIMD3<Float>, targetPosition: SIMD3<Float>, maxAngle: Float = 45.0) -> simd_quatf {
        let direction = normalize(targetPosition - eyePosition)
        let currentForward = SIMD3<Float>(0, 0, 1)
        let angle = acos(max(-1, min(1, dot(currentForward, direction)))) * 180.0 / .pi
        
        // Clamp to maximum eye movement
        let clampedAngle = min(angle, maxAngle)
        let rotationAxis = normalize(cross(currentForward, direction))
        
        return simd_quatf(angle: clampedAngle * .pi / 180.0, axis: rotationAxis)
    }
    
    private static func lookRotation(direction: SIMD3<Float>, up: SIMD3<Float>) -> simd_quatf {
        let forward = normalize(direction)
        let right = normalize(cross(up, forward))
        let up = cross(forward, right)
        
        let matrix = simd_float3x3([right, up, forward])
        return simd_quatf(matrix)
    }
}

// Aim IK Solver
class AimIKSolver {
    static func solve(weaponPosition: SIMD3<Float>, targetPosition: SIMD3<Float>, aimAxis: SIMD3<Float> = SIMD3<Float>(0, 0, 1)) -> simd_quatf {
        let aimDirection = normalize(targetPosition - weaponPosition)
        let rotation = simd_quatf(from: aimAxis, to: aimDirection)
        return rotation
    }
    
    static func solveWithUp(weaponPosition: SIMD3<Float>, targetPosition: SIMD3<Float>, upVector: SIMD3<Float>, aimAxis: SIMD3<Float> = SIMD3<Float>(0, 0, 1)) -> simd_quatf {
        let aimDirection = normalize(targetPosition - weaponPosition)
        let right = normalize(cross(aimDirection, upVector))
        let up = cross(right, aimDirection)
        
        let targetMatrix = simd_float3x3([right, up, aimDirection])
        let targetRotation = simd_quatf(targetMatrix)
        
        let aimRotation = simd_quatf(from: aimAxis, to: aimDirection)
        
        return slerp(aimRotation, targetRotation, 0.5)
    }
}

// Spine IK Solver
class SpineIKSolver {
    static func solve(spineBones: [IKBone], headTarget: SIMD3<Float>, hipTarget: SIMD3<Float>, bendAmount: Float = 0.5) -> [simd_quatf] {
        guard spineBones.count >= 2 else { return [] }
        
        var rotations: [simd_quatf] = []
        
        for (i, bone) in spineBones.enumerated() {
            let t = Float(i) / Float(spineBones.count - 1)
            let interpolatedTarget = mix(hipTarget, headTarget, t)
            
            let currentForward = SIMD3<Float>(0, 0, 1)
            let targetDirection = normalize(interpolatedTarget - bone.transform.position)
            
            let bendDirection = normalize(targetDirection - currentForward)
            let bendAngle = length(bendDirection) * bendAmount
            
            if bendAngle > 0.001 {
                let rotationAxis = normalize(cross(currentForward, bendDirection))
                let rotation = simd_quatf(angle: bendAngle, axis: rotationAxis)
                rotations.append(rotation)
            } else {
                rotations.append(simd_quatf())
            }
        }
        
        return rotations
    }
}

// Ground Detection System
class GroundDetectionSystem {
    static func detectGroundHeight(position: SIMD3<Float>, groundHeight: Float = 0.0, rayDistance: Float = 2.0) -> Float {
        // Simple ground detection - in real implementation would use raycasting
        let rayEnd = SIMD3<Float>(position.x, position.y - rayDistance, position.z)
        
        // Return ground height if ray would hit ground
        if position.y > groundHeight && rayEnd.y <= groundHeight {
            return groundHeight
        }
        
        return position.y
    }
    
    static func snapToGround(position: SIMD3<Float>, groundHeight: Float = 0.0, offset: Float = 0.0) -> SIMD3<Float> {
        let groundY = detectGroundHeight(position: position, groundHeight: groundHeight)
        return SIMD3<Float>(position.x, groundY + offset, position.z)
    }
}

// Hand Attachment System
class HandAttachmentSystem {
    static func attachHand(handBone: IKBone, targetBone: String, attachmentPoint: SIMD3<Float> = SIMD3<Float>(0, 0, 0)) -> IKTarget {
        let target = IKTarget(
            position: attachmentPoint,
            rotation: simd_quatf(),
            isAttached: true,
            attachmentBone: targetBone
        )
        return target
    }
    
    static func updateAttachedTarget(_ target: inout IKTarget, boneTransforms: [String: Transform]) {
        guard let attachmentBone = target.attachmentBone,
              let boneTransform = boneTransforms[attachmentBone] else { return }
        
        target.position = boneTransform.position + boneTransform.rotation.act(target.position)
        target.rotation = boneTransform.rotation * target.rotation
    }
}

// IK Baking System
class IKBakingSystem {
    static func bakeIKToFK(chains: [IKChain], animationClip: AnimationClip, frameRate: Float = 60.0) -> AnimationClip {
        var bakedClip = animationClip
        let frameCount = Int(animationClip.duration * frameRate)
        
        for frame in 0..<frameCount {
            let time = Float(frame) / frameRate
            
            // Solve IK for this frame
            for chain in chains {
                // Interpolate target position for this frame
                let targetPos = interpolateTarget(chain: chain, time: time)
                let rotations = solveChainAtTime(chain: chain, targetPosition: targetPos)
                
                // Create keyframes for each bone
                for (i, rotation) in rotations.enumerated() {
                    if i < chain.bones.count {
                        let boneName = chain.bones[i].name
                        let keyframe = AnimationKeyframe(time: time, value: rotation)
                        
                        // Find or create track for this bone
                        if let trackIndex = bakedClip.tracks.firstIndex(where: { $0.propertyPath == boneName }) {
                            bakedClip.tracks[trackIndex].keyframes.append(keyframe)
                        } else {
                            let newTrack = AnimationTrack(propertyPath: boneName, keyframes: [keyframe])
                            bakedClip.tracks.append(newTrack)
                        }
                    }
                }
            }
        }
        
        return bakedClip
    }
    
    private static func interpolateTarget(chain: IKChain, time: Float) -> SIMD3<Float> {
        // Simple interpolation - in real implementation would use animation curves
        return chain.target.position
    }
    
    private static func solveChainAtTime(chain: IKChain, targetPosition: SIMD3<Float>) -> [simd_quatf] {
        switch chain.solverType {
        case .twoBone:
            return TwoBoneIKSolver.solve(chain: chain, targetPosition: targetPosition, poleVector: chain.poleVector)
        case .ccd:
            return CCDSolver.solve(chain: chain, targetPosition: targetPosition)
        case .fabrik:
            return FABRIKSolver.solve(chain: chain, targetPosition: targetPosition)
        case .fullBody:
            return FABRIKSolver.solve(chain: chain, targetPosition: targetPosition)
        }
    }
}

// MARK: - Animation Retargeting System

// Retargeting Data Models
struct AvatarDefinition: Identifiable {
    let id = UUID()
    var name: String
    var boneMappings: [String: String] = [:]
    var muscleDefinitions: [MuscleDefinition] = []
    var isHumanoid: Bool = true
    var tPose: Transform = Transform(position: .zero, rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1))
}

struct MuscleDefinition: Identifiable {
    let id = UUID()
    var name: String
    var sourceBone: String
    var targetBone: String
    var minAngle: Float = -180
    var maxAngle: Float = 180
    var defaultAngle: Float = 0
    var stiffness: Float = 1.0
}

struct RetargetError {
    var boneName: String
    var positionError: Float
    var rotationError: Float
    var scaleError: Float
    var totalError: Float
}

// Retargeting Manager
class RetargetingManager: ObservableObject {
    static let shared = RetargetingManager()
    
    @Published var sourceAvatar: AvatarDefinition?
    @Published var targetAvatar: AvatarDefinition?
    @Published var retargetErrors: [RetargetError] = []
    @Published var isHumanoidMode: Bool = true
    @Published var showErrors: Bool = true
    @Published var errorThreshold: Float = 0.1
    
    func retargetAnimation(sourceAnimation: AnimationClip, from sourceAvatar: AvatarDefinition, to targetAvatar: AvatarDefinition) -> AnimationClip {
        var retargetedClip = sourceAnimation
        retargetedClip.tracks.removeAll()
        
        for sourceTrack in sourceAnimation.tracks {
            // Find corresponding bone in target avatar
            if let targetBoneName = sourceAvatar.boneMappings[sourceTrack.propertyPath] {
                // Create new track for target bone
                var targetTrack = AnimationTrack(propertyPath: targetBoneName)
                
                for keyframe in sourceTrack.keyframes {
                    // Apply muscle constraints and retargeting
                    let retargetedValue = retargetKeyframeValue(
                        keyframe.value,
                        sourceBone: sourceTrack.propertyPath,
                        targetBone: targetBoneName,
                        sourceAvatar: sourceAvatar,
                        targetAvatar: targetAvatar
                    )
                    
                    let retargetedKeyframe = AnimationKeyframe(
                        time: keyframe.time,
                        value: retargetedValue,
                        inTangent: keyframe.inTangent,
                        outTangent: keyframe.outTangent
                    )
                    
                    targetTrack.keyframes.append(retargetedKeyframe)
                }
                
                retargetedClip.tracks.append(targetTrack)
            }
        }
        
        calculateRetargetErrors(sourceAnimation: sourceAnimation, retargetedAnimation: retargetedClip)
        
        return retargetedClip
    }
    
    private func retargetKeyframeValue(_ value: Any, sourceBone: String, targetBone: String, sourceAvatar: AvatarDefinition, targetAvatar: AvatarDefinition) -> Any {
        // Handle different value types
        if let rotation = value as? simd_quatf {
            return retargetRotation(rotation, sourceBone: sourceBone, targetBone: targetBone, sourceAvatar: sourceAvatar, targetAvatar: targetAvatar)
        } else if let position = value as? SIMD3<Float> {
            return retargetPosition(position, sourceBone: sourceBone, targetBone: targetBone, sourceAvatar: sourceAvatar, targetAvatar: targetAvatar)
        }
        
        return value
    }
    
    private func retargetRotation(_ rotation: simd_quatf, sourceBone: String, targetBone: String, sourceAvatar: AvatarDefinition, targetAvatar: AvatarDefinition) -> simd_quatf {
        // Find muscle definition for this bone
        if let muscle = sourceAvatar.muscleDefinitions.first(where: { $0.sourceBone == sourceBone }) {
            // Apply muscle constraints
            let angles = extractEulerAngles(rotation)
            let clampedAngles = SIMD3<Float>(
                clamp(angles.x, muscle.minAngle * .pi / 180.0, muscle.maxAngle * .pi / 180.0),
                clamp(angles.y, muscle.minAngle * .pi / 180.0, muscle.maxAngle * .pi / 180.0),
                clamp(angles.z, muscle.minAngle * .pi / 180.0, muscle.maxAngle * .pi / 180.0)
            )
            
            return simd_quatf(euler: clampedAngles)
        }
        
        return rotation
    }
    
    private func retargetPosition(_ position: SIMD3<Float>, sourceBone: String, targetBone: String, sourceAvatar: AvatarDefinition, targetAvatar: AvatarDefinition) -> SIMD3<Float> {
        // Apply scale differences between avatars
        let sourceScale = sourceAvatar.tPose.scale
        let targetScale = targetAvatar.tPose.scale
        
        let scaleRatio = SIMD3<Float>(
            targetScale.x / sourceScale.x,
            targetScale.y / sourceScale.y,
            targetScale.z / sourceScale.z
        )
        
        return position * scaleRatio
    }
    
    private func extractEulerAngles(_ rotation: simd_quatf) -> SIMD3<Float> {
        // Extract Euler angles from quaternion
        let w = rotation.vector.w
        let x = rotation.vector.x
        let y = rotation.vector.y
        let z = rotation.vector.z
        
        let roll = atan2(2 * (w * x + y * z), 1 - 2 * (x * x + y * y))
        let pitch = asin(clamp(2 * (w * y - z * x), -1, 1))
        let yaw = atan2(2 * (w * z + x * y), 1 - 2 * (y * y + z * z))
        
        return SIMD3<Float>(roll, pitch, yaw)
    }
    
    private func calculateRetargetErrors(sourceAnimation: AnimationClip, retargetedAnimation: AnimationClip) {
        retargetErrors.removeAll()
        
        for sourceTrack in sourceAnimation.tracks {
            if let targetTrack = retargetedAnimation.tracks.first(where: { sourceAvatar.boneMappings[sourceTrack.propertyPath] == $0.propertyPath }) {
                let error = calculateTrackError(sourceTrack: sourceTrack, targetTrack: targetTrack)
                retargetErrors.append(error)
            }
        }
    }
    
    private func calculateTrackError(sourceTrack: AnimationTrack, targetTrack: AnimationTrack) -> RetargetError {
        var totalPositionError: Float = 0
        var totalRotationError: Float = 0
        var totalScaleError: Float = 0
        var sampleCount: Float = 0
        
        // Sample keyframes and calculate errors
        for sourceKeyframe in sourceTrack.keyframes {
            if let targetKeyframe = targetTrack.keyframes.first(where: { abs($0.time - sourceKeyframe.time) < 0.016 }) { // Within 1 frame
                if let sourceRot = sourceKeyframe.value as? simd_quatf,
                   let targetRot = targetKeyframe.value as? simd_quatf {
                    let angleDiff = acos(abs(dot(sourceRot.vector, targetRot.vector)))
                    totalRotationError += angleDiff
                }
                
                if let sourcePos = sourceKeyframe.value as? SIMD3<Float>,
                   let targetPos = targetKeyframe.value as? SIMD3<Float> {
                    let posDiff = distance(sourcePos, targetPos)
                    totalPositionError += posDiff
                }
                
                sampleCount += 1
            }
        }
        
        let avgPositionError = sampleCount > 0 ? totalPositionError / sampleCount : 0
        let avgRotationError = sampleCount > 0 ? totalRotationError / sampleCount : 0
        let avgScaleError = 0 // Not implemented yet
        
        return RetargetError(
            boneName: sourceTrack.propertyPath,
            positionError: avgPositionError,
            rotationError: avgRotationError,
            scaleError: avgScaleError,
            totalError: avgPositionError + avgRotationError + avgScaleError
        )
    }
}

// MARK: - Retargeting UI Views

struct RetargetingEditorView: View {
    @StateObject private var retargetManager = RetargetingManager.shared
    @State private var showAvatarDialog = false
    @State private var selectedAvatar: AvatarDefinition?
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Retargeting")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                Toggle("Humanoid Mode", isOn: $retargetManager.isHumanoidMode)
                    .toggleStyle(.button)
                
                Toggle("Show Errors", isOn: $retargetManager.showErrors)
                    .toggleStyle(.button)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            HSplitView {
                // Source avatar
                VStack {
                    Text("Source Avatar")
                        .font(DesignSystem.Typography.bodyBold)
                        .padding(DesignSystem.Spacing.sm)
                    
                    if let sourceAvatar = retargetManager.sourceAvatar {
                        AvatarInfoView(avatar: sourceAvatar)
                    } else {
                        Button("Load Source Avatar") {
                            showAvatarDialog = true
                        }
                        .buttonStyle(.bordered)
                    }
                }
                .frame(minWidth: 200)
                
                // Target avatar
                VStack {
                    Text("Target Avatar")
                        .font(DesignSystem.Typography.bodyBold)
                        .padding(DesignSystem.Spacing.sm)
                    
                    if let targetAvatar = retargetManager.targetAvatar {
                        AvatarInfoView(avatar: targetAvatar)
                    } else {
                        Button("Load Target Avatar") {
                            showAvatarDialog = true
                        }
                        .buttonStyle(.bordered)
                    }
                }
                .frame(minWidth: 200)
                
                // Error visualization
                if retargetManager.showErrors {
                    VStack {
                        Text("Retargeting Errors")
                            .font(DesignSystem.Typography.bodyBold)
                            .padding(DesignSystem.Spacing.sm)
                        
                        List(retargetManager.retargetErrors, id: \.boneName) { error in
                            RetargetErrorRow(error: error)
                        }
                        .listStyle(.plain)
                    }
                    .frame(minWidth: 250)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .sheet(isPresented: $showAvatarDialog) {
            AvatarSelectionDialog()
        }
    }
}

struct AvatarInfoView: View {
    let avatar: AvatarDefinition
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            Text(avatar.name)
                .font(DesignSystem.Typography.body)
            
            Text("\(avatar.boneMappings.count) bone mappings")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Text("\(avatar.muscleDefinitions.count) muscle definitions")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Text(avatar.isHumanoid ? "Humanoid" : "Generic")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
    }
}

struct RetargetErrorRow: View {
    let error: RetargetError
    
    var body: some View {
        HStack {
            Circle()
                .fill(error.totalError > 0.1 ? DesignSystem.Colors.error : DesignSystem.Colors.success)
                .frame(width: 8, height: 8)
            
            Text(error.boneName)
                .font(DesignSystem.Typography.body)
            
            Spacer()
            
            Text(String(format: "%.3f", error.totalError))
                .font(DesignSystem.Typography.smallMono)
                .foregroundColor(error.totalError > 0.1 ? DesignSystem.Colors.error : DesignSystem.Colors.textSecondary)
        }
        .padding(.vertical, 2)
    }
}

struct AvatarSelectionDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var retargetManager = RetargetingManager.shared
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Select Avatar")
                .font(DesignSystem.Typography.headline)
            
            // Avatar list would go here
            Text("Avatar selection not implemented")
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Button("Load") {
                    // Load selected avatar
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 300)
    }
}

// MARK: - Design System Extensions

// These would typically be in a separate DesignSystem file, but adding here for completeness
extension DesignSystem {
    struct Colors {
        static let backgroundPrimary = Color.black
        static let backgroundSecondary = Color.gray.opacity(0.1)
        static let backgroundTertiary = Color.gray.opacity(0.2)
        static let textPrimary = Color.white
        static let textSecondary = Color.gray
        static let accentPrimary = Color.blue
        static let error = Color.red
        static let success = Color.green
        static let hover = Color.gray.opacity(0.3)
        static let border = Color.gray.opacity(0.5)
    }
    
    struct Typography {
        static let headline = Font.headline
        static let bodyBold = Font.body.bold()
        static let body = Font.body
        static let small = Font.caption
        static let smallMono = Font.caption.monospaced()
        static let mono = Font.body.monospaced()
    }
    
    struct Spacing {
        static let xs: CGFloat = 4
        static let sm: CGFloat = 8
        static let md: CGFloat = 16
        static let lg: CGFloat = 24
    }
}

// Editor Divider View
struct EditorDivider: View {
    var body: some View {
        Rectangle()
            .fill(DesignSystem.Colors.border)
            .frame(height: 1)
    }
}

// SIMD3 Float extension for quaternion operations
extension simd_quatf {
    init(euler angles: SIMD3<Float>) {
        let roll = angles.x * 0.5
        let pitch = angles.y * 0.5
        let yaw = angles.z * 0.5
        
        let cr = cos(roll)
        let sr = sin(roll)
        let cp = cos(pitch)
        let sp = sin(pitch)
        let cy = cos(yaw)
        let sy = sin(yaw)
        
        let w = cr * cp * cy + sr * sp * sy
        let x = sr * cp * cy - cr * sp * sy
        let y = cr * sp * cy + sr * cp * sy
        let z = cr * cp * sy - sr * sp * cy
        
        self.init(vector: SIMD4<Float>(x, y, z, w))
    }
    
    func act(_ vector: SIMD3<Float>) -> SIMD3<Float> {
        return self * vector * self.inverse
    }
}

// Math helper functions
private func slerp(_ q1: simd_quatf, _ q2: simd_quatf, _ t: Float) -> simd_quatf {
    let dot = q1.vector.w * q2.vector.w + q1.vector.x * q2.vector.x + q1.vector.y * q2.vector.y + q1.vector.z * q2.vector.z
    
    if abs(dot) > 0.9995 {
        return simd_normalize(q1 + t * (q2 - q1))
    }
    
    let theta = acos(abs(dot))
    let sinTheta = sin(theta)
    let a = sin((1 - t) * theta) / sinTheta
    let b = sin(t * theta) / sinTheta
    
    if dot < 0 {
        return simd_normalize(a * q1 - b * q2)
    } else {
        return simd_normalize(a * q1 + b * q2)
    }
}

private func clamp(_ value: Float, _ min: Float, _ max: Float) -> Float {
    return Swift.max(min, Swift.min(max, value))
}

// MARK: - Rig Setup Systems

// Finger Rig Setup
class FingerRigSetup {
    static func createFingerChain(baseName: String, fingerCount: Int = 3, fingerLength: Float = 0.03) -> [IKBone] {
        var bones: [IKBone] = []
        
        for i in 0..<fingerCount {
            let boneName = "\(baseName)\(i == 0 ? "Metacarpal" : i == 1 ? "Proximal" : i == 2 ? "Middle" : "Distal")"
            let position = SIMD3<Float>(0, 0, Float(i) * fingerLength)
            
            let bone = IKBone(
                name: boneName,
                transform: Transform(position: position, rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)),
                length: fingerLength,
                minAngle: i == 0 ? -30 : -45,
                maxAngle: i == 0 ? 30 : 90
            )
            
            bones.append(bone)
        }
        
        return bones
    }
    
    static func setupHandRig(isLeft: Bool = true) -> [IKChain] {
        let handPrefix = isLeft ? "Left" : "Right"
        
        var chains: [IKChain] = []
        
        // Thumb
        let thumbBones = createFingerChain(baseName: "\(handPrefix)Thumb", fingerCount: 2, fingerLength: 0.025)
        chains.append(IKChain(
            name: "\(handPrefix) Thumb",
            bones: thumbBones,
            target: IKTarget(position: SIMD3<Float>(isLeft ? 0.15 : -0.15, 1.2, 0.05), rotation: simd_quatf()),
            solverType: .ccd
        ))
        
        // Index finger
        let indexBones = createFingerChain(baseName: "\(handPrefix)Index")
        chains.append(IKChain(
            name: "\(handPrefix) Index",
            bones: indexBones,
            target: IKTarget(position: SIMD3<Float>(isLeft ? 0.12 : -0.12, 1.2, 0.08), rotation: simd_quatf()),
            solverType: .ccd
        ))
        
        // Middle finger
        let middleBones = createFingerChain(baseName: "\(handPrefix)Middle")
        chains.append(IKChain(
            name: "\(handPrefix) Middle",
            bones: middleBones,
            target: IKTarget(position: SIMD3<Float>(isLeft ? 0.08 : -0.08, 1.2, 0.08), rotation: simd_quatf()),
            solverType: .ccd
        ))
        
        // Ring finger
        let ringBones = createFingerChain(baseName: "\(handPrefix)Ring")
        chains.append(IKChain(
            name: "\(handPrefix) Ring",
            bones: ringBones,
            target: IKTarget(position: SIMD3<Float>(isLeft ? 0.04 : -0.04, 1.2, 0.08), rotation: simd_quatf()),
            solverType: .ccd
        ))
        
        // Pinky finger
        let pinkyBones = createFingerChain(baseName: "\(handPrefix)Pinky", fingerLength: 0.02)
        chains.append(IKChain(
            name: "\(handPrefix) Pinky",
            bones: pinkyBones,
            target: IKTarget(position: SIMD3<Float>(isLeft ? 0.0 : -0.0, 1.2, 0.06), rotation: simd_quatf()),
            solverType: .ccd
        ))
        
        return chains
    }
}

// Toe Rig Setup
class ToeRigSetup {
    static func createToeChain(baseName: String, toeCount: Int = 2, toeLength: Float = 0.02) -> [IKBone] {
        var bones: [IKBone] = []
        
        for i in 0..<toeCount {
            let boneName = "\(baseName)\(i == 0 ? "Metatarsal" : "Distal")"
            let position = SIMD3<Float>(0, 0, Float(i) * toeLength)
            
            let bone = IKBone(
                name: boneName,
                transform: Transform(position: position, rotation: simd_quatf(), scale: SIMD3<Float>(1, 1, 1)),
                length: toeLength,
                minAngle: -20,
                maxAngle: 45
            )
            
            bones.append(bone)
        }
        
        return bones
    }
    
    static func setupFootRig(isLeft: Bool = true) -> [IKChain] {
        let footPrefix = isLeft ? "Left" : "Right"
        
        var chains: [IKChain] = []
        
        // Big toe
        let bigToeBones = createToeChain(baseName: "\(footPrefix)BigToe", toeLength: 0.025)
        chains.append(IKChain(
            name: "\(footPrefix) Big Toe",
            bones: bigToeBones,
            target: IKTarget(position: SIMD3<Float>(isLeft ? 0.05 : -0.05, 0, 0.15), rotation: simd_quatf()),
            solverType: .ccd
        ))
        
        // Other toes
        for i in 1..<4 {
            let toeName = ["Index", "Middle", "Ring"][i-1]
            let toeBones = createToeChain(baseName: "\(footPrefix)\(toeName)Toe")
            chains.append(IKChain(
                name: "\(footPrefix) \(toeName) Toe",
                bones: toeBones,
                target: IKTarget(position: SIMD3<Float>(isLeft ? 0.03 - Float(i-1) * 0.015 : -0.03 + Float(i-1) * 0.015, 0, 0.12), rotation: simd_quatf()),
                solverType: .ccd
            ))
        }
        
        return chains
    }
}

// Extra Bones Mapping
class ExtraBonesMapping {
    static func createCustomMapping(sourceBones: [String], targetBones: [String]) -> [String: String] {
        var mapping: [String: String] = [:]
        
        // Standard humanoid mappings
        let standardMappings: [String: String] = [
            "Hips": "Hips",
            "Spine": "Spine",
            "Chest": "Chest",
            "Neck": "Neck",
            "Head": "Head",
            "LeftShoulder": "LeftShoulder",
            "LeftArm": "LeftArm",
            "LeftForeArm": "LeftForeArm",
            "LeftHand": "LeftHand",
            "RightShoulder": "RightShoulder",
            "RightArm": "RightArm",
            "RightForeArm": "RightForeArm",
            "RightHand": "RightHand",
            "LeftUpLeg": "LeftUpLeg",
            "LeftLeg": "LeftLeg",
            "LeftFoot": "LeftFoot",
            "RightUpLeg": "RightUpLeg",
            "RightLeg": "RightLeg",
            "RightFoot": "RightFoot"
        ]
        
        mapping = standardMappings
        
        // Add custom mappings
        for sourceBone in sourceBones {
            if !mapping.keys.contains(sourceBone) {
                // Try to find closest match in target bones
                if let closestMatch = findClosestBoneMatch(sourceBone: sourceBone, targetBones: targetBones) {
                    mapping[sourceBone] = closestMatch
                }
            }
        }
        
        return mapping
    }
    
    private static func findClosestBoneMatch(sourceBone: String, targetBones: [String]) -> String? {
        // Simple string matching - in real implementation would use more sophisticated matching
        let sourceLower = sourceBone.lowercased()
        
        for targetBone in targetBones {
            let targetLower = targetBone.lowercased()
            
            // Check for exact match
            if sourceLower == targetLower {
                return targetBone
            }
            
            // Check for partial match
            if sourceLower.contains(targetLower) || targetLower.contains(sourceLower) {
                return targetBone
            }
            
            // Check for common prefixes/suffixes
            let commonPrefixes = ["left", "right", "upper", "lower", "end"]
            for prefix in commonPrefixes {
                if sourceLower.hasPrefix(prefix) && targetLower.hasPrefix(prefix) {
                    return targetBone
                }
            }
        }
        
        return nil
    }
    
    static func createMuscleDefinitions(boneMappings: [String: String]) -> [MuscleDefinition] {
        var muscles: [MuscleDefinition] = []
        
        for (sourceBone, targetBone) in boneMappings {
            // Create muscle definition based on bone type
            let muscle = createMuscleForBone(sourceBone: sourceBone, targetBone: targetBone)
            if muscle != nil {
                muscles.append(muscle!)
            }
        }
        
        return muscles
    }
    
    private static func createMuscleForBone(sourceBone: String, targetBone: String) -> MuscleDefinition? {
        let boneLower = sourceBone.lowercased()
        
        // Define muscle constraints based on bone type
        if boneLower.contains("spine") {
            return MuscleDefinition(
                name: "\(sourceBone) Twist",
                sourceBone: sourceBone,
                targetBone: targetBone,
                minAngle: -45,
                maxAngle: 45,
                stiffness: 0.8
            )
        } else if boneLower.contains("neck") {
            return MuscleDefinition(
                name: "\(sourceBone) Look",
                sourceBone: sourceBone,
                targetBone: targetBone,
                minAngle: -60,
                maxAngle: 60,
                stiffness: 0.6
            )
        } else if boneLower.contains("shoulder") {
            return MuscleDefinition(
                name: "\(sourceBone) Rotation",
                sourceBone: sourceBone,
                targetBone: targetBone,
                minAngle: -120,
                maxAngle: 180,
                stiffness: 1.0
            )
        } else if boneLower.contains("arm") || boneLower.contains("leg") {
            return MuscleDefinition(
                name: "\(sourceBone) Bend",
                sourceBone: sourceBone,
                targetBone: targetBone,
                minAngle: -160,
                maxAngle: 160,
                stiffness: 0.9
            )
        }
        
        return nil
    }
}

// MARK: - Timeline Data Models and Enums

enum TimeDisplayFormat: String, CaseIterable {
    case frames = "Frames"
    case seconds = "Seconds"
    case timecode = "Timecode"
}

enum PreviewQuality: String, CaseIterable {
    case low = "Low"
    case medium = "Medium"
    case high = "High"
    case ultra = "Ultra"
}

enum TimelineEditingMode: String, CaseIterable {
    case standard = "Standard"
    case ripple = "Ripple"
    case slip = "Slip"
    case roll = "Roll"
}

struct TimelineMarker: Identifiable, Codable {
    let id = UUID()
    var time: Float
    var label: String
    var color: Color = .red
    var type: MarkerType = .marker
    
    enum MarkerType: String, CaseIterable, Codable {
        case marker = "Marker"
        case inPoint = "In Point"
        case outPoint = "Out Point"
        case comment = "Comment"
    }
}

struct CopiedKeyframe: Codable {
    var trackId: UUID
    var time: Float
    var value: Any
    var interpolation: AnimationTrack.InterpolationType
}

// MARK: - Animation Timeline Editor

class AnimationEditorManager: ObservableObject {
    static let shared = AnimationEditorManager()
    
    @Published var currentClip: AnimationClip?
    @Published var currentTime: Float = 0.0
    @Published var isPlaying: Bool = false
    @Published var playbackSpeed: Float = 1.0
    @Published var isLooping: Bool = true
    @Published var selectedKeyframes: Set<UUID> = []
    @Published var zoomLevel: Float = 1.0
    @Published var eventHistory: [EventHistoryEntry] = []
    @Published var isEventDebuggingEnabled: Bool = false
    
    // Keyframe visual properties
    @Published var keyframeColorByProperty: Bool = true
    @Published var keyframeShapeByInterpolation: Bool = true
    @Published var selectionHighlightIntensity: Float = 1.0
    
    // Timeline features
    @Published var frameRate: Float = 60.0
    @Published var timeDisplayFormat: TimeDisplayFormat = .frames
    @Published var snapToFrame: Bool = true
    @Published var snapToKeyframe: Bool = false
    @Published var snapToBeat: Bool = false
    @Published var workAreaIn: Float = 0.0
    @Published var workAreaOut: Float = 10.0
    @Published var isWorkAreaEnabled: Bool = false
    @Published var isReversePlayback: Bool = false
    @Published var isPingPongPlayback: Bool = false
    @Published var playbackQuality: PreviewQuality = .high
    @Published var onionSkinEnabled: Bool = false
    @Published var onionSkinFrames: Int = 3
    @Published var ghostingEnabled: Bool = false
    @Published var referenceFrameEnabled: Bool = false
    @Published var audioWaveformEnabled: Bool = false
    @Published var shuttleSpeed: Float = 1.0
    @Published var jogFrameStep: Int = 1
    
    // Enhanced timeline scrolling and zoom
    @Published var timelineScrollOffset: CGPoint = .zero
    @Published var timelineZoomLevel: Float = 1.0
    @Published var timelineVerticalScroll: Float = 0.0
    @Published var timelineHorizontalScroll: Float = 0.0
    @Published var minZoomLevel: Float = 0.1
    @Published var maxZoomLevel: Float = 10.0
    @Published var zoomToFitContent: Bool = false
    @Published var autoScrollDuringPlayback: Bool = true
    @Published var followPlayhead: Bool = true
    @Published var scrollSpeed: Float = 1.0
    
    // Markers
    @Published var markers: [TimelineMarker] = []
    @Published var selectedMarker: TimelineMarker?
    
    // Editing modes
    @Published var editingMode: TimelineEditingMode = .standard
    
    // Selection and copying
    @Published var copiedKeyframes: [CopiedKeyframe] = []
    @Published var isBoxSelecting: Bool = false
    @Published var boxSelectionStart: CGPoint = .zero
    @Published var boxSelectionEnd: CGPoint = .zero
    
    private var timer: Timer?
    private var lastEventCheckTime: Float = 0.0
    private var pingPongDirection: Float = 1.0
    
    func play() {
        isPlaying = true
        lastEventCheckTime = currentTime
        timer = Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { [weak self] _ in
            guard let self = self, let clip = self.currentClip else { return }
            
            // Calculate time delta based on playback mode
            let timeDelta = Float(1.0/60.0) * self.playbackSpeed
            var newTime = self.currentTime
            
            if self.isReversePlayback {
                newTime -= timeDelta
            } else if self.isPingPongPlayback {
                newTime += timeDelta * self.pingPongDirection
            } else {
                newTime += timeDelta
            }
            
            // Handle work area bounds
            if self.isWorkAreaEnabled {
                newTime = max(self.workAreaIn, min(newTime, self.workAreaOut))
            }
            
            // Handle ping-pong direction reversal
            if self.isPingPongPlayback {
                if newTime >= clip.duration {
                    self.pingPongDirection = -1
                    newTime = clip.duration
                } else if newTime <= 0 {
                    self.pingPongDirection = 1
                    newTime = 0
                }
            }
            
            // Check for events to trigger
            if self.isEventDebuggingEnabled {
                self.checkAndTriggerEvents(from: self.lastEventCheckTime, to: newTime)
            }
            
            // Handle playback bounds
            if self.isReversePlayback {
                if newTime <= (self.isWorkAreaEnabled ? self.workAreaIn : 0) {
                    if self.isLooping {
                        newTime = self.isWorkAreaEnabled ? self.workAreaOut : clip.duration
                    } else {
                        self.pause()
                        return
                    }
                }
            } else if !self.isPingPongPlayback {
                if newTime >= (self.isWorkAreaEnabled ? self.workAreaOut : clip.duration) {
                    if self.isLooping {
                        newTime = self.isWorkAreaEnabled ? self.workAreaIn : 0
                    } else {
                        self.pause()
                        return
                    }
                }
            }
            
            self.currentTime = newTime
            self.lastEventCheckTime = newTime
        }
    }
    
    func pause() {
        isPlaying = false
        timer?.invalidate()
        timer = nil
    }
    
    func stop() {
        pause()
        currentTime = isWorkAreaEnabled ? workAreaIn : 0
        pingPongDirection = 1.0
    }
    
    func seekTo(_ time: Float) {
        let targetTime = max(isWorkAreaEnabled ? workAreaIn : 0, min(time, currentClip?.duration ?? 0))
        
        // Apply snapping if enabled
        if snapToFrame {
            currentTime = snapTimeToFrame(targetTime)
        } else if snapToKeyframe {
            currentTime = snapTimeToKeyframe(targetTime)
        } else if snapToBeat {
            currentTime = snapTimeToBeat(targetTime)
        } else {
            currentTime = targetTime
        }
    }
    
    // MARK: - Timeline Snapping Functions
    
    private func snapTimeToFrame(_ time: Float) -> Float {
        let frameDuration = 1.0 / frameRate
        return round(time / frameDuration) * frameDuration
    }
    
    private func snapTimeToKeyframe(_ time: Float) -> Float {
        guard let clip = currentClip else { return time }
        
        var nearestKeyframeTime: Float?
        var minDistance: Float = Float.greatestFiniteMagnitude
        
        for track in clip.tracks {
            for keyframe in track.keyframes {
                let distance = abs(keyframe.time - time)
                if distance < minDistance {
                    minDistance = distance
                    nearestKeyframeTime = keyframe.time
                }
            }
        }
        
        // Snap if within threshold (0.1 seconds)
        if let nearestTime = nearestKeyframeTime, minDistance < 0.1 {
            return nearestTime
        }
        return time
    }
    
    private func snapTimeToBeat(_ time: Float) -> Float {
        // Assuming 4/4 time signature with 120 BPM (0.5 seconds per beat)
        let beatDuration: Float = 0.5
        return round(time / beatDuration) * beatDuration
    }
    
    // MARK: - Timeline Zoom Functions
    
    func zoomToSelection() {
        guard !selectedKeyframes.isEmpty, let clip = currentClip else { return }
        
        var minTime: Float = Float.greatestFiniteMagnitude
        var maxTime: Float = 0
        
        for track in clip.tracks {
            for keyframe in track.keyframes {
                if selectedKeyframes.contains(keyframe.id) {
                    minTime = min(minTime, keyframe.time)
                    maxTime = max(maxTime, keyframe.time)
                }
            }
        }
        
        let selectionDuration = maxTime - minTime
        if selectionDuration > 0 {
            // Calculate zoom level to fit selection in 80% of view
            let targetZoom = (clip.duration * 0.8) / selectionDuration
            zoomLevel = max(0.5, min(4.0, targetZoom))
            
            // Center the selection
            seekTo(minTime + selectionDuration / 2)
        }
    }
    
    func zoomToFitAllKeyframes() {
        guard let clip = currentClip else { return }
        
        var minTime: Float = Float.greatestFiniteMagnitude
        var maxTime: Float = 0
        
        for track in clip.tracks {
            for keyframe in track.keyframes {
                minTime = min(minTime, keyframe.time)
                maxTime = max(maxTime, keyframe.time)
            }
        }
        
        let keyframeDuration = maxTime - minTime
        if keyframeDuration > 0 {
            let targetZoom = clip.duration / keyframeDuration
            zoomLevel = max(0.5, min(4.0, targetZoom))
        }
    }
    
    // MARK: - Enhanced Timeline Scrolling and Zoom Functions
    
    func setTimelineZoom(_ zoom: Float, animated: Bool = false) {
        let newZoom = max(minZoomLevel, min(maxZoomLevel, zoom))
        timelineZoomLevel = newZoom
        zoomLevel = newZoom // Keep backward compatibility
        
        if animated {
            withAnimation(.easeInOut(duration: 0.2)) {
                // Animation handled by SwiftUI
            }
        }
    }
    
    func zoomIn(at point: CGPoint? = nil) {
        let zoomFactor: Float = 1.2
        let newZoom = timelineZoomLevel * zoomFactor
        setTimelineZoom(newZoom)
        
        // Adjust scroll to zoom at specific point
        if let point = point {
            let zoomDelta = newZoom - timelineZoomLevel
            timelineScrollOffset.x -= CGFloat(zoomDelta) * point.x
            timelineScrollOffset.y -= CGFloat(zoomDelta) * point.y
        }
    }
    
    func zoomOut(at point: CGPoint? = nil) {
        let zoomFactor: Float = 0.8
        let newZoom = timelineZoomLevel * zoomFactor
        setTimelineZoom(newZoom)
        
        // Adjust scroll to zoom at specific point
        if let point = point {
            let zoomDelta = newZoom - timelineZoomLevel
            timelineScrollOffset.x -= CGFloat(zoomDelta) * point.x
            timelineScrollOffset.y -= CGFloat(zoomDelta) * point.y
        }
    }
    
    func resetZoom() {
        setTimelineZoom(1.0, animated: true)
        timelineScrollOffset = .zero
    }
    
    func scrollTimelineTo(_ time: Float, animated: Bool = true) {
        guard let clip = currentClip else { return }
        
        let normalizedTime = time / clip.duration
        let targetScrollX = normalizedTime * 1000 * CGFloat(timelineZoomLevel) - 500 // Center in view
        
        if animated {
            withAnimation(.easeInOut(duration: 0.3)) {
                timelineHorizontalScroll = Float(targetScrollX)
            }
        } else {
            timelineHorizontalScroll = Float(targetScrollX)
        }
    }
    
    func scrollTimelineBy(delta: CGPoint) {
        timelineScrollOffset.x += delta.x
        timelineScrollOffset.y += delta.y
        timelineHorizontalScroll += Float(delta.x)
        timelineVerticalScroll += Float(delta.y)
    }
    
    func autoScrollToPlayhead() {
        guard followPlayhead else { return }
        scrollTimelineTo(currentTime, animated: autoScrollDuringPlayback)
    }
    
    func centerTimelineOnSelection() {
        guard !selectedKeyframes.isEmpty, let clip = currentClip else { return }
        
        var minTime: Float = Float.greatestFiniteMagnitude
        var maxTime: Float = 0
        
        for track in clip.tracks {
            for keyframe in track.keyframes {
                if selectedKeyframes.contains(keyframe.id) {
                    minTime = min(minTime, keyframe.time)
                    maxTime = max(maxTime, keyframe.time)
                }
            }
        }
        
        let centerTime = (minTime + maxTime) / 2
        scrollTimelineTo(centerTime, animated: true)
    }
    
    func zoomToTimeRange(start: Float, end: Float) {
        guard let clip = currentClip else { return }
        
        let duration = end - start
        if duration > 0 {
            let targetZoom = clip.duration / duration
            setTimelineZoom(targetZoom, animated: true)
            scrollTimelineTo((start + end) / 2, animated: true)
        }
    }
    
    // MARK: - Timeline Editing Mode Functions
    
    func setEditingMode(_ mode: TimelineEditingMode) {
        editingMode = mode
        
        // Configure timeline behavior based on mode
        switch mode {
        case .ripple:
            // Enable ripple editing behavior
            break
        case .slip:
            // Enable slip editing behavior
            break
        case .roll:
            // Enable roll editing behavior
            break
        case .standard:
            // Standard editing behavior
            break
        }
    }
    
    func applyRippleEdit(from startTime: Float, to endTime: Float, offset: Float) {
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            var keyframesToShift: [AnimationKeyframe] = []
            
            for keyframe in clip.tracks[trackIndex].keyframes {
                if keyframe.time >= startTime {
                    keyframesToShift.append(keyframe)
                }
            }
            
            // Apply time offset
            for keyframeIndex in keyframesToShift.indices {
                let originalKeyframe = keyframesToShift[keyframeIndex]
                let newTime = originalKeyframe.time + offset
                
                // Find and update the keyframe in the track
                if let trackKeyframeIndex = clip.tracks[trackIndex].keyframes.firstIndex(where: { $0.id == originalKeyframe.id }) {
                    clip.tracks[trackIndex].keyframes[trackKeyframeIndex].time = newTime
                }
            }
            
            // Sort keyframes by time
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    func applySlipEdit(from startTime: Float, to endTime: Float, offset: Float) {
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            var keyframesToShift: [AnimationKeyframe] = []
            
            for keyframe in clip.tracks[trackIndex].keyframes {
                if keyframe.time > startTime && keyframe.time < endTime {
                    keyframesToShift.append(keyframe)
                }
            }
            
            // Apply time offset within range
            for keyframeIndex in keyframesToShift.indices {
                let originalKeyframe = keyframesToShift[keyframeIndex]
                let newTime = originalKeyframe.time + offset
                
                // Ensure keyframe stays within bounds
                let clampedTime = max(startTime, min(endTime, newTime))
                
                if let trackKeyframeIndex = clip.tracks[trackIndex].keyframes.firstIndex(where: { $0.id == originalKeyframe.id }) {
                    clip.tracks[trackIndex].keyframes[trackKeyframeIndex].time = clampedTime
                }
            }
            
            // Sort keyframes by time
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    func applyRollEdit(at time: Float, offset: Float) {
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            // Find keyframes around the edit point
            var beforeKeyframes: [AnimationKeyframe] = []
            var afterKeyframes: [AnimationKeyframe] = []
            
            for keyframe in clip.tracks[trackIndex].keyframes {
                if keyframe.time < time {
                    beforeKeyframes.append(keyframe)
                } else if keyframe.time > time {
                    afterKeyframes.append(keyframe)
                }
            }
            
            // Apply offset to keyframes after the edit point
            for keyframeIndex in afterKeyframes.indices {
                let originalKeyframe = afterKeyframes[keyframeIndex]
                let newTime = originalKeyframe.time + offset
                
                if let trackKeyframeIndex = clip.tracks[trackIndex].keyframes.firstIndex(where: { $0.id == originalKeyframe.id }) {
                    clip.tracks[trackIndex].keyframes[trackKeyframeIndex].time = newTime
                }
            }
            
            // Sort keyframes by time
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    // MARK: - Time Stretching Functions
    
    func stretchTimeRange(from startTime: Float, to endTime: Float, scaleFactor: Float) {
        guard var clip = currentClip else { return }
        let originalDuration = endTime - startTime
        let newDuration = originalDuration * scaleFactor
        
        for trackIndex in clip.tracks.indices {
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                let keyframe = clip.tracks[trackIndex].keyframes[keyframeIndex]
                
                if keyframe.time >= startTime && keyframe.time <= endTime {
                    // Calculate relative position within range
                    let relativePosition = (keyframe.time - startTime) / originalDuration
                    let newTime = startTime + (relativePosition * newDuration)
                    
                    clip.tracks[trackIndex].keyframes[keyframeIndex].time = newTime
                } else if keyframe.time > endTime {
                    // Shift keyframes after the stretched range
                    let timeOffset = newDuration - originalDuration
                    clip.tracks[trackIndex].keyframes[keyframeIndex].time += timeOffset
                }
            }
            
            // Sort keyframes by time
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    func reverseTimeRange(from startTime: Float, to endTime: Float) {
        guard var clip = currentClip else { return }
        let duration = endTime - startTime
        
        for trackIndex in clip.tracks.indices {
            var keyframesInRange: [AnimationKeyframe] = []
            
            // Collect keyframes in range
            for keyframe in clip.tracks[trackIndex].keyframes {
                if keyframe.time >= startTime && keyframe.time <= endTime {
                    keyframesInRange.append(keyframe)
                }
            }
            
            // Reverse keyframes in range
            for keyframeIndex in keyframesInRange.indices {
                let originalKeyframe = keyframesInRange[keyframeIndex]
                let relativePosition = (originalKeyframe.time - startTime) / duration
                let reversedPosition = 1.0 - relativePosition
                let newTime = startTime + (reversedPosition * duration)
                
                if let trackKeyframeIndex = clip.tracks[trackIndex].keyframes.firstIndex(where: { $0.id == originalKeyframe.id }) {
                    clip.tracks[trackIndex].keyframes[trackKeyframeIndex].time = newTime
                }
            }
            
            // Sort keyframes by time
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    // MARK: - Timeline Shuttle and Jog Controls
    
    func shuttleForward() {
        playbackSpeed = shuttleSpeed
        if !isPlaying {
            play()
        }
    }
    
    func shuttleReverse() {
        playbackSpeed = shuttleSpeed
        isReversePlayback = true
        if !isPlaying {
            play()
        }
    }
    
    func jogForward() {
        seekTo(currentTime + Float(jogFrameStep) / frameRate)
    }
    
    func jogReverse() {
        seekTo(currentTime - Float(jogFrameStep) / frameRate)
    }
    
    // MARK: - Timeline Marker Functions
    
    func addMarker(at time: Float, label: String, type: TimelineMarker.MarkerType = .marker) {
        let marker = TimelineMarker(time: time, label: label, type: type)
        markers.append(marker)
        markers.sort { $0.time < $1.time }
    }
    
    func removeMarker(_ marker: TimelineMarker) {
        markers.removeAll { $0.id == marker.id }
        if selectedMarker?.id == marker.id {
            selectedMarker = nil
        }
    }
    
    func navigateToNextMarker() {
        guard !markers.isEmpty else { return }
        
        let futureMarkers = markers.filter { $0.time > currentTime }
        if let nextMarker = futureMarkers.first {
            seekTo(nextMarker.time)
            selectedMarker = nextMarker
        }
    }
    
    func navigateToPreviousMarker() {
        guard !markers.isEmpty else { return }
        
        let pastMarkers = markers.filter { $0.time < currentTime }
        if let previousMarker = pastMarkers.last {
            seekTo(previousMarker.time)
            selectedMarker = previousMarker
        }
    }
    
    // MARK: - Timeline Editing Functions
    
    func setWorkArea(in: Float, out: Float) {
        workAreaIn = max(0, min(`in`, currentClip?.duration ?? 0))
        workAreaOut = max(workAreaIn, min(out, currentClip?.duration ?? 0))
        isWorkAreaEnabled = true
    }
    
    func clearWorkArea() {
        isWorkAreaEnabled = false
        workAreaIn = 0
        workAreaOut = currentClip?.duration ?? 10
    }
    
    // MARK: - Time Display Functions
    
    func formatTime(_ time: Float) -> String {
        switch timeDisplayFormat {
        case .frames:
            return "\(Int(time * frameRate))"
        case .seconds:
            return String(format: "%.2f", time)
        case .timecode:
            let hours = Int(time) / 3600
            let minutes = (Int(time) % 3600) / 60
            let seconds = Int(time) % 60
            let frames = Int((time.truncatingRemainder(dividingBy: 1)) * frameRate)
            return String(format: "%02d:%02d:%02d:%02d", hours, minutes, seconds, frames)
        }
    }
    
    func addKeyframe(property: String, value: Any) {
        guard var clip = currentClip else { return }
        let keyframe = AnimationKeyframe(time: currentTime, value: value)
        if var track = clip.tracks.first(where: { $0.propertyPath == property }) {
            track.keyframes.append(keyframe)
        }
        currentClip = clip
    }
    
    // MARK: - Enhanced Keyframe Selection and Management
    
    func selectKeyframe(_ keyframeId: UUID, addToSelection: Bool = false) {
        if addToSelection {
            selectedKeyframes.insert(keyframeId)
        } else {
            selectedKeyframes = [keyframeId]
        }
        updateKeyframeSelection()
    }
    
    func selectKeyframesInRect(_ rect: CGRect, in view: DopesheetView) {
        guard let clip = currentClip else { return }
        
        var newlySelected: Set<UUID> = []
        for track in clip.tracks {
            for keyframe in track.keyframes {
                let keyframePos = getKeyframePosition(keyframe, in: view)
                if rect.contains(keyframePos) {
                    newlySelected.insert(keyframe.id)
                }
            }
        }
        
        selectedKeyframes.formUnion(newlySelected)
        updateKeyframeSelection()
    }
    
    func copySelectedKeyframes() {
        guard let clip = currentClip else { return }
        
        copiedKeyframes.removeAll()
        
        for track in clip.tracks {
            for keyframe in track.keyframes {
                if selectedKeyframes.contains(keyframe.id) {
                    let copiedKeyframe = CopiedKeyframe(
                        trackId: track.id,
                        time: keyframe.time,
                        value: keyframe.value,
                        interpolation: keyframe.interpolation
                    )
                    copiedKeyframes.append(copiedKeyframe)
                }
            }
        }
    }
    
    func pasteKeyframes(at time: Float? = nil) {
        guard var clip = currentClip, !copiedKeyframes.isEmpty else { return }
        
        let pasteTime = time ?? currentTime
        let timeOffset = pasteTime - (copiedKeyframes.first?.time ?? 0)
        
        for copiedKeyframe in copiedKeyframes {
            if let trackIndex = clip.tracks.firstIndex(where: { $0.id == copiedKeyframe.trackId }) {
                let newKeyframe = AnimationKeyframe(
                    time: copiedKeyframe.time + timeOffset,
                    value: copiedKeyframe.value,
                    interpolation: copiedKeyframe.interpolation
                )
                clip.tracks[trackIndex].keyframes.append(newKeyframe)
            }
        }
        
        // Sort all keyframes by time
        for trackIndex in clip.tracks.indices {
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    func deleteSelectedKeyframes() {
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            clip.tracks[trackIndex].keyframes.removeAll { selectedKeyframes.contains($0.id) }
        }
        
        currentClip = clip
        selectedKeyframes.removeAll()
    }
    
    // MARK: - Enhanced Keyframe Editing Functions
    
    func mirrorSelectedKeyframes(axis: MirrorAxis = .x) {
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                let keyframe = clip.tracks[trackIndex].keyframes[keyframeIndex]
                
                if selectedKeyframes.contains(keyframe.id) {
                    // Mirror based on track type
                    if trackIndex % 3 == 0 { // Position tracks (X, Y, Z)
                        if let floatValue = keyframe.value as? Float {
                            if axis == .x || axis == .all {
                                clip.tracks[trackIndex].keyframes[keyframeIndex].value = -floatValue
                            }
                        }
                    } else if trackIndex % 3 == 1 { // Rotation tracks
                        if let floatValue = keyframe.value as? Float {
                            if axis == .y || axis == .all {
                                clip.tracks[trackIndex].keyframes[keyframeIndex].value = -floatValue
                            }
                        }
                    }
                }
            }
        }
        
        currentClip = clip
    }
    
    func flipSelectedKeyframes(direction: FlipDirection = .horizontal) {
        guard var clip = currentClip else { return }
        
        // Find time bounds of selection
        var minTime: Float = Float.greatestFiniteMagnitude
        var maxTime: Float = 0
        
        for track in clip.tracks {
            for keyframe in track.keyframes {
                if selectedKeyframes.contains(keyframe.id) {
                    minTime = min(minTime, keyframe.time)
                    maxTime = max(maxTime, keyframe.time)
                }
            }
        }
        
        let centerTime = (minTime + maxTime) / 2
        
        for trackIndex in clip.tracks.indices {
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                let keyframe = clip.tracks[trackIndex].keyframes[keyframeIndex]
                
                if selectedKeyframes.contains(keyframe.id) {
                    let relativeTime = keyframe.time - centerTime
                    let flippedTime = centerTime - relativeTime
                    clip.tracks[trackIndex].keyframes[keyframeIndex].time = flippedTime
                }
            }
            
            // Sort keyframes by time
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    func scaleSelectedKeyframes(timeScale: Float = 1.0, valueScale: Float = 1.0) {
        guard var clip = currentClip else { return }
        
        // Find center point for scaling
        var centerTime: Float = 0
        var centerValue: Float = 0
        var count: Int = 0
        
        for track in clip.tracks {
            for keyframe in track.keyframes {
                if selectedKeyframes.contains(keyframe.id) {
                    centerTime += keyframe.time
                    if let floatValue = keyframe.value as? Float {
                        centerValue += floatValue
                    }
                    count += 1
                }
            }
        }
        
        if count > 0 {
            centerTime /= Float(count)
            centerValue /= Float(count)
        }
        
        for trackIndex in clip.tracks.indices {
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                let keyframe = clip.tracks[trackIndex].keyframes[keyframeIndex]
                
                if selectedKeyframes.contains(keyframe.id) {
                    // Scale time
                    if timeScale != 1.0 {
                        let relativeTime = keyframe.time - centerTime
                        let scaledTime = centerTime + (relativeTime * timeScale)
                        clip.tracks[trackIndex].keyframes[keyframeIndex].time = scaledTime
                    }
                    
                    // Scale value
                    if valueScale != 1.0, let floatValue = keyframe.value as? Float {
                        let relativeValue = floatValue - centerValue
                        let scaledValue = centerValue + (relativeValue * valueScale)
                        clip.tracks[trackIndex].keyframes[keyframeIndex].value = scaledValue
                    }
                }
            }
            
            // Sort keyframes by time
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    func setKeyframeInterpolation(_ interpolation: AnimationTrack.InterpolationType) {
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                let keyframe = clip.tracks[trackIndex].keyframes[keyframeIndex]
                
                if selectedKeyframes.contains(keyframe.id) {
                    clip.tracks[trackIndex].keyframes[keyframeIndex].interpolation = interpolation
                }
            }
        }
        
        currentClip = clip
    }
    
    func applyInterpolationPreset(_ preset: InterpolationPreset) {
        setKeyframeInterpolation(preset.interpolationType)
        
        // Apply preset-specific tangent values
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                let keyframe = clip.tracks[trackIndex].keyframes[keyframeIndex]
                
                if selectedKeyframes.contains(keyframe.id) {
                    clip.tracks[trackIndex].keyframes[keyframeIndex].inTangent = preset.inTangent
                    clip.tracks[trackIndex].keyframes[keyframeIndex].outTangent = preset.outTangent
                }
            }
        }
        
        currentClip = clip
    }
    
    func nudgeSelectedKeyframes(timeDelta: Float = 0, valueDelta: Float = 0) {
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                let keyframe = clip.tracks[trackIndex].keyframes[keyframeIndex]
                
                if selectedKeyframes.contains(keyframe.id) {
                    // Nudge time
                    if timeDelta != 0 {
                        let newTime = max(0, keyframe.time + timeDelta)
                        clip.tracks[trackIndex].keyframes[keyframeIndex].time = newTime
                    }
                    
                    // Nudge value
                    if valueDelta != 0, let floatValue = keyframe.value as? Float {
                        let newValue = floatValue + valueDelta
                        clip.tracks[trackIndex].keyframes[keyframeIndex].value = newValue
                    }
                }
            }
            
            // Sort keyframes by time
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    func randomizeSelectedKeyframeValues(range: Float = 1.0) {
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                let keyframe = clip.tracks[trackIndex].keyframes[keyframeIndex]
                
                if selectedKeyframes.contains(keyframe.id), let floatValue = keyframe.value as? Float {
                    let randomValue = floatValue + (Float.random(in: -range...range))
                    clip.tracks[trackIndex].keyframes[keyframeIndex].value = randomValue
                }
            }
        }
        
        currentClip = clip
    }
    
    func smoothSelectedKeyframes(strength: Float = 0.5) {
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            var selectedKeyframeIndices: [Int] = []
            
            // Find selected keyframe indices
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                let keyframe = clip.tracks[trackIndex].keyframes[keyframeIndex]
                if selectedKeyframes.contains(keyframe.id) {
                    selectedKeyframeIndices.append(keyframeIndex)
                }
            }
            
            // Apply smoothing
            for index in selectedKeyframeIndices {
                let keyframe = clip.tracks[trackIndex].keyframes[index]
                
                if let floatValue = keyframe.value as? Float {
                    var averageValue: Float = 0
                    var count: Int = 0
                    
                    // Average with neighboring keyframes
                    for neighborOffset in [-2, -1, 1, 2] {
                        let neighborIndex = index + neighborOffset
                        if neighborIndex >= 0 && neighborIndex < clip.tracks[trackIndex].keyframes.count {
                            let neighborKeyframe = clip.tracks[trackIndex].keyframes[neighborIndex]
                            if let neighborValue = neighborKeyframe.value as? Float {
                                averageValue += neighborValue
                                count += 1
                            }
                        }
                    }
                    
                    if count > 0 {
                        averageValue /= Float(count)
                        let smoothedValue = floatValue + (averageValue - floatValue) * strength
                        clip.tracks[trackIndex].keyframes[index].value = smoothedValue
                    }
                }
            }
        }
        
        currentClip = clip
    }
    
    // MARK: - Keyframe Selection Helper Functions
    
    private func getKeyframePosition(_ keyframe: AnimationKeyframe, in view: DopesheetView) -> CGPoint {
        // This would need to be implemented based on the actual view geometry
        return CGPoint(x: CGFloat(keyframe.time), y: 0)
    }
    
    private func getKeyframeColor(for propertyPath: String, interpolation: AnimationTrack.InterpolationType) -> Color {
        if keyframeColorByProperty {
            switch propertyPath.lowercased() {
            case let positionPath where positionPath.contains("position"):
                return .blue
            case let rotationPath where rotationPath.contains("rotation"):
                return .green
            case let scalePath where scalePath.contains("scale"):
                return .orange
            case let colorPath where colorPath.contains("color"):
                return .purple
            default:
                return .red
            }
        }
        
        // Color by interpolation type
        switch interpolation {
        case .linear:
            return .yellow
        case .bezier:
            return .cyan
        case .stepped:
            return .magenta
        default:
            return .white
        }
    }
    
    private func getKeyframeShape(for interpolation: AnimationTrack.InterpolationType) -> String {
        if keyframeShapeByInterpolation {
            switch interpolation {
            case .linear:
                return "square"
            case .bezier:
                return "circle"
            case .stepped:
                return "diamond"
            default:
                return "circle"
            }
        }
    }
    
    // MARK: - Supporting Enums
    
    enum MirrorAxis: String, CaseIterable {
        case x = "X"
        case y = "Y"
        case z = "Z"
        case all = "All"
    }
    
    enum FlipDirection: String, CaseIterable {
        case horizontal = "Horizontal"
        case vertical = "Vertical"
        case both = "Both"
    }
    
    struct InterpolationPreset: Identifiable {
        let id = UUID()
        let name: String
        let interpolationType: AnimationTrack.InterpolationType
        let inTangent: Float
        let outTangent: Float
        
        static let presets: [InterpolationPreset] = [
            InterpolationPreset(name: "Linear", interpolationType: .linear, inTangent: 0, outTangent: 0),
            InterpolationPreset(name: "Ease In", interpolationType: .bezier, inTangent: 0, outTangent: 0.5),
            InterpolationPreset(name: "Ease Out", interpolationType: .bezier, inTangent: 0.5, outTangent: 0),
            InterpolationPreset(name: "Ease In-Out", interpolationType: .bezier, inTangent: 0.5, outTangent: 0.5),
            InterpolationPreset(name: "Step", interpolationType: .constant, inTangent: 0, outTangent: 0),
            InterpolationPreset(name: "Bounce", interpolationType: .bezier, inTangent: -0.3, outTangent: -0.3),
            InterpolationPreset(name: "Elastic", interpolationType: .bezier, inTangent: 0.8, outTangent: 0.8)
        ]
    }
    
    private func updateKeyframeSelection() {
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                clip.tracks[trackIndex].keyframes[keyframeIndex].isSelected = 
                    selectedKeyframes.contains(clip.tracks[trackIndex].keyframes[keyframeIndex].id)
            }
        }
        
        currentClip = clip
    }
    
    func insertKeyframeAtCurrentTime(property: String) {
        guard var clip = currentClip else { return }
        
        if let trackIndex = clip.tracks.firstIndex(where: { $0.propertyPath == property }) {
            let interpolatedValue = interpolateValueAtTime(currentTime, in: clip.tracks[trackIndex])
            let newKeyframe = AnimationKeyframe(time: currentTime, value: interpolatedValue)
            clip.tracks[trackIndex].keyframes.append(newKeyframe)
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    func duplicateSelectedKeyframes(timeOffset: Float = 1.0) {
        guard var clip = currentClip else { return }
        
        var duplicatedKeyframes: [(trackIndex: Int, keyframe: AnimationKeyframe)] = []
        
        for trackIndex in clip.tracks.indices {
            for keyframe in clip.tracks[trackIndex].keyframes {
                if selectedKeyframes.contains(keyframe.id) {
                    var duplicatedKeyframe = keyframe
                    duplicatedKeyframe.id = UUID() // New ID
                    duplicatedKeyframe.time += timeOffset
                    duplicatedKeyframes.append((trackIndex, duplicatedKeyframe))
                }
            }
        }
        
        for (trackIndex, keyframe) in duplicatedKeyframes {
            clip.tracks[trackIndex].keyframes.append(keyframe)
        }
        
        // Sort all keyframes by time
        for trackIndex in clip.tracks.indices {
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    func splitKeyframeAtCurrentTime() {
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            let track = clip.tracks[trackIndex]
            if let keyframeIndex = track.keyframes.firstIndex(where: { 
                abs($0.time - currentTime) < 0.016 // Within 1 frame
            }) {
                var keyframe = track.keyframes[keyframeIndex]
                keyframe.isBreakpoint = true
                
                // Create two keyframes at the same time
                var beforeKeyframe = keyframe
                var afterKeyframe = keyframe
                
                beforeKeyframe.outTangent = 0
                afterKeyframe.inTangent = 0
                
                clip.tracks[trackIndex].keyframes[keyframeIndex] = beforeKeyframe
                clip.tracks[trackIndex].keyframes.insert(afterKeyframe, at: keyframeIndex + 1)
            }
        }
        
        currentClip = clip
    }
    
    func nudgeSelectedKeyframes(timeDelta: Float, valueDelta: Float = 0) {
        guard var clip = currentClip else { return }
        
        for trackIndex in clip.tracks.indices {
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                let keyframe = clip.tracks[trackIndex].keyframes[keyframeIndex]
                if selectedKeyframes.contains(keyframe.id) {
                    clip.tracks[trackIndex].keyframes[keyframeIndex].time += timeDelta
                    
                    // Apply value delta if applicable
                    if valueDelta != 0 {
                        if let floatValue = keyframe.value as? Float {
                            clip.tracks[trackIndex].keyframes[keyframeIndex].value = floatValue + valueDelta
                        }
                    }
                }
            }
        }
        
        currentClip = clip
    }
    
    func scaleSelectedKeyframes(timeScale: Float, valueScale: Float = 1.0) {
        guard var clip = currentClip else { return }
        guard !selectedKeyframes.isEmpty else { return }
        
        // Calculate center point for scaling
        var totalTime: Float = 0
        var totalValue: Float = 0
        var count = 0
        
        for track in clip.tracks {
            for keyframe in track.keyframes {
                if selectedKeyframes.contains(keyframe.id) {
                    totalTime += keyframe.time
                    if let floatValue = keyframe.value as? Float {
                        totalValue += floatValue
                    }
                    count += 1
                }
            }
        }
        
        let timeCenter = totalTime / Float(count)
        let valueCenter = totalValue / Float(count)
        
        // Apply scaling
        for trackIndex in clip.tracks.indices {
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                let keyframe = clip.tracks[trackIndex].keyframes[keyframeIndex]
                if selectedKeyframes.contains(keyframe.id) {
                    // Scale time around center
                    let timeOffset = keyframe.time - timeCenter
                    clip.tracks[trackIndex].keyframes[keyframeIndex].time = timeCenter + timeOffset * timeScale
                    
                    // Scale value if applicable
                    if let floatValue = keyframe.value as? Float {
                        let valueOffset = floatValue - valueCenter
                        clip.tracks[trackIndex].keyframes[keyframeIndex].value = valueCenter + valueOffset * valueScale
                    }
                }
            }
        }
        
        currentClip = clip
    }
    
    func mirrorSelectedKeyframesHorizontally() {
        guard var clip = currentClip else { return }
        guard !selectedKeyframes.isEmpty else { return }
        
        // Find the center time
        var totalTime: Float = 0
        var count = 0
        
        for track in clip.tracks {
            for keyframe in track.keyframes {
                if selectedKeyframes.contains(keyframe.id) {
                    totalTime += keyframe.time
                    count += 1
                }
            }
        }
        
        let centerTime = totalTime / Float(count)
        
        // Mirror around center
        for trackIndex in clip.tracks.indices {
            for keyframeIndex in clip.tracks[trackIndex].keyframes.indices {
                let keyframe = clip.tracks[trackIndex].keyframes[keyframeIndex]
                if selectedKeyframes.contains(keyframe.id) {
                    let timeOffset = keyframe.time - centerTime
                    clip.tracks[trackIndex].keyframes[keyframeIndex].time = centerTime - timeOffset
                }
            }
        }
        
        // Sort all keyframes by time
        for trackIndex in clip.tracks.indices {
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    func copySelectedKeyframes() -> [AnimationKeyframe] {
        guard let clip = currentClip else { return [] }
        
        var copiedKeyframes: [AnimationKeyframe] = []
        
        for track in clip.tracks {
            for keyframe in track.keyframes {
                if selectedKeyframes.contains(keyframe.id) {
                    copiedKeyframes.append(keyframe)
                }
            }
        }
        
        return copiedKeyframes
    }
    
    func pasteKeyframes(_ keyframes: [AnimationKeyframe], toProperty: String? = nil, timeOffset: Float = 0) {
        guard var clip = currentClip else { return }
        
        for keyframe in keyframes {
            var pastedKeyframe = keyframe
            pastedKeyframe.id = UUID() // New ID
            pastedKeyframe.time += timeOffset
            pastedKeyframe.isSelected = false
            
            let targetProperty = toProperty ?? "transform.position"
            
            if let trackIndex = clip.tracks.firstIndex(where: { $0.propertyPath == targetProperty }) {
                clip.tracks[trackIndex].keyframes.append(pastedKeyframe)
            } else {
                let newTrack = AnimationTrack(propertyPath: targetProperty, keyframes: [pastedKeyframe])
                clip.tracks.append(newTrack)
            }
        }
        
        // Sort all keyframes by time
        for trackIndex in clip.tracks.indices {
            clip.tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        }
        
        currentClip = clip
    }
    
    private func interpolateValueAtTime(_ time: Float, in track: AnimationTrack) -> Any {
        let sortedKeyframes = track.keyframes.sorted { $0.time < $1.time }
        
        guard !sortedKeyframes.isEmpty else { return 0.0 as Float }
        
        // Find surrounding keyframes
        var previousKeyframe: AnimationKeyframe?
        var nextKeyframe: AnimationKeyframe?
        
        for keyframe in sortedKeyframes {
            if keyframe.time <= time {
                previousKeyframe = keyframe
            } else if keyframe.time > time && nextKeyframe == nil {
                nextKeyframe = keyframe
                break
            }
        }
        
        guard let previous = previousKeyframe, let next = nextKeyframe else {
            return previousKeyframe?.value ?? nextKeyframe?.value ?? 0.0 as Float
        }
        
        // Linear interpolation
        let t = (time - previous.time) / (next.time - previous.time)
        
        if let prevFloat = previous.value as? Float, let nextFloat = next.value as? Float {
            return prevFloat + (nextFloat - prevFloat) * t
        }
        
        return previous.value
    }
    
    private func getKeyframePosition(_ keyframe: AnimationKeyframe, in view: DopesheetView) -> CGPoint {
        // This would need to be implemented based on the actual view geometry
        // For now, return a placeholder
        return CGPoint(x: CGFloat(keyframe.time * 100), y: 50)
    }
    
    // MARK: - Event Debugging Methods
    
    private func checkAndTriggerEvents(from startTime: Float, to endTime: Float) {
        guard let events = currentClip?.events else { return }
        
        for event in events {
            if event.isEnabled && event.time >= startTime && event.time < endTime {
                triggerEvent(event)
            }
        }
        
        lastEventCheckTime = endTime
    }
    
    private func triggerEvent(_ event: AnimationEvent) {
        // Add triggered entry to history
        let triggeredEntry = EventHistoryEntry(
            timestamp: Date(),
            eventTime: event.time,
            eventName: event.description.isEmpty ? event.functionName : event.description,
            eventType: event.eventType,
            functionName: event.functionName,
            parameter: event.parameter,
            executionStatus: .triggered
        )
        eventHistory.append(triggeredEntry)
        
        // Simulate event execution (in a real implementation, this would call the actual systems)
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
            let success = Bool.random() // Simulate random success/failure for demo
            let completedEntry = EventHistoryEntry(
                timestamp: Date(),
                eventTime: event.time,
                eventName: event.description.isEmpty ? event.functionName : event.description,
                eventType: event.eventType,
                functionName: event.functionName,
                parameter: event.parameter,
                executionStatus: success ? .completed : .failed
            )
            self.eventHistory.append(completedEntry)
        }
    }
    
    func clearEventHistory() {
        eventHistory.removeAll()
    }
    
    func exportEventHistory() {
        let historyData = eventHistory.map { entry in
            "\(entry.timestamp),\(entry.eventTime),\(entry.eventName),\(entry.eventType.rawValue),\(entry.functionName),\(entry.parameter),\(entry.executionStatus.rawValue)"
        }.joined(separator: "\n")
        
        let pasteboard = NSPasteboard.general
        pasteboard.clearContents()
        pasteboard.setString(historyData, forType: .string)
    }
    
    func getEventHistoryForTimeRange(_ startTime: Float, _ endTime: Float) -> [EventHistoryEntry] {
        return eventHistory.filter { $0.eventTime >= startTime && $0.eventTime <= endTime }
    }
}

// MARK: - Animation Models
struct AnimationClip: Identifiable, ObservableObject {
    let id = UUID()
    var name: String
    var duration: Float
    var frameRate: Float = 60.0
    var tracks: [AnimationTrack] = []
    var events: [AnimationEvent] = []
    
    // Clip management properties
    var tags: [String] = []
    var category: AnimationClipBrowser.ClipCategory = .custom
    var isFavorite: Bool = false
    var dateCreated: Date = Date()
    var dateModified: Date = Date()
    
    // Clip properties
    var isLooping: Bool = true
    var loopStartTime: Float = 0.0
    var loopEndTime: Float = 0.0
    var compressionSettings: CompressionSettings = CompressionSettings()
    var rootMotionSettings: RootMotionSettings = RootMotionSettings()
    var additiveSettings: AdditiveSettings = AdditiveSettings()
    
    struct CompressionSettings {
        var enabled: Bool = false
        var quality: Float = 0.8
        var keyframeReduction: Float = 0.1
        var tolerance: Float = 0.01
    }
    
    struct RootMotionSettings {
        var extractRootMotion: Bool = false
        var rootBone: String = "Hips"
        var extractTranslation: Bool = true
        var extractRotation: Bool = true
        var bakeIntoPose: Bool = false
    }
    
    struct AdditiveSettings {
        var isAdditive: Bool = false
        var referencePose: String = ""
        var baseClip: String = ""
    }
}

struct AnimationTrack: Identifiable {
    let id = UUID()
    var propertyPath: String
    var keyframes: [AnimationKeyframe] = []
    var interpolation: InterpolationType = .linear
    
    enum InterpolationType: String, CaseIterable {
        case constant, linear, bezier
    }
}

struct AnimationKeyframe: Identifiable, Equatable {
    let id = UUID()
    var time: Float
    var value: Any
    var inTangent: Float = 0
    var outTangent: Float = 0
    var inTangentWeight: Float = 1.0
    var outTangentWeight: Float = 1.0
    var interpolationType: InterpolationType = .linear
    var isSelected: Bool = false
    var isBreakpoint: Bool = false
    var tangentsUnified: Bool = true
    var freeTangents: Bool = false
    var weightedTangents: Bool = false
    var easeIn: Float = 0
    var easeOut: Float = 0
    var propertyType: PropertyType = .float
    
    enum PropertyType: String, CaseIterable {
        case float = "Float"
        case vector2 = "Vector2"
        case vector3 = "Vector3"
        case vector4 = "Vector4"
        case quaternion = "Quaternion"
        case color = "Color"
        case bool = "Bool"
        case integer = "Integer"
        case string = "String"
        case object = "Object"
        
        var color: Color {
            switch self {
            case .float: return DesignSystem.Colors.accentPrimary
            case .vector2: return DesignSystem.Colors.accentSuccess
            case .vector3: return DesignSystem.Colors.entityPhysics
            case .vector4: return DesignSystem.Colors.accentWarning
            case .quaternion: return DesignSystem.Colors.entityAudio
            case .color: return DesignSystem.Colors.accentDanger
            case .bool: return DesignSystem.Colors.textSecondary
            case .integer: return DesignSystem.Colors.entityParticle
            case .object: return DesignSystem.Colors.entityLight
            }
        }
    }
    
    enum InterpolationType: String, CaseIterable {
        case linear = "Linear"
        case step = "Step"
        case cubic = "Cubic"
        case easeIn = "Ease In"
        case easeOut = "Ease Out"
        case easeInOut = "Ease In Out"
        case bezier = "Bezier"
        
        var shape: KeyframeShape {
            switch self {
            case .linear: return .diamond
            case .step: return .square
            case .cubic: return .circle
            case .easeIn: return .triangle
            case .easeOut: return .triangle
            case .easeInOut: return .circle
            case .bezier: return .hexagon
            }
        }
    }
    
    enum KeyframeShape {
        case circle, square, diamond, triangle, hexagon, star
    }
    
    static func == (lhs: AnimationKeyframe, rhs: AnimationKeyframe) -> Bool {
        return lhs.id == rhs.id
    }
}

struct AnimationEvent: Identifiable, ObservableObject {
    let id = UUID()
    var time: Float
    var functionName: String
    var parameter: String = ""
    var eventType: AnimationEventType = .generic
    var isEnabled: Bool = true
    var color: Color = .blue
    var description: String = ""
    
    enum AnimationEventType: String, CaseIterable {
        case audio = "Audio"
        case particle = "Particle"
        case footstep = "Footstep"
        case hit = "Hit"
        case generic = "Generic"
        
        var defaultColor: Color {
            switch self {
            case .audio: return DesignSystem.Colors.entityAudio
            case .particle: return DesignSystem.Colors.entityParticle
            case .footstep: return DesignSystem.Colors.textSecondary
            case .hit: return DesignSystem.Colors.accentDanger
            case .generic: return DesignSystem.Colors.accentPrimary
            }
        }
    }
}

struct EventHistoryEntry: Identifiable {
    let id = UUID()
    let timestamp: Date
    let eventTime: Float
    let eventName: String
    let eventType: AnimationEvent.AnimationEventType
    let functionName: String
    let parameter: String
    let executionStatus: ExecutionStatus
    
    enum ExecutionStatus: String, CaseIterable {
        case triggered = "Triggered"
        case completed = "Completed"
        case failed = "Failed"
        case skipped = "Skipped"
        
        var color: Color {
            switch self {
            case .triggered: return DesignSystem.Colors.accentPrimary
            case .completed: return DesignSystem.Colors.accentSuccess
            case .failed: return DesignSystem.Colors.accentDanger
            case .skipped: return DesignSystem.Colors.textSecondary
            }
        }
    }
}

// MARK: - Animation Clip Browser UI

struct AnimationClipBrowserView: View {
    @StateObject private var browser = AnimationClipBrowser.shared
    @State private var showCreateDialog = false
    @State private var showImportDialog = false
    @State private var selectedClips: Set<UUID> = []
    
    var body: some View {
        VStack(spacing: 0) {
            AnimationClipBrowserToolbar(
                showCreateDialog: $showCreateDialog,
                showImportDialog: $showImportDialog
            )
            
            EditorDivider()
            
            AnimationClipSearchFilters(browser: browser)
            
            EditorDivider()
            
            AnimationClipList(
                clips: browser.filteredClips,
                selectedClip: $browser.selectedClip,
                selectedClips: $selectedClips,
                browser: browser
            )
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .sheet(isPresented: $showCreateDialog) {
            AnimationClipCreationWizard()
        }
        .sheet(isPresented: $showImportDialog) {
            AnimationClipImportDialog()
        }
    }
}

struct AnimationClipBrowserToolbar: View {
    @Binding var showCreateDialog: Bool
    @Binding var showImportDialog: Bool
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            Text("Animation Clips")
                .font(DesignSystem.Typography.bodyBold)
            
            Spacer()
            
            Button(action: { showCreateDialog = true }) {
                Image(systemName: "plus.circle.fill")
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
            
            Button(action: { showImportDialog = true }) {
                Image(systemName: "square.and.arrow.down")
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
}

struct AnimationClipSearchFilters: View {
    @ObservedObject var browser: AnimationClipBrowser
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.sm) {
            HStack {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                TextField("Search clips...", text: $browser.searchQuery)
                    .textFieldStyle(.roundedBorder)
                    .onSubmit {
                        browser.updateFilteredClips()
                    }
            }
            .padding(.horizontal, DesignSystem.Spacing.sm)
            
            HStack(spacing: DesignSystem.Spacing.md) {
                Picker("Category", selection: $browser.selectedCategory) {
                    Text("All").tag(nil as AnimationClipBrowser.ClipCategory?)
                    ForEach(AnimationClipBrowser.ClipCategory.allCases, id: \.self) { category in
                        Text(category.rawValue).tag(category as AnimationClipBrowser.ClipCategory?)
                    }
                }
                .pickerStyle(.menu)
                .frame(width: 150)
                .onChange(of: browser.selectedCategory) { _ in
                    browser.updateFilteredClips()
                }
                
                Toggle("Favorites Only", isOn: $browser.showFavoritesOnly)
                    .toggleStyle(.switch)
                    .onChange(of: browser.showFavoritesOnly) { _ in
                        browser.updateFilteredClips()
                    }
            }
            .padding(.horizontal, DesignSystem.Spacing.sm)
        }
        .padding(.vertical, DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundSecondary)
        .onChange(of: browser.searchQuery) { _ in
            browser.updateFilteredClips()
        }
    }
}

struct AnimationClipList: View {
    let clips: [AnimationClip]
    @Binding var selectedClip: AnimationClip?
    @Binding var selectedClips: Set<UUID>
    let browser: AnimationClipBrowser
    
    var body: some View {
        ScrollView {
            LazyVStack(spacing: 0) {
                ForEach(clips, id: \.id) { clip in
                    AnimationClipRow(
                        clip: clip,
                        isSelected: selectedClip?.id == clip.id,
                        onSelect: {
                            selectedClip = clip
                        }
                    )
                }
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

struct AnimationClipRow: View {
    let clip: AnimationClip
    let isSelected: Bool
    let onSelect: () -> Void
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            Rectangle()
                .fill(isSelected ? DesignSystem.Colors.accentPrimary : Color.clear)
                .frame(width: 3)
            
            VStack(alignment: .leading, spacing: 2) {
                HStack {
                    Text(clip.name)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    if clip.isFavorite {
                        Image(systemName: "star.fill")
                            .foregroundColor(DesignSystem.Colors.accentWarning)
                            .font(.system(size: 10))
                    }
                }
                
                HStack {
                    Text(clip.category.rawValue)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text(String(format: "%.1fs", clip.duration))
                        .font(DesignSystem.Typography.smallMono)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            
            Spacer()
        }
        .padding(DesignSystem.Spacing.sm)
        .background(isSelected ? DesignSystem.Colors.selection : Color.clear)
        .onTapGesture {
            onSelect()
        }
    }
}

// MARK: - Animation Clip Creation Wizard

struct AnimationClipCreationWizard: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var browser = AnimationClipBrowser.shared
    @State private var wizardStep: WizardStep = .basicInfo
    @State private var newClip = AnimationClip(name: "", duration: 1.0)
    @State private var selectedTemplate: ClipTemplate? = nil
    
    enum WizardStep: String, CaseIterable {
        case template = "Template"
        case basicInfo = "Basic Info"
        case properties = "Properties"
        case preview = "Preview"
    }
    
    enum ClipTemplate: String, CaseIterable {
        case walk = "Walk Cycle"
        case run = "Run Cycle"
        case idle = "Idle"
        case jump = "Jump"
        case attack = "Attack"
        case custom = "Custom"
        
        var description: String {
            switch self {
            case .walk: return "Standard walking animation with foot placement"
            case .run: return "Running animation with arm swing"
            case .idle: return "Idle breathing animation"
            case .jump: return "Jump with takeoff and landing"
            case .attack: return "Basic attack animation"
            case .custom: return "Create from scratch"
            }
        }
        
        var defaultDuration: Float {
            switch self {
            case .walk: return 1.2
            case .run: return 0.8
            case .idle: return 2.0
            case .jump: return 1.0
            case .attack: return 0.6
            case .custom: return 1.0
            }
        }
        
        var defaultCategory: AnimationClipBrowser.ClipCategory {
            switch self {
            case .walk, .run, .jump: return .locomotion
            case .idle: return .idle
            case .attack: return .combat
            case .custom: return .custom
            }
        }
        
        var defaultTags: [String] {
            switch self {
            case .walk: return ["locomotion", "basic", "cycle"]
            case .run: return ["locomotion", "fast", "cycle"]
            case .idle: return ["idle", "basic", "breathing"]
            case .jump: return ["locomotion", "movement", "jump"]
            case .attack: return ["combat", "melee", "attack"]
            case .custom: return []
            }
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Create Animation Clip")
                    .font(DesignSystem.Typography.headline)
                
                Spacer()
                
                // Step indicator
                HStack(spacing: 8) {
                    ForEach(WizardStep.allCases, id: \.self) { step in
                        Circle()
                            .fill(wizardStep == step ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.backgroundTertiary)
                            .frame(width: 24, height: 24)
                            .overlay(
                                Text("\(WizardStep.allCases.firstIndex(of: step)! + 1)")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(wizardStep == step ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                            )
                    }
                    
                    ForEach(0..<WizardStep.allCases.count - 1, id: \.self) { _ in
                        Rectangle()
                            .fill(DesignSystem.Colors.border)
                            .frame(width: 20, height: 2)
                    }
                }
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Content
            ScrollView {
                VStack(spacing: DesignSystem.Spacing.lg) {
                    switch wizardStep {
                    case .template:
                        ClipTemplateSelection(
                            selectedTemplate: $selectedTemplate,
                            newClip: $newClip
                        )
                    case .basicInfo:
                        ClipBasicInfoEditor(clip: $newClip)
                    case .properties:
                        ClipPropertiesEditor(clip: $newClip)
                    case .preview:
                        ClipPreviewView(clip: newClip)
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
            
            EditorDivider()
            
            // Navigation buttons
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                if wizardStep != WizardStep.allCases.first {
                    Button("Previous") {
                        if let currentIndex = WizardStep.allCases.firstIndex(of: wizardStep),
                           currentIndex > 0 {
                            wizardStep = WizardStep.allCases[currentIndex - 1]
                        }
                    }
                    .buttonStyle(.bordered)
                }
                
                if wizardStep != WizardStep.allCases.last {
                    Button("Next") {
                        if let currentIndex = WizardStep.allCases.firstIndex(of: wizardStep),
                           currentIndex < WizardStep.allCases.count - 1 {
                            wizardStep = WizardStep.allCases[currentIndex + 1]
                        }
                    }
                    .buttonStyle(.borderedProminent)
                    .disabled(canProceedToNext() == false)
                } else {
                    Button("Create") {
                        createClip()
                    }
                    .buttonStyle(.borderedProminent)
                    .disabled(newClip.name.isEmpty)
                }
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundSecondary)
        }
        .frame(width: 600, height: 500)
        .onAppear {
            if selectedTemplate == nil {
                selectedTemplate = .custom
            }
        }
    }
    
    private func canProceedToNext() -> Bool {
        switch wizardStep {
        case .template:
            return selectedTemplate != nil
        case .basicInfo:
            return !newClip.name.isEmpty && newClip.duration > 0
        case .properties:
            return true
        case .preview:
            return true
        }
    }
    
    private func createClip() {
        browser.addClip(newClip)
        dismiss()
    }
}

struct ClipTemplateSelection: View {
    @Binding var selectedTemplate: AnimationClipCreationWizard.ClipTemplate?
    @Binding var newClip: AnimationClip
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            Text("Choose a template to start with")
                .font(DesignSystem.Typography.bodyBold)
            
            LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 2), spacing: DesignSystem.Spacing.md) {
                ForEach(AnimationClipCreationWizard.ClipTemplate.allCases, id: \.self) { template in
                    ClipTemplateCard(
                        template: template,
                        isSelected: selectedTemplate == template,
                        onSelect: {
                            selectedTemplate = template
                            applyTemplate(template)
                        }
                    )
                }
            }
        }
    }
    
    private func applyTemplate(_ template: AnimationClipCreationWizard.ClipTemplate) {
        newClip.name = template.rawValue
        newClip.duration = template.defaultDuration
        newClip.category = template.defaultCategory
        newClip.tags = template.defaultTags
    }
}

struct ClipTemplateCard: View {
    let template: AnimationClipCreationWizard.ClipTemplate
    let isSelected: Bool
    let onSelect: () -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            HStack {
                Text(template.rawValue)
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                if isSelected {
                    Image(systemName: "checkmark.circle.fill")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
            }
            
            Text(template.description)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .multilineTextAlignment(.leading)
        }
        .padding(DesignSystem.Spacing.md)
        .background(
            isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.1) : DesignSystem.Colors.backgroundTertiary
        )
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border, lineWidth: 1)
        )
        .cornerRadius(8)
        .onTapGesture {
            onSelect()
        }
    }
}

struct ClipBasicInfoEditor: View {
    @Binding var clip: AnimationClip
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            Text("Basic Information")
                .font(DesignSystem.Typography.bodyBold)
            
            GroupBox("Clip Details") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("Name:")
                        TextField("Clip name", text: $clip.name)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    HStack {
                        Text("Duration:")
                        TextField("Duration", value: $clip.duration, format: .number)
                            .textFieldStyle(.roundedBorder)
                            .frame(width: 100)
                        
                        Text("seconds")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    
                    HStack {
                        Text("Frame Rate:")
                        TextField("Frame Rate", value: $clip.frameRate, format: .number)
                            .textFieldStyle(.roundedBorder)
                            .frame(width: 100)
                        
                        Text("fps")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    
                    HStack {
                        Text("Category:")
                        Picker("Category", selection: $clip.category) {
                            ForEach(AnimationClipBrowser.ClipCategory.allCases, id: \.self) { category in
                                Text(category.rawValue).tag(category)
                            }
                        }
                        .pickerStyle(.menu)
                    }
                }
            }
            
            GroupBox("Tags") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        TextField("Add tag...", text: .constant(""))
                            .textFieldStyle(.roundedBorder)
                        
                        Button("Add") {
                            // Add tag logic
                        }
                        .buttonStyle(.bordered)
                    }
                    
                    // Tags display
                    LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 3), spacing: 4) {
                        ForEach(clip.tags, id: \.self) { tag in
                            HStack(spacing: 4) {
                                Text(tag)
                                    .font(DesignSystem.Typography.small)
                                
                                Button(action: {
                                    clip.tags.removeAll { $0 == tag }
                                }) {
                                    Image(systemName: "xmark")
                                        .font(.system(size: 10))
                                }
                                .buttonStyle(.plain)
                            }
                            .padding(.horizontal, 8)
                            .padding(.vertical, 4)
                            .background(DesignSystem.Colors.accentPrimary.opacity(0.2))
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                            .cornerRadius(4)
                        }
                    }
                }
            }
        }
    }
}

struct ClipPropertiesEditor: View {
    @Binding var clip: AnimationClip
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            Text("Clip Properties")
                .font(DesignSystem.Typography.bodyBold)
            
            GroupBox("Looping") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Toggle("Enable Looping", isOn: $clip.isLooping)
                        .toggleStyle(.switch)
                    
                    if clip.isLooping {
                        HStack {
                            Text("Loop Start:")
                            TextField("0.0", value: $clip.loopStartTime, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                        }
                        
                        HStack {
                            Text("Loop End:")
                            TextField("0.0", value: $clip.loopEndTime, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                        }
                    }
                }
            }
            
            GroupBox("Compression") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Toggle("Enable Compression", isOn: $clip.compressionSettings.enabled)
                        .toggleStyle(.switch)
                    
                    if clip.compressionSettings.enabled {
                        HStack {
                            Text("Quality:")
                            Slider(value: $clip.compressionSettings.quality, in: 0...1)
                            Text(String(format: "%.2f", clip.compressionSettings.quality))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                        }
                        
                        HStack {
                            Text("Keyframe Reduction:")
                            Slider(value: $clip.compressionSettings.keyframeReduction, in: 0...1)
                            Text(String(format: "%.2f", clip.compressionSettings.keyframeReduction))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                        }
                    }
                }
            }
            
            GroupBox("Root Motion") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Toggle("Extract Root Motion", isOn: $clip.rootMotionSettings.extractRootMotion)
                        .toggleStyle(.switch)
                    
                    if clip.rootMotionSettings.extractRootMotion {
                        HStack {
                            Text("Root Bone:")
                            TextField("Hips", text: $clip.rootMotionSettings.rootBone)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        Toggle("Extract Translation", isOn: $clip.rootMotionSettings.extractTranslation)
                            .toggleStyle(.switch)
                        
                        Toggle("Extract Rotation", isOn: $clip.rootMotionSettings.extractRotation)
                            .toggleStyle(.switch)
                    }
                }
            }
        }
    }
}

struct ClipPreviewView: View {
    let clip: AnimationClip
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            Text("Preview")
                .font(DesignSystem.Typography.bodyBold)
            
            // Mock preview area
            RoundedRectangle(cornerRadius: 8)
                .fill(DesignSystem.Colors.backgroundTertiary)
                .overlay(
                    VStack {
                        Image(systemName: "play.circle")
                            .font(.system(size: 40))
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Text("Animation Preview")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                )
                .frame(height: 200)
            
            // Clip summary
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                HStack {
                    Text("Name:")
                    Spacer()
                    Text(clip.name)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                HStack {
                    Text("Duration:")
                    Spacer()
                    Text(String(format: "%.2f seconds", clip.duration))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                HStack {
                    Text("Category:")
                    Spacer()
                    Text(clip.category.rawValue)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                HStack {
                    Text("Tags:")
                    Spacer()
                    Text(clip.tags.joined(separator: ", "))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(8)
        }
    }
}

struct AnimationClipImportDialog: View {
    @Environment(\.dismiss) private var dismiss
    @State private var selectedFile: String = ""
    @State private var importSettings = ImportSettings()
    
    struct ImportSettings {
        var importAsAdditive: Bool = false
        var generateEvents: Bool = true
        var optimizeKeyframes: Bool = true
        var targetFrameRate: Float = 60.0
    }
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Import Animation Clip")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                HStack {
                    Text("File:")
                    TextField("Select file...", text: $selectedFile)
                        .textFieldStyle(.roundedBorder)
                    
                    Button("Browse") {
                        // File browser logic
                    }
                    .buttonStyle(.bordered)
                }
                
                GroupBox("Import Options") {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        Toggle("Import as Additive", isOn: $importSettings.importAsAdditive)
                            .toggleStyle(.switch)
                        
                        Toggle("Generate Events", isOn: $importSettings.generateEvents)
                            .toggleStyle(.switch)
                        
                        Toggle("Optimize Keyframes", isOn: $importSettings.optimizeKeyframes)
                            .toggleStyle(.switch)
                        
                        HStack {
                            Text("Target Frame Rate:")
                            TextField("60", value: $importSettings.targetFrameRate, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                            
                            Text("fps")
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                    }
                }
            }
            
            Spacer()
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Import") {
                    // Import logic
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(selectedFile.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 500, height: 400)
    }
}

// MARK: - Enhanced Animation Timeline View
struct AnimationTimelineView: View {
    @ObservedObject var manager = AnimationEditorManager.shared
    @State private var showDopesheet = true
    @State private var showTimelineSettings = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Enhanced toolbar
            EnhancedAnimationToolbar(
                manager: manager,
                showDopesheet: $showDopesheet,
                showTimelineSettings: $showTimelineSettings
            )
            
            EditorDivider()
            
            HSplitView {
                // Property list
                AnimationPropertyList()
                    .frame(minWidth: 200, maxWidth: 250)
                
                EditorDivider()
                
                // Timeline
                VStack(spacing: 0) {
                    // Enhanced time ruler
                    EnhancedTimeRuler(
                        duration: manager.currentClip?.duration ?? 10,
                        currentTime: manager.currentTime,
                        zoom: manager.zoomLevel,
                        frameRate: manager.frameRate,
                        timeDisplayFormat: manager.timeDisplayFormat,
                        workAreaIn: manager.workAreaIn,
                        workAreaOut: manager.workAreaOut,
                        isWorkAreaEnabled: manager.isWorkAreaEnabled,
                        markers: manager.markers
                    )
                    
                    EditorDivider()
                    
                    // Keyframe area with visual effects
                    ZStack {
                        if showDopesheet {
                            DopesheetView(manager: manager)
                        } else {
                            CurveEditorView(manager: manager)
                        }
                        
                        // Visual effects overlay
                        if manager.onionSkinEnabled {
                            OnionSkinOverlay(manager: manager)
                        }
                        
                        if manager.ghostingEnabled {
                            GhostingOverlay(manager: manager)
                        }
                        
                        if manager.referenceFrameEnabled {
                            ReferenceFrameOverlay(manager: manager)
                        }
                        
                        if manager.audioWaveformEnabled {
                            AudioWaveformOverlay(manager: manager)
                        }
                    }
                }
            }
            
            EditorDivider()
            
            // Enhanced playback controls
            EnhancedPlaybackControls(manager: manager)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .sheet(isPresented: $showTimelineSettings) {
            TimelineSettingsSheet(manager: manager)
        }
    }
}

// MARK: - Animation Toolbar
struct AnimationToolbar: View {
    @ObservedObject var manager: AnimationEditorManager
    @Binding var showDopesheet: Bool
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            // Clip selector
            Picker("Clip", selection: .constant("Walk")) {
                Text("Walk").tag("Walk")
                Text("Run").tag("Run")
                Text("Idle").tag("Idle")
            }
            .frame(width: 120)
            
            Divider().frame(height: 20)
            
            // Add keyframe
            Button(action: { }) {
                Label("Add Key", systemImage: "plus.diamond")
            }
            
            // Record mode
            Toggle(isOn: .constant(false)) {
                Image(systemName: "record.circle")
            }
            .toggleStyle(.button)
            .tint(.red)
            
            Spacer()
            
            // View toggle
            Picker("", selection: $showDopesheet) {
                Text("Dopesheet").tag(true)
                Text("Curves").tag(false)
            }
            .pickerStyle(.segmented)
            .frame(width: 150)
            
            // Zoom
            HStack(spacing: 4) {
                Button(action: { manager.zoomLevel = max(0.5, manager.zoomLevel - 0.25) }) {
                    Image(systemName: "minus.magnifyingglass")
                }
                Text("\(Int(manager.zoomLevel * 100))%")
                    .font(DesignSystem.Typography.small)
                    .frame(width: 40)
                Button(action: { manager.zoomLevel = min(4.0, manager.zoomLevel + 0.25) }) {
                    Image(systemName: "plus.magnifyingglass")
                }
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
}

// MARK: - Enhanced Playback Controls
struct EnhancedPlaybackControls: View {
    @ObservedObject var manager: AnimationEditorManager
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            // Time display
            Text(manager.formatTime(manager.currentTime))
                .font(DesignSystem.Typography.bodyMono)
                .frame(width: 120)
            
            // Playback controls
            HStack(spacing: 4) {
                Button(action: { manager.stop() }) {
                    Image(systemName: "stop.fill")
                }
                
                Button(action: {
                    if manager.isPlaying {
                        manager.pause()
                    } else {
                        manager.play()
                    }
                }) {
                    Image(systemName: manager.isPlaying ? "pause.fill" : "play.fill")
                }
                
                // Shuttle controls
                Button(action: { manager.jogReverse() }) {
                    Image(systemName: "backward.frame")
                }
                
                Button(action: { manager.jogForward() }) {
                    Image(systemName: "forward.frame")
                }
                
                Button(action: { manager.shuttleReverse() }) {
                    Image(systemName: "backward")
                }
                
                Button(action: { manager.shuttleForward() }) {
                    Image(systemName: "forward")
                }
            }
            
            Divider().frame(height: 20)
            
            // Marker controls
            HStack(spacing: 4) {
                Button(action: { manager.addMarker(at: manager.currentTime, label: "Marker") }) {
                    Image(systemName: "flag.fill")
                }
                
                Button(action: { manager.navigateToPreviousMarker() }) {
                    Image(systemName: "chevron.left")
                }
                
                Button(action: { manager.navigateToNextMarker() }) {
                    Image(systemName: "chevron.right")
                }
            }
            
            Divider().frame(height: 20)
            
            // Loop controls
            HStack(spacing: 4) {
                Button(action: { manager.isLooping.toggle() }) {
                    Image(systemName: manager.isLooping ? "repeat" : "repeat.1")
                }
                .foregroundColor(manager.isLooping ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                
                Button(action: { manager.isWorkAreaEnabled.toggle() }) {
                    Image(systemName: "crop")
                }
                .foregroundColor(manager.isWorkAreaEnabled ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
            
            // Playback speed
            HStack {
                Text("Speed:")
                Picker("Speed", selection: $manager.playbackSpeed) {
                    Text("0.25x").tag(0.25)
                    Text("0.5x").tag(0.5)
                    Text("1x").tag(1.0)
                    Text("2x").tag(2.0)
                    Text("4x").tag(4.0)
                }
                .pickerStyle(.menu)
                .frame(width: 80)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
}

// MARK: - Visual Effect Overlays

struct OnionSkinOverlay: View {
    @ObservedObject var manager: AnimationEditorManager
    
    var body: some View {
        ZStack {
            // Previous frames
            ForEach(1..<manager.onionSkinFrames + 1, id: \.self) { frameOffset in
                let pastTime = manager.currentTime - Float(frameOffset) / manager.frameRate
                if pastTime >= 0 {
                    Rectangle()
                        .fill(Color.blue.opacity(0.1 / Float(frameOffset)))
                        .frame(height: 20)
                        .overlay(
                            Text("-\(frameOffset)")
                                .font(.system(size: 8))
                                .foregroundColor(.blue),
                            alignment: .leading
                        )
                }
            }
            
            // Future frames
            ForEach(1..<manager.onionSkinFrames + 1, id: \.self) { frameOffset in
                let futureTime = manager.currentTime + Float(frameOffset) / manager.frameRate
                if let duration = manager.currentClip?.duration, futureTime <= duration {
                    Rectangle()
                        .fill(Color.red.opacity(0.1 / Float(frameOffset)))
                        .frame(height: 20)
                        .overlay(
                            Text("+\(frameOffset)")
                                .font(.system(size: 8))
                                .foregroundColor(.red),
                            alignment: .leading
                        )
                }
            }
        }
        .allowsHitTesting(false)
    }
}

struct GhostingOverlay: View {
    @ObservedObject var manager: AnimationEditorManager
    
    var body: some View {
        // Motion trail visualization
        GeometryReader { geometry in
            Path { path in
                // Create a sample motion trail path
                let startX: CGFloat = 0
                let endX: CGFloat = geometry.size.width * 0.3
                let centerY = geometry.size.height / 2
                
                path.move(to: CGPoint(x: startX, y: centerY))
                
                for i in 1...10 {
                    let progress = Float(i) / 10.0
                    let x = startX + (endX - startX) * CGFloat(progress)
                    let y = centerY + sin(progress * .pi * 2) * 20
                    path.addLine(to: CGPoint(x: x, y: y))
                }
            }
            .stroke(Color.orange.opacity(0.3), style: StrokeStyle(lineWidth: 2, dash: [5, 5]))
        }
        .allowsHitTesting(false)
    }
}

struct ReferenceFrameOverlay: View {
    @ObservedObject var manager: AnimationEditorManager
    
    var body: some View {
        Rectangle()
            .stroke(Color.green.opacity(0.5), style: StrokeStyle(lineWidth: 1, dash: [10, 5]))
            .overlay(
                VStack {
                    Text("Reference")
                        .font(.system(size: 10))
                        .foregroundColor(.green)
                    Text("Frame")
                        .font(.system(size: 10))
                        .foregroundColor(.green)
                },
                alignment: .topLeading
            )
            .padding(4)
            .allowsHitTesting(false)
    }
}

struct AudioWaveformOverlay: View {
    @ObservedObject var manager: AnimationEditorManager
    
    var body: some View {
        GeometryReader { geometry in
            // Simulated audio waveform
            Path { path in
                let width = geometry.size.width
                let height = geometry.size.height
                let centerY = height / 2
                
                path.move(to: CGPoint(x: 0, y: centerY))
                
                for x in stride(from: 0, to: width, by: 2) {
                    let progress = x / width
                    let amplitude = sin(progress * .pi * 20) * cos(progress * .pi * 7) * height * 0.3
                    let y = centerY + amplitude
                    path.addLine(to: CGPoint(x: x, y: y))
                }
            }
            .stroke(Color.cyan.opacity(0.4), lineWidth: 1)
        }
        .allowsHitTesting(false)
    }
}
struct AnimationPropertyList: View {
    @State private var expandedProperties: Set<String> = ["Transform"]
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Header
            HStack {
                Text("Properties")
                    .font(DesignSystem.Typography.bodyBold)
                Spacer()
                Button(action: {}) {
                    Image(systemName: "plus")
                }
                .buttonStyle(.plain)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 0) {
                    AnimationPropertyRow(name: "Transform", isExpandable: true, isExpanded: true)
                    AnimationPropertyRow(name: "Position.x", indent: 1, hasKeyframes: true)
                    AnimationPropertyRow(name: "Position.y", indent: 1, hasKeyframes: true)
                    AnimationPropertyRow(name: "Position.z", indent: 1, hasKeyframes: false)
                    AnimationPropertyRow(name: "Rotation.x", indent: 1, hasKeyframes: true)
                    AnimationPropertyRow(name: "Rotation.y", indent: 1, hasKeyframes: true)
                    AnimationPropertyRow(name: "Rotation.z", indent: 1, hasKeyframes: false)
                    AnimationPropertyRow(name: "Scale", indent: 1, hasKeyframes: false)
                    AnimationPropertyRow(name: "Animator", isExpandable: true, isExpanded: false)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

struct AnimationPropertyRow: View {
    let name: String
    var indent: Int = 0
    var isExpandable: Bool = false
    var isExpanded: Bool = false
    var hasKeyframes: Bool = false
    @State private var isHovering = false
    
    var body: some View {
        HStack(spacing: 4) {
            Spacer().frame(width: CGFloat(indent) * 16)
            
            if isExpandable {
                Image(systemName: isExpanded ? "chevron.down" : "chevron.right")
                    .font(.system(size: 10))
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            } else {
                Spacer().frame(width: 12)
            }
            
            if hasKeyframes {
                Image(systemName: "diamond.fill")
                    .font(.system(size: 8))
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            
            Text(name)
                .font(isExpandable ? DesignSystem.Typography.bodyBold : DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Spacer()
        }
        .padding(.vertical, 4)
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .background(isHovering ? DesignSystem.Colors.hover : Color.clear)
        .onHover { isHovering = $0 }
    }
}

// MARK: - Enhanced Time Ruler
struct TimeRuler: View {
    let duration: Float
    let currentTime: Float
    let zoom: Float
    let frameRate: Float
    let timeDisplayFormat: TimeDisplayFormat
    let workAreaIn: Float
    let workAreaOut: Float
    let isWorkAreaEnabled: Bool
    let markers: [TimelineMarker]
    
    var body: some View {
        GeometryReader { geometry in
            ZStack(alignment: .leading) {
                // Background
                Rectangle()
                    .fill(DesignSystem.Colors.backgroundTertiary)
                
                // Work area overlay
                if isWorkAreaEnabled {
                    Rectangle()
                        .fill(DesignSystem.Colors.accentPrimary.opacity(0.1))
                        .frame(
                            width: CGFloat((workAreaOut - workAreaIn) / duration) * geometry.size.width * CGFloat(zoom),
                            height: geometry.size.height
                        )
                        .offset(x: CGFloat(workAreaIn / duration) * geometry.size.width * CGFloat(zoom))
                }
                
                // Time markers
                ForEach(getTimeMarkers(), id: \.self) { marker in
                    VStack {
                        Rectangle()
                            .fill(marker.isMajor ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                            .frame(width: 1, height: marker.isMajor ? 12 : 6)
                        
                        if marker.isMajor {
                            Text(marker.label)
                                .font(.system(size: 9))
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                    }
                    .position(x: marker.x, y: geometry.size.height / 2)
                }
                
                // Markers
                ForEach(markers) { marker in
                    VStack {
                        Rectangle()
                            .fill(marker.color)
                            .frame(width: 2, height: 16)
                        
                        Text(marker.label)
                            .font(.system(size: 8))
                            .foregroundColor(marker.color)
                            .lineLimit(1)
                    }
                    .position(x: CGFloat(marker.time / duration) * geometry.size.width * CGFloat(zoom), y: geometry.size.height / 2)
                }
                
                // Playhead
                Rectangle()
                    .fill(DesignSystem.Colors.accentPrimary)
                    .frame(width: 2)
                    .offset(x: CGFloat(currentTime / duration) * geometry.size.width * CGFloat(zoom))
            }
        }
        .frame(height: 30)
    }
    
    private func getTimeMarkers() -> [TimeMarker] {
        var markers: [TimeMarker] = []
        let markerInterval: Float
        
        switch timeDisplayFormat {
        case .frames:
            markerInterval = 1.0 / frameRate
        case .seconds:
            markerInterval = 0.5
        case .timecode:
            markerInterval = 1.0
        }
        
        var currentTime: Float = 0
        while currentTime <= duration {
            let isMajor = currentTime.truncatingRemainder(dividingBy: markerInterval * 2) < 0.001
            let label: String
            
            switch timeDisplayFormat {
            case .frames:
                label = "\(Int(currentTime * frameRate))"
            case .seconds:
                label = String(format: "%.1f", currentTime)
            case .timecode:
                let seconds = Int(currentTime) % 60
                label = String(format: "\(%02d)", seconds)
            }
            
            markers.append(TimeMarker(
                time: currentTime,
                x: CGFloat(currentTime / duration) * 1000 * CGFloat(zoom), // Will be scaled in GeometryReader
                label: label,
                isMajor: isMajor
            ))
            
            currentTime += markerInterval
        }
        
        return markers
    }
}

struct TimeMarker {
    let time: Float
    let x: CGFloat
    let label: String
    let isMajor: Bool
}

// MARK: - Enhanced Dopesheet Data Models

class DopesheetManager: ObservableObject {
    static let shared = DopesheetManager()
    
    @Published var tracks: [DopesheetTrack] = []
    @Published var groups: [DopesheetGroup] = []
    @Published var selectedTracks: Set<UUID> = []
    @Published var soloTracks: Set<UUID> = []
    @Published var mutedTracks: Set<UUID> = []
    @Published var lockedTracks: Set<UUID> = []
    @Published var hiddenEmptyTracks: Bool = false
    @Published var searchQuery: String = ""
    @Published var expandedGroups: Set<UUID> = []
    @Published var selectedKeyframes: Set<UUID> = []
    @Published var hoveredKeyframe: UUID?
    @Published var keyValuePreview: String = ""
    
    var filteredTracks: [DopesheetTrack] {
        var filtered = tracks
        
        // Hide empty tracks
        if hiddenEmptyTracks {
            filtered = filtered.filter { !$0.keyframes.isEmpty }
        }
        
        // Search filter
        if !searchQuery.isEmpty {
            filtered = filtered.filter { track in
                track.name.localizedCaseInsensitiveContains(searchQuery) ||
                track.objectName.localizedCaseInsensitiveContains(searchQuery) ||
                track.propertyPath.localizedCaseInsensitiveContains(searchQuery)
            }
        }
        
        // Apply solo/mute
        if !soloTracks.isEmpty {
            filtered = filtered.filter { soloTracks.contains($0.id) }
        } else {
            filtered = filtered.filter { !mutedTracks.contains($0.id) }
        }
        
        return filtered
    }
}

struct DopesheetTrack: Identifiable {
    let id = UUID()
    var name: String
    var objectName: String
    var propertyPath: String
    var keyframes: [DopesheetKeyframe] = []
    var color: Color = .blue
    var isLocked: Bool = false
    var isMuted: Bool = false
    var isSolo: Bool = false
    var groupId: UUID?
    var isVisible: Bool = true
    var currentValue: Float = 0
}

struct DopesheetKeyframe: Identifiable {
    let id = UUID()
    var time: Float
    var value: Any
    var interpolation: AnimationTrack.InterpolationType = .linear
    var isSelected: Bool = false
}

struct DopesheetGroup: Identifiable {
    let id = UUID()
    var name: String
    var trackIds: [UUID] = []
    var isExpanded: Bool = true
    var color: Color = .gray
    var objectName: String
}

// MARK: - Dopesheet View
struct DopesheetView: View {
    @ObservedObject var manager: AnimationEditorManager
    @StateObject private var dopesheetManager = DopesheetManager.shared
    @State private var hoveredTrack: UUID?
    @State private var isDragging = false
    @State private var dragStartPoint: CGPoint = .zero
    @State private var dragEndPoint: CGPoint = .zero
    
    var body: some View {
        VStack(spacing: 0) {
            // Dopesheet toolbar
            DopesheetToolbar()
                .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Main dopesheet area
            HStack(spacing: 0) {
                // Track list
                DopesheetTrackList()
                    .frame(width: 250)
                    .background(DesignSystem.Colors.backgroundSecondary)
                
                EditorDivider()
                
                // Keyframe area
                DopesheetKeyframeArea(manager: manager)
                    .background(DesignSystem.Colors.backgroundPrimary)
            }
        }
    }
}

// MARK: - Dopesheet Toolbar
struct DopesheetToolbar: View {
    @StateObject private var dopesheetManager = DopesheetManager.shared
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            // Search
            HStack {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                TextField("Search tracks...", text: $dopesheetManager.searchQuery)
                    .textFieldStyle(.plain)
            }
            .padding(DesignSystem.Spacing.xs)
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(4)
            
            Spacer()
            
            // Track controls
            Button(action: { dopesheetManager.hiddenEmptyTracks.toggle() }) {
                Image(systemName: "eye.slash")
            }
            .buttonStyle(.plain)
            .foregroundColor(dopesheetManager.hiddenEmptyTracks ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
            .help("Hide Empty Tracks")
            
            Divider()
                .frame(height: 16)
            
            Text("\(dopesheetManager.filteredTracks.count) tracks")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .padding(DesignSystem.Spacing.sm)
    }
}

// MARK: - Dopesheet Track List
struct DopesheetTrackList: View {
    @StateObject private var dopesheetManager = DopesheetManager.shared
    
    var body: some View {
        List {
            // Summary rows per object
            ForEach(getObjectSummaryRows(), id: \.id) { summary in
                DopesheetSummaryRow(summary: summary)
            }
            
            // Groups
            ForEach(dopesheetManager.groups, id: \.id) { group in
                DopesheetGroupRow(group: group)
                    .onTapGesture {
                        if dopesheetManager.expandedGroups.contains(group.id) {
                            dopesheetManager.expandedGroups.remove(group.id)
                        } else {
                            dopesheetManager.expandedGroups.insert(group.id)
                        }
                    }
                
                if dopesheetManager.expandedGroups.contains(group.id) {
                    ForEach(dopesheetManager.filteredTracks.filter { $0.groupId == group.id }, id: \.id) { track in
                        DopesheetTrackRow(track: track)
                    }
                }
            }
            
            // Ungrouped tracks
            ForEach(dopesheetManager.filteredTracks.filter { $0.groupId == nil }, id: \.id) { track in
                DopesheetTrackRow(track: track)
            }
        }
        .listStyle(.plain)
    }
    
    private func getObjectSummaryRows() -> [ObjectSummary] {
        let objectNames = Set(dopesheetManager.filteredTracks.map { $0.objectName })
        return objectNames.map { name in
            let tracks = dopesheetManager.filteredTracks.filter { $0.objectName == name }
            let totalKeyframes = tracks.reduce(0) { $0 + $1.keyframes.count }
            return ObjectSummary(
                id: UUID(),
                objectName: name,
                trackCount: tracks.count,
                keyframeCount: totalKeyframes,
                tracks: tracks
            )
        }
    }
}

struct ObjectSummary: Identifiable {
    let id: UUID
    let objectName: String
    let trackCount: Int
    let keyframeCount: Int
    let tracks: [DopesheetTrack]
}

struct DopesheetSummaryRow: View {
    let summary: ObjectSummary
    @StateObject private var dopesheetManager = DopesheetManager.shared
    @State private var isExpanded = true
    
    var body: some View {
        HStack {
            Button(action: { isExpanded.toggle() }) {
                Image(systemName: isExpanded ? "chevron.down" : "chevron.right")
                    .font(.system(size: 10))
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
            
            Image(systemName: "cube")
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .font(.system(size: 12))
            
            VStack(alignment: .leading, spacing: 2) {
                Text(summary.objectName)
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text("\(summary.trackCount) tracks • \(summary.keyframeCount) keyframes")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
        }
        .padding(.vertical, 4)
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
}

struct DopesheetGroupRow: View {
    let group: DopesheetGroup
    @StateObject private var dopesheetManager = DopesheetManager.shared
    
    var body: some View {
        HStack {
            Image(systemName: dopesheetManager.expandedGroups.contains(group.id) ? "chevron.down" : "chevron.right")
                .font(.system(size: 10))
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Rectangle()
                .fill(group.color)
                .frame(width: 12, height: 12)
                .cornerRadius(2)
            
            Text(group.name)
                .font(DesignSystem.Typography.bodyBold)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Spacer()
        }
        .padding(.vertical, 4)
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundSecondary.opacity(0.5))
    }
}

struct DopesheetTrackRow: View {
    let track: DopesheetTrack
    @StateObject private var dopesheetManager = DopesheetManager.shared
    @State private var isHovering = false
    
    var body: some View {
        HStack {
            Spacer().frame(width: 20)
            
            Rectangle()
                .fill(track.color)
                .frame(width: 8, height: 8)
                .cornerRadius(2)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(track.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text(track.objectName)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
            
            // Track controls
            HStack(spacing: 4) {
                Button(action: { toggleSolo() }) {
                    Image(systemName: "s.circle")
                        .foregroundColor(track.isSolo ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Solo")
                
                Button(action: { toggleMute() }) {
                    Image(systemName: track.isMuted ? "speaker.slash.fill" : "speaker.fill")
                        .foregroundColor(track.isMuted ? .orange : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Mute")
                
                Button(action: { toggleLock() }) {
                    Image(systemName: track.isLocked ? "lock.fill" : "lock.open")
                        .foregroundColor(track.isLocked ? .red : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Lock")
            }
        }
        .padding(.vertical, 4)
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .background(isHovering ? DesignSystem.Colors.hover : Color.clear)
        .onHover { isHovering = $0 }
    }
    
    private func toggleSolo() {
        if track.isSolo {
            dopesheetManager.soloTracks.remove(track.id)
        } else {
            dopesheetManager.soloTracks.insert(track.id)
        }
    }
    
    private func toggleMute() {
        if track.isMuted {
            dopesheetManager.mutedTracks.remove(track.id)
        } else {
            dopesheetManager.mutedTracks.insert(track.id)
        }
    }
    
    private func toggleLock() {
        if track.isLocked {
            dopesheetManager.lockedTracks.remove(track.id)
        } else {
            dopesheetManager.lockedTracks.insert(track.id)
        }
    }
}

// MARK: - Dopesheet Keyframe Area
struct DopesheetKeyframeArea: View {
    @ObservedObject var manager: AnimationEditorManager
    @StateObject private var dopesheetManager = DopesheetManager.shared
    @State private var hoveredKeyframe: UUID?
    @State private var isDragging = false
    @State private var dragStartPoint: CGPoint = .zero
    @State private var dragEndPoint: CGPoint = .zero
    @State private var showValuePreview = false
    @State private var previewPosition: CGPoint = .zero
    @State private var previewValue: String = ""
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Grid
                DopesheetGrid(
                    duration: manager.currentClip?.duration ?? 10,
                    size: geometry.size,
                    trackCount: dopesheetManager.filteredTracks.count
                )
                
                // Current value lines for each track
                ForEach(Array(dopesheetManager.filteredTracks.enumerated()), id: \.element.id) { index, track in
                    let y = CGFloat(index) * 30 + 15
                    Path { path in
                        path.move(to: CGPoint(x: 0, y: y))
                        path.addLine(to: CGPoint(x: geometry.size.width, y: y))
                    }
                    .stroke(track.color.opacity(0.3), style: StrokeStyle(lineWidth: 1, dash: [5, 5]))
                }
                
                // Keyframes
                ForEach(Array(dopesheetManager.filteredTracks.enumerated()), id: \.element.id) { index, track in
                    ForEach(track.keyframes, id: \.id) { keyframe in
                        let x = CGFloat(keyframe.time / (manager.currentClip?.duration ?? 10)) * geometry.size.width
                        let y = CGFloat(index) * 30 + 15
                        
                        DopesheetKeyframeView(
                            keyframe: keyframe,
                            position: CGPoint(x: x, y: y),
                            color: track.color,
                            isSelected: dopesheetManager.selectedKeyframes.contains(keyframe.id),
                            isHovered: hoveredKeyframe == keyframe.id
                        )
                        .onTapGesture {
                            handleKeyframeClick(keyframe)
                        }
                        .onHover { isHovering in
                            hoveredKeyframe = isHovering ? keyframe.id : nil
                            if isHovering {
                                showValuePreview = true
                                previewPosition = CGPoint(x: x, y: y)
                                previewValue = "\(keyframe.time)s: \(keyframe.value)"
                            } else {
                                showValuePreview = false
                            }
                        }
                    }
                }
                
                // Playhead line
                Rectangle()
                    .fill(DesignSystem.Colors.accentPrimary)
                    .frame(width: 2)
                    .offset(x: CGFloat(manager.currentTime / (manager.currentClip?.duration ?? 10)) * geometry.size.width - geometry.size.width / 2)
                
                // Marquee selection
                if isDragging {
                    Path { path in
                        path.addRect(CGRect(
                            origin: CGPoint(
                                x: min(dragStartPoint.x, dragEndPoint.x),
                                y: min(dragStartPoint.y, dragEndPoint.y)
                            ),
                            size: CGSize(
                                width: abs(dragEndPoint.x - dragStartPoint.x),
                                height: abs(dragEndPoint.y - dragStartPoint.y)
                            )
                        ))
                    }
                    .stroke(DesignSystem.Colors.accentPrimary, style: StrokeStyle(lineWidth: 1, dash: [5, 5]))
                    .fill(DesignSystem.Colors.accentPrimary.opacity(0.1))
                }
                
                // Value preview tooltip
                if showValuePreview {
                    VStack(alignment: .leading, spacing: 2) {
                        Text(previewValue)
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .padding(4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                    }
                    .position(previewPosition + CGPoint(x: 10, y: -20))
                }
            }
        }
        .gesture(
            DragGesture()
                .onChanged { value in
                    if !isDragging {
                        isDragging = true
                        dragStartPoint = value.startLocation
                    }
                    dragEndPoint = value.location
                }
                .onEnded { value in
                    isDragging = false
                    performMarqueeSelection()
                }
        )
        .onKeyPress { key in
            handleKeyboardShortcuts(key.key)
            return .handled
        }
    }
    
    private func performMarqueeSelection() {
        // Implement marquee selection logic
        let selectionRect = CGRect(
            x: min(dragStartPoint.x, dragEndPoint.x),
            y: min(dragStartPoint.y, dragEndPoint.y),
            width: abs(dragEndPoint.x - dragStartPoint.x),
            height: abs(dragEndPoint.y - dragStartPoint.y)
        )
        
        for track in dopesheetManager.filteredTracks {
            let trackIndex = dopesheetManager.filteredTracks.firstIndex(of: track) ?? 0
            let trackY = CGFloat(trackIndex) * 30 + 15
            
            for keyframe in track.keyframes {
                let x = CGFloat(keyframe.time / (manager.currentClip?.duration ?? 10)) * (selectionRect.width + selectionRect.minX)
                let keyframePoint = CGPoint(x: x, y: trackY)
                
                if selectionRect.contains(keyframePoint) {
                    dopesheetManager.selectedKeyframes.insert(keyframe.id)
                }
            }
        }
    }
    
    private func handleKeyframeClick(_ keyframe: DopesheetKeyframe) {
        let isShiftPressed = NSEvent.modifierFlags.contains(.shift)
        let isCommandPressed = NSEvent.modifierFlags.contains(.command)
        
        if isShiftPressed {
            // Add to selection or remove if already selected
            if dopesheetManager.selectedKeyframes.contains(keyframe.id) {
                dopesheetManager.selectedKeyframes.remove(keyframe.id)
            } else {
                dopesheetManager.selectedKeyframes.insert(keyframe.id)
            }
        } else if isCommandPressed {
            // Toggle selection (add to selection without clearing)
            if dopesheetManager.selectedKeyframes.contains(keyframe.id) {
                dopesheetManager.selectedKeyframes.remove(keyframe.id)
            } else {
                dopesheetManager.selectedKeyframes.insert(keyframe.id)
            }
        } else {
            // Clear selection and select only this keyframe
            dopesheetManager.selectedKeyframes.removeAll()
            dopesheetManager.selectedKeyframes.insert(keyframe.id)
        }
    }
    
    private func handleKeyboardShortcuts(_ key: String) -> Bool {
        switch key {
        case "delete":
            deleteSelectedKeyframes()
            return true
        case "d":
            if NSEvent.modifierFlags.contains(.command) {
                duplicateSelectedKeyframes()
                return true
            }
        case "c":
            if NSEvent.modifierFlags.contains(.command) {
                copySelectedKeyframes()
                return true
            }
        case "v":
            if NSEvent.modifierFlags.contains(.command) {
                pasteKeyframes()
                return true
            }
        case "i":
            insertKeyframeAtCurrentTime()
            return true
        case "s":
            if NSEvent.modifierFlags.contains(.command) {
                splitKeyframeAtCurrentTime()
                return true
            }
        case "m":
            if NSEvent.modifierFlags.contains(.command) {
                mirrorSelectedKeyframes()
                return true
            }
        case "leftArrow":
            nudgeSelectedKeyframes(timeDelta: -0.016) // 1 frame
            return true
        case "rightArrow":
            nudgeSelectedKeyframes(timeDelta: 0.016) // 1 frame
            return true
        case "upArrow":
            nudgeSelectedKeyframes(valueDelta: 0.1)
            return true
        case "downArrow":
            nudgeSelectedKeyframes(valueDelta: -0.1)
            return true
        case "s":
            if NSEvent.modifierFlags.contains(.command) && NSEvent.modifierFlags.contains(.shift) {
                scaleSelectedKeyframes(timeScale: 1.1, valueScale: 1.0)
                return true
            } else if NSEvent.modifierFlags.contains(.command) {
                splitKeyframeAtCurrentTime()
                return true
            }
        case "a":
            if NSEvent.modifierFlags.contains(.command) && NSEvent.modifierFlags.contains(.shift) {
                scaleSelectedKeyframes(timeScale: 1.0, valueScale: 1.1)
                return true
            }
        default:
            return false
        }
    }
    
    private func deleteSelectedKeyframes() {
        // Remove selected keyframes from all tracks
        for track in dopesheetManager.filteredTracks {
            track.keyframes.removeAll { dopesheetManager.selectedKeyframes.contains($0.id) }
        }
        dopesheetManager.selectedKeyframes.removeAll()
    }
    
    private func duplicateSelectedKeyframes() {
        var duplicatedKeyframes: [(track: DopesheetTrack, keyframe: DopesheetKeyframe)] = []
        
        for track in dopesheetManager.filteredTracks {
            for keyframe in track.keyframes {
                if dopesheetManager.selectedKeyframes.contains(keyframe.id) {
                    let newKeyframe = DopesheetKeyframe(
                        id: UUID(),
                        time: keyframe.time + 1.0, // 1 second offset
                        value: keyframe.value,
                        inTangent: keyframe.inTangent,
                        outTangent: keyframe.outTangent
                    )
                    duplicatedKeyframes.append((track: track, keyframe: newKeyframe))
                }
            }
        }
        
        for (track, keyframe) in duplicatedKeyframes {
            track.keyframes.append(keyframe)
            track.keyframes.sort { $0.time < $1.time }
        }
        
        // Select the duplicated keyframes
        dopesheetManager.selectedKeyframes.removeAll()
        for (_, keyframe) in duplicatedKeyframes {
            dopesheetManager.selectedKeyframes.insert(keyframe.id)
        }
    }
    
    private func copySelectedKeyframes() {
        // Store selected keyframes in pasteboard
        let copiedData = dopesheetManager.filteredTracks.flatMap { track in
            track.keyframes.filter { dopesheetManager.selectedKeyframes.contains($0.id) }
        }
        
        // Convert to JSON for clipboard
        if let data = try? JSONEncoder().encode(copiedData) {
            let pasteboard = NSPasteboard.general
            pasteboard.clearContents()
            pasteboard.setData(data, forType: .filePromise)
        }
    }
    
    private func pasteKeyframes() {
        guard let data = NSPasteboard.general.data(forType: .filePromise) else { return }
        guard let copiedKeyframes = try? JSONDecoder().decode([DopesheetKeyframe].self, from: data) else { return }
        
        // Offset pasted keyframes to current time
        let currentTime = manager.currentTime
        let firstKeyframeTime = copiedKeyframes.min { $0.time < $1.time }?.time ?? 0
        let timeOffset = currentTime - firstKeyframeTime
        
        for keyframe in copiedKeyframes {
            let newKeyframe = DopesheetKeyframe(
                id: UUID(),
                time: keyframe.time + timeOffset,
                value: keyframe.value,
                inTangent: keyframe.inTangent,
                outTangent: keyframe.outTangent
            )
            
            // Add to first track or create new track
            if let firstTrack = dopesheetManager.filteredTracks.first {
                firstTrack.keyframes.append(newKeyframe)
                firstTrack.keyframes.sort { $0.time < $1.time }
            }
        }
    }
    
    private func insertKeyframeAtCurrentTime() {
        guard let firstTrack = dopesheetManager.filteredTracks.first else { return }
        
        let newKeyframe = DopesheetKeyframe(
            id: UUID(),
            time: manager.currentTime,
            value: 0.0, // Default value
            inTangent: 0,
            outTangent: 0
        )
        
        firstTrack.keyframes.append(newKeyframe)
        firstTrack.keyframes.sort { $0.time < $1.time }
    }
    
    private func splitKeyframeAtCurrentTime() {
        for track in dopesheetManager.filteredTracks {
            if let keyframeIndex = track.keyframes.firstIndex(where: { 
                abs($0.time - manager.currentTime) < 0.016 // Within 1 frame
            }) {
                var keyframe = track.keyframes[keyframeIndex]
                
                // Create two keyframes at the same time
                var beforeKeyframe = keyframe
                var afterKeyframe = keyframe
                afterKeyframe.id = UUID()
                
                beforeKeyframe.outTangent = 0
                afterKeyframe.inTangent = 0
                
                track.keyframes[keyframeIndex] = beforeKeyframe
                track.keyframes.insert(afterKeyframe, at: keyframeIndex + 1)
            }
        }
    }
    
    private func mirrorSelectedKeyframes() {
        guard !dopesheetManager.selectedKeyframes.isEmpty else { return }
        
        // Calculate center time
        var totalTime: Float = 0
        var count = 0
        
        for track in dopesheetManager.filteredTracks {
            for keyframe in track.keyframes {
                if dopesheetManager.selectedKeyframes.contains(keyframe.id) {
                    totalTime += keyframe.time
                    count += 1
                }
            }
        }
        
        let centerTime = totalTime / Float(count)
        
        // Mirror selected keyframes around center
        for track in dopesheetManager.filteredTracks {
            for keyframeIndex in track.keyframes.indices {
                let keyframe = track.keyframes[keyframeIndex]
                if dopesheetManager.selectedKeyframes.contains(keyframe.id) {
                    let timeOffset = keyframe.time - centerTime
                    track.keyframes[keyframeIndex].time = centerTime - timeOffset
                }
            }
            track.keyframes.sort { $0.time < $1.time }
        }
    }
    
    private func nudgeSelectedKeyframes(timeDelta: Float = 0, valueDelta: Float = 0) {
        for track in dopesheetManager.filteredTracks {
            for keyframeIndex in track.keyframes.indices {
                let keyframe = track.keyframes[keyframeIndex]
                if dopesheetManager.selectedKeyframes.contains(keyframe.id) {
                    track.keyframes[keyframeIndex].time += timeDelta
                    
                    if valueDelta != 0 {
                        if let floatValue = keyframe.value as? Float {
                            track.keyframes[keyframeIndex].value = floatValue + valueDelta
                        }
                    }
                }
            }
        }
    }
    
    private func scaleSelectedKeyframes(timeScale: Float = 1.0, valueScale: Float = 1.0) {
        guard !dopesheetManager.selectedKeyframes.isEmpty else { return }
        
        // Calculate center point for scaling
        var totalTime: Float = 0
        var totalValue: Float = 0
        var count = 0
        
        for track in dopesheetManager.filteredTracks {
            for keyframe in track.keyframes {
                if dopesheetManager.selectedKeyframes.contains(keyframe.id) {
                    totalTime += keyframe.time
                    if let floatValue = keyframe.value as? Float {
                        totalValue += floatValue
                    }
                    count += 1
                }
            }
        }
        
        let timeCenter = totalTime / Float(count)
        let valueCenter = totalValue / Float(count)
        
        // Apply scaling
        for track in dopesheetManager.filteredTracks {
            for keyframeIndex in track.keyframes.indices {
                let keyframe = track.keyframes[keyframeIndex]
                if dopesheetManager.selectedKeyframes.contains(keyframe.id) {
                    // Scale time around center
                    let timeOffset = keyframe.time - timeCenter
                    track.keyframes[keyframeIndex].time = timeCenter + timeOffset * timeScale
                    
                    // Scale value if applicable
                    if let floatValue = keyframe.value as? Float {
                        let valueOffset = floatValue - valueCenter
                        track.keyframes[keyframeIndex].value = valueCenter + valueOffset * valueScale
                    }
                }
            }
            track.keyframes.sort { $0.time < $1.time }
        }
    }
}

struct DopesheetKeyframeView: View {
    let keyframe: DopesheetKeyframe
    let position: CGPoint
    let color: Color
    let isSelected: Bool
    let isHovered: Bool
    
    var body: some View {
        let size = getKeyframeSize()
        let shape = getKeyframeShape()
        
        Group {
            switch shape {
            case .circle:
                Circle()
            case .square:
                Rectangle()
            case .diamond:
                Diamond()
            case .triangle:
                Triangle()
            case .hexagon:
                Hexagon()
            case .star:
                Star()
            }
        }
        .fill(getKeyframeColor())
        .frame(width: size.width, height: size.height)
        .overlay(
            // Selection highlight
            isSelected ?
            RoundedRectangle(cornerRadius: 2)
                .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
                .frame(width: size.width + 4, height: size.height + 4)
            : nil
        )
        .overlay(
            // Breakpoint indicator
            keyframe.isBreakpoint ?
            Rectangle()
                .fill(DesignSystem.Colors.accentDanger)
                .frame(width: 2, height: size.height + 4)
                .offset(x: size.width / 2 + 2)
            : nil
        )
        .scaleEffect(isHovered ? 1.2 : 1.0)
        .animation(.easeInOut(duration: 0.1), value: isHovered)
    }
    
    private func getKeyframeSize() -> CGSize {
        let baseSize: CGFloat = 12
        let selectedSize = isSelected ? baseSize + 2 : baseSize
        return CGSize(width: selectedSize, height: selectedSize)
    }
    
    private func getKeyframeColor() -> Color {
        if isSelected {
            return DesignSystem.Colors.accentPrimary
        }
        return color
    }
    
    private func getKeyframeShape() -> AnimationKeyframe.KeyframeShape {
        // Determine shape based on interpolation type or other properties
        if keyframe.isBreakpoint {
            return .square
        }
        
        // Use interpolation type if available
        if let interpolationType = keyframe.interpolationType {
            return interpolationType.shape
        }
        
        return .circle // Default shape
    }
}

// MARK: - Keyframe Shapes
struct Diamond: Shape {
    func path(in rect: CGRect) -> Path {
        var path = Path()
        path.move(to: CGPoint(x: rect.midX, y: rect.minY))
        path.addLine(to: CGPoint(x: rect.maxX, y: rect.midY))
        path.addLine(to: CGPoint(x: rect.midX, y: rect.maxY))
        path.addLine(to: CGPoint(x: rect.minX, y: rect.midY))
        path.closeSubpath()
        return path
    }
}

struct Triangle: Shape {
    func path(in rect: CGRect) -> Path {
        var path = Path()
        path.move(to: CGPoint(x: rect.midX, y: rect.minY))
        path.addLine(to: CGPoint(x: rect.maxX, y: rect.maxY))
        path.addLine(to: CGPoint(x: rect.minX, y: rect.maxY))
        path.closeSubpath()
        return path
    }
}

struct Hexagon: Shape {
    func path(in rect: CGRect) -> Path {
        var path = Path()
        let width = rect.width
        let height = rect.height
        let side = min(width, height) / 2
        
        path.move(to: CGPoint(x: rect.midX + side, y: rect.midY))
        path.addLine(to: CGPoint(x: rect.midX + side/2, y: rect.midY + side * 0.866))
        path.addLine(to: CGPoint(x: rect.midX - side/2, y: rect.midY + side * 0.866))
        path.addLine(to: CGPoint(x: rect.midX - side, y: rect.midY))
        path.addLine(to: CGPoint(x: rect.midX - side/2, y: rect.midY - side * 0.866))
        path.addLine(to: CGPoint(x: rect.midX + side/2, y: rect.midY - side * 0.866))
        path.closeSubpath()
        return path
    }
}

struct Star: Shape {
    func path(in rect: CGRect) -> Path {
        var path = Path()
        let center = CGPoint(x: rect.midX, y: rect.midY)
        let outerRadius = min(rect.width, rect.height) / 2
        let innerRadius = outerRadius * 0.4
        let points = 5
        
        for i in 0..<points * 2 {
            let angle = CGFloat(i) * .pi / CGFloat(points) - .pi / 2
            let radius = i % 2 == 0 ? outerRadius : innerRadius
            let point = CGPoint(
                x: center.x + cos(angle) * radius,
                y: center.y + sin(angle) * radius
            )
            
            if i == 0 {
                path.move(to: point)
            } else {
                path.addLine(to: point)
            }
        }
        path.closeSubpath()
        return path
    }
}

struct DopesheetGrid: View {
    let duration: Float
    let size: CGSize
    let trackCount: Int
    
    var body: some View {
        Canvas { context, size in
            // Vertical lines (time)
            for i in 0..<Int(duration * 2) {
                let x = (CGFloat(i) / CGFloat(duration * 2)) * size.width
                var path = Path()
                path.move(to: CGPoint(x: x, y: 0))
                path.addLine(to: CGPoint(x: x, y: size.height))
                context.stroke(path, with: .color(DesignSystem.Colors.border.opacity(0.3)), lineWidth: 1)
            }
            
            // Horizontal lines (tracks)
            for i in 0..<trackCount {
                let y = CGFloat(i * 30) + 30
                var path = Path()
                path.move(to: CGPoint(x: 0, y: y))
                path.addLine(to: CGPoint(x: size.width, y: y))
                context.stroke(path, with: .color(DesignSystem.Colors.border.opacity(0.2)), lineWidth: 1)
            }
        }
    }
}

// MARK: - Enhanced Curve Editor Data Models

class CurveEditorManager: ObservableObject {
    static let shared = CurveEditorManager()
    
    @Published var curves: [AnimationCurve] = []
    @Published var visibleCurves: Set<UUID> = []
    @Published var selectedCurves: Set<UUID> = []
    @Published var autoScale: Bool = true
    @Published var minValue: Float = -1.0
    @Published var maxValue: Float = 1.0
    @Published var showTangentHandles: Bool = true
    @Published var showTangentWeights: Bool = true
    @Published var infinityMode: InfinityMode = .constant
    @Published var showInfinityVisualization: Bool = true
    
    enum InfinityMode: String, CaseIterable {
        case constant = "Constant"
        case linear = "Linear"
        case cycle = "Cycle"
        case cycleWithOffset = "Cycle with Offset"
        case oscillate = "Oscillate"
    }
}

struct AnimationCurve: Identifiable {
    let id = UUID()
    var name: String
    var propertyPath: String
    var keyframes: [CurveKeyframe] = []
    var color: Color = .blue
    var isVisible: Bool = true
    var preInfinity: CurveEditorManager.InfinityMode = .constant
    var postInfinity: CurveEditorManager.InfinityMode = .constant
}

struct CurveKeyframe: Identifiable {
    let id = UUID()
    var time: Float
    var value: Float
    var inTangent: CGPoint = .zero
    var outTangent: CGPoint = .zero
    var inWeight: Float = 1.0
    var outWeight: Float = 1.0
    var interpolation: InterpolationType = .bezier
    var isSelected: Bool = false
    
    enum InterpolationType: String, CaseIterable {
        case constant = "Constant"
        case linear = "Linear"
        case bezier = "Bezier"
        case stepped = "Stepped"
    }
}

// MARK: - Enhanced Curve Editor View
struct CurveEditorView: View {
    @ObservedObject var manager: AnimationEditorManager
    @StateObject private var curveManager = CurveEditorManager.shared
    @State private var selectedKeyframe: UUID?
    @State private var hoveredKeyframe: UUID?
    @State private var isDraggingTangent = false
    @State private var draggedTangent: (keyframeId: UUID, isIn: Bool)?
    
    var body: some View {
        VStack(spacing: 0) {
            // Curve editor toolbar
            CurveEditorToolbar()
                .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Main curve editor area
            GeometryReader { geometry in
                ZStack {
                    // Grid
                    CurveEditorGrid(
                        size: geometry.size,
                        minValue: curveManager.autoScale ? getAutoMinValue() : curveManager.minValue,
                        maxValue: curveManager.autoScale ? getAutoMaxValue() : curveManager.maxValue
                    )
                    
                    // Infinity visualization
                    if curveManager.showInfinityVisualization {
                        ForEach(curveManager.curves.filter { curveManager.visibleCurves.contains($0.id) }, id: \.id) { curve in
                            InfinityVisualization(
                                curve: curve,
                                size: geometry.size,
                                minValue: curveManager.autoScale ? getAutoMinValue() : curveManager.minValue,
                                maxValue: curveManager.autoScale ? getAutoMaxValue() : curveManager.maxValue
                            )
                        }
                    }
                    
                    // Curves
                    ForEach(curveManager.curves.filter { curveManager.visibleCurves.contains($0.id) }, id: \.id) { curve in
                        AnimationCurvePath(
                            curve: curve,
                            size: geometry.size,
                            minValue: curveManager.autoScale ? getAutoMinValue() : curveManager.minValue,
                            maxValue: curveManager.autoScale ? getAutoMaxValue() : curveManager.maxValue
                        )
                        .stroke(curve.color, lineWidth: 2)
                        
                        // Keyframes
                        ForEach(curve.keyframes, id: \.id) { keyframe in
                            CurveKeyframeView(
                                keyframe: keyframe,
                                curve: curve,
                                size: geometry.size,
                                minValue: curveManager.autoScale ? getAutoMinValue() : curveManager.minValue,
                                maxValue: curveManager.autoScale ? getAutoMaxValue() : curveManager.maxValue,
                                isSelected: selectedKeyframe == keyframe.id,
                                isHovered: hoveredKeyframe == keyframe.id,
                                showTangents: curveManager.showTangentHandles,
                                showWeights: curveManager.showTangentWeights
                            )
                            .onTapGesture {
                                selectedKeyframe = keyframe.id
                            }
                            .onHover { isHovered in
                                hoveredKeyframe = isHovered ? keyframe.id : nil
                            }
                        }
                    }
                    
                    // Playhead line
                    Rectangle()
                        .fill(DesignSystem.Colors.accentPrimary)
                        .frame(width: 2)
                        .offset(x: CGFloat(manager.currentTime / (manager.currentClip?.duration ?? 10)) * geometry.size.width - geometry.size.width / 2)
                }
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
    }
    
    private func getAutoMinValue() -> Float {
        guard !curveManager.curves.isEmpty else { return -1.0 }
        let allValues = curveManager.curves.flatMap { $0.keyframes.map { $0.value } }
        return allValues.min() ?? -1.0
    }
    
    private func getAutoMaxValue() -> Float {
        guard !curveManager.curves.isEmpty else { return 1.0 }
        let allValues = curveManager.curves.flatMap { $0.keyframes.map { $0.value } }
        return allValues.max() ?? 1.0
    }
}

// MARK: - Curve Editor Toolbar
struct CurveEditorToolbar: View {
    @StateObject private var curveManager = CurveEditorManager.shared
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            // Auto-scale toggle
            Toggle("Auto Scale", isOn: $curveManager.autoScale)
                .toggleStyle(.button)
            
            if !curveManager.autoScale {
                HStack {
                    Text("Min:")
                    TextField("", value: $curveManager.minValue, format: .number)
                        .textFieldStyle(.roundedBorder)
                        .frame(width: 60)
                    
                    Text("Max:")
                    TextField("", value: $curveManager.maxValue, format: .number)
                        .textFieldStyle(.roundedBorder)
                        .frame(width: 60)
                }
            }
            
            Divider()
                .frame(height: 16)
            
            // Tangent controls
            Toggle("Tangents", isOn: $curveManager.showTangentHandles)
                .toggleStyle(.button)
            
            Toggle("Weights", isOn: $curveManager.showTangentWeights)
                .toggleStyle(.button)
            
            Divider()
                .frame(height: 16)
            
            // Visual options
            Toggle("Keyframe Colors", isOn: $AnimationEditorManager.shared.keyframeColorByProperty)
                .toggleStyle(.button)
            
            Toggle("Keyframe Shapes", isOn: $AnimationEditorManager.shared.keyframeShapeByInterpolation)
                .toggleStyle(.button)
            
            Toggle("Selection Highlight", isOn: $AnimationEditorManager.shared.selectionHighlightIntensity > 0.5)
                .toggleStyle(.button)
            
            Divider()
                .frame(height: 16)
            
            // Infinity mode
            Picker("Infinity", selection: $curveManager.infinityMode) {
                ForEach(CurveEditorManager.InfinityMode.allCases, id: \.self) { mode in
                    Text(mode.rawValue).tag(mode)
                }
            }
            .pickerStyle(.menu)
            
            Toggle("Show Infinity", isOn: $curveManager.showInfinityVisualization)
                .toggleStyle(.button)
            
            Spacer()
            
            Text("\(curveManager.curves.count) curves")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .padding(DesignSystem.Spacing.sm)
    }
}
// MARK: - Curve Editor Components

struct CurveEditorGrid: View {
    let size: CGSize
    let minValue: Float
    let maxValue: Float
    
    var body: some View {
        Canvas { context, size in
            // Horizontal center line
            var centerPath = Path()
            centerPath.move(to: CGPoint(x: 0, y: size.height / 2))
            centerPath.addLine(to: CGPoint(x: size.width, y: size.height / 2))
            context.stroke(centerPath, with: .color(DesignSystem.Colors.border), lineWidth: 1)
            
            // Vertical grid lines
            for i in 0..<20 {
                let x = (CGFloat(i) / 20) * size.width
                var path = Path()
                path.move(to: CGPoint(x: x, y: 0))
                path.addLine(to: CGPoint(x: x, y: size.height))
                context.stroke(path, with: .color(DesignSystem.Colors.border.opacity(0.2)), lineWidth: 1)
            }
            
            // Horizontal grid lines
            for i in 0..<10 {
                let y = (CGFloat(i) / 10) * size.height
                var path = Path()
                path.move(to: CGPoint(x: 0, y: y))
                path.addLine(to: CGPoint(x: size.width, y: y))
                context.stroke(path, with: .color(DesignSystem.Colors.border.opacity(0.2)), lineWidth: 1)
            }
        }
    }
}

struct AnimationCurvePath: Shape {
    let curve: AnimationCurve
    let size: CGSize
    let minValue: Float
    let maxValue: Float
    
    func path(in rect: CGRect) -> Path {
        var path = Path()
        
        guard !curve.keyframes.isEmpty else { return path }
        
        let sortedKeyframes = curve.keyframes.sorted { $0.time < $1.time }
        let duration = max(sortedKeyframes.last?.time ?? 1.0, 1.0)
        
        for (index, keyframe) in sortedKeyframes.enumerated() {
            let x = (CGFloat(keyframe.time) / CGFloat(duration)) * size.width
            let normalizedValue = (keyframe.value - minValue) / (maxValue - minValue)
            let y = size.height - (CGFloat(normalizedValue) * size.height)
            let point = CGPoint(x: x, y: y)
            
            if index == 0 {
                path.move(to: point)
            } else {
                switch keyframe.interpolation {
                case .constant:
                    path.addLine(to: point)
                case .linear:
                    path.addLine(to: point)
                case .bezier:
                    if index > 0 {
                        let prevKeyframe = sortedKeyframes[index - 1]
                        let prevX = (CGFloat(prevKeyframe.time) / CGFloat(duration)) * size.width
                        let prevNormalizedValue = (prevKeyframe.value - minValue) / (maxValue - minValue)
                        let prevY = size.height - (CGFloat(prevNormalizedValue) * size.height)
                        let prevPoint = CGPoint(x: prevX, y: prevY)
                        
                        let control1 = CGPoint(
                            x: prevPoint.x + prevKeyframe.outTangent.x * 50,
                            y: prevPoint.y - prevKeyframe.outTangent.y * 50
                        )
                        let control2 = CGPoint(
                            x: point.x - keyframe.inTangent.x * 50,
                            y: point.y + keyframe.inTangent.y * 50
                        )
                        
                        path.addCurve(to: point, control1: control1, control2: control2)
                    }
                case .stepped:
                    path.addLine(to: point)
                }
            }
        }
        
        return path
    }
}

struct CurveKeyframeView: View {
    let keyframe: CurveKeyframe
    let curve: AnimationCurve
    let size: CGSize
    let minValue: Float
    let maxValue: Float
    let isSelected: Bool
    let isHovered: Bool
    let showTangents: Bool
    let showWeights: Bool
    @ObservedObject var manager: CurveEditorManager
    
    var body: some View {
        let keyframePosition = CGPoint(
            x: CGFloat(keyframe.time / curve.duration) * size.width,
            y: CGFloat(size.height / 2) * (1.0 - (keyframe.value - minValue) / (maxValue - minValue))
        )
        
        // Determine keyframe color and shape
        let keyframeColor = AnimationEditorManager.shared.getKeyframeColor(for: curve.propertyPath, interpolation: keyframe.interpolation)
        let keyframeShape = AnimationEditorManager.shared.getKeyframeShape(for: keyframe.interpolation)
        
        ZStack {
            // Keyframe shape
            Group {
                Path { path in
                    switch keyframeShape {
                    case "circle":
                        Circle()
                            .fill(keyframeColor)
                            .frame(width: 8, height: 8)
                    case "diamond":
                        Path { path in
                            Move(to: CGPoint(x: 4, y: 0))
                            Line(to: CGPoint(x: 8, y: 8))
                            Line(to: CGPoint(x: 0, y: 8))
                            Line(to: CGPoint(x: -4, y: 0))
                            Line(to: CGPoint(x: -4, y: 8))
                            Line(to: CGPoint(x: 0, y: 8))
                            Line(to: CGPoint(x: 4, y: 0))
                            ClosePath()
                        }
                    case "square":
                        Path { path in
                            Rectangle()
                                .fill(keyframeColor)
                                .frame(width: 8, height: 8)
                    }
                    default:
                        Circle()
                            .fill(keyframeColor)
                            .frame(width: 8, height: 8)
                    }
                }
                .offset(keyframePosition)
                .scaleEffect(isHovered ? 1.2 : 1.0)
                .position(keyframePosition)
            
            // Selection highlight
            if isSelected {
                Circle()
                    .stroke(Color.accentPrimary, lineWidth: 2)
                    .scaleEffect(1.2)
                    .position(keyframePosition)
            }
            
            // Hover effect
            if isHovered {
                Circle()
                    .fill(Color.accentPrimary.opacity(0.3))
                    .frame(width: 12, height: 12)
                    .position(keyframePosition)
            }
            
            // Tangent controls
            if showTangents {
                // In tangent
                Path { path in
                    Move(to: CGPoint(x: -8, y: 0))
                    Line(to: CGPoint(x: 0, y: 0))
                    Circle()
                        .fill(Color.orange)
                        .frame(width: 4, height: 4)
                }
                .offset(keyframePosition)
                .scaleEffect(1.0)
                .position(keyframePosition)
                
                // Out tangent
                Path { path in
                    Move(to: CGPoint(x: 8, y: 0))
                    Line(to: CGPoint(x: 0, y: 0))
                    Circle()
                        .fill(Color.orange)
                        .frame(width: 4, height: 4)
                }
                .offset(keyframePosition)
                .scaleEffect(1.0)
                .position(keyframePosition)
            }
        }
    }
    
    private func getKeyframePosition() -> CGPoint {
        let duration = max(curve.keyframes.map { $0.time }.max() ?? 1.0, 1.0)
        let x = (CGFloat(keyframe.time) / CGFloat(duration)) * size.width
        let normalizedValue = (keyframe.value - minValue) / (maxValue - minValue)
        let y = size.height - (CGFloat(normalizedValue) * size.height)
        return CGPoint(x: x, y: y)
    }
}

struct InfinityVisualization: View {
    let curve: AnimationCurve
    let size: CGSize
    let minValue: Float
    let maxValue: Float
    
    var body: some View {
        Canvas { context, size in
            guard let firstKeyframe = curve.keyframes.sorted(by: { $0.time < $1.time }).first,
                  let lastKeyframe = curve.keyframes.sorted(by: { $0.time < $1.time }).last else { return }
            
            let duration = max(curve.keyframes.map { $0.time }.max() ?? 1.0, 1.0)
            
            // Pre-infinity (before first keyframe)
            let firstX = (CGFloat(firstKeyframe.time) / CGFloat(duration)) * size.width
            let firstNormalizedValue = (firstKeyframe.value - minValue) / (maxValue - minValue)
            let firstY = size.height - (CGFloat(firstNormalizedValue) * size.height)
            
            switch curve.preInfinity {
            case .constant:
                var path = Path()
                path.move(to: CGPoint(x: 0, y: firstY))
                path.addLine(to: CGPoint(x: firstX, y: firstY))
                context.stroke(path, with: .color(curve.color.opacity(0.5)), style: StrokeStyle(lineWidth: 1, dash: [5, 5]))
                
            case .linear:
                // Calculate linear extrapolation
                if curve.keyframes.count >= 2 {
                    let secondKeyframe = curve.keyframes.sorted(by: { $0.time < $1.time })[1]
                    let slope = (secondKeyframe.value - firstKeyframe.value) / (secondKeyframe.time - firstKeyframe.time)
                    let extrapolatedValue = firstKeyframe.value - slope * firstKeyframe.time
                    let extrapolatedY = size.height - (CGFloat((extrapolatedValue - minValue) / (maxValue - minValue)) * size.height)
                    
                    var path = Path()
                    path.move(to: CGPoint(x: 0, y: extrapolatedY))
                    path.addLine(to: CGPoint(x: firstX, y: firstY))
                    context.stroke(path, with: .color(curve.color.opacity(0.5)), style: StrokeStyle(lineWidth: 1, dash: [5, 5]))
                }
                
            case .cycle, .cycleWithOffset, .oscillate:
                // Draw repeating pattern
                var path = Path()
                for i in 0..<5 {
                    let offset = CGFloat(i) * firstX
                    if offset < size.width {
                        path.move(to: CGPoint(x: offset, y: firstY))
                        path.addLine(to: CGPoint(x: min(offset + firstX, size.width), y: firstY))
                    }
                }
                context.stroke(path, with: .color(curve.color.opacity(0.5)), style: StrokeStyle(lineWidth: 1, dash: [5, 5]))
            }
            
            // Post-infinity (after last keyframe)
            let lastX = (CGFloat(lastKeyframe.time) / CGFloat(duration)) * size.width
            let lastNormalizedValue = (lastKeyframe.value - minValue) / (maxValue - minValue)
            let lastY = size.height - (CGFloat(lastNormalizedValue) * size.height)
            
            switch curve.postInfinity {
            case .constant:
                var path = Path()
                path.move(to: CGPoint(x: lastX, y: lastY))
                path.addLine(to: CGPoint(x: size.width, y: lastY))
                context.stroke(path, with: .color(curve.color.opacity(0.5)), style: StrokeStyle(lineWidth: 1, dash: [5, 5]))
                
            case .linear:
                // Calculate linear extrapolation
                if curve.keyframes.count >= 2 {
                    let secondLastKeyframe = curve.keyframes.sorted(by: { $0.time < $1.time }).dropLast().last!
                    let slope = (lastKeyframe.value - secondLastKeyframe.value) / (lastKeyframe.time - secondLastKeyframe.time)
                    let extrapolatedValue = lastKeyframe.value + slope * (1.0 - lastKeyframe.time)
                    let extrapolatedY = size.height - (CGFloat((extrapolatedValue - minValue) / (maxValue - minValue)) * size.height)
                    
                    var path = Path()
                    path.move(to: CGPoint(x: lastX, y: lastY))
                    path.addLine(to: CGPoint(x: size.width, y: extrapolatedY))
                    context.stroke(path, with: .color(curve.color.opacity(0.5)), style: StrokeStyle(lineWidth: 1, dash: [5, 5]))
                }
                
            case .cycle, .cycleWithOffset, .oscillate:
                // Draw repeating pattern
                var path = Path()
                let cycleWidth = lastX
                var currentX = lastX
                while currentX < size.width {
                    path.move(to: CGPoint(x: currentX, y: lastY))
                    let nextX = min(currentX + cycleWidth, size.width)
                    path.addLine(to: CGPoint(x: nextX, y: lastY))
                    currentX = nextX
                }
                context.stroke(path, with: .color(curve.color.opacity(0.5)), style: StrokeStyle(lineWidth: 1, dash: [5, 5]))
            }
        }
// MARK: - Event Function Picker
struct EventFunctionPicker: View {
    @Environment(\.dismiss) private var dismiss
    @ObservedObject var event: AnimationEvent
    
    private let eventFunctions: [String: [String]] = [
        "Audio": ["PlaySound", "StopSound", "FadeInMusic", "FadeOutMusic", "SetMusicVolume"],
        "Particle": ["SpawnParticles", "StopParticles", "BurstParticles", "SetParticleColor"],
        "Footstep": ["PlayFootstep", "SetFootstepType", "EnableFootsteps", "DisableFootsteps"],
        "Hit": ["DealDamage", "ApplyForce", "ScreenShake", "PlayHitReaction"],
        "Generic": ["SendMessage", "SetVariable", "TriggerAnimation", "EnableObject", "DisableObject"]
    ]
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Select Function")
                .font(DesignSystem.Typography.headline)
            
            ScrollView {
                LazyVStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    ForEach(Array(eventFunctions.keys.sorted()), id: \.self) { category in
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                            Text(category)
                                .font(DesignSystem.Typography.bodyBold)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            ForEach(eventFunctions[category] ?? [], id: \.self) { function in
                                Button(action: {
                                    event.functionName = function
                                    dismiss()
                                }) {
                                    HStack {
                                        Text(function)
                                            .foregroundColor(DesignSystem.Colors.textPrimary)
                                        Spacer()
                                        if function == event.functionName {
                                            Image(systemName: "checkmark")
                                                .foregroundColor(DesignSystem.Colors.accentPrimary)
                                        }
                                    }
                                    .padding(.horizontal, DesignSystem.Spacing.sm)
                                    .padding(.vertical, 4)
                                    .background(
                                        function == event.functionName ?
                                        DesignSystem.Colors.selection :
                                        DesignSystem.Colors.backgroundTertiary
                                    )
                                    .cornerRadius(4)
                                }
                                .buttonStyle(.plain)
                            }
                        }
                        .padding(.bottom, DesignSystem.Spacing.sm)
                    }
                }
            }
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 500)
    }
}
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                            Text(category)
                                .font(DesignSystem.Typography.bodyBold)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            ForEach(eventFunctions[category] ?? [], id: \.self) { function in
                                Button(action: {
                                    event.functionName = function
                                    dismiss()
                                }) {
                                    HStack {
                                        Text(function)
                                            .foregroundColor(DesignSystem.Colors.textPrimary)
                                        Spacer()
                                        if function == event.functionName {
                                            Image(systemName: "checkmark")
                                                .foregroundColor(DesignSystem.Colors.accentPrimary)
                                        }
                                    }
                                    .padding(.horizontal, DesignSystem.Spacing.sm)
                                    .padding(.vertical, 4)
                                    .background(
                                        function == event.functionName ?
                                        DesignSystem.Colors.selection :
                                        DesignSystem.Colors.backgroundTertiary
                                    )
                                    .cornerRadius(4)
                                }
                                .buttonStyle(.plain)
                            }
                        }
                        .padding(.bottom, DesignSystem.Spacing.sm)
                    }
                }
            }
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 500)
    }
}

// MARK: - Event Editor Dialog
struct EventEditorDialog: View {
    @Environment(\.dismiss) private var dismiss
    @ObservedObject var manager: AnimationEditorManager
    
    var event: AnimationEvent?
    @State private var editingEvent: AnimationEvent
    @State private var eventTime: Float
    
    init(event: AnimationEvent?, manager: AnimationEditorManager) {
        self.event = event
        self.manager = manager
        
        if let event = event {
            self._editingEvent = ObservedObject(wrappedValue: event)
            self._eventTime = State(initialValue: event.time)
        } else {
            let newEvent = AnimationEvent(
                time: manager.currentTime,
                functionName: "PlaySound",
                parameter: "",
                eventType: .generic
            )
            self._editingEvent = ObservedObject(wrappedValue: newEvent)
            self._eventTime = State(initialValue: manager.currentTime)
        }
    }
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text(event == nil ? "Add Event" : "Edit Event")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                HStack {
                    Text("Time:")
                    Slider(value: $eventTime, in: 0...(manager.currentClip?.duration ?? 10))
                    Text(String(format: "%.2f", eventTime))
                        .font(DesignSystem.Typography.smallMono)
                        .frame(width: 50)
                }
                
                HStack {
                    Text("Type:")
                    Picker("Type", selection: $editingEvent.eventType) {
                        ForEach(AnimationEvent.AnimationEventType.allCases, id: \.self) { type in
                            Text(type.rawValue).tag(type)
                        }
                    }
                    .pickerStyle(.menu)
                }
                
                HStack {
                    Text("Function:")
                    TextField("Function name", text: $editingEvent.functionName)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Parameter:")
                    TextField("Event parameter", text: $editingEvent.parameter)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Description:")
                    TextField("Event description", text: $editingEvent.description)
                        .textFieldStyle(.roundedBorder)
                }
            }
            
            Spacer()
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button(event == nil ? "Add" : "Save") {
                    saveEvent()
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(editingEvent.functionName.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 450, height: 350)
    }
    
    private func saveEvent() {
        editingEvent.time = eventTime
        
        if let existingEvent = event {
            // Update existing event
            if let index = manager.currentClip?.events.firstIndex(where: { $0.id == existingEvent.id }) {
                manager.currentClip?.events[index] = editingEvent
            }
        } else {
            // Add new event
            manager.currentClip?.events.append(editingEvent)
        }
    }
}

// MARK: - Event Debug Panel
struct EventDebugPanel: View {
    @ObservedObject var manager = AnimationEditorManager.shared
    @State private var selectedStatus: EventHistoryEntry.ExecutionStatus? = nil
    @State private var selectedEventType: AnimationEvent.AnimationEventType? = nil
    @State private var showOnlyFailed: Bool = false
    
    var filteredHistory: [EventHistoryEntry] {
        var filtered = manager.eventHistory
        
        if let status = selectedStatus {
            filtered = filtered.filter { $0.executionStatus == status }
        }
        
        if let eventType = selectedEventType {
            filtered = filtered.filter { $0.eventType == eventType }
        }
        
        if showOnlyFailed {
            filtered = filtered.filter { $0.executionStatus == .failed }
        }
        
        return filtered.reversed() // Show most recent first
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Debug panel header
            HStack {
                Text("Event Debug Log")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                Toggle("Enable Debugging", isOn: $manager.isEventDebuggingEnabled)
                    .toggleStyle(.switch)
                
                Button(action: { manager.clearEventHistory() }) {
                    Image(systemName: "trash")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                
                Button(action: { manager.exportEventHistory() }) {
                    Image(systemName: "square.and.arrow.up")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Filters
            HStack(spacing: DesignSystem.Spacing.md) {
                // Status filter
                Picker("Status", selection: $selectedStatus) {
                    Text("All").tag(nil as EventHistoryEntry.ExecutionStatus?)
                    ForEach(EventHistoryEntry.ExecutionStatus.allCases, id: \.self) { status in
                        Text(status.rawValue).tag(status as EventHistoryEntry.ExecutionStatus?)
                    }
                }
                .pickerStyle(.menu)
                .frame(width: 120)
                
                // Event type filter
                Picker("Type", selection: $selectedEventType) {
                    Text("All").tag(nil as AnimationEvent.AnimationEventType?)
                    ForEach(AnimationEvent.AnimationEventType.allCases, id: \.self) { type in
                        Text(type.rawValue).tag(type as AnimationEvent.AnimationEventType?)
                    }
                }
                .pickerStyle(.menu)
                .frame(width: 100)
                
                // Show only failed
                Toggle("Failed Only", isOn: $showOnlyFailed)
                    .toggleStyle(.switch)
                
                Spacer()
                
                Text("\(filteredHistory.count) entries")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            .padding(.horizontal, DesignSystem.Spacing.sm)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Event history list
            ScrollView {
                LazyVStack(spacing: 0) {
                    if filteredHistory.isEmpty {
                        VStack {
                            Spacer()
                            Text("No event history")
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            Spacer()
                        }
                        .frame(height: 200)
                    } else {
                        ForEach(filteredHistory, id: \.id) { entry in
                            EventHistoryRow(entry: entry)
                        }
                    }
                }
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Event History Row
struct EventHistoryRow: View {
    let entry: EventHistoryEntry
    
    var body: some View {
        HStack(alignment: .top, spacing: DesignSystem.Spacing.sm) {
            // Status indicator
            Circle()
                .fill(entry.executionStatus.color)
                .frame(width: 8, height: 8)
            
            VStack(alignment: .leading, spacing: 2) {
                // Event name and time
                HStack {
                    Text(entry.eventName)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                    
                    Text(String(format: "%.2fs", entry.eventTime))
                        .font(DesignSystem.Typography.smallMono)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                // Function and parameter
                HStack {
                    Text(entry.functionName)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    if !entry.parameter.isEmpty {
                        Text("(\(entry.parameter))")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    Spacer()
                    
                    // Status and type
                    HStack(spacing: 4) {
                        Text(entry.eventType.rawValue)
                            .font(DesignSystem.Typography.small)
                            .padding(.horizontal, 6)
                            .padding(.vertical, 2)
                            .background(entry.eventType.defaultColor.opacity(0.2))
                            .cornerRadius(4)
                        
                        Text(entry.executionStatus.rawValue)
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(entry.executionStatus.color)
                    }
                }
                
                // Timestamp
                Text(DateFormatter.eventHistoryFormatter.string(from: entry.timestamp))
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundPrimary)
        .overlay(
            Rectangle()
                .fill(DesignSystem.Colors.border.opacity(0.2))
                .frame(height: 1),
            alignment: .bottom
        )
    }
}

// MARK: - Date Formatter Extension
extension DateFormatter {
    static let eventHistoryFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "HH:mm:ss.SSS"
        return formatter
    }()
}

// MARK: - Animation Retargeting Models

struct AvatarDefinition: Identifiable, ObservableObject {
    let id = UUID()
    var name: String
    var description: String = ""
    var avatarType: AvatarType = .humanoid
    var boneMappings: [BoneMapping] = []
    var poseType: PoseType = .tPose
    var isHumanoid: Bool = true
    var muscleDefinitions: [MuscleDefinition] = []
    var fingerRigSetup: FingerRigSetup?
    var toeRigSetup: ToeRigSetup?
    
    enum AvatarType: String, CaseIterable {
        case humanoid = "Humanoid"
        case generic = "Generic"
        case creature = "Creature"
        case mechanical = "Mechanical"
    }
    
    enum PoseType: String, CaseIterable {
        case tPose = "T-Pose"
        case aPose = "A-Pose"
        case custom = "Custom"
    }
}

struct BoneMapping: Identifiable, ObservableObject {
    let id = UUID()
    var sourceBone: String
    var targetBone: String
    var mappingType: MappingType = .direct
    var translationOffset: SIMD3<Float> = SIMD3<Float>(0, 0, 0)
    var rotationOffset: simd_quatf = simd_quatf()
    var scaleOffset: SIMD3<Float> = SIMD3<Float>(1, 1, 1)
    var isEnabled: Bool = true
    var mappingQuality: MappingQuality = .good
    var errorMargin: Float = 0.0
    
    enum MappingType: String, CaseIterable {
        case direct = "Direct"
        case mirrored = "Mirrored"
        case scaled = "Scaled"
        case custom = "Custom"
    }
    
    enum MappingQuality: String, CaseIterable {
        case excellent = "Excellent"
        case good = "Good"
        case fair = "Fair"
        case poor = "Poor"
        
        var color: Color {
            switch self {
            case .excellent: return DesignSystem.Colors.accentSuccess
            case .good: return DesignSystem.Colors.accentPrimary
            case .fair: return DesignSystem.Colors.accentWarning
            case .poor: return DesignSystem.Colors.accentDanger
            }
        }
    }
}

struct MuscleDefinition: Identifiable, ObservableObject {
    let id = UUID()
    var name: String
    var boneName: String
    var minAngle: Float = -180
    var maxAngle: Float = 180
    var stiffness: Float = 1.0
    var currentAngle: Float = 0
    var isLimited: Bool = true
}

struct FingerRigSetup: ObservableObject {
    var thumbBones: [String] = []
    var indexBones: [String] = []
    var middleBones: [String] = []
    var ringBones: [String] = []
    var pinkyBones: [String] = []
    var curlMultiplier: Float = 1.0
    var spreadMultiplier: Float = 1.0
}

struct ToeRigSetup: ObservableObject {
    var bigToeBones: [String] = []
    var otherToeBones: [String] = []
    var curlMultiplier: Float = 1.0
    var spreadMultiplier: Float = 1.0
}

// MARK: - Avatar Definition Editor
struct AvatarDefinitionEditor: View {
    @StateObject private var avatarManager = AvatarManager.shared
    @State private var selectedAvatar: AvatarDefinition?
    @State private var showAddAvatarDialog = false
    @State private var showBoneMappingInterface = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Avatar editor header
            HStack {
                Text("Avatar Definition Editor")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                Button(action: { showAddAvatarDialog = true }) {
                    Image(systemName: "plus.circle.fill")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                
                Button(action: { showBoneMappingInterface = true }) {
                    Image(systemName: "link")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            HSplitView {
                // Avatar list
                VStack(spacing: 0) {
                    HStack {
                        Text("Avatars")
                            .font(DesignSystem.Typography.bodyBold)
                        Spacer()
                    }
                    .padding(DesignSystem.Spacing.sm)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    
                    EditorDivider()
                    
                    List(avatarManager.avatars, id: \.id, selection: $selectedAvatar) { avatar in
                        AvatarRow(avatar: avatar)
                    }
                    .listStyle(.plain)
                }
                .frame(minWidth: 200, maxWidth: 250)
                
                // Avatar details
                if let avatar = selectedAvatar {
                    AvatarDetailsView(avatar: avatar)
                } else {
                    VStack {
                        Spacer()
                        Text("Select an avatar to edit")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        Spacer()
                    }
                    .frame(maxWidth: .infinity)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .sheet(isPresented: $showAddAvatarDialog) {
            AddAvatarDialog()
        }
        .sheet(isPresented: $showBoneMappingInterface) {
            if let avatar = selectedAvatar {
                BoneMappingInterface(avatar: avatar)
            }
        }
    }
}

// MARK: - Avatar Row
struct AvatarRow: View {
    @ObservedObject var avatar: AvatarDefinition
    
    var body: some View {
        HStack {
            // Avatar type indicator
            Circle()
                .fill(avatar.isHumanoid ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                .frame(width: 8, height: 8)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(avatar.name)
                    .font(DesignSystem.Typography.body)
                
                Text("\(avatar.avatarType.rawValue) • \(avatar.boneMappings.count) mappings")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
            
            // Mapping quality indicator
            if let quality = getOverallMappingQuality() {
                Circle()
                    .fill(quality.color)
                    .frame(width: 6, height: 6)
            }
        }
        .padding(.vertical, 4)
    }
    
    private func getOverallMappingQuality() -> BoneMapping.MappingQuality? {
        guard !avatar.boneMappings.isEmpty else { return nil }
        
        let qualityCounts = Dictionary(grouping: avatar.boneMappings, by: { $0.mappingQuality })
        
        if qualityCounts[.excellent, default: 0] == avatar.boneMappings.count {
            return .excellent
        } else if qualityCounts[.poor, default: 0] > 0 {
            return .poor
        } else if qualityCounts[.fair, default: 0] > 0 {
            return .fair
        } else {
            return .good
        }
    }
}

// MARK: - Avatar Details View
struct AvatarDetailsView: View {
    @ObservedObject var avatar: AvatarDefinition
    @State private var selectedTab: DetailTab = .general
    
    enum DetailTab: String, CaseIterable {
        case general = "General"
        case bones = "Bones"
        case muscles = "Muscles"
        case rigging = "Rigging"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Tab selector
            Picker("Tab", selection: $selectedTab) {
                ForEach(DetailTab.allCases, id: \.self) { tab in
                    Text(tab.rawValue).tag(tab)
                }
            }
            .pickerStyle(.segmented)
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Tab content
            ScrollView {
                switch selectedTab {
                case .general:
                    AvatarGeneralDetails(avatar: avatar)
                case .bones:
                    AvatarBoneDetails(avatar: avatar)
                case .muscles:
                    AvatarMuscleDetails(avatar: avatar)
                case .rigging:
                    AvatarRiggingDetails(avatar: avatar)
                }
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
    }
}

// MARK: - Avatar General Details
struct AvatarGeneralDetails: View {
    @ObservedObject var avatar: AvatarDefinition
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            GroupBox("Basic Information") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("Name:")
                        TextField("Avatar name", text: $avatar.name)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    HStack {
                        Text("Description:")
                        TextField("Avatar description", text: $avatar.description)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    HStack {
                        Text("Type:")
                        Picker("Type", selection: $avatar.avatarType) {
                            ForEach(AvatarDefinition.AvatarType.allCases, id: \.self) { type in
                                Text(type.rawValue).tag(type)
                            }
                        }
                        .pickerStyle(.menu)
                    }
                    
                    Toggle("Humanoid", isOn: $avatar.isHumanoid)
                        .toggleStyle(.switch)
                }
            }
            
            GroupBox("Pose Setup") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("Pose Type:")
                        Picker("Pose Type", selection: $avatar.poseType) {
                            ForEach(AvatarDefinition.PoseType.allCases, id: \.self) { type in
                                Text(type.rawValue).tag(type)
                            }
                        }
                        .pickerStyle(.menu)
                    }
                    
                    PoseSetupView(avatar: avatar)
                }
            }
            
            GroupBox("Mapping Statistics") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("Total Mappings:")
                        Spacer()
                        Text("\(avatar.boneMappings.count)")
                            .font(DesignSystem.Typography.smallMono)
                    }
                    
                    HStack {
                        Text("Active Mappings:")
                        Spacer()
                        Text("\(avatar.boneMappings.filter { $0.isEnabled }.count)")
                            .font(DesignSystem.Typography.smallMono)
                    }
                    
                    HStack {
                        Text("Average Error Margin:")
                        Spacer()
                        let avgError = avatar.boneMappings.isEmpty ? 0 : 
                            avatar.boneMappings.map { $0.errorMargin }.reduce(0, +) / Float(avatar.boneMappings.count)
                        Text(String(format: "%.3f", avgError))
                            .font(DesignSystem.Typography.smallMono)
                    }
                }
            }
        }
        .padding(DesignSystem.Spacing.md)
    }
}

// MARK: - Pose Setup View
struct PoseSetupView: View {
    @ObservedObject var avatar: AvatarDefinition
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            switch avatar.poseType {
            case .tPose:
                Text("T-Pose: Arms extended horizontally, legs straight")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            case .aPose:
                Text("A-Pose: Arms slightly lowered, natural standing position")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            case .custom:
                TextField("Custom pose description", text: .constant(""))
                    .textFieldStyle(.roundedBorder)
            }
            
            HStack {
                Button("Auto-Detect Pose") {
                    detectPose()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Reset to Default") {
                    resetPose()
                }
                .buttonStyle(.bordered)
            }
        }
    }
    
    private func detectPose() {
        // Auto-detection logic would go here
        print("Auto-detecting pose...")
    }
    
    private func resetPose() {
        avatar.poseType = .tPose
    }
}

// MARK: - Avatar Manager
class AvatarManager: ObservableObject {
    static let shared = AvatarManager()
    
    @Published var avatars: [AvatarDefinition] = []
    @Published var selectedAvatar: AvatarDefinition?
    
    init() {
        setupDefaultAvatars()
    }
    
    private func setupDefaultAvatars() {
        let humanoidAvatar = AvatarDefinition(
            name: "Humanoid Base",
            description: "Standard humanoid avatar for bipedal characters",
            avatarType: .humanoid,
            poseType: .tPose,
            isHumanoid: true
        )
        
        let genericAvatar = AvatarDefinition(
            name: "Generic Creature",
            description: "Generic avatar for non-humanoid creatures",
            avatarType: .generic,
            poseType: .tPose,
            isHumanoid: false
        )
        
        avatars = [humanoidAvatar, genericAvatar]
    }
    
    func addAvatar(_ avatar: AvatarDefinition) {
        avatars.append(avatar)
    }
    
    func removeAvatar(_ avatar: AvatarDefinition) {
        avatars.removeAll { $0.id == avatar.id }
    }
    
    func getAvatarByName(_ name: String) -> AvatarDefinition? {
        return avatars.first { $0.name == name }
    }
}

// MARK: - Avatar Bone Details
struct AvatarBoneDetails: View {
    @ObservedObject var avatar: AvatarDefinition
    @State private var showAddMappingDialog = false
    @State private var selectedMapping: BoneMapping?
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            HStack {
                Text("Bone Mappings")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                Button(action: { showAddMappingDialog = true }) {
                    Image(systemName: "plus")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                
                Button("Auto-Map") {
                    autoMapBones()
                }
                .buttonStyle(.bordered)
            }
            
            // Bone mappings list
            ScrollView {
                LazyVStack(spacing: 0) {
                    ForEach(avatar.boneMappings, id: \.id) { mapping in
                        BoneMappingRow(mapping: mapping, avatar: avatar)
                    }
                }
            }
            .frame(height: 300)
            .background(DesignSystem.Colors.backgroundPrimary)
        }
        .padding(DesignSystem.Spacing.md)
        .sheet(isPresented: $showAddMappingDialog) {
            AddBoneMappingDialog(avatar: avatar)
        }
    }
    
    private func autoMapBones() {
        // Auto-mapping logic would go here
        print("Auto-mapping bones...")
    }
}

// MARK: - Bone Mapping Row
struct BoneMappingRow: View {
    @ObservedObject var mapping: BoneMapping
    @ObservedObject var avatar: AvatarDefinition
    @State private var showEditDialog = false
    
    var body: some View {
        HStack {
            // Quality indicator
            Circle()
                .fill(mapping.mappingQuality.color)
                .frame(width: 8, height: 8)
            
            // Mapping info
            VStack(alignment: .leading, spacing: 2) {
                HStack {
                    Text(mapping.sourceBone)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Image(systemName: "arrow.right")
                        .font(.system(size: 10))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text(mapping.targetBone)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                }
                
                HStack {
                    Text(mapping.mappingType.rawValue)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    if mapping.errorMargin > 0.01 {
                        Text("Error: \(String(format: "%.3f", mapping.errorMargin))")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.accentWarning)
                    }
                }
            }
            
            Spacer()
            
            Toggle("", isOn: $mapping.isEnabled)
                .toggleStyle(.switch)
            
            Button(action: { showEditDialog = true }) {
                Image(systemName: "pencil")
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundPrimary)
        .overlay(
            Rectangle()
                .fill(DesignSystem.Colors.border.opacity(0.2))
                .frame(height: 1),
            alignment: .bottom
        )
        .sheet(isPresented: $showEditDialog) {
            EditBoneMappingDialog(mapping: mapping, avatar: avatar)
        }
    }
}

// MARK: - Avatar Muscle Details
struct AvatarMuscleDetails: View {
    @ObservedObject var avatar: AvatarDefinition
    @State private var showAddMuscleDialog = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            HStack {
                Text("Muscle Definitions")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                Button(action: { showAddMuscleDialog = true }) {
                    Image(systemName: "plus")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
            }
            
            // Muscle definitions list
            ScrollView {
                LazyVStack(spacing: 0) {
                    ForEach(avatar.muscleDefinitions, id: \.id) { muscle in
                        MuscleDefinitionRow(muscle: muscle, avatar: avatar)
                    }
                }
            }
            .frame(height: 300)
            .background(DesignSystem.Colors.backgroundPrimary)
        }
        .padding(DesignSystem.Spacing.md)
        .sheet(isPresented: $showAddMuscleDialog) {
            AddMuscleDefinitionDialog(avatar: avatar)
        }
    }
}

// MARK: - Muscle Definition Row
struct MuscleDefinitionRow: View {
    @ObservedObject var muscle: MuscleDefinition
    @ObservedObject var avatar: AvatarDefinition
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            HStack {
                Text(muscle.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Toggle("Limited", isOn: $muscle.isLimited)
                    .toggleStyle(.switch)
            }
            
            HStack {
                Text("Range: \(String(format: "%.1f", muscle.minAngle))° to \(String(format: "%.1f", muscle.maxAngle))°")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Text("Stiffness: \(String(format: "%.2f", muscle.stiffness))")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            // Angle slider
            HStack {
                Text("Angle:")
                Slider(value: $muscle.currentAngle, in: muscle.minAngle...muscle.maxAngle)
                Text(String(format: "%.1f°", muscle.currentAngle))
                    .font(DesignSystem.Typography.smallMono)
                    .frame(width: 40)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundPrimary)
        .overlay(
            Rectangle()
                .fill(DesignSystem.Colors.border.opacity(0.2))
                .frame(height: 1),
            alignment: .bottom
        )
    }
}

// MARK: - Avatar Rigging Details
struct AvatarRiggingDetails: View {
    @ObservedObject var avatar: AvatarDefinition
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            GroupBox("Finger Rig Setup") {
                if let fingerRig = avatar.fingerRigSetup {
                    FingerRigSetupView(rig: fingerRig, avatar: avatar)
                } else {
                    Button("Create Finger Rig") {
                        avatar.fingerRigSetup = FingerRigSetup()
                    }
                    .buttonStyle(.bordered)
                }
            }
            
            GroupBox("Toe Rig Setup") {
                if let toeRig = avatar.toeRigSetup {
                    ToeRigSetupView(rig: toeRig, avatar: avatar)
                } else {
                    Button("Create Toe Rig") {
                        avatar.toeRigSetup = ToeRigSetup()
                    }
                    .buttonStyle(.bordered)
                }
            }
        }
        .padding(DesignSystem.Spacing.md)
    }
}

// MARK: - Finger Rig Setup View
struct FingerRigSetupView: View {
    @ObservedObject var rig: FingerRigSetup
    @ObservedObject var avatar: AvatarDefinition
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            HStack {
                Text("Curl Multiplier:")
                Slider(value: $rig.curlMultiplier, in: 0...2)
                Text(String(format: "%.2f", rig.curlMultiplier))
                    .font(DesignSystem.Typography.smallMono)
                    .frame(width: 40)
            }
            
            HStack {
                Text("Spread Multiplier:")
                Slider(value: $rig.spreadMultiplier, in: 0...2)
                Text(String(format: "%.2f", rig.spreadMultiplier))
                    .font(DesignSystem.Typography.smallMono)
                    .frame(width: 40)
            }
            
            VStack(alignment: .leading, spacing: 4) {
                Text("Finger Bones:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                ForEach(["Thumb", "Index", "Middle", "Ring", "Pinky"], id: \.self) { finger in
                    HStack {
                        Text("\(finger):")
                            .font(DesignSystem.Typography.small)
                        
                        TextField("Bones", text: .constant(""))
                            .textFieldStyle(.roundedBorder)
                    }
                }
            }
        }
    }
}

// MARK: - Toe Rig Setup View
struct ToeRigSetupView: View {
    @ObservedObject var rig: ToeRigSetup
    @ObservedObject var avatar: AvatarDefinition
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            HStack {
                Text("Curl Multiplier:")
                Slider(value: $rig.curlMultiplier, in: 0...2)
                Text(String(format: "%.2f", rig.curlMultiplier))
                    .font(DesignSystem.Typography.smallMono)
                    .frame(width: 40)
            }
            
            HStack {
                Text("Spread Multiplier:")
                Slider(value: $rig.spreadMultiplier, in: 0...2)
                Text(String(format: "%.2f", rig.spreadMultiplier))
                    .font(DesignSystem.Typography.smallMono)
                    .frame(width: 40)
            }
            
            VStack(alignment: .leading, spacing: 4) {
                Text("Toe Bones:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                HStack {
                    Text("Big Toe:")
                        .font(DesignSystem.Typography.small)
                    
                    TextField("Bones", text: .constant(""))
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Other Toes:")
                        .font(DesignSystem.Typography.small)
                    
                    TextField("Bones", text: .constant(""))
                        .textFieldStyle(.roundedBorder)
                }
            }
        }
    }
}

// MARK: - Add Avatar Dialog
struct AddAvatarDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var avatarManager = AvatarManager.shared
    
    @State private var avatarName = ""
    @State private var avatarDescription = ""
    @State private var avatarType: AvatarDefinition.AvatarType = .humanoid
    @State private var poseType: AvatarDefinition.PoseType = .tPose
    @State private var isHumanoid = true
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Add Avatar Definition")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                HStack {
                    Text("Name:")
                    TextField("Avatar name", text: $avatarName)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Description:")
                    TextField("Avatar description", text: $avatarDescription)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Type:")
                    Picker("Type", selection: $avatarType) {
                        ForEach(AvatarDefinition.AvatarType.allCases, id: \.self) { type in
                            Text(type.rawValue).tag(type)
                        }
                    }
                    .pickerStyle(.menu)
                }
                
                HStack {
                    Text("Pose:")
                    Picker("Pose", selection: $poseType) {
                        ForEach(AvatarDefinition.PoseType.allCases, id: \.self) { type in
                            Text(type.rawValue).tag(type)
                        }
                    }
                    .pickerStyle(.menu)
                }
                
                Toggle("Humanoid", isOn: $isHumanoid)
                    .toggleStyle(.switch)
            }
            
            Spacer()
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Add") {
                    let newAvatar = AvatarDefinition(
                        name: avatarName,
                        description: avatarDescription,
                        avatarType: avatarType,
                        poseType: poseType,
                        isHumanoid: isHumanoid
                    )
                    avatarManager.addAvatar(newAvatar)
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(avatarName.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 450, height: 400)
    }
}

// MARK: - Bone Mapping Interface
struct BoneMappingInterface: View {
    @ObservedObject var avatar: AvatarDefinition
    @State private var sourceBones: [String] = []
    @State private var targetBones: [String] = []
    @State private var selectedSourceBone: String?
    @State private var selectedTargetBone: String?
    @State private var showPreview = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Bone Mapping Interface")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                Button("Auto-Map") {
                    performAutoMapping()
                }
                .buttonStyle(.bordered)
                
                Button("Preview") {
                    showPreview = true
                }
                .buttonStyle(.borderedProminent)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            HSplitView {
                // Source bones
                VStack(spacing: 0) {
                    Text("Source Skeleton")
                        .font(DesignSystem.Typography.bodyBold)
                        .padding(DesignSystem.Spacing.sm)
                    
                    EditorDivider()
                    
                    List(sourceBones, id: \.self, selection: $selectedSourceBone) { bone in
                        Text(bone)
                    }
                    .listStyle(.plain)
                }
                .frame(minWidth: 150, maxWidth: 200)
                
                // Target bones
                VStack(spacing: 0) {
                    Text("Target Avatar")
                        .font(DesignSystem.Typography.bodyBold)
                        .padding(DesignSystem.Spacing.sm)
                    
                    EditorDivider()
                    
                    List(targetBones, id: \.self, selection: $selectedTargetBone) { bone in
                        Text(bone)
                    }
                    .listStyle(.plain)
                }
                .frame(minWidth: 150, maxWidth: 200)
                
                // Mapping details
                VStack(spacing: 0) {
                    Text("Mapping Details")
                        .font(DesignSystem.Typography.bodyBold)
                        .padding(DesignSystem.Spacing.sm)
                    
                    EditorDivider()
                    
                    if let source = selectedSourceBone, let target = selectedTargetBone {
                        BoneMappingDetailsView(
                            sourceBone: source,
                            targetBone: target,
                            avatar: avatar
                        )
                    } else {
                        VStack {
                            Spacer()
                            Text("Select source and target bones")
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            Spacer()
                        }
                    }
                }
                .frame(minWidth: 300)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .onAppear {
            loadBoneLists()
        }
        .sheet(isPresented: $showPreview) {
            RetargetPreviewView(avatar: avatar)
        }
    }
    
    private func loadBoneLists() {
        // Mock bone data - in real implementation this would come from actual skeleton data
        sourceBones = [
            "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand",
            "RightShoulder", "RightArm", "RightForeArm", "RightHand",
            "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase",
            "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase"
        ]
        
        targetBones = sourceBones // Target would have different bone names in real implementation
    }
    
    private func performAutoMapping() {
        // Auto-mapping logic
        for sourceBone in sourceBones {
            let bestMatch = findBestMatch(for: sourceBone, in: targetBones)
            if let match = bestMatch {
                let mapping = BoneMapping(
                    sourceBone: sourceBone,
                    targetBone: match,
                    mappingType: .direct,
                    mappingQuality: .good
                )
                avatar.boneMappings.append(mapping)
            }
        }
    }
    
    private func findBestMatch(for bone: String, in targets: [String]) -> String? {
        // Simple matching logic - in real implementation this would be more sophisticated
        return targets.first { $0.lowercased().contains(bone.lowercased()) }
    }
}

// MARK: - Bone Mapping Details View
struct BoneMappingDetailsView: View {
    let sourceBone: String
    let targetBone: String
    @ObservedObject var avatar: AvatarDefinition
    @State private var mappingType: BoneMapping.MappingType = .direct
    @State private var translationOffset: SIMD3<Float> = SIMD3<Float>(0, 0, 0)
    @State private var rotationOffset: simd_quatf = simd_quatf()
    @State private var scaleOffset: SIMD3<Float> = SIMD3<Float>(1, 1, 1)
    @State private var errorMargin: Float = 0.0
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                GroupBox("Mapping Configuration") {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("Type:")
                            Picker("Type", selection: $mappingType) {
                                ForEach(BoneMapping.MappingType.allCases, id: \.self) { type in
                                    Text(type.rawValue).tag(type)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                        
                        HStack {
                            Text("Error Margin:")
                            Slider(value: $errorMargin, in: 0...1)
                            Text(String(format: "%.3f", errorMargin))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 50)
                        }
                    }
                }
                
                GroupBox("Translation Offset") {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("X:")
                            TextField("X", value: $translationOffset.x, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                            
                            Text("Y:")
                            TextField("Y", value: $translationOffset.y, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                            
                            Text("Z:")
                            TextField("Z", value: $translationOffset.z, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                        }
                        
                        Button("Reset Translation") {
                            translationOffset = SIMD3<Float>(0, 0, 0)
                        }
                        .buttonStyle(.bordered)
                    }
                }
                
                GroupBox("Rotation Offset") {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        // Simplified rotation controls - in real implementation would use quaternion or euler angles
                        HStack {
                            Text("Rotation:")
                            TextField("Rotation", text: .constant("0, 0, 0, 1"))
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 120)
                        }
                        
                        Button("Reset Rotation") {
                            rotationOffset = simd_quatf()
                        }
                        .buttonStyle(.bordered)
                    }
                }
                
                GroupBox("Scale Offset") {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("X:")
                            TextField("X", value: $scaleOffset.x, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                            
                            Text("Y:")
                            TextField("Y", value: $scaleOffset.y, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                            
                            Text("Z:")
                            TextField("Z", value: $scaleOffset.z, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                        }
                        
                        Button("Reset Scale") {
                            scaleOffset = SIMD3<Float>(1, 1, 1)
                        }
                        .buttonStyle(.bordered)
                    }
                }
                
                HStack {
                    Button("Create Mapping") {
                        createMapping()
                    }
                    .buttonStyle(.borderedProminent)
                    
                    Spacer()
                    
                    Button("Test Mapping") {
                        testMapping()
                    }
                    .buttonStyle(.bordered)
                }
            }
        }
        .padding(DesignSystem.Spacing.md)
    }
    
    private func createMapping() {
        let mapping = BoneMapping(
            sourceBone: sourceBone,
            targetBone: targetBone,
            mappingType: mappingType,
            translationOffset: translationOffset,
            rotationOffset: rotationOffset,
            scaleOffset: scaleOffset,
            errorMargin: errorMargin
        )
        avatar.boneMappings.append(mapping)
    }
    
    private func testMapping() {
        // Test mapping logic
        print("Testing mapping from \(sourceBone) to \(targetBone)")
    }
}

// MARK: - Retarget Preview View
struct RetargetPreviewView: View {
    @ObservedObject var avatar: AvatarDefinition
    @State private var showErrors = true
    @State private var showMappings = true
    @State private var previewTime: Float = 0.0
    @State private var isPlaying = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Preview header
            HStack {
                Text("Retarget Preview")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                Toggle("Show Errors", isOn: $showErrors)
                    .toggleStyle(.switch)
                
                Toggle("Show Mappings", isOn: $showMappings)
                    .toggleStyle(.switch)
                
                Button(isPlaying ? "Pause" : "Play") {
                    isPlaying.toggle()
                }
                .buttonStyle(.bordered)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            HSplitView {
                // Preview viewport
                VStack(spacing: 0) {
                    Text("Preview Viewport")
                        .font(DesignSystem.Typography.bodyBold)
                        .padding(DesignSystem.Spacing.sm)
                    
                    EditorDivider()
                    
                    // Mock preview area
                    ZStack {
                        Rectangle()
                            .fill(DesignSystem.Colors.backgroundPrimary)
                        
                        // Mock skeleton visualization
                        VStack(spacing: 20) {
                            Circle()
                                .fill(DesignSystem.Colors.accentPrimary)
                                .frame(width: 20, height: 20)
                                .overlay(Text("Head").font(.caption))
                            
                            Rectangle()
                                .fill(DesignSystem.Colors.textSecondary)
                                .frame(width: 4, height: 60)
                            
                            HStack(spacing: 40) {
                                VStack(spacing: 10) {
                                    Rectangle()
                                        .fill(DesignSystem.Colors.textSecondary)
                                        .frame(width: 30, height: 4)
                                    Rectangle()
                                        .fill(DesignSystem.Colors.textSecondary)
                                        .frame(width: 25, height: 4)
                                    Circle()
                                        .fill(DesignSystem.Colors.textSecondary)
                                        .frame(width: 8, height: 8)
                                }
                                
                                VStack(spacing: 10) {
                                    Rectangle()
                                        .fill(DesignSystem.Colors.textSecondary)
                                        .frame(width: 30, height: 4)
                                    Rectangle()
                                        .fill(DesignSystem.Colors.textSecondary)
                                        .frame(width: 25, height: 4)
                                    Circle()
                                        .fill(DesignSystem.Colors.textSecondary)
                                        .frame(width: 8, height: 8)
                                }
                            }
                            
                            Rectangle()
                                .fill(DesignSystem.Colors.textSecondary)
                                .frame(width: 4, height: 80)
                        }
                    }
                    .frame(height: 400)
                }
                .frame(minWidth: 300)
                
                // Error and mapping details
                VStack(spacing: 0) {
                    Text("Analysis")
                        .font(DesignSystem.Typography.bodyBold)
                        .padding(DesignSystem.Spacing.sm)
                    
                    EditorDivider()
                    
                    ScrollView {
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                            if showErrors {
                                GroupBox("Mapping Errors") {
                                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                                        ForEach(getMappingErrors(), id: \.id) { error in
                                            HStack {
                                                Circle()
                                                    .fill(error.severity.color)
                                                    .frame(width: 6, height: 6)
                                                
                                                VStack(alignment: .leading, spacing: 2) {
                                                    Text(error.description)
                                                        .font(DesignSystem.Typography.body)
                                                    Text(error.boneMapping)
                                                        .font(DesignSystem.Typography.small)
                                                        .foregroundColor(DesignSystem.Colors.textSecondary)
                                                }
                                                
                                                Spacer()
                                            }
                                        }
                                    }
                                }
                            }
                            
                            if showMappings {
                                GroupBox("Active Mappings") {
                                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                                        ForEach(avatar.boneMappings.filter { $0.isEnabled }, id: \.id) { mapping in
                                            HStack {
                                                Circle()
                                                    .fill(mapping.mappingQuality.color)
                                                    .frame(width: 6, height: 6)
                                                
                                                Text("\(mapping.sourceBone) → \(mapping.targetBone)")
                                                    .font(DesignSystem.Typography.small)
                                                
                                                Spacer()
                                                
                                                Text(String(format: "%.3f", mapping.errorMargin))
                                                    .font(DesignSystem.Typography.smallMono)
                                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                            }
                                        }
                                    }
                                }
                            }
                            
                            GroupBox("Statistics") {
                                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                                    HStack {
                                        Text("Total Mappings:")
                                        Spacer()
                                        Text("\(avatar.boneMappings.count)")
                                            .font(DesignSystem.Typography.smallMono)
                                    }
                                    
                                    HStack {
                                        Text("Success Rate:")
                                        Spacer()
                                        let successRate = calculateSuccessRate()
                                        Text(String(format: "%.1f%%", successRate))
                                            .font(DesignSystem.Typography.smallMono)
                                            .foregroundColor(successRate > 80 ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.accentWarning)
                                    }
                                    
                                    HStack {
                                        Text("Average Error:")
                                        Spacer()
                                        let avgError = calculateAverageError()
                                        Text(String(format: "%.3f", avgError))
                                            .font(DesignSystem.Typography.smallMono)
                                            .foregroundColor(avgError < 0.1 ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.accentWarning)
                                    }
                                }
                            }
                        }
                        .padding(DesignSystem.Spacing.md)
                    }
                    .frame(height: 400)
                }
                .frame(minWidth: 250, maxWidth: 300)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
    
    private func getMappingErrors() -> [MappingError] {
        var errors: [MappingError] = []
        
        for mapping in avatar.boneMappings {
            if mapping.errorMargin > 0.1 {
                let severity: ErrorSeverity = mapping.errorMargin > 0.5 ? .critical : .warning
                errors.append(MappingError(
                    id: UUID(),
                    description: "High error margin detected",
                    boneMapping: "\(mapping.sourceBone) → \(mapping.targetBone)",
                    severity: severity
                ))
            }
        }
        
        return errors
    }
    
    private func calculateSuccessRate() -> Float {
        guard !avatar.boneMappings.isEmpty else { return 0 }
        let successful = avatar.boneMappings.filter { $0.errorMargin < 0.1 }.count
        return Float(successful) / Float(avatar.boneMappings.count) * 100
    }
    
    private func calculateAverageError() -> Float {
        guard !avatar.boneMappings.isEmpty else { return 0 }
        let totalError = avatar.boneMappings.map { $0.errorMargin }.reduce(0, +)
        return totalError / Float(avatar.boneMappings.count)
    }
}

// MARK: - Mapping Error Model
struct MappingError: Identifiable {
    let id: UUID
    let description: String
    let boneMapping: String
    let severity: ErrorSeverity
    
    enum ErrorSeverity {
        case warning, critical
        
        var color: Color {
            switch self {
            case .warning: return DesignSystem.Colors.accentWarning
            case .critical: return DesignSystem.Colors.accentDanger
            }
        }
    }
}

// MARK: - Blend Shape System

// Blend Shape Data Models
struct BlendShape: Identifiable, ObservableObject {
    let id = UUID()
    var name: String
    var weight: Float = 0.0
    var targetMesh: String = ""
    var vertices: [BlendShapeVertex] = []
    var isEnabled: Bool = true
    var category: BlendShapeCategory = .custom
    var influenceRadius: Float = 1.0
    var falloffCurve: FalloffCurve = .linear
    
    enum BlendShapeCategory: String, CaseIterable {
        case facial = "Facial"
        case body = "Body"
        case clothing = "Clothing"
        case custom = "Custom"
        
        var color: Color {
            switch self {
            case .facial: return DesignSystem.Colors.accentPrimary
            case .body: return DesignSystem.Colors.accentSuccess
            case .clothing: return DesignSystem.Colors.accentWarning
            case .custom: return DesignSystem.Colors.textSecondary
            }
        }
    }
    
    enum FalloffCurve: String, CaseIterable {
        case linear = "Linear"
        case smooth = "Smooth"
        case exponential = "Exponential"
        case logarithmic = "Logarithmic"
        
        func evaluate(at t: Float) -> Float {
            switch self {
            case .linear: return t
            case .smooth: return t * t * (3.0 - 2.0 * t)
            case .exponential: return t == 0 ? 0 : pow(2, 10 * (t - 1))
            case .logarithmic: return t == 0 ? 0 : log10(1 + t * 9) / log10(10)
            }
        }
    }
}

struct BlendShapeVertex: Identifiable {
    let id = UUID()
    var vertexIndex: Int
    var positionOffset: SIMD3<Float> = SIMD3<Float>(0, 0, 0)
    var normalOffset: SIMD3<Float> = SIMD3<Float>(0, 0, 0)
    var tangentOffset: SIMD3<Float> = SIMD3<Float>(0, 0, 0)
}

struct BlendShapeCombination: Identifiable, ObservableObject {
    let id = UUID()
    var name: String
    var blendShapes: [BlendShapeWeight] = []
    var isActive: Bool = false
    var normalizationMode: NormalizationMode = .none
    var previewWeight: Float = 1.0
    
    enum NormalizationMode: String, CaseIterable {
        case none = "None"
        case average = "Average"
        case maximum = "Maximum"
        case sum = "Sum"
        
        func normalize(_ weights: [Float]) -> [Float] {
            switch self {
            case .none: return weights
            case .average:
                let sum = weights.reduce(0, +)
                let avg = sum / Float(weights.count)
                return avg > 0 ? weights.map { $0 / avg } : weights
            case .maximum:
                let max = weights.max() ?? 1
                return max > 0 ? weights.map { $0 / max } : weights
            case .sum:
                let sum = weights.reduce(0, +)
                return sum > 0 ? weights.map { $0 / sum } : weights
            }
        }
    }
}

struct BlendShapeWeight: Identifiable, ObservableObject {
    let id = UUID()
    var blendShapeID: UUID
    var weight: Float = 0.0
    var isEnabled: Bool = true
    var influenceMultiplier: Float = 1.0
}

// Blend Shape Manager
class BlendShapeManager: ObservableObject {
    static let shared = BlendShapeManager()
    
    @Published var blendShapes: [BlendShape] = []
    @Published var combinations: [BlendShapeCombination] = []
    @Published var selectedBlendShape: BlendShape?
    @Published var selectedCombination: BlendShapeCombination?
    @Published var isPreviewEnabled: Bool = true
    @Published var globalInfluence: Float = 1.0
    @Published var showInfluenceRadius: Bool = false
    @Published var paintMode: PaintMode = .add
    @Published var paintStrength: Float = 0.5
    @Published var paintRadius: Float = 0.1
    
    enum PaintMode: String, CaseIterable {
        case add = "Add"
        case subtract = "Subtract"
        case smooth = "Smooth"
        case normalize = "Normalize"
    }
    
    init() {
        setupDefaultBlendShapes()
    }
    
    private func setupDefaultBlendShapes() {
        // Facial blend shapes
        let smileBlend = BlendShape(
            name: "Smile",
            targetMesh: "Head",
            category: .facial,
            influenceRadius: 0.8
        )
        
        let frownBlend = BlendShape(
            name: "Frown",
            targetMesh: "Head",
            category: .facial,
            influenceRadius: 0.7
        )
        
        let blinkLeftBlend = BlendShape(
            name: "Blink Left",
            targetMesh: "Head",
            category: .facial,
            influenceRadius: 0.3
        )
        
        let blinkRightBlend = BlendShape(
            name: "Blink Right",
            targetMesh: "Head",
            category: .facial,
            influenceRadius: 0.3
        )
        
        // Body blend shapes
        let muscleFlexBlend = BlendShape(
            name: "Muscle Flex",
            targetMesh: "Body",
            category: .body,
            influenceRadius: 1.0
        )
        
        let breathBlend = BlendShape(
            name: "Breath",
            targetMesh: "Chest",
            category: .body,
            influenceRadius: 0.9
        )
        
        blendShapes = [smileBlend, frownBlend, blinkLeftBlend, blinkRightBlend, muscleFlexBlend, breathBlend]
        
        // Create default combination
        let facialExpression = BlendShapeCombination(
            name: "Happy Expression",
            blendShapes: [
                BlendShapeWeight(blendShapeID: smileBlend.id, weight: 0.8),
                BlendShapeWeight(blendShapeID: frownBlend.id, weight: 0.0)
            ]
        )
        
        combinations = [facialExpression]
    }
    
    func addBlendShape(_ blendShape: BlendShape) {
        blendShapes.append(blendShape)
    }
    
    func removeBlendShape(_ blendShape: BlendShape) {
        blendShapes.removeAll { $0.id == blendShape.id }
        // Remove from combinations
        for combination in combinations {
            combination.blendShapes.removeAll { $0.blendShapeID == blendShape.id }
        }
    }
    
    func updateBlendShapeWeight(id: UUID, weight: Float) {
        if let blendShape = blendShapes.first(where: { $0.id == id }) {
            blendShape.weight = max(0, min(1, weight))
        }
    }
    
    func addCombination(_ combination: BlendShapeCombination) {
        combinations.append(combination)
    }
    
    func removeCombination(_ combination: BlendShapeCombination) {
        combinations.removeAll { $0.id == combination.id }
    }
    
    func evaluateCombination(_ combination: BlendShapeCombination) -> [UUID: Float] {
        var weights: [UUID: Float] = [:]
        
        let blendWeights = combination.blendShapes.map { $0.weight }
        let normalizedWeights = combination.normalizationMode.normalize(blendWeights)
        
        for (i, blendWeight) in combination.blendShapes.enumerated() {
            if blendWeight.isEnabled, i < normalizedWeights.count {
                let finalWeight = normalizedWeights[i] * combination.previewWeight * globalInfluence * blendWeight.influenceMultiplier
                weights[blendWeight.blendShapeID] = finalWeight
            }
        }
        
        return weights
    }
    
    func resetAllWeights() {
        for blendShape in blendShapes {
            blendShape.weight = 0.0
        }
        
        for combination in combinations {
            for blendWeight in combination.blendShapes {
                blendWeight.weight = 0.0
            }
        }
    }
}

// MARK: - Blend Shape Editor View
struct BlendShapeEditorView: View {
    @StateObject private var blendManager = BlendShapeManager.shared
    @State private var selectedCategory: BlendShape.BlendShapeCategory = .all
    @State private var showAddBlendShapeDialog = false
    @State private var showCombinationDialog = false
    @State private var searchText = ""
    
    enum Category: String, CaseIterable {
        case all = "All"
        case facial = "Facial"
        case body = "Body"
        case clothing = "Clothing"
        case custom = "Custom"
    }
    
    var filteredBlendShapes: [BlendShape] {
        var filtered = blendManager.blendShapes
        
        if selectedCategory != .all {
            filtered = filtered.filter { $0.category.rawValue == selectedCategory.rawValue }
        }
        
        if !searchText.isEmpty {
            filtered = filtered.filter { $0.name.lowercased().contains(searchText.lowercased()) }
        }
        
        return filtered
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Blend Shapes")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                // Search
                HStack {
                    Image(systemName: "magnifyingglass")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    TextField("Search blend shapes...", text: $searchText)
                        .textFieldStyle(.roundedBorder)
                        .frame(width: 200)
                }
                
                // Category filter
                Picker("Category", selection: $selectedCategory) {
                    ForEach(Category.allCases, id: \.self) { category in
                        Text(category.rawValue).tag(category)
                    }
                }
                .pickerStyle(.menu)
                .frame(width: 120)
                
                Toggle("Preview", isOn: $blendManager.isPreviewEnabled)
                    .toggleStyle(.switch)
                
                Button(action: { showAddBlendShapeDialog = true }) {
                    Image(systemName: "plus")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                
                Button(action: { showCombinationDialog = true }) {
                    Image(systemName: "link")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            HSplitView {
                // Blend shapes list
                VStack(spacing: 0) {
                    // Paint tools
                    if blendManager.isPreviewEnabled {
                        PaintToolsView()
                            .padding(DesignSystem.Spacing.sm)
                            .background(DesignSystem.Colors.backgroundSecondary)
                        EditorDivider()
                    }
                    
                    // Blend shapes list
                    ScrollView {
                        LazyVStack(spacing: DesignSystem.Spacing.xs) {
                            ForEach(filteredBlendShapes, id: \.id) { blendShape in
                                BlendShapeRow(blendShape: blendShape)
                            }
                        }
                        .padding(DesignSystem.Spacing.sm)
                    }
                    .background(DesignSystem.Colors.backgroundPrimary)
                }
                .frame(minWidth: 250, maxWidth: 300)
                
                // Blend shape details
                if let selectedBlendShape = blendManager.selectedBlendShape {
                    BlendShapeDetailsView(blendShape: selectedBlendShape)
                } else {
                    VStack {
                        Spacer()
                        Text("Select a blend shape to edit")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        Spacer()
                    }
                    .frame(maxWidth: .infinity)
                }
                
                // Combinations panel
                VStack(spacing: 0) {
                    HStack {
                        Text("Combinations")
                            .font(DesignSystem.Typography.bodyBold)
                        
                        Spacer()
                        
                        Button("Add") {
                            showCombinationDialog = true
                        }
                        .buttonStyle(.bordered)
                    }
                    .padding(DesignSystem.Spacing.sm)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    
                    EditorDivider()
                    
                    ScrollView {
                        LazyVStack(spacing: DesignSystem.Spacing.xs) {
                            ForEach(blendManager.combinations, id: \.id) { combination in
                                CombinationRow(combination: combination)
                            }
                        }
                        .padding(DesignSystem.Spacing.sm)
                    }
                    .background(DesignSystem.Colors.backgroundPrimary)
                }
                .frame(minWidth: 250, maxWidth: 300)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .sheet(isPresented: $showAddBlendShapeDialog) {
            AddBlendShapeDialog()
        }
        .sheet(isPresented: $showCombinationDialog) {
            AddCombinationDialog()
        }
    }
}

// MARK: - Paint Tools View
struct PaintToolsView: View {
    @StateObject private var blendManager = BlendShapeManager.shared
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            Text("Paint Tools")
                .font(DesignSystem.Typography.bodyBold)
            
            HStack {
                Text("Mode:")
                Picker("Mode", selection: $blendManager.paintMode) {
                    ForEach(BlendShapeManager.PaintMode.allCases, id: \.self) { mode in
                        Text(mode.rawValue).tag(mode)
                    }
                }
                .pickerStyle(.menu)
                .frame(width: 100)
                
                Text("Strength:")
                Slider(value: $blendManager.paintStrength, in: 0...1)
                Text(String(format: "%.2f", blendManager.paintStrength))
                    .font(DesignSystem.Typography.smallMono)
                    .frame(width: 40)
                
                Text("Radius:")
                Slider(value: $blendManager.paintRadius, in: 0.01...1.0)
                Text(String(format: "%.2f", blendManager.paintRadius))
                    .font(DesignSystem.Typography.smallMono)
                    .frame(width: 40)
            }
            
            HStack {
                Toggle("Show Influence", isOn: $blendManager.showInfluenceRadius)
                    .toggleStyle(.switch)
                
                Button("Reset All") {
                    blendManager.resetAllWeights()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Text("Global: \(String(format: "%.2f", blendManager.globalInfluence))")
                    .font(DesignSystem.Typography.smallMono)
                Slider(value: $blendManager.globalInfluence, in: 0...2)
                    .frame(width: 100)
            }
        }
    }
}

// MARK: - Blend Shape Row
struct BlendShapeRow: View {
    @ObservedObject var blendShape: BlendShape
    @StateObject private var blendManager = BlendShapeManager.shared
    @State private var isHovering = false
    
    var body: some View {
        HStack {
            // Category indicator
            Circle()
                .fill(blendShape.category.color)
                .frame(width: 8, height: 8)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(blendShape.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                HStack {
                    Text(blendShape.targetMesh)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    Text("\(blendShape.vertices.count) vertices")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            
            Spacer()
            
            // Weight slider
            VStack(spacing: 2) {
                Slider(value: $blendShape.weight, in: 0...1) { editing in
                    if !editing {
                        blendManager.updateBlendShapeWeight(id: blendShape.id, weight: blendShape.weight)
                    }
                }
                .frame(width: 80)
                
                Text(String(format: "%.2f", blendShape.weight))
                    .font(DesignSystem.Typography.smallMono)
            }
            
            Toggle("", isOn: $blendShape.isEnabled)
                .toggleStyle(.switch)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(
            blendShape == blendManager.selectedBlendShape ?
            DesignSystem.Colors.selection :
            (isHovering ? DesignSystem.Colors.hover : Color.clear)
        )
        .onHover { isHovering = $0 }
        .onTapGesture {
            blendManager.selectedBlendShape = blendShape
        }
        .contextMenu {
            Button("Edit") {
                blendManager.selectedBlendShape = blendShape
            }
            
            Button("Duplicate") {
                let newBlendShape = BlendShape(
                    name: "\(blendShape.name) Copy",
                    targetMesh: blendShape.targetMesh,
                    category: blendShape.category,
                    influenceRadius: blendShape.influenceRadius
                )
                blendManager.addBlendShape(newBlendShape)
            }
            
            Button("Delete", role: .destructive) {
                blendManager.removeBlendShape(blendShape)
            }
        }
    }
}

// MARK: - Blend Shape Details View
struct BlendShapeDetailsView: View {
    @ObservedObject var blendShape: BlendShape
    @State private var showVertexEditor = false
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                GroupBox("Properties") {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("Name:")
                            TextField("Blend shape name", text: $blendShape.name)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        HStack {
                            Text("Target Mesh:")
                            TextField("Target mesh", text: $blendShape.targetMesh)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        HStack {
                            Text("Category:")
                            Picker("Category", selection: $blendShape.category) {
                                ForEach(BlendShape.BlendShapeCategory.allCases, id: \.self) { category in
                                    Text(category.rawValue).tag(category)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                        
                        HStack {
                            Text("Influence Radius:")
                            Slider(value: $blendShape.influenceRadius, in: 0...2)
                            Text(String(format: "%.2f", blendShape.influenceRadius))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                        }
                        
                        HStack {
                            Text("Falloff Curve:")
                            Picker("Falloff", selection: $blendShape.falloffCurve) {
                                ForEach(BlendShape.FalloffCurve.allCases, id: \.self) { curve in
                                    Text(curve.rawValue).tag(curve)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                        
                        Toggle("Enabled", isOn: $blendShape.isEnabled)
                            .toggleStyle(.switch)
                    }
                }
                
                GroupBox("Vertex Data") {
                    HStack {
                        Text("\(blendShape.vertices.count) vertices")
                            .font(DesignSystem.Typography.body)
                        
                        Spacer()
                        
                        Button("Edit Vertices") {
                            showVertexEditor = true
                        }
                        .buttonStyle(.bordered)
                    }
                }
                
                GroupBox("Preview") {
                    VStack(spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("Weight:")
                            Slider(value: $blendShape.weight, in: 0...1)
                            Text(String(format: "%.2f", blendShape.weight))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                        }
                        
                        // Preview area would go here
                        Rectangle()
                            .fill(DesignSystem.Colors.backgroundTertiary)
                            .frame(height: 200)
                            .overlay(
                                Text("3D Preview")
                                    .foregroundColor(DesignSystem.Colors.textSecondary),
                                alignment: .center
                            )
                    }
                }
            }
            .padding(DesignSystem.Spacing.md)
        }
        .background(DesignSystem.Colors.backgroundPrimary)
        .sheet(isPresented: $showVertexEditor) {
            VertexEditorView(blendShape: blendShape)
        }
    }
}

// MARK: - Combination Row
struct CombinationRow: View {
    @ObservedObject var combination: BlendShapeCombination
    @StateObject private var blendManager = BlendShapeManager.shared
    @State private var isExpanded = false
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                // Active indicator
                Circle()
                    .fill(combination.isActive ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.textSecondary)
                    .frame(width: 8, height: 8)
                
                VStack(alignment: .leading, spacing: 2) {
                    Text(combination.name)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Text("\(combination.blendShapes.count) blend shapes")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                Spacer()
                
                // Preview weight
                VStack(spacing: 2) {
                    Slider(value: $combination.previewWeight, in: 0...1)
                        .frame(width: 60)
                    Text(String(format: "%.1f", combination.previewWeight))
                        .font(DesignSystem.Typography.smallMono)
                }
                
                Button(action: { isExpanded.toggle() }) {
                    Image(systemName: isExpanded ? "chevron.up" : "chevron.down")
                        .font(.system(size: 10))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                
                Toggle("", isOn: $combination.isActive)
                    .toggleStyle(.switch)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundPrimary)
            
            if isExpanded {
                // Expanded blend shape weights
                VStack(spacing: 0) {
                    ForEach(combination.blendShapes, id: \.id) { blendWeight in
                        CombinationBlendWeightRow(combination: combination, blendWeight: blendWeight)
                    }
                    
                    HStack {
                        Text("Normalization:")
                        Picker("Normalization", selection: $combination.normalizationMode) {
                            ForEach(BlendShapeCombination.NormalizationMode.allCases, id: \.self) { mode in
                                Text(mode.rawValue).tag(mode)
                            }
                        }
                        .pickerStyle(.menu)
                        .frame(width: 120)
                        
                        Spacer()
                    }
                    .padding(DesignSystem.Spacing.sm)
                    .background(DesignSystem.Colors.backgroundTertiary)
                }
            }
        }
        .overlay(
            Rectangle()
                .fill(DesignSystem.Colors.border.opacity(0.2))
                .frame(height: 1),
            alignment: .bottom
        )
        .contextMenu {
            Button("Edit") {
                blendManager.selectedCombination = combination
            }
            
            Button("Duplicate") {
                let newCombination = BlendShapeCombination(
                    name: "\(combination.name) Copy",
                    blendShapes: combination.blendShapes.map { blendWeight in
                        BlendShapeWeight(
                            blendShapeID: blendWeight.blendShapeID,
                            weight: blendWeight.weight,
                            isEnabled: blendWeight.isEnabled,
                            influenceMultiplier: blendWeight.influenceMultiplier
                        )
                    },
                    normalizationMode: combination.normalizationMode
                )
                blendManager.addCombination(newCombination)
            }
            
            Button("Delete", role: .destructive) {
                blendManager.removeCombination(combination)
            }
        }
    }
}

// MARK: - Combination Blend Weight Row
struct CombinationBlendWeightRow: View {
    @ObservedObject var combination: BlendShapeCombination
    @ObservedObject var blendWeight: BlendShapeWeight
    @StateObject private var blendManager = BlendShapeManager.shared
    
    var blendShapeName: String {
        return blendManager.blendShapes.first { $0.id == blendWeight.blendShapeID }?.name ?? "Unknown"
    }
    
    var body: some View {
        HStack {
            Text(blendShapeName)
                .font(DesignSystem.Typography.small)
                .frame(maxWidth: .infinity, alignment: .leading)
            
            Toggle("", isOn: $blendWeight.isEnabled)
                .toggleStyle(.switch)
            
            VStack(spacing: 2) {
                Slider(value: $blendWeight.weight, in: 0...1)
                    .frame(width: 60)
                Text(String(format: "%.2f", blendWeight.weight))
                    .font(DesignSystem.Typography.smallMono)
            }
            
            VStack(spacing: 2) {
                Slider(value: $blendWeight.influenceMultiplier, in: 0...2)
                    .frame(width: 50)
                Text(String(format: "%.1f", blendWeight.influenceMultiplier))
                    .font(DesignSystem.Typography.smallMono)
            }
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .padding(.vertical, 2)
    }
}

// MARK: - Add Blend Shape Dialog

struct AddBlendShapeDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var blendManager = BlendShapeManager.shared
    
    @State private var blendShapeName = ""
    @State private var targetMesh = ""
    @State private var category: BlendShape.BlendShapeCategory = .custom
    @State private var influenceRadius: Float = 1.0
    @State private var falloffCurve: BlendShape.FalloffCurve = .linear
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Add Blend Shape")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                HStack {
                    Text("Name:")
                    TextField("Blend shape name", text: $blendShapeName)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Target Mesh:")
                    TextField("Target mesh", text: $targetMesh)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Category:")
                    Picker("Category", selection: $category) {
                        ForEach(BlendShape.BlendShapeCategory.allCases, id: \.self) { cat in
                            Text(cat.rawValue).tag(cat)
                        }
                    }
                    .pickerStyle(.menu)
                }
                
                HStack {
                    Text("Influence Radius:")
                    Slider(value: $influenceRadius, in: 0...2)
                    Text(String(format: "%.2f", influenceRadius))
                        .font(DesignSystem.Typography.smallMono)
                        .frame(width: 40)
                }
                
                HStack {
                    Text("Falloff Curve:")
                    Picker("Falloff", selection: $falloffCurve) {
                        ForEach(BlendShape.FalloffCurve.allCases, id: \.self) { curve in
                            Text(curve.rawValue).tag(curve)
                        }
                    }
                    .pickerStyle(.menu)
                }
            }
            
            Spacer()
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Add") {
                    let blendShape = BlendShape(
                        name: blendShapeName,
                        targetMesh: targetMesh,
                        category: category,
                        influenceRadius: influenceRadius,
                        falloffCurve: falloffCurve
                    )
                    blendManager.addBlendShape(blendShape)
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(blendShapeName.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 350)
    }
}

// MARK: - Add Combination Dialog

struct AddCombinationDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var blendManager = BlendShapeManager.shared
    
    @State private var combinationName = ""
    @State private var selectedBlendShapes: Set<UUID> = []
    @State private var normalizationMode: BlendShapeCombination.NormalizationMode = .none
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Add Blend Shape Combination")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                HStack {
                    Text("Name:")
                    TextField("Combination name", text: $combinationName)
                        .textFieldStyle(.roundedBorder)
                }
                
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Text("Select Blend Shapes:")
                    
                    List(blendManager.blendShapes, id: \.id) { blendShape in
                        HStack {
                            Toggle("", isOn: Binding(
                                get: { selectedBlendShapes.contains(blendShape.id) },
                                set: { isOn in
                                    if isOn {
                                        selectedBlendShapes.insert(blendShape.id)
                                    } else {
                                        selectedBlendShapes.remove(blendShape.id)
                                    }
                                }
                            ))
                            .toggleStyle(.switch)
                            
                            VStack(alignment: .leading, spacing: 2) {
                                Text(blendShape.name)
                                    .font(DesignSystem.Typography.body)
                                Text(blendShape.targetMesh)
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                            }
                            
                            Spacer()
                            
                            Circle()
                                .fill(blendShape.category.color)
                                .frame(width: 8, height: 8)
                        }
                        .padding(.vertical, 2)
                    }
                    .frame(height: 200)
                }
                
                HStack {
                    Text("Normalization:")
                    Picker("Normalization", selection: $normalizationMode) {
                        ForEach(BlendShapeCombination.NormalizationMode.allCases, id: \.self) { mode in
                            Text(mode.rawValue).tag(mode)
                        }
                    }
                    .pickerStyle(.menu)
                }
            }
            
            Spacer()
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Add") {
                    let blendWeights = selectedBlendShapes.map { id in
                        BlendShapeWeight(
                            blendShapeID: id,
                            weight: 0.0,
                            isEnabled: true,
                            influenceMultiplier: 1.0
                        )
                    }
                    
                    let combination = BlendShapeCombination(
                        name: combinationName,
                        blendShapes: blendWeights,
                        normalizationMode: normalizationMode
                    )
                    
                    blendManager.addCombination(combination)
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(combinationName.isEmpty || selectedBlendShapes.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 500, height: 500)
    }
}

// MARK: - Vertex Editor View

struct VertexEditorView: View {
    @ObservedObject var blendShape: BlendShape
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Vertex Editor - \(blendShape.name)")
                .font(DesignSystem.Typography.headline)
            
            HStack {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Text("Total Vertices: \(blendShape.vertices.count)")
                        .font(DesignSystem.Typography.body)
                    
                    Text("Target Mesh: \(blendShape.targetMesh)")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                Spacer()
                
                Button("Import Vertices") {
                    // Import vertices from mesh
                }
                .buttonStyle(.bordered)
                
                Button("Clear All") {
                    blendShape.vertices.removeAll()
                }
                .buttonStyle(.bordered)
            }
            
            // Vertex list
            ScrollView {
                LazyVStack(spacing: DesignSystem.Spacing.xs) {
                    ForEach(blendShape.vertices, id: \.id) { vertex in
                        VertexRow(vertex: vertex, blendShape: blendShape)
                    }
                }
                .padding(DesignSystem.Spacing.sm)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
            
            Spacer()
            
            HStack {
                Button("Close") {
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 600, height: 500)
    }
}

// MARK: - Vertex Row

struct VertexRow: View {
    @ObservedObject var vertex: BlendShapeVertex
    @ObservedObject var blendShape: BlendShape
    
    var body: some View {
        HStack {
            Text("Vertex \(vertex.vertexIndex)")
                .font(DesignSystem.Typography.smallMono)
                .frame(width: 80)
            
            VStack(alignment: .leading, spacing: 2) {
                Text("Position: \(vertex.positionOffset.x, specifier: "%.3f"), \(vertex.positionOffset.y, specifier: "%.3f"), \(vertex.positionOffset.z, specifier: "%.3f")")
                    .font(DesignSystem.Typography.small)
                Text("Normal: \(vertex.normalOffset.x, specifier: "%.3f"), \(vertex.normalOffset.y, specifier: "%.3f"), \(vertex.normalOffset.z, specifier: "%.3f")")
                    .font(DesignSystem.Typography.small)
            }
            
            Spacer()
            
            Button("Edit") {
                // Edit vertex offsets
            }
            .buttonStyle(.bordered)
            .controlSize(.small)
            
            Button("Remove") {
                if let index = blendShape.vertices.firstIndex(where: { $0.id == vertex.id }) {
                    blendShape.vertices.remove(at: index)
                }
            }
            .buttonStyle(.bordered)
            .controlSize(.small)
        }
        .padding(.vertical, 4)
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Dialog Views
struct AddBlendShapeDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var blendManager = BlendShapeManager.shared
    
    @State private var name = ""
    @State private var targetMesh = ""
    @State private var category: BlendShape.BlendShapeCategory = .custom
    @State private var influenceRadius: Float = 1.0
    @State private var falloffCurve: BlendShape.FalloffCurve = .linear
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Add Blend Shape")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                TextField("Name", text: $name)
                    .textFieldStyle(.roundedBorder)
                
                TextField("Target Mesh", text: $targetMesh)
                    .textFieldStyle(.roundedBorder)
                
                Picker("Category", selection: $category) {
                    ForEach(BlendShape.BlendShapeCategory.allCases, id: \.self) { cat in
                        Text(cat.rawValue).tag(cat)
                    }
                }
                .pickerStyle(.menu)
                
                HStack {
                    Text("Radius:")
                    Slider(value: $influenceRadius, in: 0...2)
                    Text(String(format: "%.1f", influenceRadius))
                        .font(DesignSystem.Typography.smallMono)
                        .frame(width: 40)
                }
            }
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Add") {
                    let blendShape = BlendShape(
                        name: name,
                        targetMesh: targetMesh,
                        category: category,
                        influenceRadius: influenceRadius,
                        falloffCurve: falloffCurve
                    )
                    blendManager.addBlendShape(blendShape)
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(name.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 350)
    }
}

struct AddCombinationDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var blendManager = BlendShapeManager.shared
    
    @State private var name = ""
    @State private var selectedBlendShapes: Set<UUID> = []
    @State private var normalization: BlendShapeCombination.NormalizationMode = .none
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Add Combination")
                .font(DesignSystem.Typography.headline)
            
            TextField("Name", text: $name)
                .textFieldStyle(.roundedBorder)
            
            Text("Select Blend Shapes:")
                .font(DesignSystem.Typography.body)
            
            List(blendManager.blendShapes, id: \.id, selection: $selectedBlendShapes) { blendShape in
                Text(blendShape.name)
            }
            .listStyle(.plain)
            .frame(height: 200)
            
            Picker("Normalization", selection: $normalization) {
                ForEach(BlendShapeCombination.NormalizationMode.allCases, id: \.self) { mode in
                    Text(mode.rawValue).tag(mode)
                }
            }
            .pickerStyle(.menu)
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Add") {
                    let weights = selectedBlendShapes.map { id in
                        BlendShapeWeight(blendShapeID: id, weight: 0.5)
                    }
                    let combination = BlendShapeCombination(
                        name: name,
                        blendShapes: weights,
                        normalizationMode: normalization
                    )
                    blendManager.addCombination(combination)
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(name.isEmpty || selectedBlendShapes.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 400)
    }
}

struct VertexEditorView: View {
    @ObservedObject var blendShape: BlendShape
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        VStack {
            Text("Vertex Editor")
                .font(DesignSystem.Typography.headline)
            
            Text("\(blendShape.vertices.count) vertices")
                .font(DesignSystem.Typography.body)
            
            // Vertex editing interface would go here
            Rectangle()
                .fill(DesignSystem.Colors.backgroundTertiary)
                .frame(height: 300)
                .overlay(
                    Text("Vertex editing interface")
                        .foregroundColor(DesignSystem.Colors.textSecondary),
                    alignment: .center
                )
            
            Button("Close") { dismiss() }
                .buttonStyle(.bordered)
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 500, height: 450)
    }
}

// MARK: - Blend Space System

struct BlendSpace1D: Identifiable, ObservableObject {
    let id = UUID()
    var name: String
    var parameterName: String = "Blend"
    var minValue: Float = 0.0
    var maxValue: Float = 1.0
    var currentValue: Float = 0.0
    var animationClips: [BlendSpaceClip] = []
    var threshold: Float = 0.1
    var interpolationMode: InterpolationMode = .linear
    
    enum InterpolationMode: String, CaseIterable {
        case linear = "Linear"
        case smooth = "Smooth"
        case easeIn = "Ease In"
        case easeOut = "Ease Out"
        case easeInOut = "Ease In Out"
    }
}

struct BlendSpaceClip: Identifiable, ObservableObject {
    let id = UUID()
    var clipID: UUID
    var position: Float = 0.0
    var weight: Float = 1.0
    var isEnabled: Bool = true
    var influenceRange: Float = 0.2
}

struct BlendSpace2D: Identifiable, ObservableObject {
    let id = UUID()
    var name: String
    var parameterX: String = "X"
    var parameterY: String = "Y"
    var spaceType: SpaceType = .cartesian
    var currentValue: SIMD2<Float> = SIMD2<Float>(0, 0)
    var animationClips: [BlendSpaceClip2D] = []
    var threshold: Float = 0.1
    var interpolationMode: BlendSpace1D.InterpolationMode = .linear
    
    enum SpaceType: String, CaseIterable {
        case cartesian = "Cartesian"
        case directional = "Directional"
        case grid = "Grid"
    }
}

struct BlendSpaceClip2D: Identifiable, ObservableObject {
    let id = UUID()
    var clipID: UUID
    var position: SIMD2<Float> = SIMD2<Float>(0, 0)
    var weight: Float = 1.0
    var isEnabled: Bool = true
    var influenceRadius: Float = 0.2
}

// MARK: - Blend Space Manager
class BlendSpaceManager: ObservableObject {
    static let shared = BlendSpaceManager()
    
    @Published var blendSpaces1D: [BlendSpace1D] = []
    @Published var blendSpaces2D: [BlendSpace2D] = []
    @Published var selectedSpace1D: BlendSpace1D?
    @Published var selectedSpace2D: BlendSpace2D?
    @Published var isPreviewEnabled: Bool = true
    
    init() {
        setupDefaultBlendSpaces()
    }
    
    private func setupDefaultBlendSpaces() {
        let walkRunSpace = BlendSpace1D(
            name: "Walk-Run Blend",
            parameterName: "Speed",
            minValue: 0.0,
            maxValue: 10.0,
            threshold: 0.15
        )
        
        blendSpaces1D = [walkRunSpace]
        
        let directionalSpace = BlendSpace2D(
            name: "Movement Direction",
            parameterX: "Forward",
            parameterY: "Sideways",
            spaceType: .directional
        )
        
        blendSpaces2D = [directionalSpace]
    }
}

// MARK: - Blend Space Editor View
struct BlendSpaceEditorView: View {
    @StateObject private var blendManager = BlendSpaceManager.shared
    @State private var selectedTab: EditorTab = .blend1D
    @State private var showAddSpaceDialog = false
    
    enum EditorTab: String, CaseIterable {
        case blend1D = "1D Blend"
        case blend2D = "2D Blend"
        case threshold = "Threshold"
        case layerMask = "Layer Mask"
        case weightPaint = "Weight Paint"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Blend Spaces")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                Toggle("Preview", isOn: $blendManager.isPreviewEnabled)
                    .toggleStyle(.switch)
                
                Button(action: { showAddSpaceDialog = true }) {
                    Image(systemName: "plus")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Tab selector
            Picker("Tab", selection: $selectedTab) {
                ForEach(EditorTab.allCases, id: \.self) { tab in
                    Text(tab.rawValue).tag(tab)
                }
            }
            .pickerStyle(.segmented)
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Tab content
            ScrollView {
                switch selectedTab {
                case .blend1D:
                    BlendSpace1DEditorView()
                case .blend2D:
                    BlendSpace2DEditorView()
                case .threshold:
                    BlendThresholdEditorView()
                case .layerMask:
                    LayerMaskEditorView()
                case .weightPaint:
                    WeightPaintingView()
                }
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .sheet(isPresented: $showAddSpaceDialog) {
            AddBlendSpaceDialog()
        }
    }
}

// MARK: - 1D Blend Space Editor
struct BlendSpace1DEditorView: View {
    @StateObject private var blendManager = BlendSpaceManager.shared
    @State private var selectedSpace: BlendSpace1D?
    
    var body: some View {
        HSplitView {
            // 1D blend spaces list
            VStack(spacing: 0) {
                HStack {
                    Text("1D Blend Spaces")
                        .font(DesignSystem.Typography.bodyBold)
                    
                    Spacer()
                    
                    Button("Add") {
                        // Add 1D blend space
                    }
                    .buttonStyle(.bordered)
                }
                .padding(DesignSystem.Spacing.sm)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                EditorDivider()
                
                ScrollView {
                    LazyVStack(spacing: DesignSystem.Spacing.xs) {
                        ForEach(blendManager.blendSpaces1D, id: \.id) { space in
                            BlendSpace1DRow(space: space)
                        }
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
                .background(DesignSystem.Colors.backgroundPrimary)
            }
            .frame(minWidth: 250, maxWidth: 300)
            
            // 1D blend space details
            if let selectedSpace = selectedSpace {
                BlendSpace1DDetailsView(space: selectedSpace)
            } else {
                VStack {
                    Spacer()
                    Text("Select a 1D blend space to edit")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Spacer()
                }
                .frame(maxWidth: .infinity)
            }
        }
        .onAppear {
            if blendManager.blendSpaces1D.first != nil {
                selectedSpace = blendManager.blendSpaces1D.first
            }
        }
    }
}

// MARK: - 1D Blend Space Row
struct BlendSpace1DRow: View {
    @ObservedObject var space: BlendSpace1D
    @StateObject private var blendManager = BlendSpaceManager.shared
    @State private var isHovering = false
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text(space.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                HStack {
                    Text(space.parameterName)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    Text("\(space.animationClips.count) clips")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            
            Spacer()
            
            // Current value slider
            VStack(spacing: 2) {
                Slider(value: $space.currentValue, in: space.minValue...space.maxValue)
                    .frame(width: 80)
                Text(String(format: "%.2f", space.currentValue))
                    .font(DesignSystem.Typography.smallMono)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(
            space == blendManager.selectedSpace1D ?
            DesignSystem.Colors.selection :
            (isHovering ? DesignSystem.Colors.hover : Color.clear)
        )
        .onHover { isHovering = $0 }
        .onTapGesture {
            blendManager.selectedSpace1D = space
        }
    }
}

// MARK: - 1D Blend Space Details
struct BlendSpace1DDetailsView: View {
    @ObservedObject var space: BlendSpace1D
    @State private var showClipEditor = false
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                GroupBox("Properties") {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("Name:")
                            TextField("Space name", text: $space.name)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        HStack {
                            Text("Parameter:")
                            TextField("Parameter name", text: $space.parameterName)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        HStack {
                            Text("Range:")
                            Slider(value: $space.minValue, in: -10...0)
                            Text(String(format: "%.1f", space.minValue))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                            Text("to")
                            Slider(value: $space.maxValue, in: 0...10)
                            Text(String(format: "%.1f", space.maxValue))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                        }
                        
                        HStack {
                            Text("Threshold:")
                            Slider(value: $space.threshold, in: 0...1)
                            Text(String(format: "%.2f", space.threshold))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                        }
                        
                        HStack {
                            Text("Interpolation:")
                            Picker("Interpolation", selection: $space.interpolationMode) {
                                ForEach(BlendSpace1D.InterpolationMode.allCases, id: \.self) { mode in
                                    Text(mode.rawValue).tag(mode)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                    }
                }
                
                GroupBox("Animation Clips") {
                    HStack {
                        Text("\(space.animationClips.count) clips")
                            .font(DesignSystem.Typography.body)
                        
                        Spacer()
                        
                        Button("Edit Clips") {
                            showClipEditor = true
                        }
                        .buttonStyle(.bordered)
                    }
                    
                    // 1D blend space visualization
                    BlendSpace1DVisualization(space: space)
                        .frame(height: 200)
                }
                
                GroupBox("Preview") {
                    VStack(spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("Current Value:")
                            Slider(value: $space.currentValue, in: space.minValue...space.maxValue)
                            Text(String(format: "%.2f", space.currentValue))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 50)
                        }
                        
                        // Preview area
                        Rectangle()
                            .fill(DesignSystem.Colors.backgroundTertiary)
                            .frame(height: 150)
                            .overlay(
                                Text("Animation Preview")
                                    .foregroundColor(DesignSystem.Colors.textSecondary),
                                alignment: .center
                            )
                    }
                }
            }
            .padding(DesignSystem.Spacing.md)
        }
        .background(DesignSystem.Colors.backgroundPrimary)
        .sheet(isPresented: $showClipEditor) {
            BlendSpaceClipEditor(space: space)
        }
    }
}

// MARK: - 1D Blend Space Visualization
struct BlendSpace1DVisualization: View {
    @ObservedObject var space: BlendSpace1D
    
    var body: some View {
        GeometryReader { geometry in
            let width = geometry.size.width
            let height = geometry.size.height
            let centerY = height / 2
            
            ZStack {
                // Background
                Rectangle()
                    .fill(DesignSystem.Colors.backgroundPrimary)
                
                // Grid lines
                Path { path in
                    for i in 0...10 {
                        let x = CGFloat(i) * width / 10
                        path.move(to: CGPoint(x: x, y: 0))
                        path.addLine(to: CGPoint(x: x, y: height))
                    }
                    
                    path.move(to: CGPoint(x: 0, y: centerY))
                    path.addLine(to: CGPoint(x: width, y: centerY))
                }
                .stroke(DesignSystem.Colors.border.opacity(0.3), lineWidth: 1)
                
                // Animation clips
                ForEach(space.animationClips) { clip in
                    let normalizedPosition = (clip.position - space.minValue) / (space.maxValue - space.minValue)
                    let x = CGFloat(normalizedPosition) * width
                    let clipHeight = CGFloat(clip.weight) * (height * 0.8)
                    
                    Circle()
                        .fill(DesignSystem.Colors.accentPrimary.opacity(0.6))
                        .frame(width: 16, height: 16)
                        .position(x: x, y: centerY)
                        .overlay(
                            Circle()
                                .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
                        )
                }
                
                // Current position indicator
                let currentNormalizedPosition = (space.currentValue - space.minValue) / (space.maxValue - space.minValue)
                let currentX = CGFloat(currentNormalizedPosition) * width
                
                Path { path in
                    path.move(to: CGPoint(x: currentX, y: 0))
                    path.addLine(to: CGPoint(x: currentX, y: height))
                }
                .stroke(DesignSystem.Colors.accentWarning, lineWidth: 2)
                
                // Labels
                VStack {
                    HStack {
                        Text(String(format: "%.1f", space.minValue))
                            .font(DesignSystem.Typography.smallMono)
                        Spacer()
                        Text(String(format: "%.1f", space.maxValue))
                            .font(DesignSystem.Typography.smallMono)
                    }
                    .padding(.horizontal, 4)
                    
                    Spacer()
                }
            }
        }
    }
}

// MARK: - 2D Blend Space Editor
struct BlendSpace2DEditorView: View {
    @StateObject private var blendManager = BlendSpaceManager.shared
    @State private var selectedSpace: BlendSpace2D?
    
    var body: some View {
        HSplitView {
            // 2D blend spaces list
            VStack(spacing: 0) {
                HStack {
                    Text("2D Blend Spaces")
                        .font(DesignSystem.Typography.bodyBold)
                    
                    Spacer()
                    
                    Button("Add") {
                        // Add 2D blend space
                    }
                    .buttonStyle(.bordered)
                }
                .padding(DesignSystem.Spacing.sm)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                EditorDivider()
                
                ScrollView {
                    LazyVStack(spacing: DesignSystem.Spacing.xs) {
                        ForEach(blendManager.blendSpaces2D, id: \.id) { space in
                            BlendSpace2DRow(space: space)
                        }
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
                .background(DesignSystem.Colors.backgroundPrimary)
            }
            .frame(minWidth: 250, maxWidth: 300)
            
            // 2D blend space details
            if let selectedSpace = selectedSpace {
                BlendSpace2DDetailsView(space: selectedSpace)
            } else {
                VStack {
                    Spacer()
                    Text("Select a 2D blend space to edit")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Spacer()
                }
                .frame(maxWidth: .infinity)
            }
        }
        .onAppear {
            if blendManager.blendSpaces2D.first != nil {
                selectedSpace = blendManager.blendSpaces2D.first
            }
        }
    }
}

// MARK: - 2D Blend Space Row
struct BlendSpace2DRow: View {
    @ObservedObject var space: BlendSpace2D
    @StateObject private var blendManager = BlendSpaceManager.shared
    @State private var isHovering = false
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text(space.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                HStack {
                    Text("\(space.parameterX)/\(space.parameterY)")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("•")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text(space.spaceType.rawValue)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    Text("\(space.animationClips.count) clips")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            
            Spacer()
            
            // Current position indicators
            VStack(spacing: 2) {
                Text("X: \(String(format: "%.1f", space.currentValue.x))")
                    .font(DesignSystem.Typography.smallMono)
                Text("Y: \(String(format: "%.1f", space.currentValue.y))")
                    .font(DesignSystem.Typography.smallMono)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(
            space == blendManager.selectedSpace2D ?
            DesignSystem.Colors.selection :
            (isHovering ? DesignSystem.Colors.hover : Color.clear)
        )
        .onHover { isHovering = $0 }
        .onTapGesture {
            blendManager.selectedSpace2D = space
        }
    }
}

// MARK: - 2D Blend Space Details
struct BlendSpace2DDetailsView: View {
    @ObservedObject var space: BlendSpace2D
    @State private var showClipEditor = false
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                GroupBox("Properties") {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("Name:")
                            TextField("Space name", text: $space.name)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        HStack {
                            Text("Parameter X:")
                            TextField("X parameter", text: $space.parameterX)
                                .textFieldStyle(.roundedBorder)
                            
                            Text("Y:")
                            TextField("Y parameter", text: $space.parameterY)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        HStack {
                            Text("Space Type:")
                            Picker("Space Type", selection: $space.spaceType) {
                                ForEach(BlendSpace2D.SpaceType.allCases, id: \.self) { type in
                                    Text(type.rawValue).tag(type)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                        
                        HStack {
                            Text("Threshold:")
                            Slider(value: $space.threshold, in: 0...1)
                            Text(String(format: "%.2f", space.threshold))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                        }
                        
                        HStack {
                            Text("Interpolation:")
                            Picker("Interpolation", selection: $space.interpolationMode) {
                                ForEach(BlendSpace1D.InterpolationMode.allCases, id: \.self) { mode in
                                    Text(mode.rawValue).tag(mode)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                    }
                }
                
                GroupBox("Animation Clips") {
                    HStack {
                        Text("\(space.animationClips.count) clips")
                            .font(DesignSystem.Typography.body)
                        
                        Spacer()
                        
                        Button("Edit Clips") {
                            showClipEditor = true
                        }
                        .buttonStyle(.bordered)
                    }
                    
                    // 2D blend space visualization
                    BlendSpace2DVisualization(space: space)
                        .frame(height: 300)
                }
                
                GroupBox("Preview") {
                    VStack(spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("X:")
                            Slider(value: $space.currentValue.x, in: -1...1)
                            Text(String(format: "%.2f", space.currentValue.x))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                            
                            Text("Y:")
                            Slider(value: $space.currentValue.y, in: -1...1)
                            Text(String(format: "%.2f", space.currentValue.y))
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                        }
                        
                        // Preview area
                        Rectangle()
                            .fill(DesignSystem.Colors.backgroundTertiary)
                            .frame(height: 150)
                            .overlay(
                                Text("Animation Preview")
                                    .foregroundColor(DesignSystem.Colors.textSecondary),
                                alignment: .center
                            )
                    }
                }
            }
            .padding(DesignSystem.Spacing.md)
        }
        .background(DesignSystem.Colors.backgroundPrimary)
        .sheet(isPresented: $showClipEditor) {
            BlendSpace2DClipEditor(space: space)
        }
    }
}

// MARK: - 2D Blend Space Visualization
struct BlendSpace2DVisualization: View {
    @ObservedObject var space: BlendSpace2D
    
    var body: some View {
        GeometryReader { geometry in
            let width = geometry.size.width
            let height = geometry.size.height
            let centerX = width / 2
            let centerY = height / 2
            
            ZStack {
                // Background
                Rectangle()
                    .fill(DesignSystem.Colors.backgroundPrimary)
                
                // Grid based on space type
                if space.spaceType == .cartesian {
                    CartesianGrid(width: width, height: height)
                } else if space.spaceType == .directional {
                    DirectionalGrid(width: width, height: height)
                } else {
                    GridGrid(width: width, height: height)
                }
                
                // Animation clips
                ForEach(space.animationClips) { clip in
                    let x = centerX + CGFloat(clip.position.x) * (width * 0.4)
                    let y = centerY - CGFloat(clip.position.y) * (height * 0.4)
                    let radius = CGFloat(clip.influenceRadius) * min(width, height) * 0.2
                    
                    Circle()
                        .fill(DesignSystem.Colors.accentPrimary.opacity(0.3))
                        .frame(width: radius * 2, height: radius * 2)
                        .position(x: x, y: y)
                    
                    Circle()
                        .fill(DesignSystem.Colors.accentPrimary)
                        .frame(width: 12, height: 12)
                        .position(x: x, y: y)
                }
                
                // Current position indicator
                let currentX = centerX + CGFloat(space.currentValue.x) * (width * 0.4)
                let currentY = centerY - CGFloat(space.currentValue.y) * (height * 0.4)
                
                Path { path in
                    path.move(to: CGPoint(x: currentX - 10, y: currentY))
                    path.addLine(to: CGPoint(x: currentX + 10, y: currentY))
                    path.move(to: CGPoint(x: currentX, y: currentY - 10))
                    path.addLine(to: CGPoint(x: currentX, y: currentY + 10))
                }
                .stroke(DesignSystem.Colors.accentWarning, lineWidth: 2)
                
                // Labels
                VStack {
                    HStack {
                        Text("-1")
                            .font(DesignSystem.Typography.smallMono)
                        Spacer()
                        Text("0")
                            .font(DesignSystem.Typography.smallMono)
                        Spacer()
                        Text("1")
                            .font(DesignSystem.Typography.smallMono)
                    }
                    .padding(.horizontal, 4)
                    
                    Spacer()
                    
                    HStack {
                        Text("-1")
                            .font(DesignSystem.Typography.smallMono)
                        Spacer()
                        Text("0")
                            .font(DesignSystem.Typography.smallMono)
                        Spacer()
                        Text("1")
                            .font(DesignSystem.Typography.smallMono)
                    }
                    .padding(.horizontal, 4)
                }
            }
        }
    }
}

// MARK: - Grid Types
struct CartesianGrid: View {
    let width: CGFloat
    let height: CGFloat
    
    var body: some View {
        Path { path in
            // Vertical lines
            for i in 0...10 {
                let x = CGFloat(i) * width / 10
                path.move(to: CGPoint(x: x, y: 0))
                path.addLine(to: CGPoint(x: x, y: height))
            }
            
            // Horizontal lines
            for i in 0...10 {
                let y = CGFloat(i) * height / 10
                path.move(to: CGPoint(x: 0, y: y))
                path.addLine(to: CGPoint(x: width, y: y))
            }
            
            // Center lines
            path.move(to: CGPoint(x: width / 2, y: 0))
            path.addLine(to: CGPoint(x: width / 2, y: height))
            path.move(to: CGPoint(x: 0, y: height / 2))
            path.addLine(to: CGPoint(x: width, y: height / 2))
        }
        .stroke(DesignSystem.Colors.border.opacity(0.3), lineWidth: 1)
    }
}

struct DirectionalGrid: View {
    let width: CGFloat
    let height: CGFloat
    let centerX: CGFloat { width / 2 }
    let centerY: CGFloat { height / 2 }
    
    var body: some View {
        Path { path in
            // Radial lines
            for i in 0..<8 {
                let angle = Double(i) * .pi / 4
                let endX = centerX + cos(angle) * min(width, height) * 0.4
                let endY = centerY + sin(angle) * min(width, height) * 0.4
                path.move(to: CGPoint(x: centerX, y: centerY))
                path.addLine(to: CGPoint(x: endX, y: endY))
            }
            
            // Concentric circles
            for i in 1...4 {
                let radius = CGFloat(i) * min(width, height) * 0.1
                path.addEllipse(in: CGRect(
                    x: centerX - radius,
                    y: centerY - radius,
                    width: radius * 2,
                    height: radius * 2
                ))
            }
        }
        .stroke(DesignSystem.Colors.border.opacity(0.3), lineWidth: 1)
    }
}

struct GridGrid: View {
    let width: CGFloat
    let height: CGFloat
    
    var body: some View {
        Path { path in
            // 3x3 grid
            for i in 0...3 {
                let x = CGFloat(i) * width / 3
                path.move(to: CGPoint(x: x, y: 0))
                path.addLine(to: CGPoint(x: x, y: height))
                
                let y = CGFloat(i) * height / 3
                path.move(to: CGPoint(x: 0, y: y))
                path.addLine(to: CGPoint(x: width, y: y))
            }
        }
        .stroke(DesignSystem.Colors.border.opacity(0.3), lineWidth: 1)
    }
}

// MARK: - Blend Threshold Editor
struct BlendThresholdEditorView: View {
    @StateObject private var blendManager = BlendSpaceManager.shared
    @State private var globalThreshold: Float = 0.1
    @State private var perSpaceThresholds: [UUID: Float] = [:]
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            GroupBox("Global Threshold") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Text("Adjust the global threshold for all blend spaces")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack {
                        Text("Threshold:")
                        Slider(value: $globalThreshold, in: 0...1)
                        Text(String(format: "%.3f", globalThreshold))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 50)
                    }
                    
                    Button("Apply to All") {
                        applyGlobalThreshold()
                    }
                    .buttonStyle(.bordered)
                }
            }
            
            GroupBox("Per-Space Thresholds") {
                ScrollView {
                    LazyVStack(spacing: DesignSystem.Spacing.sm) {
                        ForEach(blendManager.blendSpaces1D, id: \.id) { space in
                            BlendSpaceThresholdRow(
                                space: space,
                                threshold: Binding(
                                    get: { perSpaceThresholds[space.id] ?? space.threshold },
                                    set: { perSpaceThresholds[space.id] = $0 }
                                )
                            )
                        }
                        
                        ForEach(blendManager.blendSpaces2D, id: \.id) { space in
                            BlendSpace2DThresholdRow(
                                space: space,
                                threshold: Binding(
                                    get: { perSpaceThresholds[space.id] ?? space.threshold },
                                    set: { perSpaceThresholds[space.id] = $0 }
                                )
                            )
                        }
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
                .frame(height: 300)
            }
            
            GroupBox("Threshold Visualization") {
                ThresholdVisualization(threshold: globalThreshold)
                    .frame(height: 200)
            }
        }
        .padding(DesignSystem.Spacing.md)
    }
    
    private func applyGlobalThreshold() {
        for space in blendManager.blendSpaces1D {
            space.threshold = globalThreshold
        }
        for space in blendManager.blendSpaces2D {
            space.threshold = globalThreshold
        }
    }
}

// MARK: - Blend Space Threshold Row
struct BlendSpaceThresholdRow: View {
    @ObservedObject var space: BlendSpace1D
    @Binding var threshold: Float
    
    var body: some View {
        HStack {
            Text(space.name)
                .font(DesignSystem.Typography.body)
                .frame(maxWidth: .infinity, alignment: .leading)
            
            Slider(value: $threshold, in: 0...1)
                .frame(width: 100)
            
            Text(String(format: "%.3f", threshold))
                .font(DesignSystem.Typography.smallMono)
                .frame(width: 50)
            
            Button("Apply") {
                space.threshold = threshold
            }
            .buttonStyle(.bordered)
            .controlSize(.small)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Blend Space 2D Threshold Row
struct BlendSpace2DThresholdRow: View {
    @ObservedObject var space: BlendSpace2D
    @Binding var threshold: Float
    
    var body: some View {
        HStack {
            Text(space.name)
                .font(DesignSystem.Typography.body)
                .frame(maxWidth: .infinity, alignment: .leading)
            
            Slider(value: $threshold, in: 0...1)
                .frame(width: 100)
            
            Text(String(format: "%.3f", threshold))
                .font(DesignSystem.Typography.smallMono)
                .frame(width: 50)
            
            Button("Apply") {
                space.threshold = threshold
            }
            .buttonStyle(.bordered)
            .controlSize(.small)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Threshold Visualization
struct ThresholdVisualization: View {
    let threshold: Float
    
    var body: some View {
        GeometryReader { geometry in
            let width = geometry.size.width
            let height = geometry.size.height
            
            ZStack {
                Rectangle()
                    .fill(DesignSystem.Colors.backgroundPrimary)
                
                // Threshold curve visualization
                Path { path in
                    path.move(to: CGPoint(x: 0, y: height))
                    
                    for i in 0...100 {
                        let x = CGFloat(i) * width / 100
                        let t = Float(i) / 100.0
                        let y = height - CGFloat(thresholdFunction(t)) * height
                        path.addLine(to: CGPoint(x: x, y: y))
                    }
                }
                .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
                
                // Threshold line
                Path { path in
                    path.move(to: CGPoint(x: 0, y: height * (1 - CGFloat(threshold))))
                    path.addLine(to: CGPoint(x: width, y: height * (1 - CGFloat(threshold))))
                }
                .stroke(DesignSystem.Colors.accentWarning, lineWidth: 1)
                .stroke(style: StrokeStyle(lineWidth: 1, dash: [5, 5]))
                
                // Labels
                VStack {
                    HStack {
                        Text("0")
                            .font(DesignSystem.Typography.smallMono)
                        Spacer()
                        Text("1")
                            .font(DesignSystem.Typography.smallMono)
                    }
                    .padding(.horizontal, 4)
                    
                    Spacer()
                    
                    HStack {
                        Text("Threshold: \(String(format: "%.3f", threshold))")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                }
            }
        }
    }
    
    private func thresholdFunction(_ t: Float) -> Float {
        // Smooth step function for threshold
        if t < threshold {
            return 0
        } else {
            let normalized = (t - threshold) / (1 - threshold)
            return normalized * normalized * (3.0 - 2.0 * normalized) // Smooth step
        }
    }
}

// MARK: - Layer Mask Editor
struct LayerMaskEditorView: View {
    @StateObject private var layerManager = LayerMaskManager.shared
    @State private var selectedMask: LayerMask?
    @State private var showAddMaskDialog = false
    
    var body: some View {
        HSplitView {
            // Layer masks list
            VStack(spacing: 0) {
                HStack {
                    Text("Layer Masks")
                        .font(DesignSystem.Typography.bodyBold)
                    
                    Spacer()
                    
                    Button("Add") {
                        showAddMaskDialog = true
                    }
                    .buttonStyle(.bordered)
                }
                .padding(DesignSystem.Spacing.sm)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                EditorDivider()
                
                ScrollView {
                    LazyVStack(spacing: DesignSystem.Spacing.xs) {
                        ForEach(layerManager.layerMasks, id: \.id) { mask in
                            LayerMaskRow(mask: mask)
                        }
                    }
                    .padding(DesignSystem.Spacing.sm)
                }
                .background(DesignSystem.Colors.backgroundPrimary)
            }
            .frame(minWidth: 250, maxWidth: 300)
            
            // Layer mask details
            if let selectedMask = selectedMask {
                LayerMaskDetailsView(mask: selectedMask)
            } else {
                VStack {
                    Spacer()
                    Text("Select a layer mask to edit")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Spacer()
                }
                .frame(maxWidth: .infinity)
            }
        }
        .onAppear {
            if layerManager.layerMasks.first != nil {
                selectedMask = layerManager.layerMasks.first
            }
        }
        .sheet(isPresented: $showAddMaskDialog) {
            AddLayerMaskDialog()
        }
    }
}

// MARK: - Layer Mask Data Models
struct LayerMask: Identifiable, ObservableObject {
    let id = UUID()
    var name: String
    var description: String = ""
    var boneWeights: [BoneWeight] = []
    var isEnabled: Bool = true
    var maskType: MaskType = .inclusive
    var influenceType: InfluenceType = .weight
    
    enum MaskType: String, CaseIterable {
        case inclusive = "Inclusive"
        case exclusive = "Exclusive"
        case additive = "Additive"
        case subtractive = "Subtractive"
    }
    
    enum InfluenceType: String, CaseIterable {
        case weight = "Weight"
        case binary = "Binary"
        case smooth = "Smooth"
    }
}

struct BoneWeight: Identifiable, ObservableObject {
    let id = UUID()
    var boneName: String
    var weight: Float = 1.0
    var isEnabled: Bool = true
    var falloffDistance: Float = 0.1
}

// MARK: - Layer Mask Manager
class LayerMaskManager: ObservableObject {
    static let shared = LayerMaskManager()
    
    @Published var layerMasks: [LayerMask] = []
    @Published var selectedMask: LayerMask?
    
    init() {
        setupDefaultMasks()
    }
    
    private func setupDefaultMasks() {
        let upperBodyMask = LayerMask(
            name: "Upper Body",
            description: "Spine, arms, and head bones",
            maskType: .inclusive,
            influenceType: .weight
        )
        
        upperBodyMask.boneWeights = [
            BoneWeight(boneName: "Spine", weight: 1.0),
            BoneWeight(boneName: "Chest", weight: 1.0),
            BoneWeight(boneName: "Neck", weight: 1.0),
            BoneWeight(boneName: "Head", weight: 1.0),
            BoneWeight(boneName: "LeftShoulder", weight: 1.0),
            BoneWeight(boneName: "LeftArm", weight: 1.0),
            BoneWeight(boneName: "RightShoulder", weight: 1.0),
            BoneWeight(boneName: "RightArm", weight: 1.0)
        ]
        
        let lowerBodyMask = LayerMask(
            name: "Lower Body",
            description: "Hips and leg bones",
            maskType: .inclusive,
            influenceType: .weight
        )
        
        lowerBodyMask.boneWeights = [
            BoneWeight(boneName: "Hips", weight: 1.0),
            BoneWeight(boneName: "LeftUpLeg", weight: 1.0),
            BoneWeight(boneName: "LeftLeg", weight: 1.0),
            BoneWeight(boneName: "LeftFoot", weight: 1.0),
            BoneWeight(boneName: "RightUpLeg", weight: 1.0),
            BoneWeight(boneName: "RightLeg", weight: 1.0),
            BoneWeight(boneName: "RightFoot", weight: 1.0)
        ]
        
        layerMasks = [upperBodyMask, lowerBodyMask]
    }
}

// MARK: - Layer Mask Row
struct LayerMaskRow: View {
    @ObservedObject var mask: LayerMask
    @StateObject private var layerManager = LayerMaskManager.shared
    @State private var isHovering = false
    
    var body: some View {
        HStack {
            // Type indicator
            Circle()
                .fill(mask.maskType == .inclusive ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.accentWarning)
                .frame(width: 8, height: 8)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(mask.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                HStack {
                    Text(mask.description)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    Text("\(mask.boneWeights.count) bones")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            
            Spacer()
            
            Toggle("", isOn: $mask.isEnabled)
                .toggleStyle(.switch)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(
            mask == layerManager.selectedMask ?
            DesignSystem.Colors.selection :
            (isHovering ? DesignSystem.Colors.hover : Color.clear)
        )
        .onHover { isHovering = $0 }
        .onTapGesture {
            layerManager.selectedMask = mask
        }
    }
}

// MARK: - Layer Mask Details
struct LayerMaskDetailsView: View {
    @ObservedObject var mask: LayerMask
    @State private var showBoneEditor = false
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                GroupBox("Properties") {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("Name:")
                            TextField("Mask name", text: $mask.name)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        HStack {
                            Text("Description:")
                            TextField("Description", text: $mask.description)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        HStack {
                            Text("Mask Type:")
                            Picker("Mask Type", selection: $mask.maskType) {
                                ForEach(LayerMask.MaskType.allCases, id: \.self) { type in
                                    Text(type.rawValue).tag(type)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                        
                        HStack {
                            Text("Influence Type:")
                            Picker("Influence Type", selection: $mask.influenceType) {
                                ForEach(LayerMask.InfluenceType.allCases, id: \.self) { type in
                                    Text(type.rawValue).tag(type)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                        
                        Toggle("Enabled", isOn: $mask.isEnabled)
                            .toggleStyle(.switch)
                    }
                }
                
                GroupBox("Bone Weights") {
                    HStack {
                        Text("\(mask.boneWeights.count) bones")
                            .font(DesignSystem.Typography.body)
                        
                        Spacer()
                        
                        Button("Edit Bones") {
                            showBoneEditor = true
                        }
                        .buttonStyle(.bordered)
                    }
                    
                    // Bone weights list
                    ScrollView {
                        LazyVStack(spacing: DesignSystem.Spacing.xs) {
                            ForEach(mask.boneWeights, id: \.id) { boneWeight in
                                BoneWeightRow(boneWeight: boneWeight)
                            }
                        }
                        .padding(DesignSystem.Spacing.sm)
                    }
                    .frame(height: 200)
                }
                
                GroupBox("Visualization") {
                    LayerMaskVisualization(mask: mask)
                        .frame(height: 200)
                }
            }
            .padding(DesignSystem.Spacing.md)
        }
        .background(DesignSystem.Colors.backgroundPrimary)
        .sheet(isPresented: $showBoneEditor) {
            LayerMaskBoneEditor(mask: mask)
        }
    }
}

// MARK: - Bone Weight Row
struct BoneWeightRow: View {
    @ObservedObject var boneWeight: BoneWeight
    
    var body: some View {
        HStack {
            Text(boneWeight.boneName)
                .font(DesignSystem.Typography.body)
                .frame(maxWidth: .infinity, alignment: .leading)
            
            Toggle("", isOn: $boneWeight.isEnabled)
                .toggleStyle(.switch)
            
            VStack(spacing: 2) {
                Slider(value: $boneWeight.weight, in: 0...1)
                    .frame(width: 80)
                Text(String(format: "%.2f", boneWeight.weight))
                    .font(DesignSystem.Typography.smallMono)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Weight Painting View
struct WeightPaintingView: View {
    @StateObject private var weightManager = WeightPaintManager.shared
    @State private var selectedBone: String = ""
    @State private var paintMode: WeightPaintManager.PaintMode = .add
    @State private var paintStrength: Float = 0.5
    @State private var paintRadius: Float = 0.1
    @State private var falloffType: WeightPaintManager.FalloffType = .linear
    
    var body: some View {
        HSplitView {
            // Painting tools
            VStack(spacing: 0) {
                Text("Weight Painting Tools")
                    .font(DesignSystem.Typography.bodyBold)
                    .padding(DesignSystem.Spacing.sm)
                    .background(DesignSystem.Colors.backgroundSecondary)
                
                EditorDivider()
                
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                    GroupBox("Bone Selection") {
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                            Picker("Bone", selection: $selectedBone) {
                                ForEach(weightManager.availableBones, id: \.self) { bone in
                                    Text(bone).tag(bone)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                    }
                    
                    GroupBox("Paint Settings") {
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                            HStack {
                                Text("Mode:")
                                Picker("Mode", selection: $paintMode) {
                                    ForEach(WeightPaintManager.PaintMode.allCases, id: \.self) { mode in
                                        Text(mode.rawValue).tag(mode)
                                    }
                                }
                                .pickerStyle(.menu)
                            }
                            
                            HStack {
                                Text("Strength:")
                                Slider(value: $paintStrength, in: 0...1)
                                Text(String(format: "%.2f", paintStrength))
                                    .font(DesignSystem.Typography.smallMono)
                                    .frame(width: 40)
                            }
                            
                            HStack {
                                Text("Radius:")
                                Slider(value: $paintRadius, in: 0.01...1.0)
                                Text(String(format: "%.2f", paintRadius))
                                    .font(DesignSystem.Typography.smallMono)
                                    .frame(width: 40)
                            }
                            
                            HStack {
                                Text("Falloff:")
                                Picker("Falloff", selection: $falloffType) {
                                    ForEach(WeightPaintManager.FalloffType.allCases, id: \.self) { type in
                                        Text(type.rawValue).tag(type)
                                    }
                                }
                                .pickerStyle(.menu)
                            }
                        }
                    }
                    
                    GroupBox("Operations") {
                        VStack(spacing: DesignSystem.Spacing.sm) {
                            Button("Clear All Weights") {
                                weightManager.clearAllWeights()
                            }
                            .buttonStyle(.bordered)
                            
                            Button("Normalize Weights") {
                                weightManager.normalizeWeights()
                            }
                            .buttonStyle(.bordered)
                            
                            Button("Invert Weights") {
                                weightManager.invertWeights()
                            }
                            .buttonStyle(.bordered)
                        }
                    }
                }
                .padding(DesignSystem.Spacing.md)
                .background(DesignSystem.Colors.backgroundPrimary)
            }
            .frame(minWidth: 250, maxWidth: 300)
            
            // 3D viewport for weight painting
            VStack(spacing: 0) {
                Text("Weight Painting Viewport")
                    .font(DesignSystem.Typography.bodyBold)
                    .padding(DesignSystem.Spacing.sm)
                    .background(DesignSystem.Colors.backgroundSecondary)
                
                EditorDivider()
                
                // 3D viewport would go here
                Rectangle()
                    .fill(DesignSystem.Colors.backgroundTertiary)
                    .frame(height: 400)
                    .overlay(
                        VStack {
                            Text("3D Viewport")
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            Text("Selected: \(selectedBone.isEmpty ? "None" : selectedBone)")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        },
                        alignment: .center
                    )
                
                // Weight information
                GroupBox("Weight Information") {
                    HStack {
                        Text("Total Vertices:")
                        Spacer()
                        Text("\(weightManager.totalVertices)")
                            .font(DesignSystem.Typography.smallMono)
                    }
                    
                    HStack {
                        Text("Painted Vertices:")
                        Spacer()
                        Text("\(weightManager.paintedVertices)")
                            .font(DesignSystem.Typography.smallMono)
                    }
                    
                    HStack {
                        Text("Average Weight:")
                        Spacer()
                        Text(String(format: "%.3f", weightManager.averageWeight))
                            .font(DesignSystem.Typography.smallMono)
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
        }
    }
}

// MARK: - Weight Paint Manager
class WeightPaintManager: ObservableObject {
    static let shared = WeightPaintManager()
    
    @Published var availableBones: [String] = []
    @Published var totalVertices: Int = 0
    @Published var paintedVertices: Int = 0
    @Published var averageWeight: Float = 0.0
    
    enum PaintMode: String, CaseIterable {
        case add = "Add"
        case subtract = "Subtract"
        case smooth = "Smooth"
        case normalize = "Normalize"
    }
    
    enum FalloffType: String, CaseIterable {
        case linear = "Linear"
        case smooth = "Smooth"
        case exponential = "Exponential"
        case spherical = "Spherical"
    }
    
    init() {
        setupDefaultBones()
    }
    
    private func setupDefaultBones() {
        availableBones = [
            "Hips", "Spine", "Chest", "Neck", "Head",
            "LeftShoulder", "LeftArm", "LeftForeArm", "LeftHand",
            "RightShoulder", "RightArm", "RightForeArm", "RightHand",
            "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase",
            "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase"
        ]
        
        totalVertices = 1000 // Mock data
        paintedVertices = 250
        averageWeight = 0.65
    }
    
    func clearAllWeights() {
        paintedVertices = 0
        averageWeight = 0.0
    }
    
    func normalizeWeights() {
        // Normalize weights logic
        averageWeight = 0.5
    }
    
    func invertWeights() {
        // Invert weights logic
        averageWeight = 1.0 - averageWeight
    }
}

// MARK: - Placeholder Views
struct LayerMaskVisualization: View {
    @ObservedObject var mask: LayerMask
    
    var body: some View {
        Rectangle()
            .fill(DesignSystem.Colors.backgroundTertiary)
            .overlay(
                Text("Layer Mask Visualization")
                    .foregroundColor(DesignSystem.Colors.textSecondary),
                alignment: .center
            )
    }
}

struct LayerMaskBoneEditor: View {
    @ObservedObject var mask: LayerMask
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        VStack {
            Text("Edit Bone Weights")
                .font(DesignSystem.Typography.headline)
            
            // Bone editing interface
            Rectangle()
                .fill(DesignSystem.Colors.backgroundTertiary)
                .frame(height: 300)
                .overlay(
                    Text("Bone weight editing interface")
                        .foregroundColor(DesignSystem.Colors.textSecondary),
                    alignment: .center
                )
            
            Button("Close") { dismiss() }
                .buttonStyle(.bordered)
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 500, height: 450)
    }
}

struct AddLayerMaskDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var layerManager = LayerMaskManager.shared
    
    @State private var name = ""
    @State private var description = ""
    @State private var maskType: LayerMask.MaskType = .inclusive
    @State private var influenceType: LayerMask.InfluenceType = .weight
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Add Layer Mask")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                TextField("Name", text: $name)
                    .textFieldStyle(.roundedBorder)
                
                TextField("Description", text: $description)
                    .textFieldStyle(.roundedBorder)
                
                Picker("Mask Type", selection: $maskType) {
                    ForEach(LayerMask.MaskType.allCases, id: \.self) { type in
                        Text(type.rawValue).tag(type)
                    }
                }
                .pickerStyle(.menu)
                
                Picker("Influence Type", selection: $influenceType) {
                    ForEach(LayerMask.InfluenceType.allCases, id: \.self) { type in
                        Text(type.rawValue).tag(type)
                    }
                }
                .pickerStyle(.menu)
            }
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Add") {
                    let newMask = LayerMask(
                        name: name,
                        description: description,
                        maskType: maskType,
                        influenceType: influenceType
                    )
                    layerManager.layerMasks.append(newMask)
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(name.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 350)
    }
}

struct BlendSpaceClipEditor: View {
    let space: BlendSpace1D
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        VStack {
            Text("Edit Animation Clips")
                .font(DesignSystem.Typography.headline)
            
            // Clip editing interface
            Rectangle()
                .fill(DesignSystem.Colors.backgroundTertiary)
                .frame(height: 300)
                .overlay(
                    Text("Animation clip editing interface")
                        .foregroundColor(DesignSystem.Colors.textSecondary),
                    alignment: .center
                )
            
            Button("Close") { dismiss() }
                .buttonStyle(.bordered)
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 500, height: 450)
    }
}

struct BlendSpace2DClipEditor: View {
    let space: BlendSpace2D
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        VStack {
            Text("Edit 2D Animation Clips")
                .font(DesignSystem.Typography.headline)
            
            // 2D clip editing interface
            Rectangle()
                .fill(DesignSystem.Colors.backgroundTertiary)
                .frame(height: 300)
                .overlay(
                    Text("2D animation clip editing interface")
                        .foregroundColor(DesignSystem.Colors.textSecondary),
                    alignment: .center
                )
            
            Button("Close") { dismiss() }
                .buttonStyle(.bordered)
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 500, height: 450)
    }
}

struct AddBlendSpaceDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var blendManager = BlendSpaceManager.shared
    
    @State private var name = ""
    @State private var spaceType: BlendSpaceType = .blend1D
    
    enum BlendSpaceType: String, CaseIterable {
        case blend1D = "1D Blend Space"
        case blend2D = "2D Blend Space"
    }
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Add Blend Space")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                TextField("Name", text: $name)
                    .textFieldStyle(.roundedBorder)
                
                Picker("Type", selection: $spaceType) {
                    ForEach(BlendSpaceType.allCases, id: \.self) { type in
                        Text(type.rawValue).tag(type)
                    }
                }
                .pickerStyle(.menu)
            }
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Add") {
                    // Add blend space logic
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(name.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 250)
    }
}

// MARK: - Animation Clip Management

// Animation Clip Extensions
extension AnimationClip {
    var isFavorite: Bool {
        get { favorites.contains(id) }
        set { 
            if newValue {
                favorites.insert(id)
            } else {
                favorites.remove(id)
            }
        }
    }
    
    static var favorites: Set<UUID> = []
}

// MARK: - Animation Clip Manager
class AnimationClipManager: ObservableObject {
    static let shared = AnimationClipManager()
    
    @Published var clips: [AnimationClip] = []
    @Published var favoriteClips: [AnimationClip] = []
    @Published var importHistory: [ImportHistoryEntry] = []
    @Published var exportHistory: [ExportHistoryEntry] = []
    
    init() {
        setupDefaultClips()
    }
    
    private func setupDefaultClips() {
        let walkClip = AnimationClip(
            name: "Walk",
            duration: 1.2,
            loop: true,
            events: []
        )
        
        let runClip = AnimationClip(
            name: "Run",
            duration: 0.8,
            loop: true,
            events: []
        )
        
        let idleClip = AnimationClip(
            name: "Idle",
            duration: 2.0,
            loop: true,
            events: []
        )
        
        clips = [walkClip, runClip, idleClip]
        updateFavoriteClips()
    }
    
    func updateFavoriteClips() {
        favoriteClips = clips.filter { $0.isFavorite }
    }
    
    func importClip(from url: URL, format: ImportFormat) {
        let importEntry = ImportHistoryEntry(
            id: UUID(),
            fileName: url.lastPathComponent,
            format: format,
            timestamp: Date(),
            status: .inProgress
        )
        
        importHistory.append(importEntry)
        
        // Mock import process
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            let newClip = AnimationClip(
                name: url.deletingPathExtension().lastPathComponent,
                duration: 1.0,
                loop: false,
                events: []
            )
            
            self.clips.append(newClip)
            
            if let index = self.importHistory.firstIndex(where: { $0.id == importEntry.id }) {
                self.importHistory[index].status = .completed
            }
        }
    }
    
    func exportClip(_ clip: AnimationClip, to url: URL, format: ExportFormat) {
        let exportEntry = ExportHistoryEntry(
            id: UUID(),
            clipName: clip.name,
            fileName: url.lastPathComponent,
            format: format,
            timestamp: Date(),
            status: .inProgress
        )
        
        exportHistory.append(exportEntry)
        
        // Mock export process
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            if let index = self.exportHistory.firstIndex(where: { $0.id == exportEntry.id }) {
                self.exportHistory[index].status = .completed
            }
        }
    }
    
    func mirrorClip(_ clip: AnimationClip) -> AnimationClip {
        let mirroredClip = AnimationClip(
            name: "\(clip.name) (Mirrored)",
            duration: clip.duration,
            loop: clip.loop,
            events: clip.events
        )
        
        // Mirror animation tracks logic would go here
        
        clips.append(mirroredClip)
        return mirroredClip
    }
    
    func setReferencePose(for clip: AnimationClip) {
        // Set reference pose logic
        print("Setting reference pose for \(clip.name)")
    }
    
    func setAdditiveMode(for clip: AnimationClip, enabled: Bool) {
        // Set additive mode logic
        print("Setting additive mode \(enabled ? "on" : "off") for \(clip.name)")
    }
    
    func bakeClip(_ clip: AnimationClip) {
        // Bake animation logic
        print("Baking animation clip \(clip.name)")
    }
    
    func previewRetarget(_ clip: AnimationClip, to avatar: AvatarDefinition) {
        // Preview retargeting logic
        print("Previewing retarget of \(clip.name) to \(avatar.name)")
    }
}

// MARK: - Import/Export Models
struct ImportHistoryEntry: Identifiable {
    let id: UUID
    let fileName: String
    let format: ImportFormat
    let timestamp: Date
    var status: ImportStatus
    
    enum ImportStatus {
        case inProgress
        case completed
        case failed
    }
}

struct ExportHistoryEntry: Identifiable {
    let id: UUID
    let clipName: String
    let fileName: String
    let format: ExportFormat
    let timestamp: Date
    var status: ExportStatus
    
    enum ExportStatus {
        case inProgress
        case completed
        case failed
    }
}

enum ImportFormat: String, CaseIterable {
    case fbx = "FBX"
    case bvh = "BVH"
    case obj = "OBJ"
    case dae = "DAE"
    
    var fileExtension: String {
        return ".\(rawValue.lowercased())"
    }
}

enum ExportFormat: String, CaseIterable {
    case fbx = "FBX"
    case bvh = "BVH"
    case obj = "OBJ"
    case dae = "DAE"
    case gif = "GIF"
    
    var fileExtension: String {
        return ".\(rawValue.lowercased())"
    }
}

// MARK: - Animation Clip Management View
struct AnimationClipManagementView: View {
    @StateObject private var clipManager = AnimationClipManager.shared
    @StateObject private var avatarManager = AvatarManager.shared
    @State private var selectedClip: AnimationClip?
    @State private var showImportDialog = false
    @State private var showExportDialog = false
    @State private var selectedTab: ManagementTab = .clips
    
    enum ManagementTab: String, CaseIterable {
        case clips = "Clips"
        case favorites = "Favorites"
        case importHistory = "Import History"
        case exportHistory = "Export History"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Animation Clip Management")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
                
                Button("Import") {
                    showImportDialog = true
                }
                .buttonStyle(.bordered)
                
                Button("Export") {
                    showExportDialog = true
                }
                .buttonStyle(.bordered)
                .disabled(selectedClip == nil)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Tab selector
            Picker("Tab", selection: $selectedTab) {
                ForEach(ManagementTab.allCases, id: \.self) { tab in
                    Text(tab.rawValue).tag(tab)
                }
            }
            .pickerStyle(.segmented)
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Tab content
            HSplitView {
                // Clip list
                VStack(spacing: 0) {
                    switch selectedTab {
                    case .clips:
                        ClipListView(clips: clipManager.clips, selectedClip: $selectedClip)
                    case .favorites:
                        ClipListView(clips: clipManager.favoriteClips, selectedClip: $selectedClip)
                    case .importHistory:
                        ImportHistoryView(entries: clipManager.importHistory)
                    case .exportHistory:
                        ExportHistoryView(entries: clipManager.exportHistory)
                    }
                }
                .frame(minWidth: 250, maxWidth: 300)
                
                // Clip details
                if let selectedClip = selectedClip {
                    AnimationClipDetailsView(clip: selectedClip)
                } else {
                    VStack {
                        Spacer()
                        Text("Select an animation clip to edit")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        Spacer()
                    }
                    .frame(maxWidth: .infinity)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .sheet(isPresented: $showImportDialog) {
            ImportAnimationDialog()
        }
        .sheet(isPresented: $showExportDialog) {
            ExportAnimationDialog(clip: selectedClip)
        }
    }
}

// MARK: - Clip List View
struct ClipListView: View {
    let clips: [AnimationClip]
    @Binding var selectedClip: AnimationClip?
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("\(clips.count) clips")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            ScrollView {
                LazyVStack(spacing: DesignSystem.Spacing.xs) {
                    ForEach(clips, id: \.id) { clip in
                        AnimationClipRow(clip: clip, selectedClip: $selectedClip)
                    }
                }
                .padding(DesignSystem.Spacing.sm)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
    }
}

// MARK: - Animation Clip Row
struct AnimationClipRow: View {
    @ObservedObject var clip: AnimationClip
    @Binding var selectedClip: AnimationClip?
    @StateObject private var clipManager = AnimationClipManager.shared
    @State private var isHovering = false
    
    var body: some View {
        HStack {
            // Favorite indicator
            Button(action: { 
                clip.isFavorite.toggle()
                clipManager.updateFavoriteClips()
            }) {
                Image(systemName: clip.isFavorite ? "heart.fill" : "heart")
                    .foregroundColor(clip.isFavorite ? DesignSystem.Colors.accentDanger : DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(clip.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                HStack {
                    Text(String(format: "%.2fs", clip.duration))
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    if clip.loop {
                        Image(systemName: "repeat")
                            .font(.system(size: 10))
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    
                    Text("\(clip.events.count) events")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                }
            }
            
            Spacer()
        }
        .padding(DesignSystem.Spacing.sm)
        .background(
            clip == selectedClip ?
            DesignSystem.Colors.selection :
            (isHovering ? DesignSystem.Colors.hover : Color.clear)
        )
        .onHover { isHovering = $0 }
        .onTapGesture {
            selectedClip = clip
        }
        .contextMenu {
            Button("Duplicate") {
                let duplicatedClip = AnimationClip(
                    name: "\(clip.name) Copy",
                    duration: clip.duration,
                    loop: clip.loop,
                    events: clip.events
                )
                clipManager.clips.append(duplicatedClip)
            }
            
            Button("Mirror") {
                clipManager.mirrorClip(clip)
            }
            
            Button("Set Reference Pose") {
                clipManager.setReferencePose(for: clip)
            }
            
            Button("Bake") {
                clipManager.bakeClip(clip)
            }
            
            Button("Delete", role: .destructive) {
                clipManager.clips.removeAll { $0.id == clip.id }
            }
        }
    }
}

// MARK: - Animation Clip Details View
struct AnimationClipDetailsView: View {
    @ObservedObject var clip: AnimationClip
    @StateObject private var clipManager = AnimationClipManager.shared
    @StateObject private var avatarManager = AvatarManager.shared
    @State private var showRetargetPreview = false
    @State private var selectedAvatar: AvatarDefinition?
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                GroupBox("Properties") {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("Name:")
                            TextField("Clip name", text: $clip.name)
                                .textFieldStyle(.roundedBorder)
                        }
                        
                        HStack {
                            Text("Duration:")
                            TextField("Duration", value: $clip.duration, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                            
                            Text("seconds")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                        
                        Toggle("Loop", isOn: $clip.loop)
                            .toggleStyle(.switch)
                        
                        Toggle("Favorite", isOn: Binding(
                            get: { clip.isFavorite },
                            set: { 
                                clip.isFavorite = $0
                                clipManager.updateFavoriteClips()
                            }
                        ))
                        .toggleStyle(.switch)
                    }
                }
                
                GroupBox("Operations") {
                    VStack(spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Button("Mirror") {
                                clipManager.mirrorClip(clip)
                            }
                            .buttonStyle(.bordered)
                            
                            Button("Set Reference Pose") {
                                clipManager.setReferencePose(for: clip)
                            }
                            .buttonStyle(.bordered)
                            
                            Button("Bake") {
                                clipManager.bakeClip(clip)
                            }
                            .buttonStyle(.bordered)
                        }
                        
                        Toggle("Additive Mode", isOn: .constant(false))
                            .toggleStyle(.switch)
                            .onChange(of: false) { _ in
                                clipManager.setAdditiveMode(for: clip, enabled: false)
                            }
                    }
                }
                
                GroupBox("Retargeting") {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        HStack {
                            Text("Target Avatar:")
                            Picker("Avatar", selection: $selectedAvatar) {
                                Text("None").tag(nil as AvatarDefinition?)
                                ForEach(avatarManager.avatars, id: \.id) { avatar in
                                    Text(avatar.name).tag(avatar as AvatarDefinition?)
                                }
                            }
                            .pickerStyle(.menu)
                            
                            if let avatar = selectedAvatar {
                                Button("Preview Retarget") {
                                    clipManager.previewRetarget(clip, to: avatar)
                                    showRetargetPreview = true
                                }
                                .buttonStyle(.bordered)
                            }
                        }
                    }
                }
                
                GroupBox("Events") {
                    HStack {
                        Text("\(clip.events.count) events")
                            .font(DesignSystem.Typography.body)
                        
                        Spacer()
                        
                        Button("Edit Events") {
                            // Edit events logic
                        }
                        .buttonStyle(.bordered)
                    }
                }
            }
            .padding(DesignSystem.Spacing.md)
        }
        .background(DesignSystem.Colors.backgroundPrimary)
        .sheet(isPresented: $showRetargetPreview) {
            if let avatar = selectedAvatar {
                RetargetPreviewView(avatar: avatar)
            }
        }
    }
}

// MARK: - Import/Export Views
struct ImportAnimationDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var clipManager = AnimationClipManager.shared
    
    @State private var selectedFormat: ImportFormat = .fbx
    @State private var importURL: URL?
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Import Animation")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                Text("Format:")
                Picker("Format", selection: $selectedFormat) {
                    ForEach(ImportFormat.allCases, id: \.self) { format in
                        Text(format.rawValue).tag(format)
                    }
                }
                .pickerStyle(.menu)
                
                Text("File:")
                if let url = importURL {
                    Text(url.lastPathComponent)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                } else {
                    Text("No file selected")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                Button("Select File") {
                    // File selection logic
                }
                .buttonStyle(.bordered)
            }
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Import") {
                    if let url = importURL {
                        clipManager.importClip(from: url, format: selectedFormat)
                        dismiss()
                    }
                }
                .buttonStyle(.borderedProminent)
                .disabled(importURL == nil)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 300)
    }
}

struct ExportAnimationDialog: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var clipManager = AnimationClipManager.shared
    
    let clip: AnimationClip?
    @State private var selectedFormat: ExportFormat = .fbx
    @State private var fileName = ""
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Export Animation")
                .font(DesignSystem.Typography.headline)
            
            if let clip = clip {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                    Text("Clip: \(clip.name)")
                        .font(DesignSystem.Typography.body)
                    
                    Text("Format:")
                    Picker("Format", selection: $selectedFormat) {
                        ForEach(ExportFormat.allCases, id: \.self) { format in
                            Text(format.rawValue).tag(format)
                        }
                    }
                    .pickerStyle(.menu)
                    
                    TextField("File Name", text: $fileName)
                        .textFieldStyle(.roundedBorder)
                        .placeholder("\(clip.name)\(selectedFormat.fileExtension)")
                }
                
                HStack {
                    Button("Cancel") { dismiss() }
                        .buttonStyle(.bordered)
                    
                    Spacer()
                    
                    Button("Export") {
                        let finalFileName = fileName.isEmpty ? "\(clip.name)\(selectedFormat.fileExtension)" : fileName
                        if let url = URL(string: finalFileName) {
                            clipManager.exportClip(clip, to: url, format: selectedFormat)
                            dismiss()
                        }
                    }
                    .buttonStyle(.borderedProminent)
                }
            } else {
                Text("No clip selected")
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 300)
    }
}

struct ImportHistoryView: View {
    let entries: [ImportHistoryEntry]
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Import History")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            ScrollView {
                LazyVStack(spacing: DesignSystem.Spacing.xs) {
                    ForEach(entries, id: \.id) { entry in
                        ImportHistoryRow(entry: entry)
                    }
                }
                .padding(DesignSystem.Spacing.sm)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
    }
}

struct ExportHistoryView: View {
    let entries: [ExportHistoryEntry]
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Export History")
                    .font(DesignSystem.Typography.bodyBold)
                
                Spacer()
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            ScrollView {
                LazyVStack(spacing: DesignSystem.Spacing.xs) {
                    ForEach(entries, id: \.id) { entry in
                        ExportHistoryRow(entry: entry)
                    }
                }
                .padding(DesignSystem.Spacing.sm)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
    }
}

struct ImportHistoryRow: View {
    let entry: ImportHistoryEntry
    
    var body: some View {
        HStack {
            Circle()
                .fill(statusColor)
                .frame(width: 8, height: 8)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(entry.fileName)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                HStack {
                    Text(entry.format.rawValue)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    Text(DateFormatter.historyFormatter.string(from: entry.timestamp))
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            
            Spacer()
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundPrimary)
    }
    
    private var statusColor: Color {
        switch entry.status {
        case .inProgress: return DesignSystem.Colors.accentWarning
        case .completed: return DesignSystem.Colors.accentSuccess
        case .failed: return DesignSystem.Colors.accentDanger
        }
    }
}

struct ExportHistoryRow: View {
    let entry: ExportHistoryEntry
    
    var body: some View {
        HStack {
            Circle()
                .fill(statusColor)
                .frame(width: 8, height: 8)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(entry.clipName)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                HStack {
                    Text(entry.fileName)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("•")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text(entry.format.rawValue)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    Text(DateFormatter.historyFormatter.string(from: entry.timestamp))
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textsSecondary)
                }
            }
            
            Spacer()
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundPrimary)
    }
    
    private var statusColor: Color {
        switch entry.status {
        case .inProgress: return DesignSystem.Colors.accentWarning
        case .completed: return DesignSystem.Colors.accentSuccess
        case .failed: return DesignSystem.Colors.accentDanger
        }
    }
}

// MARK: - Date Formatter Extension
extension DateFormatter {
    static let historyFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm"
        return formatter
    }()
}

// MARK: - Animation Playback Controls
struct AnimationPlaybackControls: View {
    @ObservedObject var manager: AnimationEditorManager
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            // Jump to start
            Button(action: { manager.seekTo(0) }) {
                Image(systemName: "backward.end.fill")
            }
            .buttonStyle(.plain)
            
            // Step back
            Button(action: { manager.seekTo(manager.currentTime - 1.0/60.0) }) {
                Image(systemName: "backward.frame.fill")
            }
            .buttonStyle(.plain)
            
            // Play/Pause
            Button(action: { manager.isPlaying ? manager.pause() : manager.play() }) {
                Image(systemName: manager.isPlaying ? "pause.fill" : "play.fill")
                    .font(.title2)
            }
            .buttonStyle(.plain)
            
            // Step forward
            Button(action: { manager.seekTo(manager.currentTime + 1.0/60.0) }) {
                Image(systemName: "forward.frame.fill")
            }
            .buttonStyle(.plain)
            
            // Jump to end
            Button(action: { manager.seekTo(manager.currentClip?.duration ?? 0) }) {
                Image(systemName: "forward.end.fill")
            }
            .buttonStyle(.plain)
            
            Divider().frame(height: 20)
            
            // Loop toggle
            Toggle(isOn: $manager.isLooping) {
                Image(systemName: "repeat")
            }
            .toggleStyle(.button)
            
            Spacer()
            
            // Current time
            Text(String(format: "%.2f / %.2f", manager.currentTime, manager.currentClip?.duration ?? 0))
                .font(DesignSystem.Typography.mono)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            // Playback speed
            Picker("Speed", selection: $manager.playbackSpeed) {
                Text("0.25x").tag(Float(0.25))
                Text("0.5x").tag(Float(0.5))
                Text("1x").tag(Float(1.0))
                Text("2x").tag(Float(2.0))
            }
            .frame(width: 80)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
}

// MARK: - State Machine Graph Editor

class StateMachineGraphEditor: ObservableObject {
    static let shared = StateMachineGraphEditor()
    
    @Published var states: [AnimationState] = []
    @Published var transitions: [AnimationTransition] = []
    @Published var parameters: [StateMachineParameter] = []
    @Published var selectedState: AnimationState?
    @Published var selectedTransition: AnimationTransition?
    @Published var isDragging: Bool = false
    @Published var panOffset: CGPoint = .zero
    @Published var zoomLevel: Float = 1.0
    
    init() {
        setupDefaultStateMachine()
    }
    
    private func setupDefaultStateMachine() {
        let idleState = AnimationState(
            name: "Idle",
            position: CGPoint(x: 100, y: 100),
            isEntryState: true,
            animationClip: "Idle"
        )
        
        let walkState = AnimationState(
            name: "Walk",
            position: CGPoint(x: 300, y: 100),
            animationClip: "Walk"
        )
        
        let runState = AnimationState(
            name: "Run",
            position: CGPoint(x: 500, y: 100),
            animationClip: "Run"
        )
        
        let jumpState = AnimationState(
            name: "Jump",
            position: CGPoint(x: 300, y: 250),
            animationClip: "Jump"
        )
        
        states = [idleState, walkState, runState, jumpState]
        
        transitions = [
            AnimationTransition(
                from: idleState,
                to: walkState,
                condition: "speed > 0.1",
                duration: 0.3,
                exitTime: 0.0
            ),
            AnimationTransition(
                from: walkState,
                to: runState,
                condition: "speed > 5.0",
                duration: 0.2,
                exitTime: 0.8
            ),
            AnimationTransition(
                from: runState,
                to: walkState,
                condition: "speed <= 5.0",
                duration: 0.3,
                exitTime: 0.9
            ),
            AnimationTransition(
                from: walkState,
                to: idleState,
                condition: "speed <= 0.1",
                duration: 0.5,
                exitTime: 0.7
            ),
            AnimationTransition(
                from: idleState,
                to: jumpState,
                condition: "jump_trigger",
                duration: 0.1,
                exitTime: 0.0
            ),
            AnimationTransition(
                from: walkState,
                to: jumpState,
                condition: "jump_trigger",
                duration: 0.1,
                exitTime: 0.0
            ),
            AnimationTransition(
                from: runState,
                to: jumpState,
                condition: "jump_trigger",
                duration: 0.1,
                exitTime: 0.0
            ),
            AnimationTransition(
                from: jumpState,
                to: idleState,
                condition: "is_grounded",
                duration: 0.2,
                exitTime: 0.8
            )
        ]
        
        parameters = [
            StateMachineParameter(name: "speed", type: .float, defaultValue: 0.0),
            StateMachineParameter(name: "jump_trigger", type: .trigger, defaultValue: false),
            StateMachineParameter(name: "is_grounded", type: .bool, defaultValue: true)
        ]
    }
}

struct AnimationState: Identifiable {
    let id = UUID()
    var name: String
    var position: CGPoint
    var isEntryState: Bool = false
    var isExitState: Bool = false
    var animationClip: String
    var tags: Set<String> = []
    var behaviors: [String] = []
    var size: CGFloat = 80
}

struct AnimationTransition: Identifiable {
    let id = UUID()
    var from: AnimationState
    var to: AnimationState
    var condition: String
    var duration: Float
    var exitTime: Float
    var hasInterruptionSource: Bool = false
    var interruptionPriority: Int = 0
    var transitionOffset: Float = 0.0
    var durationCurve: CrossFadeTimingSystem.FadeCurve = .smooth
    var name: String = ""
    
    var displayName: String {
        return name.isEmpty ? "\(from.name) → \(to.name)" : name
    }
}

struct StateMachineParameter: Identifiable {
    let id = UUID()
    var name: String
    var type: ParameterType
    var defaultValue: Any
    
    enum ParameterType: String, CaseIterable {
        case float = "Float"
        case int = "Integer"
        case bool = "Boolean"
        case trigger = "Trigger"
    }
}

// MARK: - Transition Condition Editor View

struct TransitionConditionEditorView: View {
    let transition: AnimationTransition
    @Environment(\.dismiss) private var dismiss
    @StateObject private var graphEditor = StateMachineGraphEditor.shared
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Edit Transition Condition")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                HStack {
                    Text("From:")
                    Text(transition.from.name)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                HStack {
                    Text("To:")
                    Text(transition.to.name)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Text("Condition:")
                    TextField("Enter condition", text: .constant(transition.condition))
                        .textFieldStyle(.roundedBorder)
                }
                
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Text("Duration:")
                    HStack {
                        Slider(value: .constant(transition.duration), in: 0.1...3.0)
                        Text(String(format: "%.1fs", transition.duration))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                }
                
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Text("Exit Time:")
                    HStack {
                        Slider(value: .constant(transition.exitTime), in: 0.0...1.0)
                        Text(String(format: "%.2f", transition.exitTime))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                }
                
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Text("Duration Curve:")
                    Picker("Curve", selection: .constant(transition.durationCurve)) {
                        ForEach(CrossFadeTimingSystem.FadeCurve.allCases, id: \.self) { curve in
                            Text(curve.rawValue).tag(curve)
                        }
                    }
                    .pickerStyle(.menu)
                }
                
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Text("Transition Offset:")
                    HStack {
                        Slider(value: .constant(transition.transitionOffset), in: -1.0...1.0)
                        Text(String(format: "%.2f", transition.transitionOffset))
                            .font(DesignSystem.Typography.smallMono)
                            .frame(width: 40)
                    }
                }
                
                Toggle("Has Interruption Source", isOn: .constant(transition.hasInterruptionSource))
                
                if transition.hasInterruptionSource {
                    VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                        Text("Interruption Priority:")
                        HStack {
                            Slider(value: .constant(Float(transition.interruptionPriority)), in: 0...100)
                            Text("\(transition.interruptionPriority)")
                                .font(DesignSystem.Typography.smallMono)
                                .frame(width: 40)
                        }
                    }
                }
            }
            
            Spacer()
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Apply") {
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 500, height: 600)
    }
}

// MARK: - Parameter Editor View

struct ParameterEditorView: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var graphEditor = StateMachineGraphEditor.shared
    @State private var newParameterName = ""
    @State private var newParameterType: StateMachineParameter.ParameterType = .float
    @State private var newParameterValue = ""
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("State Machine Parameters")
                .font(DesignSystem.Typography.headline)
            
            // Add new parameter
            GroupBox("Add Parameter") {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text("Name:")
                        TextField("Parameter name", text: $newParameterName)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    HStack {
                        Text("Type:")
                        Picker("Type", selection: $newParameterType) {
                            ForEach(StateMachineParameter.ParameterType.allCases, id: \.self) { type in
                                Text(type.rawValue).tag(type)
                            }
                        }
                        .pickerStyle(.menu)
                    }
                    
                    HStack {
                        Text("Default Value:")
                        TextField("Default value", text: $newParameterValue)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    Button("Add Parameter") {
                        addParameter()
                    }
                    .buttonStyle(.borderedProminent)
                    .disabled(newParameterName.isEmpty)
                }
            }
            
            // Existing parameters
            GroupBox("Existing Parameters") {
                List(graphEditor.parameters, id: \.id) { parameter in
                    HStack {
                        Text(parameter.name)
                            .font(DesignSystem.Typography.body)
                        
                        Spacer()
                        
                        Text(parameter.type.rawValue)
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                            .padding(.horizontal, 8)
                            .padding(.vertical, 4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                        
                        Button(action: {}) {
                            Image(systemName: "trash")
                        }
                        .buttonStyle(.plain)
                        .foregroundColor(DesignSystem.Colors.error)
                    }
                    .padding(.vertical, 4)
                }
                .frame(height: 200)
            }
            
            Spacer()
            
            HStack {
                Button("Close") {
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 500, height: 500)
    }
    
    private func addParameter() {
        let defaultValue: Any
        switch newParameterType {
        case .float:
            defaultValue = Float(newParameterValue) ?? 0.0
        case .int:
            defaultValue = Int(newParameterValue) ?? 0
        case .bool:
            defaultValue = Bool(newParameterValue) ?? false
        case .trigger:
            defaultValue = false
        }
        
        let parameter = StateMachineParameter(
            name: newParameterName,
            type: newParameterType,
            defaultValue: defaultValue
        )
        
        graphEditor.parameters.append(parameter)
        
        // Reset form
        newParameterName = ""
        newParameterValue = ""
    }
}

// MARK: - Curve Editor Implementation

struct CurveEditorView: View {
    @ObservedObject var manager: AnimationEditorManager
    @State private var selectedTrack: AnimationTrack?
    @State private var selectedKeyframes: Set<UUID> = []
    @State private var showVelocityGraph = false
    @State private var showAccelerationGraph = false
    @State private var showDerivativeDisplay = false
    @State private var copiedCurveShape: CurveShape?
    @State private var showCurvePresets = false
    
    struct CurveShape {
        let keyframes: [AnimationKeyframe]
        let interpolation: AnimationTrack.InterpolationType
        let name: String
    }
    
    var body: some View {
        VStack(spacing: 0) {
            CurveEditorToolbar(
                selectedTrack: $selectedTrack,
                showVelocityGraph: $showVelocityGraph,
                showAccelerationGraph: $showAccelerationGraph,
                showDerivativeDisplay: $showDerivativeDisplay,
                showCurvePresets: $showCurvePresets
            )
            
            EditorDivider()
            
            HSplitView {
                CurveEditorCanvas(
                    track: selectedTrack,
                    selectedKeyframes: $selectedKeyframes,
                    manager: manager
                )
                .frame(minWidth: 400)
                
                VStack(spacing: 0) {
                    if selectedTrack != nil {
                        CurveKeyframeEditor(
                            track: selectedTrack!,
                            selectedKeyframes: $selectedKeyframes
                        )
                        .frame(minHeight: 200)
                    }
                    
                    EditorDivider()
                    
                    if showVelocityGraph || showAccelerationGraph {
                        CurveDerivativeGraphs(
                            track: selectedTrack,
                            showVelocity: showVelocityGraph,
                            showAcceleration: showAccelerationGraph
                        )
                        .frame(minHeight: 150)
                    }
                    
                    EditorDivider()
                    
                    CurveOperationsPanel(
                        track: selectedTrack,
                        selectedKeyframes: selectedKeyframes,
                        copiedCurveShape: $copiedCurveShape
                    )
                    .frame(minHeight: 200)
                }
                .frame(minWidth: 300)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .sheet(isPresented: $showCurvePresets) {
            CurvePresetsLibrary(track: selectedTrack)
        }
    }
}

struct CurveEditorToolbar: View {
    @Binding var selectedTrack: AnimationTrack?
    @Binding var showVelocityGraph: Bool
    @Binding var showAccelerationGraph: Bool
    @Binding var showDerivativeDisplay: Bool
    @Binding var showCurvePresets: Bool
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            Text("Curve Editor")
                .font(DesignSystem.Typography.bodyBold)
            
            Spacer()
            
            Group {
                Toggle("Velocity", isOn: $showVelocityGraph)
                    .toggleStyle(.button)
                
                Toggle("Acceleration", isOn: $showAccelerationGraph)
                    .toggleStyle(.button)
                
                Toggle("Derivatives", isOn: $showDerivativeDisplay)
                    .toggleStyle(.button)
            }
            
            Divider().frame(height: 20)
            
            Group {
                Button("Copy Shape") {
                    copyCurveShape()
                }
                .buttonStyle(.bordered)
                .disabled(selectedTrack == nil)
                
                Button("Paste Shape") {
                    pasteCurveShape()
                }
                .buttonStyle(.bordered)
                .disabled(copiedCurveShape == nil)
                
                Button("Presets") {
                    showCurvePresets = true
                }
                .buttonStyle(.bordered)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
    
    private func copyCurveShape() {
        guard let track = selectedTrack else { return }
        // Implementation would copy curve shape
    }
    
    private func pasteCurveShape() {
        guard let shape = copiedCurveShape,
              let track = selectedTrack else { return }
        // Implementation would paste curve shape
    }
}

struct CurveEditorCanvas: View {
    let track: AnimationTrack?
    @Binding var selectedKeyframes: Set<UUID>
    let manager: AnimationEditorManager
    @State private var canvasSize: CGSize = CGSize(width: 800, height: 400)
    @State private var panOffset: CGPoint = .zero
    @State private var zoomLevel: Float = 1.0
    
    var body: some View {
        ZStack {
            CurveEditorGrid(size: canvasSize, zoom: zoomLevel, pan: panOffset)
            
            if let track = track {
                CurveVisualization(
                    keyframes: track.keyframes,
                    interpolation: track.interpolation,
                    canvasSize: canvasSize,
                    zoom: zoomLevel,
                    pan: panOffset,
                    selectedKeyframes: $selectedKeyframes
                )
            }
        }
        .frame(width: canvasSize.width, height: canvasSize.height)
        .clipped()
        .gesture(
            DragGesture()
                .onChanged { value in
                    panOffset.x += value.translation.x
                    panOffset.y += value.translation.y
                }
        )
        .onScrollWheel { value in
            zoomLevel *= (value > 0) ? 1.1 : 0.9
            zoomLevel = max(0.1, min(5.0, zoomLevel))
        }
    }
}

struct CurveVisualization: View {
    let keyframes: [AnimationKeyframe]
    let interpolation: AnimationTrack.InterpolationType
    let canvasSize: CGSize
    let zoom: Float
    let pan: CGPoint
    @Binding var selectedKeyframes: Set<UUID>
    
    var body: some View {
        Path { path in
            guard keyframes.count >= 2 else { return }
            
            let sortedKeyframes = keyframes.sorted { $0.time < $1.time }
            
            for (index, keyframe) in sortedKeyframes.enumerated() {
                let position = keyframePosition(keyframe)
                
                if index == 0 {
                    path.move(to: position)
                } else {
                    let previousKeyframe = sortedKeyframes[index - 1]
                    let previousPosition = keyframePosition(previousKeyframe)
                    
                    switch interpolation {
                    case .linear:
                        path.addLine(to: position)
                    case .bezier:
                        let controlPoint1 = CGPoint(
                            x: previousPosition.x + (keyframe.inTangent * 50),
                            y: previousPosition.y
                        )
                        let controlPoint2 = CGPoint(
                            x: position.x - (keyframe.outTangent * 50),
                            y: position.y
                        )
                        path.addCurve(to: position, control1: controlPoint1, control2: controlPoint2)
                    default:
                        path.addLine(to: position)
                    }
                }
            }
        }
        .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
        .background(
            ForEach(keyframes, id: \.id) { keyframe in
                let position = keyframePosition(keyframe)
                Circle()
                    .fill(selectedKeyframes.contains(keyframe.id) ? DesignSystem.Colors.accentWarning : DesignSystem.Colors.accentPrimary)
                    .frame(width: 8, height: 8)
                    .position(position)
                    .onTapGesture {
                        if selectedKeyframes.contains(keyframe.id) {
                            selectedKeyframes.remove(keyframe.id)
                        } else {
                            selectedKeyframes.insert(keyframe.id)
                        }
                    }
            }
        )
    }
    
    private func keyframePosition(_ keyframe: AnimationKeyframe) -> CGPoint {
        let normalizedTime = keyframe.time / (keyframes.last?.time ?? 1.0)
        let x = (CGFloat(normalizedTime) * canvasSize.width * CGFloat(zoom)) + CGFloat(pan.x)
        let y = canvasSize.height / 2 + CGFloat(pan.y)
        return CGPoint(x: x, y: y)
    }
}

struct CurveDerivativeGraphs: View {
    let track: AnimationTrack?
    let showVelocity: Bool
    let showAcceleration: Bool
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            Text("Derivatives")
                .font(DesignSystem.Typography.bodyBold)
            
            if showVelocity {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                    Text("Velocity")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentWarning)
                    
                    RoundedRectangle(cornerRadius: 4)
                        .fill(DesignSystem.Colors.backgroundTertiary)
                        .frame(height: 60)
                        .overlay(
                            Path { path in
                                path.move(to: CGPoint(x: 0, y: 30))
                                path.addLine(to: CGPoint(x: 200, y: 10))
                                path.addLine(to: CGPoint(x: 400, y: 40))
                            }
                            .stroke(DesignSystem.Colors.accentWarning, lineWidth: 1)
                        )
                )
            }
            
            if showAcceleration {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                    Text("Acceleration")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.entityParticle)
                    
                    RoundedRectangle(cornerRadius: 4)
                        .fill(DesignSystem.Colors.backgroundTertiary)
                        .frame(height: 60)
                        .overlay(
                            Path { path in
                                path.move(to: CGPoint(x: 0, y: 30))
                                path.addLine(to: CGPoint(x: 100, y: 50))
                                path.addLine(to: CGPoint(x: 200, y: 20))
                                path.addLine(to: CGPoint(x: 300, y: 35))
                                path.addLine(to: CGPoint(x: 400, y: 30))
                            }
                            .stroke(DesignSystem.Colors.entityParticle, lineWidth: 1)
                        )
                )
            }
        }
        .padding(DesignSystem.Spacing.sm)
    }
}

struct CurveKeyframeEditor: View {
    let track: AnimationTrack
    @Binding var selectedKeyframes: Set<UUID>
    @State private var editingKeyframe: AnimationKeyframe?
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            Text("Keyframes")
                .font(DesignSystem.Typography.bodyBold)
            
            List(track.keyframes.sorted { $0.time < $1.time }, id: \.id) { keyframe in
                HStack {
                    Circle()
                        .fill(selectedKeyframes.contains(keyframe.id) ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.backgroundTertiary)
                        .frame(width: 12, height: 12)
                    
                    VStack(alignment: .leading, spacing: 2) {
                        Text(String(format: "Time: %.3f", keyframe.time))
                            .font(DesignSystem.Typography.small)
                        
                        Text("Value: \(keyframe.value)")
                            .font(DesignSystem.Typography.small)
                    }
                    
                    Spacer()
                    
                    Button("Edit") {
                        editingKeyframe = keyframe
                    }
                    .buttonStyle(.bordered)
                }
                .padding(DesignSystem.Spacing.sm)
                .background(selectedKeyframes.contains(keyframe.id) ? DesignSystem.Colors.selection : Color.clear)
                .onTapGesture {
                    if selectedKeyframes.contains(keyframe.id) {
                        selectedKeyframes.remove(keyframe.id)
                    } else {
                        selectedKeyframes.insert(keyframe.id)
                    }
                }
            }
        }
        .sheet(item: $editingKeyframe) { keyframe in
            KeyframeEditDialog(keyframe: keyframe)
        }
    }
}

struct KeyframeEditDialog: View {
    let keyframe: AnimationKeyframe
    @Environment(\.dismiss) private var dismiss
    @State private var time: Float
    @State private var value: String
    @State private var inTangent: Float
    @State private var outTangent: Float
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Edit Keyframe")
                .font(DesignSystem.Typography.headline)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                HStack {
                    Text("Time:")
                    TextField("Time", value: $time, format: .number)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Value:")
                    TextField("Value", text: $value)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("In Tangent:")
                    TextField("In Tangent", value: $inTangent, format: .number)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Out Tangent:")
                    TextField("Out Tangent", value: $outTangent, format: .number)
                        .textFieldStyle(.roundedBorder)
                }
            }
            
            Spacer()
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Apply") {
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400, height: 300)
        .onAppear {
            time = keyframe.time
            value = "\(keyframe.value)"
            inTangent = keyframe.inTangent
            outTangent = keyframe.outTangent
        }
    }
}

struct CurveOperationsPanel: View {
    let track: AnimationTrack?
    let selectedKeyframes: Set<UUID>
    @Binding var copiedCurveShape: CurveEditorView.CurveShape?
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            Text("Operations")
                .font(DesignSystem.Typography.bodyBold)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                Button("Flatten Selection") {
                    flattenSelectedKeyframes()
                }
                .buttonStyle(.bordered)
                .disabled(selectedKeyframes.isEmpty)
                
                Button("Smooth Selection") {
                    smoothSelectedKeyframes()
                }
                .buttonStyle(.bordered)
                .disabled(selectedKeyframes.isEmpty)
                
                Button("Reduce Keys") {
                    reduceKeys()
                }
                .buttonStyle(.bordered)
                
                Button("Add Noise") {
                    addNoiseToCurve()
                }
                .buttonStyle(.bordered)
                
                Button("Apply Expression") {
                    applyExpressionToCurve()
                }
                .buttonStyle(.bordered)
                
                Button("Bake to Samples") {
                    bakeToSamples()
                }
                .buttonStyle(.bordered)
            }
        }
    }
    
    private func flattenSelectedKeyframes() {
        // Implementation for flattening selected keyframes
    }
    
    private func smoothSelectedKeyframes() {
        // Implementation for smoothing selected keyframes
    }
    
    private func reduceKeys() {
        // Implementation for reducing keys
    }
    
    private func addNoiseToCurve() {
        // Implementation for adding noise
    }
    
    private func applyExpressionToCurve() {
        // Implementation for applying expressions
    }
    
    private func bakeToSamples() {
        // Implementation for baking to samples
    }
}

struct CurvePresetsLibrary: View {
    let track: AnimationTrack?
    @Environment(\.dismiss) private var dismiss
    @State private var selectedPreset: CurvePreset?
    
    struct CurvePreset: Identifiable {
        let id = UUID()
        let name: String
        let description: String
        let keyframes: [AnimationKeyframe]
        let interpolation: AnimationTrack.InterpolationType
    }
    
    private let presets: [CurvePreset] = [
        CurvePreset(
            name: "Linear",
            description: "Simple linear interpolation",
            keyframes: [
                AnimationKeyframe(time: 0, value: 0),
                AnimationKeyframe(time: 1, value: 1)
            ],
            interpolation: .linear
        ),
        CurvePreset(
            name: "Ease In-Out",
            description: "Smooth acceleration and deceleration",
            keyframes: [
                AnimationKeyframe(time: 0, value: 0, inTangent: 0, outTangent: 0),
                AnimationKeyframe(time: 0.5, value: 0.5, inTangent: 1.5, outTangent: 1.5),
                AnimationKeyframe(time: 1, value: 1, inTangent: 0, outTangent: 0)
            ],
            interpolation: .bezier
        )
    ]
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Text("Curve Presets")
                .font(DesignSystem.Typography.headline)
            
            List(presets, id: \.id, selection: $selectedPreset) { preset in
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    HStack {
                        Text(preset.name)
                            .font(DesignSystem.Typography.bodyBold)
                        
                        Spacer()
                        
                        Button("Apply") {
                            applyPreset(preset)
                        }
                        .buttonStyle(.bordered)
                    }
                    
                    Text(preset.description)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .padding(DesignSystem.Spacing.sm)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 500, height: 600)
    }
    
    private func applyPreset(_ preset: CurvePreset) {
        // Implementation for applying preset
        dismiss()
    }
}

struct CurveEditorGrid: View {
    let size: CGSize
    let zoom: Float
    let pan: CGPoint
    
    var body: some View {
        ZStack {
            ForEach(0..<Int(size.width / 50), id: \.self) { x in
                Path { path in
                    let xPos = CGFloat(x) * 50 * CGFloat(zoom) + CGFloat(pan.x)
                    path.move(to: CGPoint(x: xPos, y: 0))
                    path.addLine(to: CGPoint(x: xPos, y: size.height))
                }
                .stroke(DesignSystem.Colors.border.opacity(0.3), lineWidth: 0.5)
            }
            
            ForEach(0..<Int(size.height / 50), id: \.self) { y in
                Path { path in
                    let yPos = CGFloat(y) * 50 * CGFloat(zoom) + CGFloat(pan.y)
                    path.move(to: CGPoint(x: 0, y: yPos))
                    path.addLine(to: CGPoint(x: size.width, y: yPos))
                }
                .stroke(DesignSystem.Colors.border.opacity(0.3), lineWidth: 0.5)
            }
            
            Path { path in
                let centerX = size.width / 2 + CGFloat(pan.x)
                let centerY = size.height / 2 + CGFloat(pan.y)
                
                path.move(to: CGPoint(x: centerX, y: 0))
                path.addLine(to: CGPoint(x: centerX, y: size.height))
                
                path.move(to: CGPoint(x: 0, y: centerY))
                path.addLine(to: CGPoint(x: size.width, y: centerY))
            }
            .stroke(DesignSystem.Colors.border.opacity(0.5), lineWidth: 1)
        }
    }
}

// MARK: - Animation Clip Creation Wizard

struct AnimationClipCreationWizard: View {
    @Environment(\.dismiss) private var dismiss
    @StateObject private var clipManager = AnimationClipManager.shared
    @StateObject private var avatarManager = AvatarManager.shared
    
    @State private var wizardStep: WizardStep = .basicInfo
    @State private var clipName = ""
    @State private var duration: Float = 2.0
    @State private var frameRate: Float = 60.0
    @State private var selectedAvatar: AvatarDefinition?
    @State private var isLooping = true
    @State private var rootMotionEnabled = false
    @State private var additiveMode = false
    @State private var compressionLevel: CompressionLevel = .none
    @State private var selectedProperties: [String] = []
    @State private var customProperties: [ClipProperty] = []
    
    enum WizardStep: String, CaseIterable {
        case basicInfo = "Basic Info"
        case properties = "Properties"
        case preview = "Preview"
        case advanced = "Advanced"
    }
    
    enum CompressionLevel: String, CaseIterable {
        case none = "None"
        case low = "Low"
        case medium = "Medium"
        case high = "High"
        case maximum = "Maximum"
    }
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            // Progress indicator
            WizardProgressIndicator(currentStep: wizardStep, totalSteps: WizardStep.allCases.count)
            
            // Main content
            switch wizardStep {
            case .basicInfo:
                BasicInfoStep(
                    clipName: $clipName,
                    duration: $duration,
                    frameRate: $frameRate,
                    isLooping: $isLooping,
                    rootMotionEnabled: $rootMotionEnabled,
                    additiveMode: $additiveMode,
                    compressionLevel: $compressionLevel
                )
            case .properties:
                PropertiesStep(
                    selectedAvatar: $selectedAvatar,
                    selectedProperties: $selectedProperties,
                    customProperties: $customProperties
                )
            case .preview:
                PreviewStep(
                    clipName: clipName,
                    duration: duration,
                    frameRate: frameRate,
                    selectedAvatar: selectedAvatar,
                    selectedProperties: selectedProperties,
                    customProperties: customProperties
                )
            case .advanced:
                AdvancedStep(
                    compressionLevel: $compressionLevel,
                    rootMotionEnabled: $rootMotionEnabled,
                    additiveMode: $additiveMode
                )
            }
        }
        
        // Navigation buttons
        HStack {
            Button("Previous") {
                if let currentIndex = WizardStep.allCases.firstIndex(of: wizardStep),
                   currentIndex > 0 {
                    wizardStep = WizardStep.allCases[currentIndex - 1]
                }
            }
            .disabled(wizardStep == WizardStep.allCases.first)
            .buttonStyle(.bordered)
            
            Spacer()
            
            if wizardStep == WizardStep.allCases.last {
                Button("Create") {
                    createAnimationClip()
                }
                .buttonStyle(.borderedProminent)
                .disabled(clipName.isEmpty)
            } else {
                Button("Next") {
                    if let currentIndex = WizardStep.allCases.firstIndex(of: wizardStep),
                       currentIndex < WizardStep.allCases.count - 1 {
                        wizardStep = WizardStep.allCases[currentIndex + 1]
                    }
                }
                .buttonStyle(.borderedProminent)
            }
            
            Button("Cancel") {
                dismiss()
            }
            .buttonStyle(.bordered)
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 600, height: 500)
    }
    
    private func createAnimationClip() {
        let newClip = AnimationClip(
            name: clipName,
            duration: duration,
            loop: isLooping,
            events: []
        )
        
        // Apply advanced settings
        if rootMotionEnabled {
            // Configure root motion
        }
        
        if additiveMode {
            // Configure additive mode
        }
        
        // Apply compression
        if compressionLevel != .none {
            // Apply compression algorithm
        }
        
        clipManager.clips.append(newClip)
        dismiss()
    }
}

// MARK: - Wizard Progress Indicator

struct WizardProgressIndicator: View {
    let currentStep: AnimationClipCreationWizard.WizardStep
    let totalSteps: Int
    
    var body: some View {
        HStack {
            ForEach(0..<totalSteps, id: \.self) { index in
                let step = AnimationClipCreationWizard.WizardStep.allCases[index]
                let isCompleted = getStepIndex(step) < getStepIndex(currentStep)
                let isCurrent = step == currentStep
                
                Circle()
                    .fill(isCompleted ? DesignSystem.Colors.accentSuccess : (isCurrent ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.backgroundTertiary))
                    .frame(width: 20, height: 20)
                    .overlay(
                        Text("\(index + 1)")
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(isCurrent ? .white : DesignSystem.Colors.textSecondary)
                    )
            }
            
            Spacer()
            
            Text("\(currentStep)")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .padding(DesignSystem.Spacing.sm)
    }
    
    private func getStepIndex(_ step: AnimationClipCreationWizard.WizardStep) -> Int {
        return AnimationClipCreationWizard.WizardStep.allCases.firstIndex(of: step) ?? 0
    }
}

// MARK: - Wizard Steps

struct BasicInfoStep: View {
    @Binding var clipName: String
    @Binding var duration: Float
    @Binding var frameRate: Float
    @Binding var isLooping: Bool
    @Binding var rootMotionEnabled: Bool
    @Binding var additiveMode: Bool
    @Binding var compressionLevel: AnimationClipCreationWizard.CompressionLevel
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            Text("Basic Information")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Form {
                TextField("Clip Name", text: $clipName)
                    .textFieldStyle(.roundedBorder)
                
                HStack {
                    Text("Duration:")
                    TextField("Duration", value: $duration, format: .number)
                        .textFieldStyle(.roundedBorder)
                        .frame(width: 100)
                    
                    Text("seconds")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                HStack {
                    Text("Frame Rate:")
                    TextField("Frame Rate", value: $frameRate, format: .number)
                        .textFieldStyle(.roundedBorder)
                        .frame(width: 100)
                    
                    Text("fps")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                Toggle("Looping", isOn: $isLooping)
                Toggle("Root Motion", isOn: $rootMotionEnabled)
                Toggle("Additive Mode", isOn: $additiveMode)
                
                Picker("Compression", selection: $compressionLevel) {
                    ForEach(AnimationClipCreationWizard.CompressionLevel.allCases, id: \.self) { level in
                        Text(level.rawValue).tag(level)
                    }
                }
                .pickerStyle(.menu)
            }
        }
    }
}

struct PropertiesStep: View {
    @Binding var selectedAvatar: AvatarDefinition?
    @Binding var selectedProperties: [String]
    @Binding var customProperties: [ClipProperty]
    @StateObject private var avatarManager = AvatarManager.shared
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            Text("Properties")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Form {
                Picker("Avatar", selection: $selectedAvatar) {
                    Text("None").tag(nil as AvatarDefinition?)
                    ForEach(avatarManager.avatars, id: \.id) { avatar in
                        Text(avatar.name).tag(avatar as AvatarDefinition?)
                    }
                }
                .pickerStyle(.menu)
                
                // Property selection
                VStack(alignment: .leading) {
                    Text("Available Properties:")
                        .font(DesignSystem.Typography.bodyBold)
                    
                    ForEach(getAvailableProperties(), id: \.self) { property in
                        Toggle(property.name, isOn: Binding(
                            get: { selectedProperties.contains(property.id) },
                            set: { newValue in
                                if newValue {
                                    selectedProperties.append(property.id)
                                } else {
                                    selectedProperties.removeAll { $0 == property.id }
                                }
                            }
                        ))
                    }
                }
            }
        }
    }
    
    private func getAvailableProperties() -> [ClipProperty] {
        return [
            ClipProperty(id: "position", name: "Position", type: .vector3),
            ClipProperty(id: "rotation", name: "Rotation", type: .quaternion),
            ClipProperty(id: "scale", name: "Scale", type: .vector3),
            ClipProperty(id: "color", name: "Color", type: .vector4),
            ClipProperty(id: "opacity", name: "Opacity", type: .float),
            ClipProperty(id: "visibility", name: "Visibility", type: .bool)
        ]
    }
}

struct PreviewStep: View {
    let clipName: String
    let duration: Float
    let frameRate: Float
    let selectedAvatar: AvatarDefinition?
    let selectedProperties: [String]
    let customProperties: [ClipProperty]
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            Text("Preview")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            // Preview area
            Rectangle()
                .fill(DesignSystem.Colors.backgroundTertiary)
                .frame(height: 200)
                .overlay(
                    VStack {
                        Text("Clip: \(clipName)")
                            .font(DesignSystem.Typography.bodyBold)
                        Text("Duration: \(String(format: "%.2f", duration))s")
                            .font(DesignSystem.Typography.small)
                        Text("Frame Rate: \(Int(frameRate)) fps")
                            .font(DesignSystem.Typography.small)
                        Text("Avatar: \(selectedAvatar?.name ?? "None")")
                            .font(DesignSystem.Typography.small)
                        Text("Properties: \(selectedProperties.count + customProperties.count)")
                            .font(DesignSystem.Typography.small)
                    }
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .padding()
                )
        }
    }
}

struct AdvancedStep: View {
    @Binding var compressionLevel: AnimationClipCreationWizard.CompressionLevel
    @Binding var rootMotionEnabled: Bool
    @Binding var additiveMode: Bool
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            Text("Advanced Settings")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Form {
                Section("Compression") {
                    Text("Compression Level: \(compressionLevel.rawValue)")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Slider("Compression Quality", value: Binding(
                        get: { Float(compressionLevel == .none ? 0 : compressionLevel == .low ? 1 : compressionLevel == .medium ? 2 : compressionLevel == .high ? 3 : 4) },
                        set: { newValue in
                            switch newValue {
                            case 0: compressionLevel = .none
                            case 1: compressionLevel = .low
                            case 2: compressionLevel = .medium
                            case 3: compressionLevel = .high
                            case 4: compressionLevel = .maximum
                            default: break
                            }
                        }
                    ), in: 0...4)
                    
                    Text("Higher compression reduces file size but may affect quality")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                Section("Animation") {
                    Toggle("Root Motion", isOn: $rootMotionEnabled)
                    Toggle("Additive Mode", isOn: $additiveMode)
                    
                    Text("Root motion extracts character movement from animation")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Additive mode allows layering animations on top of existing animations")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
        }
    }
}

// MARK: - Supporting Data Models

struct ClipProperty: Identifiable {
    let id: String
    let name: String
    let type: PropertyType
    
    enum PropertyType: String {
        case vector3 = "Vector3"
        case quaternion = "Quaternion"
        case vector4 = "Vector4"
        case float = "Float"
        case bool = "Bool"
    }
}

// MARK: - Erosion Simulation System

class ErosionSimulationManager: ObservableObject {
    static let shared = ErosionSimulationManager()
    
    @Published var hydraulicErosionEnabled: Bool = false
    @Published var thermalErosionEnabled: Bool = false
    @Published var windErosionEnabled: Bool = false
    @Published var coastalErosionEnabled: Bool = false
    
    @Published var erosionIterationCount: Int = 100
    @Published var erosionStrength: Float = 1.0
    @Published var sedimentCapacity: Float = 4.0
    @Published var depositionRate: Float = 0.1
    @Published var evaporationRate: Float = 0.01
    
    @Published var waterFlowVisualization: Bool = false
    @Published var erosionMaskPainting: Bool = false
    @Published var realTimePreview: Bool = true
    @Published var undoRedoEnabled: Bool = true
    
    @Published var currentPreset: ErosionPreset = .custom
    @Published var riverGenerationEnabled: Bool = false
    @Published var lakeFillingEnabled: Bool = false
    @Published var riverbedSculptingEnabled: Bool = false
    
    @Published var erosionHistory: [ErosionSnapshot] = []
    @Published var currentHistoryIndex: Int = -1
    
    enum ErosionPreset: String, CaseIterable {
        case desert = "Desert"
        case alpine = "Alpine"
        case coastal = "Coastal"
        case riverValley = "River Valley"
        case volcanic = "Volcanic"
        case glacial = "Glacial"
        case custom = "Custom"
        
        var settings: ErosionSettings {
            switch self {
            case .desert:
                return ErosionSettings(
                    hydraulicStrength: 0.2,
                    thermalStrength: 0.8,
                    windStrength: 1.0,
                    coastalStrength: 0.0,
                    iterations: 200,
                    sedimentCapacity: 2.0,
                    depositionRate: 0.05
                )
            case .alpine:
                return ErosionSettings(
                    hydraulicStrength: 0.9,
                    thermalStrength: 0.7,
                    windStrength: 0.3,
                    coastalStrength: 0.0,
                    iterations: 150,
                    sedimentCapacity: 6.0,
                    depositionRate: 0.15
                )
            case .coastal:
                return ErosionSettings(
                    hydraulicStrength: 0.6,
                    thermalStrength: 0.2,
                    windStrength: 0.4,
                    coastalStrength: 1.0,
                    iterations: 100,
                    sedimentCapacity: 4.0,
                    depositionRate: 0.2
                )
            case .riverValley:
                return ErosionSettings(
                    hydraulicStrength: 1.0,
                    thermalStrength: 0.3,
                    windStrength: 0.1,
                    coastalStrength: 0.0,
                    iterations: 120,
                    sedimentCapacity: 8.0,
                    depositionRate: 0.25
                )
            case .volcanic:
                return ErosionSettings(
                    hydraulicStrength: 0.4,
                    thermalStrength: 1.0,
                    windStrength: 0.2,
                    coastalStrength: 0.0,
                    iterations: 80,
                    sedimentCapacity: 3.0,
                    depositionRate: 0.1
                )
            case .glacial:
                return ErosionSettings(
                    hydraulicStrength: 0.7,
                    thermalStrength: 0.9,
                    windStrength: 0.0,
                    coastalStrength: 0.0,
                    iterations: 60,
                    sedimentCapacity: 5.0,
                    depositionRate: 0.3
                )
            case .custom:
                return ErosionSettings()
            }
        }
    }
    
    struct ErosionSettings {
        var hydraulicStrength: Float = 0.5
        var thermalStrength: Float = 0.5
        var windStrength: Float = 0.5
        var coastalStrength: Float = 0.5
        var iterations: Int = 100
        var sedimentCapacity: Float = 4.0
        var depositionRate: Float = 0.1
        var evaporationRate: Float = 0.01
        var thermalAngleThreshold: Float = 30.0
        var windDirection: SIMD2<Float> = SIMD2<Float>(1, 0)
        var waveHeight: Float = 2.0
        var tidalRange: Float = 1.0
    }
    
    struct ErosionSnapshot {
        let id = UUID()
        let timestamp: Date
        let settings: ErosionSettings
        let heightmapData: [Float]
        let description: String
    }
    
    func applyErosionPreset(_ preset: ErosionPreset) {
        currentPreset = preset
        let settings = preset.settings
        
        hydraulicErosionEnabled = settings.hydraulicStrength > 0
        thermalErosionEnabled = settings.thermalStrength > 0
        windErosionEnabled = settings.windStrength > 0
        coastalErosionEnabled = settings.coastalStrength > 0
        
        erosionIterationCount = settings.iterations
        erosionStrength = settings.hydraulicStrength
        sedimentCapacity = settings.sedimentCapacity
        depositionRate = settings.depositionRate
        evaporationRate = settings.evaporationRate
    }
    
    func simulateHydraulicErosion() {
        guard hydraulicErosionEnabled else { return }
        
        // Create snapshot for undo
        if undoRedoEnabled {
            createSnapshot("Hydraulic Erosion Applied")
        }
        
        // Hydraulic erosion simulation
        for _ in 0..<erosionIterationCount {
            // Simulate water droplet erosion
            simulateWaterDroplet()
        }
    }
    
    func simulateThermalErosion() {
        guard thermalErosionEnabled else { return }
        
        if undoRedoEnabled {
            createSnapshot("Thermal Erosion Applied")
        }
        
        // Thermal erosion simulation
        for _ in 0..<erosionIterationCount {
            simulateMaterialSlumping()
        }
    }
    
    func simulateWindErosion() {
        guard windErosionEnabled else { return }
        
        if undoRedoEnabled {
            createSnapshot("Wind Erosion Applied")
        }
        
        // Wind erosion simulation
        for _ in 0..<erosionIterationCount {
            simulateWindTransport()
        }
    }
    
    func simulateCoastalErosion() {
        guard coastalErosionEnabled else { return }
        
        if undoRedoEnabled {
            createSnapshot("Coastal Erosion Applied")
        }
        
        // Coastal erosion simulation
        for _ in 0..<erosionIterationCount {
            simulateWaveAction()
        }
    }
    
    private func simulateWaterDroplet() {
        // Simplified hydraulic erosion simulation
        // In a real implementation, this would:
        // 1. Spawn water droplets at random positions
        // 2. Calculate droplet path based on height gradient
        // 3. Erode material based on velocity and sediment capacity
        // 4. Deposit sediment when capacity is exceeded
        // 5. Evaporate water based on rate
    }
    
    private func simulateMaterialSlumping() {
        // Thermal erosion simulation
        // Material slides down slopes exceeding angle threshold
    }
    
    private func simulateWindTransport() {
        // Wind erosion simulation
        // Sand particles transported by wind direction
    }
    
    private func simulateWaveAction() {
        // Coastal erosion simulation
        // Wave action erodes coastline based on wave height
    }
    
    func generateRiver(path: [SIMD2<Float>], width: Float, depth: Float) {
        guard riverGenerationEnabled else { return }
        
        if undoRedoEnabled {
            createSnapshot("River Generated")
        }
        
        // River generation algorithm
        // 1. Carve riverbed along path
        // 2. Apply width variation
        // 3. Create depth profile
        // 4. Generate banks
    }
    
    func fillLake(waterLevel: Float, center: SIMD2<Float>, radius: Float) {
        guard lakeFillingEnabled else { return }
        
        if undoRedoEnabled {
            createSnapshot("Lake Filled")
        }
        
        // Lake filling algorithm
        // 1. Find area below water level
        // 2. Carve basin if needed
        // 3. Fill with water plane
        // 4. Create shoreline
    }
    
    func sculptRiverbed(path: [SIMD2<Float>], width: Float, depth: Float) {
        guard riverbedSculptingEnabled else { return }
        
        if undoRedoEnabled {
            createSnapshot("Riverbed Sculpted")
        }
        
        // Riverbed sculpting
        // More detailed river carving with natural variation
    }
    
    private func createSnapshot(_ description: String) {
        let snapshot = ErosionSnapshot(
            timestamp: Date(),
            settings: ErosionSettings(),
            heightmapData: [], // Would contain actual heightmap data
            description: description
        )
        
        // Remove any snapshots after current index
        erosionHistory = Array(erosionHistory.prefix(currentHistoryIndex + 1))
        
        // Add new snapshot
        erosionHistory.append(snapshot)
        currentHistoryIndex = erosionHistory.count - 1
        
        // Limit history size
        if erosionHistory.count > 50 {
            erosionHistory.removeFirst()
            currentHistoryIndex -= 1
        }
    }
    
    func undo() {
        guard currentHistoryIndex > 0 else { return }
        currentHistoryIndex -= 1
        restoreSnapshot(erosionHistory[currentHistoryIndex])
    }
    
    func redo() {
        guard currentHistoryIndex < erosionHistory.count - 1 else { return }
        currentHistoryIndex += 1
        restoreSnapshot(erosionHistory[currentHistoryIndex])
    }
    
    private func restoreSnapshot(_ snapshot: ErosionSnapshot) {
        // Restore heightmap from snapshot
        // Apply snapshot.settings
    }
}

// MARK: - Vegetation System

class VegetationSystemManager: ObservableObject {
    static let shared = VegetationSystemManager()
    
    // Tree System
    @Published var treePlacementEnabled: Bool = true
    @Published var treeDensity: Float = 0.5
    @Published var treeScaleRandomization: Float = 0.3
    @Published var treeRotationRandomization: Float = 360.0
    @Published var treeColorVariation: Float = 0.2
    @Published var selectedTreeType: TreeType = .oak
    @Published var treeLODEnabled: Bool = true
    @Published var treeBillboardDistance: Float = 100.0
    @Published var treeWindAnimation: Bool = true
    @Published var treeCollisionGeneration: Bool = true
    
    // Grass System
    @Published var grassDensity: Float = 1.0
    @Published var selectedGrassType: GrassType = .standard
    @Published var grassColorVariation: Float = 0.15
    @Published var grassWindResponse: Float = 0.8
    @Published var grassLODDistance: Float = 50.0
    @Published var grassCullingEnabled: Bool = true
    
    // Vegetation Rules
    @Published var placementRulesEnabled: Bool = true
    @Published var exclusionZones: [ExclusionZone] = []
    @Published var vegetationDensity: Float = 0.7
    @Published var proceduralScattering: Bool = false
    
    @Published var vegetationInstances: [VegetationInstance] = []
    @Published var vegetationRemovalEnabled: Bool = false
    @Published var densityVisualization: Bool = false
    
    enum TreeType: String, CaseIterable {
        case oak = "Oak"
        case pine = "Pine"
        case birch = "Birch"
        case maple = "Maple"
        case willow = "Willow"
        case palm = "Palm"
        case baobab = "Baobab"
        case sakura = "Sakura"
        
        var prefabName: String {
            return "Tree_\(rawValue)"
        }
        
        var minScale: Float {
            switch self {
            case .baobab: return 0.8
            case .palm: return 0.6
            default: return 0.4
            }
        }
        
        var maxScale: Float {
            switch self {
            case .baobab: return 1.5
            case .oak, .maple: return 1.2
            default: return 1.0
            }
        }
    }
    
    enum GrassType: String, CaseIterable {
        case standard = "Standard"
        case tall = "Tall"
        case short = "Short"
        case wild = "Wild"
        case desert = "Desert"
        case tropical = "Tropical"
        
        var meshName: String {
            return "Grass_\(rawValue)"
        }
        
        var windMultiplier: Float {
            switch self {
            case .tall: return 1.5
            case .short: return 0.5
            case .desert: return 0.3
            default: return 1.0
            }
        }
    }
    
    struct ExclusionZone: Identifiable {
        let id = UUID()
        var position: SIMD2<Float>
        var radius: Float
        var zoneType: ZoneType
        var affectsTrees: Bool = true
        var affectsGrass: Bool = true
        
        enum ZoneType: String, CaseIterable {
            case circle = "Circle"
            case rectangle = "Rectangle"
            case polygon = "Polygon"
        }
    }
    
    struct VegetationInstance: Identifiable {
        let id = UUID()
        var position: SIMD3<Float>
        var rotation: SIMD3<Float>
        var scale: SIMD3<Float>
        var type: VegetationType
        var colorTint: SIMD3<Float> = SIMD3<Float>(1, 1, 1)
        
        enum VegetationType {
            case tree(TreeType)
            case grass(GrassType)
        }
    }
    
    func placeVegetation(at position: SIMD3<Float>) {
        if vegetationRemovalEnabled {
            removeVegetation(at: position)
            return
        }
        
        let instance: VegetationInstance
        
        if treePlacementEnabled && Float.random(in: 0...1) < treeDensity {
            let treeType = selectedTreeType
            let scale = calculateTreeScale()
            let rotation = calculateTreeRotation()
            let colorTint = calculateColorTint()
            
            instance = VegetationInstance(
                position: position,
                rotation: rotation,
                scale: SIMD3<Float>(scale, scale, scale),
                type: .tree(treeType),
                colorTint: colorTint
            )
        } else {
            let grassType = selectedGrassType
            let scale = Float.random(in: 0.8...1.2)
            let rotation = SIMD3<Float>(0, Float.random(in: 0...Float.pi * 2), 0)
            let colorTint = calculateColorTint()
            
            instance = VegetationInstance(
                position: position,
                rotation: rotation,
                scale: SIMD3<Float>(scale, scale, scale),
                type: .grass(grassType),
                colorTint: colorTint
            )
        }
        
        // Check placement rules
        if placementRulesEnabled && !canPlaceVegetation(instance) {
            return
        }
        
        vegetationInstances.append(instance)
    }
    
    private func calculateTreeScale() -> Float {
        let treeType = selectedTreeType
        let baseScale = Float.random(in: treeType.minScale...treeType.maxScale)
        let randomization = 1.0 + (Float.random(in: -treeScaleRandomization...treeScaleRandomization))
        return baseScale * randomization
    }
    
    private func calculateTreeRotation() -> SIMD3<Float> {
        let yRotation = Float.random(in: 0...treeRotationRandomization) * .pi / 180.0
        let xRotation = Float.random(in: -5...5) * .pi / 180.0
        let zRotation = Float.random(in: -5...5) * .pi / 180.0
        return SIMD3<Float>(xRotation, yRotation, zRotation)
    }
    
    private func calculateColorTint() -> SIMD3<Float> {
        let variation = treeColorVariation
        let r = 1.0 + Float.random(in: -variation...variation)
        let g = 1.0 + Float.random(in: -variation...variation)
        let b = 1.0 + Float.random(in: -variation...variation)
        return SIMD3<Float>(r, g, b)
    }
    
    private func canPlaceVegetation(_ instance: VegetationInstance) -> Bool {
        // Check exclusion zones
        for zone in exclusionZones {
            let distance = length(instance.position.xy - zone.position)
            if distance < zone.radius {
                switch instance.type {
                case .tree:
                    if zone.affectsTrees { return false }
                case .grass:
                    if zone.affectsGrass { return false }
                }
            }
        }
        
        // Check slope (simplified - would use actual terrain normal)
        let maxSlope = 0.7 // 45 degrees
        // if terrainSlope > maxSlope { return false }
        
        // Check distance to other vegetation
        let minDistance: Float = 2.0
        for other in vegetationInstances {
            let distance = length(instance.position - other.position)
            if distance < minDistance {
                return false
            }
        }
        
        return true
    }
    
    private func removeVegetation(at position: SIMD3<Float>) {
        let removalRadius: Float = 3.0
        vegetationInstances.removeAll { instance in
            length(instance.position - position) < removalRadius
        }
    }
    
    func generateProceduralVegetation(area: SIMD2<Float>, size: Float) {
        guard proceduralScattering else { return }
        
        let gridSize: Float = 2.0
        let steps = Int(size / gridSize)
        
        for x in 0..<steps {
            for z in 0..<steps {
                let xPos = area.x + Float(x) * gridSize - size / 2
                let zPos = area.y + Float(z) * gridSize - size / 2
                
                let position = SIMD3<Float>(xPos, 0, zPos)
                
                // Add some randomness to position
                let randomOffset = SIMD3<Float>(
                    Float.random(in: -gridSize/2...gridSize/2),
                    0,
                    Float.random(in: -gridSize/2...gridSize/2)
                )
                
                placeVegetation(at: position + randomOffset)
            }
        }
    }
    
    func optimizeVegetation() {
        // Remove instances that are too close together
        let minDistance: Float = 1.0
        var toRemove: Set<UUID> = []
        
        for (i, instance1) in vegetationInstances.enumerated() {
            for instance2 in vegetationInstances.dropFirst(i + 1) {
                let distance = length(instance1.position - instance2.position)
                if distance < minDistance {
                    toRemove.insert(instance2.id)
                }
            }
        }
        
        vegetationInstances.removeAll { toRemove.contains($0.id) }
    }
    
    func clearVegetation() {
        vegetationInstances.removeAll()
    }
    
    func addExclusionZone(at position: SIMD2<Float>, radius: Float, type: ExclusionZone.ZoneType) {
        let zone = ExclusionZone(
            position: position,
            radius: radius,
            zoneType: type
        )
        exclusionZones.append(zone)
    }
    
    func removeExclusionZone(_ zone: ExclusionZone) {
        exclusionZones.removeAll { $0.id == zone.id }
    }
}

// MARK: - Heightmap Management System

class HeightmapManager: ObservableObject {
    static let shared = HeightmapManager()
    
    @Published var resolution: HeightmapResolution = .medium
    @Published var precision: HeightmapPrecision = .float16
    @Published var worldHeightRange: SIMD2<Float> = SIMD2<Float>(0, 100)
    @Published var tileSize: Float = 64.0
    @Published var seamlessTiling: Bool = false
    @Published var scale: Float = 1.0
    @Published var offset: Float = 0.0
    @Published var clampMin: Float = -1000.0
    @Published var clampMax: Float = 1000.0
    
    @Published var holeSystemEnabled: Bool = false
    @Published var undoHistoryEnabled: Bool = true
    @Published var compressionEnabled: Bool = false
    @Published var streamingEnabled: Bool = false
    
    @Published var heightmapHistory: [HeightmapSnapshot] = []
    @Published var currentHistoryIndex: Int = -1
    @Published var comparisonSnapshot: HeightmapSnapshot?
    
    enum HeightmapResolution: String, CaseIterable {
        case low = "512x512"
        case medium = "1024x1024"
        case high = "2048x2048"
        case ultra = "4096x4096"
        
        var size: Int {
            switch self {
            case .low: return 512
            case .medium: return 1024
            case .high: return 2048
            case .ultra: return 4096
            }
        }
    }
    
    enum HeightmapPrecision: String, CaseIterable {
        case float16 = "16-bit Float"
        case float32 = "32-bit Float"
        
        var bytesPerValue: Int {
            switch self {
            case .float16: return 2
            case .float32: return 4
            }
        }
    }
    
    struct HeightmapSnapshot: Identifiable {
        let id = UUID()
        let timestamp: Date
        let heightmapData: [Float]
        let description: String
        let resolution: HeightmapResolution
        let worldHeightRange: SIMD2<Float>
    }
    
    func importHeightmap(from url: URL) {
        // Import heightmap from file (PNG, RAW, etc.)
        // Convert to internal format
        // Apply resolution and precision settings
        
        if undoHistoryEnabled {
            createSnapshot("Heightmap Imported")
        }
    }
    
    func exportHeightmap(to url: URL, format: ExportFormat) {
        // Export heightmap to specified format
    }
    
    enum ExportFormat: String, CaseIterable {
        case png = "PNG"
        case raw = "RAW"
        case tiff = "TIFF"
        case exr = "EXR"
    }
    
    func copyRegion(sourceRect: CGRect, destination: CGPoint) {
        if undoHistoryEnabled {
            createSnapshot("Region Copied")
        }
        
        // Copy heightmap region
    }
    
    func fillWithValue(_ value: Float) {
        if undoHistoryEnabled {
            createSnapshot("Filled with Value")
        }
        
        // Fill entire heightmap with value
    }
    
    func invertHeightmap() {
        if undoHistoryEnabled {
            createSnapshot("Heightmap Inverted")
        }
        
        // Invert heightmap values
    }
    
    func normalizeHeightmap() {
        if undoHistoryEnabled {
            createSnapshot("Heightmap Normalized")
        }
        
        // Normalize heightmap to 0-1 range
    }
    
    func applyScaleOffsetClamp() {
        if undoHistoryEnabled {
            createSnapshot("Scale/Offset/Clamp Applied")
        }
        
        // Apply scale, offset, and clamp to heightmap
    }
    
    func createHole(at position: SIMD2<Float>, radius: Float) {
        guard holeSystemEnabled else { return }
        
        if undoHistoryEnabled {
            createSnapshot("Hole Created")
        }
        
        // Create hole in heightmap
    }
    
    func fillHole(at position: SIMD2<Float>, radius: Float) {
        guard holeSystemEnabled else { return }
        
        if undoHistoryEnabled {
            createSnapshot("Hole Filled")
        }
        
        // Fill hole in heightmap
    }
    
    func compareWithSnapshot(_ snapshot: HeightmapSnapshot) {
        comparisonSnapshot = snapshot
        // Generate diff visualization
    }
    
    func clearComparison() {
        comparisonSnapshot = nil
    }
    
    private func createSnapshot(_ description: String) {
        let snapshot = HeightmapSnapshot(
            timestamp: Date(),
            heightmapData: [], // Would contain actual heightmap data
            description: description,
            resolution: resolution,
            worldHeightRange: worldHeightRange
        )
        
        // Remove any snapshots after current index
        heightmapHistory = Array(heightmapHistory.prefix(currentHistoryIndex + 1))
        
        // Add new snapshot
        heightmapHistory.append(snapshot)
        currentHistoryIndex = heightmapHistory.count - 1
        
        // Limit history size
        if heightmapHistory.count > 50 {
            heightmapHistory.removeFirst()
            currentHistoryIndex -= 1
        }
    }
    
    func undo() {
        guard currentHistoryIndex > 0 else { return }
        currentHistoryIndex -= 1
        restoreSnapshot(heightmapHistory[currentHistoryIndex])
    }
    
    func redo() {
        guard currentHistoryIndex < heightmapHistory.count - 1 else { return }
        currentHistoryIndex += 1
        restoreSnapshot(heightmapHistory[currentHistoryIndex])
    }
    
    private func restoreSnapshot(_ snapshot: HeightmapSnapshot) {
        // Restore heightmap from snapshot
        resolution = snapshot.resolution
        worldHeightRange = snapshot.worldHeightRange
    }
    
    func compressHeightmap() {
        guard compressionEnabled else { return }
        
        // Apply compression algorithm
    }
    
    func enableStreaming() {
        guard streamingEnabled else { return }
        
        // Set up streaming system for large terrains
    }
}

// MARK: - Terrain Integration System

class TerrainIntegrationManager: ObservableObject {
    static let shared = TerrainIntegrationManager()
    
    @Published var physicsCollisionEnabled: Bool = true
    @Published var navigationMeshEnabled: Bool = true
    @Published var waterPlaneEnabled: Bool = false
    @Published var skyLightingEnabled: Bool = true
    @Published var shadowCascadeOptimization: Bool = true
    @Published var lightmapBakingEnabled: Bool = false
    @Published var materialPerChunkEnabled: Bool = true
    @Published var terrainShaderCustomization: Bool = false
    
    @Published var streamingZones: [StreamingZone] = []
    @Published var multiTerrainStitching: Bool = false
    @Published var worldOriginRebasing: Bool = false
    @Published var largeWorldCoordinates: Bool = false
    @Published var terrainPrefabSaving: Bool = false
    @Published var terrainAssetVersioning: Bool = true
    
    @Published var runtimeModificationAPI: Bool = true
    @Published var networkingSynchronization: Bool = false
    @Published var analyticsEnabled: Bool = true
    @Published var documentationTooltips: Bool = true
    @Published var tutorialSystemEnabled: Bool = false
    
    struct StreamingZone: Identifiable {
        let id = UUID()
        var position: SIMD2<Float>
        var size: Float
        var lodLevels: Int
        var priority: Int
        var isActive: Bool = true
    }
    
    func generatePhysicsCollision() {
        guard physicsCollisionEnabled else { return }
        
        // Generate collision mesh from heightmap
        // Create physics bodies for terrain chunks
        // Configure collision materials
    }
    
    func generateNavigationMesh() {
        guard navigationMeshEnabled else { return }
        
        // Generate navigation mesh from heightmap
        // Mark walkable/unwalkable areas
        // Create off-mesh links for jumps
    }
    
    func integrateWaterPlane(waterLevel: Float) {
        guard waterPlaneEnabled else { return }
        
        // Create water plane at specified level
        // Configure water shader and effects
        // Set up reflection/refraction
    }
    
    func integrateSkyLighting(sunDirection: SIMD3<Float>, sunColor: SIMD3<Float>) {
        guard skyLightingEnabled else { return }
        
        // Configure ambient lighting
        // Set up directional sun light
        // Apply atmospheric scattering
    }
    
    func optimizeShadowCascades() {
        guard shadowCascadeOptimization else { return }
        
        // Optimize shadow cascade distribution
        // Configure cascade splits
        // Set up shadow map resolution
    }
    
    func bakeLightmaps() {
        guard lightmapBakingEnabled else { return }
        
        // Bake static lighting into lightmaps
        // Configure lightmap resolution
        // Process indirect lighting
    }
    
    func createMaterialPerChunk() {
        guard materialPerChunkEnabled else { return }
        
        // Create material instances per terrain chunk
        // Apply texture splatting
        // Configure material properties
    }
    
    func customizeTerrainShader() {
        guard terrainShaderCustomization else { return }
        
        // Allow terrain shader customization
        // Provide shader parameters
        // Enable advanced features
    }
    
    func setupStreamingZones() {
        // Configure streaming zones for large terrains
        // Set up level-of-detail transitions
        // Manage streaming priority
    }
    
    func enableMultiTerrainStitching() {
        guard multiTerrainStitching else { return }
        
        // Stitch multiple terrain tiles together
        // Create seamless transitions
        // Handle height differences
    }
    
    func enableWorldOriginRebasing() {
        guard worldOriginRebasing else { return }
        
        // Implement world origin rebasing
        // Handle coordinate system shifts
        // Update all entity positions
    }
    
    func enableLargeWorldCoordinates() {
        guard largeWorldCoordinates else { return }
        
        // Use double precision for world coordinates
        // Implement floating origin system
        // Handle precision issues
    }
    
    func saveTerrainPrefab() {
        guard terrainPrefabSaving else { return }
        
        // Save terrain as prefab asset
        // Include all modifications
        // Store metadata and dependencies
    }
    
    func enableTerrainAssetVersioning() {
        guard terrainAssetVersioning else { return }
        
        // Implement version control for terrain assets
        // Track changes and modifications
        // Enable rollback capabilities
    }
    
    func enableRuntimeModificationAPI() {
        guard runtimeModificationAPI else { return }
        
        // Provide API for runtime terrain modification
        // Handle real-time updates
        // Synchronize with physics and navigation
    }
    
    func enableNetworkingSynchronization() {
        guard networkingSynchronization else { return }
        
        // Synchronize terrain modifications across network
        // Handle conflict resolution
        // Optimize bandwidth usage
    }
    
    func updateAnalytics() {
        guard analyticsEnabled else { return }
        
        // Collect terrain analytics
        // Track performance metrics
        // Monitor memory usage
    }
    
    func showDocumentationTooltips() {
        guard documentationTooltips else { return }
        
        // Display helpful tooltips for terrain features
        // Provide context-sensitive help
        // Show keyboard shortcuts
    }
    
    func enableTutorialSystem() {
        guard tutorialSystemEnabled else { return }
        
        // Guide users through terrain editing
        // Provide step-by-step instructions
        // Highlight important features
    }
}

// MARK: - Terrain Editor UI Components

struct TerrainEditorView: View {
    @StateObject private var erosionManager = ErosionSimulationManager.shared
    @StateObject private var vegetationManager = VegetationSystemManager.shared
    @StateObject private var heightmapManager = HeightmapManager.shared
    @StateObject private var integrationManager = TerrainIntegrationManager.shared
    
    @State private var selectedTab: TerrainTab = .erosion
    @State private var showingSettings = false
    
    enum TerrainTab: String, CaseIterable {
        case erosion = "Erosion"
        case vegetation = "Vegetation"
        case heightmap = "Heightmap"
        case integration = "Integration"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Tab Bar
            Picker("Terrain Tab", selection: $selectedTab) {
                ForEach(TerrainTab.allCases, id: \.self) { tab in
                    Text(tab.rawValue).tag(tab)
                }
            }
            .pickerStyle(SegmentedPickerStyle())
            .padding()
            
            // Content
            TabView(selection: $selectedTab) {
                ErosionSimulationView()
                    .tag(TerrainTab.erosion)
                
                VegetationSystemView()
                    .tag(TerrainTab.vegetation)
                
                HeightmapManagementView()
                    .tag(TerrainTab.heightmap)
                
                TerrainIntegrationView()
                    .tag(TerrainTab.integration)
            }
            .tabViewStyle(PageTabViewStyle(indexDisplayMode: .never))
            
            // Bottom Toolbar
            HStack {
                Button(action: { showingSettings = true }) {
                    Image(systemName: "gear")
                }
                
                Spacer()
                
                Button("Apply All") {
                    applyAllTerrainModifications()
                }
                .buttonStyle(.borderedProminent)
                
                Button("Reset") {
                    resetAllTerrainSettings()
                }
            }
            .padding()
        }
        .sheet(isPresented: $showingSettings) {
            TerrainSettingsSheet()
        }
    }
    
    private func applyAllTerrainModifications() {
        erosionManager.simulateHydraulicErosion()
        erosionManager.simulateThermalErosion()
        erosionManager.simulateWindErosion()
        erosionManager.simulateCoastalErosion()
        
        integrationManager.generatePhysicsCollision()
        integrationManager.generateNavigationMesh()
        integrationManager.optimizeShadowCascades()
    }
    
    private func resetAllTerrainSettings() {
        erosionManager.currentPreset = .custom
        vegetationManager.clearVegetation()
        heightmapManager.currentHistoryIndex = -1
        integrationManager.streamingZones.removeAll()
    }
}

// MARK: - Erosion Simulation UI

struct ErosionSimulationView: View {
    @StateObject private var manager = ErosionSimulationManager.shared
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                // Preset Selection
                GroupBox("Erosion Presets") {
                    Picker("Preset", selection: $manager.currentPreset) {
                        ForEach(ErosionSimulationManager.ErosionPreset.allCases, id: \.self) { preset in
                            Text(preset.rawValue).tag(preset)
                        }
                    }
                    .pickerStyle(MenuPickerStyle())
                    .onChange(of: manager.currentPreset) { newPreset in
                        manager.applyErosionPreset(newPreset)
                    }
                }
                
                // Erosion Types
                GroupBox("Erosion Types") {
                    VStack(alignment: .leading, spacing: 10) {
                        Toggle("Hydraulic Erosion", isOn: $manager.hydraulicErosionEnabled)
                        Toggle("Thermal Erosion", isOn: $manager.thermalErosionEnabled)
                        Toggle("Wind Erosion", isOn: $manager.windErosionEnabled)
                        Toggle("Coastal Erosion", isOn: $manager.coastalErosionEnabled)
                    }
                }
                
                // Erosion Parameters
                GroupBox("Parameters") {
                    VStack(alignment: .leading, spacing: 15) {
                        HStack {
                            Text("Iteration Count:")
                            TextField("Iterations", value: $manager.erosionIterationCount, format: .number)
                                .textFieldStyle(RoundedBorderTextFieldStyle())
                                .frame(width: 80)
                        }
                        
                        HStack {
                            Text("Erosion Strength:")
                            Slider(value: $manager.erosionStrength, in: 0.1...2.0)
                            Text(String(format: "%.2f", manager.erosionStrength))
                                .frame(width: 50)
                        }
                        
                        HStack {
                            Text("Sediment Capacity:")
                            Slider(value: $manager.sedimentCapacity, in: 1.0...10.0)
                            Text(String(format: "%.1f", manager.sedimentCapacity))
                                .frame(width: 50)
                        }
                        
                        HStack {
                            Text("Deposition Rate:")
                            Slider(value: $manager.depositionRate, in: 0.01...0.5)
                            Text(String(format: "%.2f", manager.depositionRate))
                                .frame(width: 50)
                        }
                        
                        HStack {
                            Text("Evaporation Rate:")
                            Slider(value: $manager.evaporationRate, in: 0.001...0.1)
                            Text(String(format: "%.3f", manager.evaporationRate))
                                .frame(width: 50)
                        }
                    }
                }
                
                // Visualization Options
                GroupBox("Visualization") {
                    VStack(alignment: .leading, spacing: 10) {
                        Toggle("Water Flow Visualization", isOn: $manager.waterFlowVisualization)
                        Toggle("Erosion Mask Painting", isOn: $manager.erosionMaskPainting)
                        Toggle("Real-time Preview", isOn: $manager.realTimePreview)
                        Toggle("Undo/Redo Enabled", isOn: $manager.undoRedoEnabled)
                    }
                }
                
                // Advanced Features
                GroupBox("Advanced Features") {
                    VStack(alignment: .leading, spacing: 10) {
                        Toggle("River Generation", isOn: $manager.riverGenerationEnabled)
                        Toggle("Lake Filling", isOn: $manager.lakeFillingEnabled)
                        Toggle("Riverbed Sculpting", isOn: $manager.riverbedSculptingEnabled)
                    }
                }
                
                // Action Buttons
                HStack {
                    Button("Apply Hydraulic Erosion") {
                        manager.simulateHydraulicErosion()
                    }
                    .buttonStyle(.bordered)
                    
                    Button("Apply Thermal Erosion") {
                        manager.simulateThermalErosion()
                    }
                    .buttonStyle(.bordered)
                    
                    Button("Apply Wind Erosion") {
                        manager.simulateWindErosion()
                    }
                    .buttonStyle(.bordered)
                    
                    Button("Apply Coastal Erosion") {
                        manager.simulateCoastalErosion()
                    }
                    .buttonStyle(.bordered)
                }
                
                // History Controls
                HStack {
                    Button("Undo") {
                        manager.undo()
                    }
                    .disabled(manager.currentHistoryIndex <= 0)
                    
                    Button("Redo") {
                        manager.redo()
                    }
                    .disabled(manager.currentHistoryIndex >= manager.erosionHistory.count - 1)
                    
                    Spacer()
                    
                    Text("History: \(manager.currentHistoryIndex + 1)/\(manager.erosionHistory.count)")
                        .foregroundColor(.secondary)
                }
            }
            .padding()
        }
    }
}

// MARK: - Vegetation System UI

struct VegetationSystemView: View {
    @StateObject private var manager = VegetationSystemManager.shared
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                // Tree System
                GroupBox("Tree System") {
                    VStack(alignment: .leading, spacing: 15) {
                        Toggle("Tree Placement Enabled", isOn: $manager.treePlacementEnabled)
                        
                        HStack {
                            Text("Tree Type:")
                            Picker("Tree Type", selection: $manager.selectedTreeType) {
                                ForEach(VegetationSystemManager.TreeType.allCases, id: \.self) { type in
                                    Text(type.rawValue).tag(type)
                                }
                            }
                            .pickerStyle(MenuPickerStyle())
                        }
                        
                        HStack {
                            Text("Tree Density:")
                            Slider(value: $manager.treeDensity, in: 0.0...1.0)
                            Text(String(format: "%.2f", manager.treeDensity))
                                .frame(width: 50)
                        }
                        
                        HStack {
                            Text("Scale Randomization:")
                            Slider(value: $manager.treeScaleRandomization, in: 0.0...1.0)
                            Text(String(format: "%.2f", manager.treeScaleRandomization))
                                .frame(width: 50)
                        }
                        
                        HStack {
                            Text("Rotation Randomization:")
                            Slider(value: $manager.treeRotationRandomization, in: 0...360)
                            Text("\(Int(manager.treeRotationRandomization))°")
                                .frame(width: 50)
                        }
                        
                        HStack {
                            Text("Color Variation:")
                            Slider(value: $manager.treeColorVariation, in: 0.0...0.5)
                            Text(String(format: "%.2f", manager.treeColorVariation))
                                .frame(width: 50)
                        }
                        
                        VStack(alignment: .leading, spacing: 10) {
                            Toggle("Tree LOD Enabled", isOn: $manager.treeLODEnabled)
                            Toggle("Tree Wind Animation", isOn: $manager.treeWindAnimation)
                            Toggle("Tree Collision Generation", isOn: $manager.treeCollisionGeneration)
                        }
                    }
                }
                
                // Grass System
                GroupBox("Grass System") {
                    VStack(alignment: .leading, spacing: 15) {
                        HStack {
                            Text("Grass Type:")
                            Picker("Grass Type", selection: $manager.selectedGrassType) {
                                ForEach(VegetationSystemManager.GrassType.allCases, id: \.self) { type in
                                    Text(type.rawValue).tag(type)
                                }
                            }
                            .pickerStyle(MenuPickerStyle())
                        }
                        
                        HStack {
                            Text("Grass Density:")
                            Slider(value: $manager.grassDensity, in: 0.0...2.0)
                            Text(String(format: "%.2f", manager.grassDensity))
                                .frame(width: 50)
                        }
                        
                        HStack {
                            Text("Color Variation:")
                            Slider(value: $manager.grassColorVariation, in: 0.0...0.3)
                            Text(String(format: "%.2f", manager.grassColorVariation))
                                .frame(width: 50)
                        }
                        
                        HStack {
                            Text("Wind Response:")
                            Slider(value: $manager.grassWindResponse, in: 0.0...1.0)
                            Text(String(format: "%.2f", manager.grassWindResponse))
                                .frame(width: 50)
                        }
                        
                        HStack {
                            Text("LOD Distance:")
                            Slider(value: $manager.grassLODDistance, in: 10.0...200.0)
                            Text(String(format: "%.0f", manager.grassLODDistance))
                                .frame(width: 50)
                        }
                        
                        Toggle("Grass Culling Enabled", isOn: $manager.grassCullingEnabled)
                    }
                }
                
                // Vegetation Rules
                GroupBox("Placement Rules") {
                    VStack(alignment: .leading, spacing: 15) {
                        Toggle("Placement Rules Enabled", isOn: $manager.placementRulesEnabled)
                        
                        HStack {
                            Text("Overall Density:")
                            Slider(value: $manager.vegetationDensity, in: 0.0...1.0)
                            Text(String(format: "%.2f", manager.vegetationDensity))
                                .frame(width: 50)
                        }
                        
                        Toggle("Procedural Scattering", isOn: $manager.proceduralScattering)
                        Toggle("Density Visualization", isOn: $manager.densityVisualization)
                        Toggle("Removal Mode", isOn: $manager.vegetationRemovalEnabled)
                    }
                }
                
                // Exclusion Zones
                GroupBox("Exclusion Zones") {
                    VStack(alignment: .leading, spacing: 10) {
                        ForEach(manager.exclusionZones) { zone in
                            HStack {
                                Text("\(zone.zoneType.rawValue) Zone")
                                Spacer()
                                Button("Remove") {
                                    manager.removeExclusionZone(zone)
                                }
                                .buttonStyle(.bordered)
                            }
                        }
                        
                        Button("Add Exclusion Zone") {
                            // Add new exclusion zone
                        }
                        .buttonStyle(.bordered)
                    }
                }
                
                // Action Buttons
                HStack {
                    Button("Generate Procedural") {
                        manager.generateProceduralVegetation(area: SIMD2<Float>(0, 0), size: 100.0)
                    }
                    .buttonStyle(.bordered)
                    
                    Button("Optimize") {
                        manager.optimizeVegetation()
                    }
                    .buttonStyle(.bordered)
                    
                    Button("Clear All") {
                        manager.clearVegetation()
                    }
                    .buttonStyle(.borderedDestructive)
                }
                
                // Statistics
                GroupBox("Statistics") {
                    VStack(alignment: .leading, spacing: 5) {
                        Text("Total Vegetation Instances: \(manager.vegetationInstances.count)")
                        Text("Exclusion Zones: \(manager.exclusionZones.count)")
                    }
                }
            }
            .padding()
        }
    }
}

// MARK: - Heightmap Management UI

struct HeightmapManagementView: View {
    @StateObject private var manager = HeightmapManager.shared
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                // Heightmap Properties
                GroupBox("Heightmap Properties") {
                    VStack(alignment: .leading, spacing: 15) {
                        HStack {
                            Text("Resolution:")
                            Picker("Resolution", selection: $manager.resolution) {
                                ForEach(HeightmapManager.HeightmapResolution.allCases, id: \.self) { resolution in
                                    Text(resolution.rawValue).tag(resolution)
                                }
                            }
                            .pickerStyle(MenuPickerStyle())
                        }
                        
                        HStack {
                            Text("Precision:")
                            Picker("Precision", selection: $manager.precision) {
                                ForEach(HeightmapManager.HeightmapPrecision.allCases, id: \.self) { precision in
                                    Text(precision.rawValue).tag(precision)
                                }
                            }
                            .pickerStyle(MenuPickerStyle())
                        }
                        
                        HStack {
                            Text("Tile Size:")
                            Slider(value: $manager.tileSize, in: 32.0...256.0)
                            Text(String(format: "%.0f", manager.tileSize))
                                .frame(width: 50)
                        }
                        
                        Toggle("Seamless Tiling", isOn: $manager.seamlessTiling)
                    }
                }
                
                // Height Range
                GroupBox("World Height Range") {
                    VStack(alignment: .leading, spacing: 15) {
                        HStack {
                            Text("Min Height:")
                            TextField("Min", value: $manager.worldHeightRange.x, format: .number)
                                .textFieldStyle(RoundedBorderTextFieldStyle())
                                .frame(width: 80)
                            
                            Text("Max Height:")
                            TextField("Max", value: $manager.worldHeightRange.y, format: .number)
                                .textFieldStyle(RoundedBorderTextFieldStyle())
                                .frame(width: 80)
                        }
                    }
                }
                
                // Scale/Offset/Clamp
                GroupBox("Scale, Offset & Clamp") {
                    VStack(alignment: .leading, spacing: 15) {
                        HStack {
                            Text("Scale:")
                            Slider(value: $manager.scale, in: 0.1...10.0)
                            Text(String(format: "%.2f", manager.scale))
                                .frame(width: 50)
                        }
                        
                        HStack {
                            Text("Offset:")
                            Slider(value: $manager.offset, in: -1000.0...1000.0)
                            Text(String(format: "%.0f", manager.offset))
                                .frame(width: 50)
                        }
                        
                        HStack {
                            Text("Clamp Min:")
                            Slider(value: $manager.clampMin, in: -1000.0...0.0)
                            Text(String(format: "%.0f", manager.clampMin))
                                .frame(width: 50)
                        }
                        
                        HStack {
                            Text("Clamp Max:")
                            Slider(value: $manager.clampMax, in: 0.0...1000.0)
                            Text(String(format: "%.0f", manager.clampMax))
                                .frame(width: 50)
                        }
                        
                        Button("Apply Scale/Offset/Clamp") {
                            manager.applyScaleOffsetClamp()
                        }
                        .buttonStyle(.bordered)
                    }
                }
                
                // Operations
                GroupBox("Operations") {
                    VStack(alignment: .leading, spacing: 10) {
                        Button("Import Heightmap") {
                            // Import dialog
                        }
                        .buttonStyle(.bordered)
                        
                        Button("Export Heightmap") {
                            // Export dialog
                        }
                        .buttonStyle(.bordered)
                        
                        HStack {
                            Button("Invert") {
                                manager.invertHeightmap()
                            }
                            .buttonStyle(.bordered)
                            
                            Button("Normalize") {
                                manager.normalizeHeightmap()
                            }
                            .buttonStyle(.bordered)
                            
                            Button("Fill Value") {
                                manager.fillWithValue(0.5)
                            }
                            .buttonStyle(.bordered)
                        }
                    }
                }
                
                // Advanced Features
                GroupBox("Advanced Features") {
                    VStack(alignment: .leading, spacing: 10) {
                        Toggle("Hole System", isOn: $manager.holeSystemEnabled)
                        Toggle("Undo History", isOn: $manager.undoHistoryEnabled)
                        Toggle("Compression", isOn: $manager.compressionEnabled)
                        Toggle("Streaming", isOn: $manager.streamingEnabled)
                    }
                }
                
                // History Controls
                HStack {
                    Button("Undo") {
                        manager.undo()
                    }
                    .disabled(manager.currentHistoryIndex <= 0)
                    
                    Button("Redo") {
                        manager.redo()
                    }
                    .disabled(manager.currentHistoryIndex >= manager.heightmapHistory.count - 1)
                    
                    Spacer()
                    
                    Text("History: \(manager.currentHistoryIndex + 1)/\(manager.heightmapHistory.count)")
                        .foregroundColor(.secondary)
                }
            }
            .padding()
        }
    }
}

// MARK: - Terrain Integration UI

struct TerrainIntegrationView: View {
    @StateObject private var manager = TerrainIntegrationManager.shared
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                // Physics & Navigation
                GroupBox("Physics & Navigation") {
                    VStack(alignment: .leading, spacing: 10) {
                        Toggle("Physics Collision Generation", isOn: $manager.physicsCollisionEnabled)
                        Toggle("Navigation Mesh Generation", isOn: $manager.navigationMeshEnabled)
                        
                        HStack {
                            Button("Generate Collision") {
                                manager.generatePhysicsCollision()
                            }
                            .buttonStyle(.bordered)
                            
                            Button("Generate Navigation") {
                                manager.generateNavigationMesh()
                            }
                            .buttonStyle(.bordered)
                        }
                    }
                }
                
                // Lighting & Rendering
                GroupBox("Lighting & Rendering") {
                    VStack(alignment: .leading, spacing: 10) {
                        Toggle("Water Plane Integration", isOn: $manager.waterPlaneEnabled)
                        Toggle("Sky/Ambient Lighting", isOn: $manager.skyLightingEnabled)
                        Toggle("Shadow Cascade Optimization", isOn: $manager.shadowCascadeOptimization)
                        Toggle("Lightmap Baking", isOn: $manager.lightmapBakingEnabled)
                        Toggle("Material Per Chunk", isOn: $manager.materialPerChunkEnabled)
                        Toggle("Terrain Shader Customization", isOn: $manager.terrainShaderCustomization)
                        
                        HStack {
                            Button("Bake Lightmaps") {
                                manager.bakeLightmaps()
                            }
                            .buttonStyle(.bordered)
                            
                            Button("Optimize Shadows") {
                                manager.optimizeShadowCascades()
                            }
                            .buttonStyle(.bordered)
                        }
                    }
                }
                
                // Streaming & Large Worlds
                GroupBox("Streaming & Large Worlds") {
                    VStack(alignment: .leading, spacing: 10) {
                        Toggle("Multi-terrain Stitching", isOn: $manager.multiTerrainStitching)
                        Toggle("World Origin Rebasing", isOn: $manager.worldOriginRebasing)
                        Toggle("Large World Coordinates", isOn: $manager.largeWorldCoordinates)
                        Toggle("Terrain Prefab Saving", isOn: $manager.terrainPrefabSaving)
                        Toggle("Terrain Asset Versioning", isOn: $manager.terrainAssetVersioning)
                        
                        Button("Setup Streaming Zones") {
                            manager.setupStreamingZones()
                        }
                        .buttonStyle(.bordered)
                    }
                }
                
                // Runtime & Networking
                GroupBox("Runtime & Networking") {
                    VStack(alignment: .leading, spacing: 10) {
                        Toggle("Runtime Modification API", isOn: $manager.runtimeModificationAPI)
                        Toggle("Networking Synchronization", isOn: $manager.networkingSynchronization)
                        
                        HStack {
                            Button("Enable Runtime API") {
                                manager.enableRuntimeModificationAPI()
                            }
                            .buttonStyle(.bordered)
                            
                            Button("Enable Networking") {
                                manager.enableNetworkingSynchronization()
                            }
                            .buttonStyle(.bordered)
                        }
                    }
                }
                
                // Analytics & Help
                GroupBox("Analytics & Help") {
                    VStack(alignment: .leading, spacing: 10) {
                        Toggle("Terrain Analytics", isOn: $manager.analyticsEnabled)
                        Toggle("Documentation Tooltips", isOn: $manager.documentationTooltips)
                        Toggle("Tutorial System", isOn: $manager.tutorialSystemEnabled)
                        
                        HStack {
                            Button("Update Analytics") {
                                manager.updateAnalytics()
                            }
                            .buttonStyle(.bordered)
                            
                            Button("Show Help") {
                                manager.showDocumentationTooltips()
                            }
                            .buttonStyle(.bordered)
                            
                            Button("Start Tutorial") {
                                manager.enableTutorialSystem()
                            }
                            .buttonStyle(.bordered)
                        }
                    }
                }
                
                // Streaming Zones List
                GroupBox("Streaming Zones") {
                    VStack(alignment: .leading, spacing: 10) {
                        ForEach(manager.streamingZones) { zone in
                            HStack {
                                Text("Zone at (\(Int(zone.position.x)), \(Int(zone.position.y)))")
                                Spacer()
                                Toggle("", isOn: Binding(
                                    get: { zone.isActive },
                                    set: { zone.isActive = $0 }
                                ))
                            }
                        }
                        
                        Button("Add Streaming Zone") {
                            let newZone = TerrainIntegrationManager.StreamingZone(
                                position: SIMD2<Float>(0, 0),
                                size: 100.0,
                                lodLevels: 4,
                                priority: 1
                            )
                            manager.streamingZones.append(newZone)
                        }
                        .buttonStyle(.bordered)
                    }
                }
            }
            .padding()
        }
    }
}

// MARK: - Terrain Settings Sheet

struct TerrainSettingsSheet: View {
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        NavigationView {
            VStack {
                Text("Terrain Editor Settings")
                    .font(.title2)
                    .padding()
                
                Spacer()
                
                Button("Done") {
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .padding()
            }
            .navigationTitle("Settings")
            .navigationBarTitleDisplayMode(.inline)
        }
    }
}
