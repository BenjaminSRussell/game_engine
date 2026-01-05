import Foundation
import AVFoundation
import Accelerate

// MARK: - Audio Analyzer
/// Analyzes audio in real-time for frequency content, beats, and amplitude
class AudioAnalyzer: ObservableObject {
    static let shared = AudioAnalyzer()
    
    // MARK: - Published Properties
    @Published var frequencyBands: FrequencyBands = FrequencyBands()
    @Published var currentAmplitude: Double = 0.0
    @Published var beatDetected: Bool = false
    @Published var bpm: Double = 0.0
    
    // MARK: - Configuration
    private let fftSize: Int = 1024
    private let sampleRate: Double = 44100.0
    private let hopSize: Int = 512
    
    // FFT Setup
    private var fftSetup: vDSP_DFT_Setup?
    private var windowBuffer: [Float]
    private var realBuffer: [Float]
    private var imagBuffer: [Float]
    private var magnitudes: [Float]
    
    // Beat Detection
    private var energyHistory: [Double] = []
    private let energyHistorySize = 43  // ~1 second at 43 FPS
    private var lastBeatTime: TimeInterval = 0
    private let minBeatInterval: TimeInterval = 0.3  // 200 BPM max
    
    // Smoothing
    private var smoothedBands: FrequencyBands = FrequencyBands()
    private let smoothingFactor: Double = 0.7
    
    // MARK: - Initialization
    init() {
        // Initialize buffers
        windowBuffer = [Float](repeating: 0, count: fftSize)
        realBuffer = [Float](repeating: 0, count: fftSize)
        imagBuffer = [Float](repeating: 0, count: fftSize)
        magnitudes = [Float](repeating: 0, count: fftSize / 2)
        
        // Create Hanning window
        vDSP_hann_window(&windowBuffer, vDSP_Length(fftSize), Int32(vDSP_HANN_NORM))
        
        // Create FFT setup
        fftSetup = vDSP_DFT_zop_CreateSetup(nil, vDSP_Length(fftSize), vDSP_DFT_Direction.FORWARD)
    }
    
    deinit {
        if let setup = fftSetup {
            vDSP_DFT_DestroySetup(setup)
        }
    }
    
    // MARK: - Public API
    
    /// Analyze audio buffer and update frequency bands, beats, etc.
    func analyze(buffer: AVAudioPCMBuffer) {
        guard let channelData = buffer.floatChannelData else { return }
        
        let frameLength = Int(buffer.frameLength)
        guard frameLength >= fftSize else { return }
        
        // Get mono audio (average channels if stereo)
        var monoData = [Float](repeating: 0, count: frameLength)
        if buffer.format.channelCount == 1 {
            monoData = Array(UnsafeBufferPointer(start: channelData[0], count: frameLength))
        } else {
            for i in 0..<frameLength {
                monoData[i] = (channelData[0][i] + channelData[1][i]) / 2.0
            }
        }
        
        // Process FFT
        performFFT(monoData: monoData)
        
        // Extract frequency bands
        extractFrequencyBands()
        
        // Calculate amplitude
        calculateAmplitude(monoData: monoData)
        
        // Detect beats
        detectBeat()
        
        // Apply smoothing
        applySmoothing()
    }
    
    // MARK: - FFT Processing
    
    private func performFFT(monoData: [Float]) {
        guard let setup = fftSetup else { return }
        
        // Apply window
        var windowed = [Float](repeating: 0, count: fftSize)
        vDSP_vmul(monoData, 1, windowBuffer, 1, &windowed, 1, vDSP_Length(fftSize))
        
        // Split into real and imaginary
        windowed.withUnsafeBufferPointer { windowedPtr in
            windowedPtr.baseAddress!.withMemoryRebound(to: DSPComplex.self, capacity: fftSize / 2) { complexPtr in
                var splitComplex = DSPSplitComplex(realp: &realBuffer, imagp: &imagBuffer)
                vDSP_ctoz(complexPtr, 2, &splitComplex, 1, vDSP_Length(fftSize / 2))
                
                // Perform FFT
                vDSP_DFT_Execute(setup, splitComplex.realp, splitComplex.imagp, splitComplex.realp, splitComplex.imagp)
            }
        }
        
        // Calculate magnitudes
        var zero: Float = 0.0
        vDSP_vdist(realBuffer, 1, imagBuffer, 1, &magnitudes, 1, vDSP_Length(fftSize / 2))
        vDSP_vsmul(magnitudes, 1, &zero, &magnitudes, 1, vDSP_Length(fftSize / 2))
    }
    
    // MARK: - Frequency Band Extraction
    
    private func extractFrequencyBands() {
        let binCount = fftSize / 2
        let binWidth = sampleRate / Double(fftSize)
        
        // Define frequency ranges
        let bassRange = (20.0, 250.0)
        let lowMidRange = (250.0, 500.0)
        let midRange = (500.0, 2000.0)
        let highMidRange = (2000.0, 4000.0)
        let trebleRange = (4000.0, 20000.0)
        
        // Calculate bin indices for each range
        func getBinRange(_ freqRange: (Double, Double)) -> (Int, Int) {
            let startBin = Int(freqRange.0 / binWidth)
            let endBin = min(Int(freqRange.1 / binWidth), binCount - 1)
            return (startBin, endBin)
        }
        
        // Sum magnitudes in each band
        func sumBand(_ range: (Int, Int)) -> Double {
            var sum: Float = 0.0
            let count = range.1 - range.0 + 1
            guard count > 0 else { return 0.0 }
            
            magnitudes.withUnsafeBufferPointer { ptr in
                guard let baseAddress = ptr.baseAddress else { return }
                vDSP_sve(baseAddress + range.0, 1, &sum, vDSP_Length(count))
            }
            return Double(sum) / Double(count)  // Average
        }
        
        frequencyBands.bass = sumBand(getBinRange(bassRange))
        frequencyBands.lowMid = sumBand(getBinRange(lowMidRange))
        frequencyBands.mid = sumBand(getBinRange(midRange))
        frequencyBands.highMid = sumBand(getBinRange(highMidRange))
        frequencyBands.treble = sumBand(getBinRange(trebleRange))
    }
    
    // MARK: - Amplitude Calculation
    
    private func calculateAmplitude(monoData: [Float]) {
        var rms: Float = 0.0
        var squaredSum: Float = 0.0
        
        vDSP_svesq(monoData, 1, &squaredSum, vDSP_Length(monoData.count))
        rms = sqrt(squaredSum / Float(monoData.count))
        
        currentAmplitude = Double(rms)
    }
    
    // MARK: - Beat Detection
    
    private func detectBeat() {
        // Calculate instant energy (sum of all frequency magnitudes)
        var energy: Float = 0.0
        vDSP_sve(magnitudes, 1, &energy, vDSP_Length(magnitudes.count))
        
        energyHistory.append(Double(energy))
        if energyHistory.count > energyHistorySize {
            energyHistory.removeFirst()
        }
        
        guard energyHistory.count >= 10 else {
            beatDetected = false
            return
        }
        
        // Calculate average energy
        let avgEnergy = energyHistory.reduce(0, +) / Double(energyHistory.count)
        
        // Calculate variance
        let variance = energyHistory.map { pow($0 - avgEnergy, 2) }.reduce(0, +) / Double(energyHistory.count)
        let stdDev = sqrt(variance)
        
        // Beat threshold: average + C * variance (C = 1.5)
        let threshold = avgEnergy + 1.5 * stdDev
        
        // Check if current energy exceeds threshold and enough time has passed
        let currentTime = CACurrentMediaTime()
        let timeSinceLastBeat = currentTime - lastBeatTime
        
        if Double(energy) > threshold && timeSinceLastBeat > minBeatInterval {
            beatDetected = true
            lastBeatTime = currentTime
            
            // Estimate BPM
            if timeSinceLastBeat > 0.1 {
                let instantBPM = 60.0 / timeSinceLastBeat
                bpm = bpm * 0.9 + instantBPM * 0.1  // Smooth BPM
            }
        } else {
            beatDetected = false
        }
    }
    
    // MARK: - Smoothing
    
    private func applySmoothing() {
        smoothedBands.bass = smoothedBands.bass * smoothingFactor + frequencyBands.bass * (1 - smoothingFactor)
        smoothedBands.lowMid = smoothedBands.lowMid * smoothingFactor + frequencyBands.lowMid * (1 - smoothingFactor)
        smoothedBands.mid = smoothedBands.mid * smoothingFactor + frequencyBands.mid * (1 - smoothingFactor)
        smoothedBands.highMid = smoothedBands.highMid * smoothingFactor + frequencyBands.highMid * (1 - smoothingFactor)
        smoothedBands.treble = smoothedBands.treble * smoothingFactor + frequencyBands.treble * (1 - smoothingFactor)
        
        frequencyBands = smoothedBands
    }
    
    // MARK: - Utility
    
    /// Get normalized frequency band (0-1 range)
    func getNormalizedBand(_ channel: AudioChannel) -> Double {
        let rawValue: Double
        switch channel {
        case .bass: rawValue = frequencyBands.bass
        case .lowMid: rawValue = frequencyBands.lowMid
        case .mid: rawValue = frequencyBands.mid
        case .highMid: rawValue = frequencyBands.highMid
        case .treble: rawValue = frequencyBands.treble
        case .beatStrength: rawValue = beatDetected ? 1.0 : 0.0
        case .amplitude: rawValue = currentAmplitude
        }
        
        // Normalize to 0-1 range (assuming max magnitude of 100)
        return min(max(rawValue / 100.0, 0.0), 1.0)
    }
}

// MARK: - Supporting Types

struct FrequencyBands {
    var bass: Double = 0.0      // 20-250 Hz
    var lowMid: Double = 0.0    // 250-500 Hz
    var mid: Double = 0.0       // 500-2000 Hz
    var highMid: Double = 0.0   // 2000-4000 Hz
    var treble: Double = 0.0    // 4000-20000 Hz
}

enum AudioChannel {
    case bass, lowMid, mid, highMid, treble
    case beatStrength
    case amplitude
    
    var displayName: String {
        switch self {
        case .bass: return "Bass (20-250 Hz)"
        case .lowMid: return "Low Mid (250-500 Hz)"
        case .mid: return "Mid (500-2000 Hz)"
        case .highMid: return "High Mid (2000-4000 Hz)"
        case .treble: return "Treble (4000+ Hz)"
        case .beatStrength: return "Beat Strength"
        case .amplitude: return "Amplitude"
        }
    }
}
