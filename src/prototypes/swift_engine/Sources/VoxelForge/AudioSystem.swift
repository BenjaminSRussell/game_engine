import Foundation
@preconcurrency import AVFoundation

@MainActor
public final class AudioSystem {
    public static let shared = AudioSystem()
    
    private let engine = AVAudioEngine()
    private let environment = AVAudioEnvironmentNode()
    
    private var buffers: [String: AVAudioPCMBuffer] = [:]
    
    private init() {
        engine.attach(environment)
        engine.connect(environment, to: engine.mainMixerNode, format: nil)
        
        do {
            try engine.start()
        } catch {
            Logger.error("Failed to start AudioEngine: \(error)")
        }
    }
    
    public func loadSound(_ name: String, from virtualPath: String) {
        let url = VirtualFileSystem.shared.resolve(virtualPath: virtualPath)
        
        do {
            let file = try AVAudioFile(forReading: url)
            let format = file.processingFormat
            let frameCount = AVAudioFrameCount(file.length)
            
            guard let buffer = AVAudioPCMBuffer(pcmFormat: format, frameCapacity: frameCount) else {
                Logger.error("Failed to create audio buffer for \(name)")
                return
            }
            
            try file.read(into: buffer)
            buffers[name] = buffer
            Logger.info("Loaded sound: \(name) (\(virtualPath))")
        } catch {
            Logger.error("Failed to load sound \(name): \(error)")
        }
    }
    
    public func updateListener(position: SIMD3<Float>, orientation: simd_quatf) {
        environment.listenerPosition = AVAudio3DPoint(x: position.x, y: position.y, z: position.z)
    }
    
    public func playSound(_ name: String, at position: SIMD3<Float>, volume: Float = 1.0) {
        guard let buffer = buffers[name] else {
            Logger.error("Sound not loaded: \(name)")
            return
        }
        
        let player = AVAudioPlayerNode()
        engine.attach(player)
        engine.connect(player, to: environment, format: buffer.format)
        
        player.position = AVAudio3DPoint(x: position.x, y: position.y, z: position.z)
        player.volume = volume
        
        player.scheduleBuffer(buffer, completionHandler: {
            Task { @MainActor in
                self.engine.detach(player)
            }
        })
        
        player.play()
    }
}
