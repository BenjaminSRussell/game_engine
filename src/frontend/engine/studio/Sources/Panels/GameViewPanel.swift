import SwiftUI
import MetalKit

// MARK: - Game View Panel

struct GameViewPanel: View {
    @StateObject private var viewModel = GameViewModel()
    @State private var selectedResolution: Resolution = .resolution1920x1080
    @State private var showStats = true
    
    enum Resolution: String, CaseIterable {
        case resolution1920x1080 = "1920×1080 (Full HD)"
        case resolution1280x720 = "1280×720 (HD)"
        case resolution2560x1440 = "2560×1440 (2K)"
        case resolution3840x2160 = "3840×2160 (4K)"
        case resolution16x9 = "16:9 (Custom)"
        case resolution4x3 = "4:3 (Classic)"
        
        var size: CGSize {
            switch self {
            case .resolution1920x1080: return CGSize(width: 1920, height: 1080)
            case .resolution1280x720: return CGSize(width: 1280, height: 720)
            case .resolution2560x1440: return CGSize(width: 2560, height: 1440)
            case .resolution3840x2160: return CGSize(width: 3840, height: 2160)
            case .resolution16x9: return CGSize(width: 1920, height: 1080)
            case .resolution4x3: return CGSize(width: 1024, height: 768)
            }
        }
        
        var aspectRatio: CGFloat {
            return size.width / size.height
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Title bar
            HStack {
                Text("Game View")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                // Resolution selector
                Menu {
                    ForEach(Resolution.allCases, id: \.self) { resolution in
                        Button(resolution.rawValue) {
                            selectedResolution = resolution
                        }
                    }
                } label: {
                    HStack {
                        Image(systemName: "aspectratio")
                        Text(selectedResolution.rawValue)
                        Image(systemName: "chevron.down")
                    }
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, DesignSystem.Spacing.sm)
                    .padding(.vertical, DesignSystem.Spacing.xs)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
                }
                .menuStyle(.borderlessButton)
                
                // Stats toggle
                EditorIconButton(icon: showStats ? "chart.bar.fill" : "chart.bar", tooltip: "Toggle Stats") {
                    showStats.toggle()
                }
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Play controls
            HStack(spacing: DesignSystem.Spacing.md) {
                // Play/Pause button
                Button(action: {
                    viewModel.togglePlayPause()
                }) {
                    HStack(spacing: DesignSystem.Spacing.sm) {
                        Image(systemName: viewModel.isPlaying ? "pause.fill" : "play.fill")
                        Text(viewModel.isPlaying ? "Pause" : "Play")
                    }
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(.white)
                    .padding(.horizontal, DesignSystem.Spacing.lg)
                    .padding(.vertical, DesignSystem.Spacing.sm)
                    .background(viewModel.isPlaying ? DesignSystem.Colors.accentWarning : DesignSystem.Colors.accentPrimary)
                    .cornerRadius(4)
                }
                .buttonStyle(.plain)
                
                // Step button
                EditorIconButton(icon: "forward.frame", tooltip: "Step Frame") {
                    viewModel.stepFrame()
                }
                .disabled(viewModel.isPlaying)
                
                // Stop button
                EditorIconButton(icon: "stop.fill", tooltip: "Stop") {
                    viewModel.stop()
                }
                
                Spacer()
                
                // Time display
                Text(String(format: "Time: %.2fs", viewModel.elapsedTime))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Game viewport
            GeometryReader { geometry in
                ZStack {
                    Color.black
                    
                    // Runtime preview (Metal view would go here)
                    GameRuntimeView(isPlaying: viewModel.isPlaying)
                        .aspectRatio(selectedResolution.aspectRatio, contentMode: .fit)
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                    
                    // Performance stats overlay
                    if showStats {
                        VStack {
                            Spacer()
                            
                            HStack {
                                Spacer()
                                
                                PerformanceStatsOverlay(stats: viewModel.stats)
                                    .padding(DesignSystem.Spacing.md)
                            }
                        }
                    }
                }
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Game Runtime View

private struct GameRuntimeView: View {
    let isPlaying: Bool
    
    var body: some View {
        ZStack {
            // Placeholder for Metal rendering
            Rectangle()
                .fill(LinearGradient(
                    colors: [Color.blue.opacity(0.3), Color.purple.opacity(0.3)],
                    startPoint: .topLeading,
                    endPoint: .bottomTrailing
                ))
            
            VStack {
                Image(systemName: isPlaying ? "play.circle.fill" : "pause.circle.fill")
                    .font(.system(size: 64))
                    .foregroundColor(.white.opacity(0.5))
                
                Text(isPlaying ? "Game Running" : "Game Paused")
                    .font(DesignSystem.Typography.h3)
                    .foregroundColor(.white.opacity(0.7))
                
                Text("Metal viewport integration pending")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(.white.opacity(0.5))
            }
        }
    }
}

// MARK: - Performance Stats Overlay

private struct PerformanceStatsOverlay: View {
    let stats: GameStats
    
    var body: some View {
        VStack(alignment: .trailing, spacing: 4) {
            StatRow(label: "FPS", value: String(format: "%.1f", stats.fps))
            StatRow(label: "Frame", value: String(format: "%.2fms", stats.frameTime))
            StatRow(label: "CPU", value: String(format: "%.1f%%", stats.cpuUsage))
            StatRow(label: "Memory", value: "\(stats.memoryUsed / 1024 / 1024)MB")
            StatRow(label: "Draw Calls", value: "\(stats.drawCalls)")
            StatRow(label: "Entities", value: "\(stats.entityCount)")
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary.opacity(0.9))
        .cornerRadius(8)
    }
}

// MARK: - Game Stats Model

struct GameStats {
    var fps: Double = 60.0
    var frameTime: Double = 16.67
    var cpuUsage: Double = 45.0
    var memoryUsed: Int64 = 256 * 1024 * 1024
    var drawCalls: Int = 124
    var entityCount: Int = 42
}

// MARK: - View Model

class GameViewModel: ObservableObject {
    @Published var isPlaying = false
    @Published var elapsedTime: Double = 0.0
    @Published var stats = GameStats()
    
    private var timer: Timer?
    
    func togglePlayPause() {
        isPlaying.toggle()
        
        if isPlaying {
            startTimer()
        } else {
            stopTimer()
        }
    }
    
    func stepFrame() {
        // Step one frame forward
        elapsedTime += 1.0 / 60.0
        updateStats()
    }
    
    func stop() {
        isPlaying = false
        stopTimer()
        elapsedTime = 0.0
        stats = GameStats()
    }
    
    private func startTimer() {
        timer = Timer.scheduledTimer(withTimeInterval: 1.0 / 60.0, repeats: true) { [weak self] _ in
            self?.elapsedTime += 1.0 / 60.0
            self?.updateStats()
        }
    }
    
    private func stopTimer() {
        timer?.invalidate()
        timer = nil
    }
    
    private func updateStats() {
        // Simulate varying stats
        stats.fps = 60.0 + Double.random(in: -5...5)
        stats.frameTime = 1000.0 / stats.fps
        stats.cpuUsage = 45.0 + Double.random(in: -10...10)
        stats.memoryUsed = Int64(256 * 1024 * 1024 + Int.random(in: -50...50) * 1024 * 1024)
        stats.drawCalls = 124 + Int.random(in: -20...20)
        stats.entityCount = 42 + Int.random(in: -5...5)
    }
}

// MARK: - Preview

#Preview {
    GameViewPanel()
        .frame(height: 600)
}
