import Foundation
import AVFoundation
import Accelerate

// MARK: - Audio Timing System
/// Provides precise timing information for syncing character actions to music
class AudioTimingSystem: ObservableObject {
    static let shared = AudioTimingSystem()
    
    // MARK: - Published Properties
    @Published var currentBeat: Int = 0
    @Published var beatPhase: Double = 0.0  // 0.0-1.0 within current beat
    @Published var bpm: Double = 120.0
    @Published var timeSignature: (beats: Int, noteValue: Int) = (4, 4)
    @Published var isPlaying: Bool = false
    
    // Beat markers
    @Published var beatMarkers: [BeatMarker] = []
    @Published var actionTriggers: [ActionTrigger] = []
    
    // Timing
    private var startTime: TimeInterval = 0
    private var currentTime: TimeInterval = 0
    private var beatDuration: TimeInterval = 0.5  // Updated based on BPM
    
    // Callbacks for actions
    private var beatCallbacks: [(BeatMarker) -> Void] = []
    
    // MARK: - Initialization
    
    init() {
        updateBeatDuration()
    }
    
    // MARK: - Public API
    
    /// Set BPM and update beat duration
    func setBPM(_ newBPM: Double) {
        bpm = max(20, min(newBPM, 300))  // Clamp to reasonable range
        updateBeatDuration()
    }
    
    /// Set time signature
    func setTimeSignature(beats: Int, noteValue: Int) {
        timeSignature = (beats, noteValue)
    }
    
    /// Start timing from beginning
    func start() {
        startTime = CACurrentMediaTime()
        currentTime = 0
        currentBeat = 0
        beatPhase = 0.0
        isPlaying = true
    }
    
    /// Stop timing
    func stop() {
        isPlaying = false
    }
    
    /// Update timing (call every frame)
    func update() {
        guard isPlaying else { return }
        
        currentTime = CACurrentMediaTime() - startTime
        
        // Calculate current beat
        let totalBeats = currentTime / beatDuration
        currentBeat = Int(totalBeats)
        beatPhase = totalBeats.truncatingRemainder(dividingBy: 1.0)
        
        // Check for beat markers
        checkBeatMarkers()
        
        // Check for action triggers
        checkActionTriggers()
    }
    
    /// Add beat marker at specific time
    func addBeatMarker(at beat: Int, type: BeatMarkerType, intensity: Double = 1.0) {
        let marker = BeatMarker(beat: beat, type: type, intensity: intensity)
        beatMarkers.append(marker)
        beatMarkers.sort { $0.beat < $1.beat }
    }
    
    /// Add action trigger
    func addActionTrigger(_ trigger: ActionTrigger) {
        actionTriggers.append(trigger)
        actionTriggers.sort { $0.beat < $1.beat }
    }
    
    /// Register callback for beat events
    func onBeat(callback: @escaping (BeatMarker) -> Void) {
        beatCallbacks.append(callback)
    }
    
    /// Get time until next beat
    func getTimeToNextBeat() -> TimeInterval {
        return (1.0 - beatPhase) * beatDuration
    }
    
    /// Get time until specific beat
    func getTimeToBeat(_ targetBeat: Int) -> TimeInterval {
        let beatsRemaining = Double(targetBeat - currentBeat) - beatPhase
        return beatsRemaining * beatDuration
    }
    
    /// Check if we're on a specific beat subdivision (eighth, quarter, etc.)
    func isOnSubdivision(_ subdivision: BeatSubdivision) -> Bool {
        let subdivisionCount = subdivision.rawValue
        let currentSubdivision = Int(beatPhase * Double(subdivisionCount))
        let previousSubdivision = Int((beatPhase - 0.05) * Double(subdivisionCount))
        return currentSubdivision != previousSubdivision
    }
    
    // MARK: - Private Methods
    
    private func updateBeatDuration() {
        beatDuration = 60.0 / bpm
    }
    
    private func checkBeatMarkers() {
        for marker in beatMarkers {
            if marker.beat == currentBeat && !marker.triggered && beatPhase < 0.1 {
                marker.triggered = true
                
                // Trigger callbacks
                for callback in beatCallbacks {
                    callback(marker)
                }
            } else if marker.beat < currentBeat - 1 {
                marker.triggered = false  // Reset for loop
            }
        }
    }
    
    private func checkActionTriggers() {
        for trigger in actionTriggers {
            if trigger.beat == currentBeat && !trigger.triggered && beatPhase < 0.1 {
                trigger.triggered = true
                trigger.execute()
            } else if trigger.beat < currentBeat - 1 {
                trigger.triggered = false  // Reset for loop
            }
        }
    }
}

// MARK: - Supporting Types

class BeatMarker: Identifiable, ObservableObject {
    let id = UUID()
    let beat: Int
    let type: BeatMarkerType
    let intensity: Double  // 0.0-1.0
    var triggered: Bool = false
    
    init(beat: Int, type: BeatMarkerType, intensity: Double) {
        self.beat = beat
        self.type = type
        self.intensity = intensity
    }
}

enum BeatMarkerType: String, CaseIterable {
    case downbeat = "Downbeat"       // Strong beat (1)
    case beat = "Beat"               // Normal beat
    case offbeat = "Offbeat"         // Weak beat
    case accent = "Accent"           // Accented note
    case step = "Step"               // Foot step marker
    case jump = "Jump"               // Jump action
    case crouch = "Crouch"           // Crouch action
    
    var icon: String {
        switch self {
        case .downbeat: return "1.circle.fill"
        case .beat: return "music.note"
        case .offbeat: return "music.quarternote.3"
        case .accent: return "exclamationmark.circle.fill"
        case .step: return "shoeprints.fill"
        case .jump: return "arrow.up.circle.fill"
        case .crouch: return "arrow.down.circle.fill"
        }
    }
}

class ActionTrigger: Identifiable, ObservableObject {
    let id = UUID()
    let beat: Int
    let actionType: CharacterAction
    var triggered: Bool = false
    var callback: (() -> Void)?
    
    init(beat: Int, actionType: CharacterAction, callback: (() -> Void)? = nil) {
        self.beat = beat
        self.actionType = actionType
        self.callback = callback
    }
    
    func execute() {
        callback?()
    }
}

enum CharacterAction: String, CaseIterable {
    case walk = "Walk"
    case run = "Run"
    case jump = "Jump"
    case crouch = "Crouch"
    case idle = "Idle"
    case dance = "Dance"
    case turnLeft = "Turn Left"
    case turnRight = "Turn Right"
    
    var icon: String {
        switch self {
        case .walk: return "figure.walk"
        case .run: return "figure.run"
        case .jump: return "figure.jumprope"
        case .crouch: return "arrow.down.to.line"
        case .idle: return "figure.stand"
        case .dance: return "figure.dance"
        case .turnLeft: return "arrow.turn.up.left"
        case .turnRight: return "arrow.turn.up.right"
        }
    }
}

enum BeatSubdivision: Int {
    case whole = 1
    case half = 2
    case quarter = 4
    case eighth = 8
    case sixteenth = 16
}

// MARK: - Audio Import Manager
class AudioImportManager: ObservableObject {
    static let shared = AudioImportManager()
    
    @Published var importedTracks: [ImportedAudioTrack] = []
    
    /// Import audio file and analyze it
    func importAudio(from url: URL, completion: @escaping (ImportedAudioTrack?) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            do {
                let audioFile = try AVAudioFile(forReading: url)
                
                // Create track
                let track = ImportedAudioTrack(
                    name: url.deletingPathExtension().lastPathComponent,
                    url: url,
                    duration: Double(audioFile.length) / audioFile.fileFormat.sampleRate
                )
                
                // Analyze audio for BPM (basic onset detection)
                self.analyzeBPM(audioFile: audioFile, track: track)
                
                DispatchQueue.main.async {
                    self.importedTracks.append(track)
                    completion(track)
                }
                
            } catch {
                print("Failed to import audio: \(error)")
                DispatchQueue.main.async {
                    completion(nil)
                }
            }
        }
    }
    
    /// Analyze BPM using onset detection
    private func analyzeBPM(audioFile: AVAudioFile, track: ImportedAudioTrack) {
        // Simple BPM detection based on energy peaks
        let frameLength = min(Int(audioFile.length), 44100 * 30)  // Analyze first 30 seconds
        guard let buffer = AVAudioPCMBuffer(pcmFormat: audioFile.processingFormat, frameCapacity: AVAudioFrameCount(frameLength)) else { return }
        
        do {
            try audioFile.read(into: buffer)
            
            // Detect onsets
            let onsets = detectOnsets(buffer: buffer)
            
            // Calculate average interval between onsets
            if onsets.count > 2 {
                var intervals: [TimeInterval] = []
                for i in 1..<onsets.count {
                    intervals.append(onsets[i] - onsets[i-1])
                }
                
                let avgInterval = intervals.reduce(0, +) / Double(intervals.count)
                let estimatedBPM = 60.0 / avgInterval
                
                DispatchQueue.main.async {
                    track.estimatedBPM = max(60, min(estimatedBPM, 200))  // Clamp to reasonable range
                    track.beatMarkers = self.generateBeatMarkers(from: onsets, bpm: track.estimatedBPM)
                }
            }
            
        } catch {
            print("BPM analysis failed: \(error)")
        }
    }
    
    private func detectOnsets(buffer: AVAudioPCMBuffer) -> [TimeInterval] {
        guard let channelData = buffer.floatChannelData else { return [] }
        
        let frameLength = Int(buffer.frameLength)
        let hopSize = 512
        let sampleRate = buffer.format.sampleRate
        
        var onsets: [TimeInterval] = []
        var previousEnergy: Float = 0
        
        for i in stride(from: 0, to: frameLength - hopSize, by: hopSize) {
            // Calculate energy in window
            var energy: Float = 0
            vDSP_svesq(channelData[0] + i, 1, &energy, vDSP_Length(hopSize))
            energy = sqrt(energy / Float(hopSize))
            
            // Detect onset (significant energy increase)
            if energy > previousEnergy * 1.5 && energy > 0.1 {
                let timeInSeconds = Double(i) / sampleRate
                onsets.append(timeInSeconds)
            }
            
            previousEnergy = energy * 0.9  // Smooth decay
        }
        
        return onsets
    }
    
    private func generateBeatMarkers(from onsets: [TimeInterval], bpm: Double) -> [BeatMarker] {
        let beatDuration = 60.0 / bpm
        var markers: [BeatMarker] = []
        
        for (_, onset) in onsets.enumerated() {
            let beat = Int(onset / beatDuration)
            let type: BeatMarkerType = (beat % 4 == 0) ? .downbeat : .beat
            markers.append(BeatMarker(beat: beat, type: type, intensity: 1.0))
        }
        
        return markers
    }
}

// MARK: - Imported Audio Track
class ImportedAudioTrack: Identifiable, ObservableObject {
    let id = UUID()
    let name: String
    let url: URL
    let duration: TimeInterval
    
    @Published var estimatedBPM: Double = 120.0
    @Published var beatMarkers: [BeatMarker] = []
    @Published var isAnalyzed: Bool = false
    
    init(name: String, url: URL, duration: TimeInterval) {
        self.name = name
        self.url = url
        self.duration = duration
    }
}
