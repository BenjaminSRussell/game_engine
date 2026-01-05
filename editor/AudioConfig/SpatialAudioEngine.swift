import Foundation
import Combine

class SpatialAudioEngine: ObservableObject {
    @Published var speakerLevels: [String: Double] = [:]
    @Published var isActive = false
    
    private var updateTimer: Timer?
    private var audioContext: OpaquePointer?
    
    init() {
        // Initialize C++ audio engine
        audioContext = spatial_audio_create()
        startMonitoring()
    }
    
    deinit {
        stopMonitoring()
        if let context = audioContext {
            spatial_audio_destroy(context)
        }
    }
    
    func setFormat(_ format: AudioFormat) {
        guard let context = audioContext else { return }
        
        let formatEnum: Int32
        switch format {
        case .dolbyAtmos:
            formatEnum = 0
        case .dtsX:
            formatEnum = 1
        case .customSurround:
            formatEnum = 2
        }
        
        spatial_audio_set_format(context, formatEnum)
        resetSpeakerLevels(for: format)
    }
    
    func playTestTone() {
        guard let context = audioContext else { return }
        spatial_audio_play_test_tone(context)
    }
    
    func setSpeakerGain(channel: String, gain: Double) {
        guard let context = audioContext else { return }
        spatial_audio_set_speaker_gain(context, channel, Float(gain))
    }
    
    func setMasterVolume(_ volume: Double) {
        guard let context = audioContext else { return }
        spatial_audio_set_master_volume(context, Float(volume))
    }
    
    private func startMonitoring() {
        updateTimer = Timer.scheduledTimer(withTimeInterval: 0.033, repeats: true) { [weak self] _ in
            self?.updateLevels()
        }
    }
    
    private func stopMonitoring() {
        updateTimer?.invalidate()
        updateTimer = nil
    }
    
    private func updateLevels() {
        guard let context = audioContext else { return }
        
        // Update levels for all active speakers
        for channel in speakerLevels.keys {
            let level = spatial_audio_get_speaker_level(context, channel)
            speakerLevels[channel] = Double(level)
        }
    }
    
    private func resetSpeakerLevels(for format: AudioFormat) {
        speakerLevels.removeAll()
        
        let speakers = format.speakers
        for speaker in speakers {
            speakerLevels[speaker.channel] = 0.0
        }
    }
}

// C API Bridge
@_silgen_name("spatial_audio_create")
func spatial_audio_create() -> OpaquePointer?

@_silgen_name("spatial_audio_destroy")
func spatial_audio_destroy(_ context: OpaquePointer)

@_silgen_name("spatial_audio_set_format")
func spatial_audio_set_format(_ context: OpaquePointer, _ format: Int32)

@_silgen_name("spatial_audio_play_test_tone")
func spatial_audio_play_test_tone(_ context: OpaquePointer)

@_silgen_name("spatial_audio_set_speaker_gain")
func spatial_audio_set_speaker_gain(_ context: OpaquePointer, _ channel: String, _ gain: Float)

@_silgen_name("spatial_audio_set_master_volume")
func spatial_audio_set_master_volume(_ context: OpaquePointer, _ volume: Float)

@_silgen_name("spatial_audio_get_speaker_level")
func spatial_audio_get_speaker_level(_ context: OpaquePointer, _ channel: String) -> Float
