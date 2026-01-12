import SwiftUI

// 
// MARK: - CATEGORY 5: ANIMATION SYSTEM (TODO-2601 to TODO-3100)
// 

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
    
    private var timer: Timer?
    
    func play() {
        isPlaying = true
        timer = Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { [weak self] _ in
            guard let self = self, let clip = self.currentClip else { return }
            self.currentTime += Float(1.0/60.0) * self.playbackSpeed
            if self.currentTime >= clip.duration {
                if self.isLooping {
                    self.currentTime = 0
                } else {
                    self.pause()
                }
            }
        }
    }
    
    func pause() {
        isPlaying = false
        timer?.invalidate()
        timer = nil
    }
    
    func stop() {
        pause()
        currentTime = 0
    }
    
    func seekTo(_ time: Float) {
        currentTime = max(0, min(time, currentClip?.duration ?? 0))
    }
    
    func addKeyframe(property: String, value: Any) {
        guard var clip = currentClip else { return }
        let keyframe = AnimationKeyframe(time: currentTime, value: value)
        if var track = clip.tracks.first(where: { $0.propertyPath == property }) {
            track.keyframes.append(keyframe)
        }
        currentClip = clip
    }
}

// MARK: - Animation Models
struct AnimationClip: Identifiable {
    let id = UUID()
    var name: String
    var duration: Float
    var frameRate: Float = 60.0
    var tracks: [AnimationTrack] = []
    var events: [AnimationEvent] = []
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

struct AnimationKeyframe: Identifiable {
    let id = UUID()
    var time: Float
    var value: Any
    var inTangent: Float = 0
    var outTangent: Float = 0
}

struct AnimationEvent: Identifiable {
    let id = UUID()
    var time: Float
    var functionName: String
    var parameter: String = ""
}

// MARK: - Animation Timeline View
struct AnimationTimelineView: View {
    @ObservedObject var manager = AnimationEditorManager.shared
    @State private var showDopesheet = true
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            AnimationToolbar(manager: manager, showDopesheet: $showDopesheet)
            
            EditorDivider()
            
            HSplitView {
                // Property list
                AnimationPropertyList()
                    .frame(minWidth: 200, maxWidth: 250)
                
                // Timeline
                VStack(spacing: 0) {
                    // Time ruler
                    TimeRuler(duration: manager.currentClip?.duration ?? 10, currentTime: manager.currentTime, zoom: manager.zoomLevel)
                    
                    EditorDivider()
                    
                    // Keyframe area
                    if showDopesheet {
                        DopesheetView(manager: manager)
                    } else {
                        CurveEditorView(manager: manager)
                    }
                }
            }
            
            EditorDivider()
            
            // Playback controls
            AnimationPlaybackControls(manager: manager)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
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

// MARK: - Animation Property List
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

// MARK: - Time Ruler
struct TimeRuler: View {
    let duration: Float
    let currentTime: Float
    let zoom: Float
    
    var body: some View {
        GeometryReader { geometry in
            ZStack(alignment: .leading) {
                // Background
                Rectangle()
                    .fill(DesignSystem.Colors.backgroundTertiary)
                
                // Time markers
                ForEach(0..<Int(duration) + 1, id: \.self) { second in
                    let x = CGFloat(Float(second) / duration) * geometry.size.width * CGFloat(zoom)
                    VStack {
                        Rectangle()
                            .fill(DesignSystem.Colors.textSecondary)
                            .frame(width: 1, height: 8)
                        Text("\(second)s")
                            .font(.system(size: 9))
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .position(x: x, y: geometry.size.height / 2)
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
}

// MARK: - Dopesheet View
struct DopesheetView: View {
    @ObservedObject var manager: AnimationEditorManager
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Grid
                DopesheetGrid(duration: manager.currentClip?.duration ?? 10, size: geometry.size)
                
                // Keyframes (demo)
                Circle()
                    .fill(DesignSystem.Colors.accentPrimary)
                    .frame(width: 12, height: 12)
                    .position(x: 100, y: 30)
                
                Circle()
                    .fill(DesignSystem.Colors.accentPrimary)
                    .frame(width: 12, height: 12)
                    .position(x: 200, y: 30)
                
                Circle()
                    .fill(DesignSystem.Colors.accentPrimary)
                    .frame(width: 12, height: 12)
                    .position(x: 350, y: 60)
                
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

struct DopesheetGrid: View {
    let duration: Float
    let size: CGSize
    
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
            for i in 0..<10 {
                let y = CGFloat(i * 30)
                var path = Path()
                path.move(to: CGPoint(x: 0, y: y))
                path.addLine(to: CGPoint(x: size.width, y: y))
                context.stroke(path, with: .color(DesignSystem.Colors.border.opacity(0.2)), lineWidth: 1)
            }
        }
    }
}

// MARK: - Curve Editor View
struct CurveEditorView: View {
    @ObservedObject var manager: AnimationEditorManager
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Grid
                CurveEditorGrid(size: geometry.size)
                
                // Demo curve
                Path { path in
                    path.move(to: CGPoint(x: 0, y: geometry.size.height * 0.8))
                    path.addCurve(
                        to: CGPoint(x: geometry.size.width * 0.3, y: geometry.size.height * 0.2),
                        control1: CGPoint(x: geometry.size.width * 0.1, y: geometry.size.height * 0.8),
                        control2: CGPoint(x: geometry.size.width * 0.2, y: geometry.size.height * 0.2)
                    )
                    path.addCurve(
                        to: CGPoint(x: geometry.size.width, y: geometry.size.height * 0.5),
                        control1: CGPoint(x: geometry.size.width * 0.5, y: geometry.size.height * 0.2),
                        control2: CGPoint(x: geometry.size.width * 0.8, y: geometry.size.height * 0.5)
                    )
                }
                .stroke(Color.red, lineWidth: 2)
                
                // Keyframe handles
                Circle()
                    .fill(Color.red)
                    .frame(width: 10, height: 10)
                    .position(x: 0, y: geometry.size.height * 0.8)
                
                Circle()
                    .fill(Color.red)
                    .frame(width: 10, height: 10)
                    .position(x: geometry.size.width * 0.3, y: geometry.size.height * 0.2)
                
                Circle()
                    .fill(Color.red)
                    .frame(width: 10, height: 10)
                    .position(x: geometry.size.width, y: geometry.size.height * 0.5)
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

struct CurveEditorGrid: View {
    let size: CGSize
    
    var body: some View {
        Canvas { context, size in
            // Horizontal center line
            var centerPath = Path()
            centerPath.move(to: CGPoint(x: 0, y: size.height / 2))
            centerPath.addLine(to: CGPoint(x: size.width, y: size.height / 2))
            context.stroke(centerPath, with: .color(DesignSystem.Colors.border), lineWidth: 1)
            
            // Grid
            for i in 0..<20 {
                let x = (CGFloat(i) / 20) * size.width
                var path = Path()
                path.move(to: CGPoint(x: x, y: 0))
                path.addLine(to: CGPoint(x: x, y: size.height))
                context.stroke(path, with: .color(DesignSystem.Colors.border.opacity(0.2)), lineWidth: 1)
            }
            
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

// MARK: - Playback Controls
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
