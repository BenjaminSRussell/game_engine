import Foundation
import AVFoundation

// MARK: - Background Music System
/// Manages background music playback with crossfades and adaptive layers
class BackgroundMusicSystem: ObservableObject {
    static let shared = BackgroundMusicSystem()
    
    @Published var currentTrack: MusicTrack?
    @Published var isPlaying: Bool = false
    @Published var volume: Double = 0.8
    @Published var currentTime: TimeInterval = 0.0
    @Published var duration: TimeInterval = 0.0
    @Published var musicState: MusicState = .calm
    
    private var audioEngine: AVAudioEngine
    private var playerNode: AVAudioPlayerNode
    private var mixerNode: AVAudioMixerNode
    
    // Layer management for adaptive music
    private var layerPlayers: [UUID: AVAudioPlayerNode] = [:]
    private var audioFiles: [UUID: AVAudioFile] = [:]
    
    // Crossfade
    private var fadeTimer: Timer?
    private var isCrossfading: Bool = false
    
    init() {
        audioEngine = AVAudioEngine()
        playerNode = AVAudioPlayerNode()
        mixerNode = AVAudioMixerNode()
        
        setupAudioEngine()
    }
    
    // MARK: - Setup
    
    private func setupAudioEngine() {
        audioEngine.attach(playerNode)
        audioEngine.attach(mixerNode)
        
        audioEngine.connect(playerNode, to: mixerNode, format: nil)
        audioEngine.connect(mixerNode, to: audioEngine.mainMixerNode, format: nil)
        
        do {
            try audioEngine.start()
        } catch {
            print("Audio engine failed to start: \(error)")
        }
    }
    
    // MARK: - Track Management
    
    func loadTrack(url: URL) {
        do {
            let audioFile = try AVAudioFile(forReading: url)
            let track = MusicTrack(
                name: url.deletingPathExtension().lastPathComponent,
                url: url,
                loopPoints: nil,
                layers: []
            )
            
            currentTrack = track
            duration = Double(audioFile.length) / audioFile.fileFormat.sampleRate
            
        } catch {
            print("Failed to load track: \(error)")
        }
    }
    
    func loadTrack(_ track: MusicTrack) {
        currentTrack = track
        
        // Load main track
        if let audioFile = try? AVAudioFile(forReading: track.url) {
            duration = Double(audioFile.length) / audioFile.fileFormat.sampleRate
        }
        
        // Load layers
        for layer in track.layers {
            if let audioFile = try? AVAudioFile(forReading: layer.url) {
                let layerPlayer = AVAudioPlayerNode()
                audioEngine.attach(layerPlayer)
                audioEngine.connect(layerPlayer, to: mixerNode, format: audioFile.processingFormat)
                
                layerPlayers[layer.id] = layerPlayer
                audioFiles[layer.id] = audioFile
            }
        }
    }
    
    // MARK: - Playback Control
    
    func play() {
        guard let track = currentTrack else { return }
        
        do {
            let audioFile = try AVAudioFile(forReading: track.url)
            
            playerNode.scheduleFile(audioFile, at: nil) {
                // Loop if needed
                if track.loopPoints != nil {
                    self.scheduleLoop(track: track)
                }
            }
            
            playerNode.play()
            isPlaying = true
            
            // Play active layers
            for layer in track.layers where layer.isActive {
                playLayer(layer)
            }
            
        } catch {
            print("Failed to play track: \(error)")
        }
    }
    
    func stop() {
        playerNode.stop()
        isPlaying = false
        currentTime = 0.0
        
        // Stop all layers
        for (_, player) in layerPlayers {
            player.stop()
        }
    }
    
    func pause() {
        playerNode.pause()
        isPlaying = false
        
        for (_, player) in layerPlayers {
            player.pause()
        }
    }
    
    func resume() {
        playerNode.play()
        isPlaying = true
        
        guard let track = currentTrack else { return }
        for layer in track.layers where layer.isActive {
            if let player = layerPlayers[layer.id] {
                player.play()
            }
        }
    }
    
    // MARK: - Crossfade
    
    func crossfadeTo(track: MusicTrack, duration: TimeInterval) {
        guard !isCrossfading else { return }
        isCrossfading = true
        
        let oldVolume = volume
        let steps = Int(duration * 60)  // 60 FPS
        var currentStep = 0
        
        fadeTimer = Timer.scheduledTimer(withTimeInterval: 1.0 / 60.0, repeats: true) { [weak self] timer in
            guard let self = self else {
                timer.invalidate()
                return
            }
            
            currentStep += 1
            let progress = Double(currentStep) / Double(steps)
            
            // Fade out current
            self.volume = oldVolume * (1.0 - progress)
            self.mixerNode.volume = Float(self.volume)
            
            if currentStep >= steps {
                timer.invalidate()
                self.stop()
                self.loadTrack(track)
                self.play()
                self.volume = oldVolume
                self.mixerNode.volume = Float(oldVolume)
                self.isCrossfading = false
            }
        }
    }
    
    // MARK: - Adaptive Music (Layer Control)
    
    func setMusicState(_ state: MusicState) {
        musicState = state
        
        guard let track = currentTrack else { return }
        
        // Update layer volumes based on state
        for i in 0..<track.layers.count {
            var layer = track.layers[i]
            
            switch state {
            case .calm:
                layer.volume = i == 0 ? 1.0 : 0.0  // Only base layer
            case .tense:
                layer.volume = i <= 1 ? 0.8 : 0.0  // Base + one layer
            case .combat:
                layer.volume = 1.0  // All layers
            case .victory:
                layer.volume = i == track.layers.count - 1 ? 1.0 : 0.3  // Emphasis on last layer
            }
            
            if let player = layerPlayers[layer.id] {
                player.volume = Float(layer.volume)
            }
        }
    }
    
    private func playLayer(_ layer: AudioLayer) {
        guard let player = layerPlayers[layer.id],
              let audioFile = audioFiles[layer.id] else { return }
        
        player.scheduleFile(audioFile, at: nil)
        player.volume = Float(layer.volume)
        player.play()
    }
    
    // MARK: - Loop Management
    
    private func scheduleLoop(track: MusicTrack) {
        guard let loopPoints = track.loopPoints else { return }
        
        do {
            let audioFile = try AVAudioFile(forReading: track.url)
            let sampleRate = audioFile.fileFormat.sampleRate
            
            let startFrame = AVAudioFramePosition(loopPoints.start * sampleRate)
            let endFrame = AVAudioFramePosition(loopPoints.end * sampleRate)
            let frameCount = AVAudioFrameCount(endFrame - startFrame)
            
            playerNode.scheduleSegment(
                audioFile,
                startingFrame: startFrame,
                frameCount: frameCount,
                at: nil
            ) {
                self.scheduleLoop(track: track)
            }
            
        } catch {
            print("Failed to schedule loop: \(error)")
        }
    }
}

// MARK: - Supporting Types

struct MusicTrack: Identifiable {
    let id: UUID
    let name: String
    let url: URL
    let loopPoints: (start: TimeInterval, end: TimeInterval)?
    var layers: [AudioLayer]
    
    init(
        id: UUID = UUID(),
        name: String,
        url: URL,
        loopPoints: (start: TimeInterval, end: TimeInterval)?,
        layers: [AudioLayer]
    ) {
        self.id = id
        self.name = name
        self.url = url
        self.loopPoints = loopPoints
        self.layers = layers
    }
}

enum MusicState: String, CaseIterable {
    case calm = "Calm"
    case tense = "Tense"
    case combat = "Combat"
    case victory = "Victory"
}

struct AudioLayer: Identifiable {
    let id: UUID
    let url: URL
    var volume: Double
    var isActive: Bool
    
    init(id: UUID = UUID(), url: URL, volume: Double = 1.0, isActive: Bool = true) {
        self.id = id
        self.url = url
        self.volume = volume
        self.isActive = isActive
    }
}
