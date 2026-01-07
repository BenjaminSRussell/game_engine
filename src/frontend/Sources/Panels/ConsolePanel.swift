import SwiftUI
import AppKit

// MARK: - Console Panel

struct ConsolePanel: View {
    @StateObject private var viewModel = ConsoleViewModel()
    @State private var searchText = ""
    @State private var selectedFilter: UILogLevel = .all
    @State private var autoScroll = true
    
    var body: some View {
        VStack(spacing: 0) {
            // Title bar with controls
            HStack {
                Text("Console")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                // Filter buttons
                HStack(spacing: 2) {
                    ForEach([UILogLevel.all, .error, .warning, .info, .debug], id: \.self) { level in
                        FilterButton(
                            level: level,
                            count: viewModel.getCount(for: level),
                            isSelected: selectedFilter == level,
                            onSelect: { selectedFilter = level }
                        )
                    }
                }
                
                Divider()
                    .frame(height: 20)
                    .padding(.horizontal, DesignSystem.Spacing.xs)
                
                // Auto-scroll toggle
                Button(action: { autoScroll.toggle() }) {
                    Image(systemName: autoScroll ? "arrow.down.circle.fill" : "arrow.down.circle")
                        .foregroundColor(autoScroll ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Auto-scroll")
                
                // Clear button
                EditorIconButton(icon: "trash", tooltip: "Clear Console") {
                    viewModel.clearLogs()
                }
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Search bar
            EditorSearchBar(text: $searchText, placeholder: "Search logs...")
                .padding(DesignSystem.Spacing.sm)
            
            EditorDivider()
            
            // Log entries
            ScrollViewReader { proxy in
                ScrollView {
                    LazyVStack(alignment: .leading, spacing: 0) {
                        ForEach(filteredLogs) { log in
                            LogEntryView(log: log)
                                .id(log.id)
                        }
                    }
                }
                .onChange(of: viewModel.logs.count) { _ in
                    if autoScroll, let lastLog = viewModel.logs.last {
                        withAnimation {
                            proxy.scrollTo(lastLog.id, anchor: .bottom)
                        }
                    }
                }
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
    
    private var filteredLogs: [LogEntry] {
        viewModel.logs.filter { log in
            let levelMatch = selectedFilter.matches(log.level)
            
            // Filter by search
            let searchMatch = searchText.isEmpty || 
                log.message.lowercased().contains(searchText.lowercased()) ||
                log.source.lowercased().contains(searchText.lowercased())
            
            return levelMatch && searchMatch
        }
    }
}

// MARK: - Filter Button

private struct FilterButton: View {
    let level: UILogLevel
    let count: Int
    let isSelected: Bool
    let onSelect: () -> Void
    
    var body: some View {
        Button(action: onSelect) {
            HStack(spacing: 4) {
                Image(systemName: level.icon)
                    .font(.system(size: 12))
                Text("\(count)")
                    .font(DesignSystem.Typography.small)
            }
            .foregroundColor(isSelected ? .white : level.color)
            .padding(.horizontal, DesignSystem.Spacing.sm)
            .padding(.vertical, DesignSystem.Spacing.xs)
            .background(isSelected ? level.color : Color.clear)
            .cornerRadius(4)
        }
        .buttonStyle(.plain)
    }
}

// MARK: - Log Entry View

private struct LogEntryView: View {
    let log: LogEntry
    @State private var isExpanded = false
    @State private var isHovering = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            HStack(spacing: DesignSystem.Spacing.sm) {
                // Timestamp
                Text(log.timestamp, style: .time)
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .frame(width: 80, alignment: .leading)
                
                // Level icon
                Image(systemName: log.level.icon)
                    .font(.system(size: 12))
                    .foregroundColor(log.level.color)
                    .frame(width: 20)
                
                // Source
                Text("[\(log.source)]")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .frame(width: 120, alignment: .leading)
                    .lineLimit(1)
                
                // Message
                Text(log.message)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .lineLimit(isExpanded ? nil : 1)
                
                Spacer()
                
                // Expand button (if has stack trace)
                if log.stackTrace != nil {
                    Button(action: { isExpanded.toggle() }) {
                        Image(systemName: isExpanded ? "chevron.up" : "chevron.down")
                            .font(.system(size: 10))
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(.horizontal, DesignSystem.Spacing.sm)
            .padding(.vertical, DesignSystem.Spacing.xs)
            .background(isHovering ? DesignSystem.Colors.hover : Color.clear)
            .onHover { hovering in
                isHovering = hovering
            }
            .contextMenu {
                Button("Copy") {
                    // Copy to clipboard
                    #if os(macOS)
                    NSPasteboard.general.clearContents()
                    NSPasteboard.general.setString(log.message, forType: .string)
                    print("[Swift] Copied log message to clipboard")
                    #endif
                }
                Button("Copy Stack Trace") {
                    // Copy stack trace (if available)
                    #if os(macOS)
                    let stackTrace = log.stackTrace ?? "No stack trace available"
                    NSPasteboard.general.clearContents()
                    NSPasteboard.general.setString(stackTrace, forType: .string)
                    print("[Swift] Copied stack trace to clipboard")
                    #endif
                }
                .disabled(log.stackTrace == nil)
            }
            
            // Stack trace (if expanded)
            if isExpanded, let stackTrace = log.stackTrace {
                Text(stackTrace)
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .padding(.leading, 220)
                    .padding(.vertical, DesignSystem.Spacing.xs)
                    .background(DesignSystem.Colors.backgroundPrimary)
            }
            
            Divider()
                .background(DesignSystem.Colors.border.opacity(0.3))
        }
    }
}

// MARK: - UI Filter Level (for console filtering)

enum UILogLevel: String, CaseIterable {
    case all = "All"
    case error = "Error"
    case warning = "Warning"
    case info = "Info"
    case debug = "Debug"
    
    var icon: String {
        switch self {
        case .all: return "list.bullet"
        case .error: return "xmark.circle.fill"
        case .warning: return "exclamationmark.triangle.fill"
        case .info: return "info.circle.fill"
        case .debug: return "ant.circle.fill"
        }
    }
    
    var color: Color {
        switch self {
        case .all: return DesignSystem.Colors.textPrimary
        case .error: return DesignSystem.Colors.accentError
        case .warning: return DesignSystem.Colors.accentWarning
        case .info: return DesignSystem.Colors.accentPrimary
        case .debug: return DesignSystem.Colors.textSecondary
        }
    }
    
    func matches(_ logLevel: LogLevel) -> Bool {
        switch self {
        case .all: return true
        case .error: return logLevel == .error
        case .warning: return logLevel == .warning
        case .info: return logLevel == .info
        case .debug: return logLevel == .debug
        }
    }
}

// MARK: - View Model

class ConsoleViewModel: ObservableObject {
    @Published var logs: [LogEntry] = []
    
    init() {
        // Generate demo logs
        generateDemoLogs()
    }
    
    func generateDemoLogs() {
        logs = [
            LogEntry(timestamp: Date(), level: .info, source: "Engine", message: "VoxelForge Engine initialized successfully", stackTrace: nil),
            LogEntry(timestamp: Date().addingTimeInterval(-1), level: .info, source: "Renderer", message: "Metal renderer initialized", stackTrace: nil),
            LogEntry(timestamp: Date().addingTimeInterval(-2), level: .info, source: "AssetManager", message: "Loading scene: MainScene.scene", stackTrace: nil),
            LogEntry(timestamp: Date().addingTimeInterval(-3), level: .warning, source: "AssetManager", message: "Texture 'grass_normal.png' not found, using default", stackTrace: nil),
            LogEntry(timestamp: Date().addingTimeInterval(-4), level: .info, source: "Physics", message: "Physics system initialized with 60Hz tick rate", stackTrace: nil),
            LogEntry(timestamp: Date().addingTimeInterval(-5), level: .debug, source: "ECS", message: "Created entity: Player (ID: 12345)", stackTrace: nil),
            LogEntry(timestamp: Date().addingTimeInterval(-6), level: .debug, source: "ECS", message: "Added component: Transform to entity 12345", stackTrace: nil),
            LogEntry(timestamp: Date().addingTimeInterval(-7), level: .error, source: "Shader", message: "Failed to compile shader: terrain.glsl", stackTrace: "stack trace", count: 1),
            LogEntry(timestamp: Date().addingTimeInterval(-8), level: .info, source: "Audio", message: "Audio system initialized with 48kHz sample rate", stackTrace: nil, count: 1),
            LogEntry(timestamp: Date().addingTimeInterval(-9), level: .debug, source: "Network", message: "Listening on port 7777", stackTrace: nil, count: 1),
        ]
    }
    
    func getCount(for level: UILogLevel) -> Int {
        if level == .all {
            return logs.count
        }
        return logs.filter { level.matches($0.level) }.count
    }
    
    func clearLogs() {
        logs.removeAll()
    }
    
    func addLog(_ log: LogEntry) {
        logs.append(log)
    }
}

// MARK: - Preview

#Preview {
    ConsolePanel()
        .frame(height: 300)
}
