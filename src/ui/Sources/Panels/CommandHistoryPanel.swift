import SwiftUI

// MARK: - Enhanced Command History Panel
// Visual panel showing undo/redo history with real-time backend integration

struct CommandHistoryPanel: View {
    @StateObject private var viewModel = CommandHistoryViewModel()
    @State private var selectedIndex: Int?
    
    var body: some View {
        EditorPanel("Command History") {
            VStack(spacing: 0) {
                // Stats and controls
                EditorCard {
                    HStack(spacing: DesignSystem.Spacing.md) {
                        // Undo button
                        Button(action: { viewModel.undo() }) {
                            HStack(spacing: DesignSystem.Spacing.xs) {
                                Image(systemName: "arrow.uturn.backward")
                                Text(viewModel.undoDescription ?? "Undo")
                                    .font(DesignSystem.Typography.caption)
                            }
                        }
                        .buttonStyle(.plain)
                        .disabled(!viewModel.canUndo)
                        .help("Undo (Z)")
                        
                        // Redo button
                        Button(action: { viewModel.redo() }) {
                            HStack(spacing: DesignSystem.Spacing.xs) {
                                Image(systemName: "arrow.uturn.forward")
                                Text(viewModel.redoDescription ?? "Redo")
                                    .font(DesignSystem.Typography.caption)
                            }
                        }
                        .buttonStyle(.plain)
                        .disabled(!viewModel.canRedo)
                        .help("Redo (Z)")
                        
                        Spacer()
                        
                        // Clear button
                        Button(action: {
                            viewModel.clearHistory()
                        }) {
                            Image(systemName: "trash")
                        }
                        .buttonStyle(.plain)
                        .help("Clear History")
                    }
                    
                    // Stack size info
                    HStack {
                        Text("\(viewModel.undoCount) undo")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Text("")
                            .foregroundColor(DesignSystem.Colors.textDisabled)
                        
                        Text("\(viewModel.redoCount) redo")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Spacer()
                        
                        Text("\(viewModel.maxStackSize) max")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textDisabled)
                    }
                    .padding(.top, DesignSystem.Spacing.xs)
                }
                
                // History timeline
                ScrollView {
                    LazyVStack(spacing: 1) {
                        // Redo stack (future)
                        ForEach(Array(viewModel.redoHistory.enumerated()), id: \.offset) { index, entry in
                            HistoryEntryRow(
                                entry: entry,
                                state: .future,
                                isSelected: false
                            )
                        }
                        
                        // Current state marker
                        HStack {
                            Rectangle()
                                .fill(DesignSystem.Colors.accentPrimary.gradient)
                                .frame(height: 2)
                            
                            Text("CURRENT")
                                .font(DesignSystem.Typography.smallBold)
                                .foregroundColor(DesignSystem.Colors.accentPrimary)
                                .padding(.horizontal, DesignSystem.Spacing.sm)
                            
                            Rectangle()
                                .fill(DesignSystem.Colors.accentPrimary.gradient)
                                .frame(height: 2)
                        }
                        .padding(.vertical, DesignSystem.Spacing.sm)
                        
                        // Undo stack (past)
                        ForEach(Array(viewModel.undoHistory.enumerated()), id: \.offset) { index, entry in
                            HistoryEntryRow(
                                entry: entry,
                                state: .past,
                                isSelected: selectedIndex == index
                            )
                            .onTapGesture {
                                selectedIndex = index
                            }
                        }
                    }
                }
            }
        }
        .task {
            // Update history loop
            while !Task.isCancelled {
                viewModel.updateHistory()
                try? await Task.sleep(nanoseconds: 500_000_000) // 0.5s
            }
        }
    }
}

// MARK: - History Entry Row

private struct HistoryEntryRow: View {
    let entry: UndoRedoAPI.HistoryEntry
    let state: HistoryState
    let isSelected: Bool
    @State private var isHovering = false
    
    enum HistoryState {
        case past, future
    }
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            // State indicator
            Image(systemName: state == .past ? "checkmark.circle.fill" : "circle.dashed")
                .font(.system(size: 12))
                .foregroundColor(state == .past ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.textDisabled)
                .frame(width: 16)
            
            // Group indicator
            if entry.isGroup {
                Image(systemName: "square.stack.3d.up.fill")
                    .font(.system(size: 10))
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                Text("\(entry.groupSize)")
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            // Description
            VStack(alignment: .leading, spacing: 2) {
                Text(entry.description)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text(formatRelativeTime(entry.timestamp))
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .padding(.vertical, DesignSystem.Spacing.xs)
        .background(
            isSelected ? DesignSystem.Colors.selection : (isHovering ? DesignSystem.Colors.backgroundSecondary : Color.clear)
        )
        .opacity(state == .future ? 0.5 : 1.0)
        .onHover { isHovering = $0 }
    }
    
    private func formatRelativeTime(_ date: Date) -> String {
        let formatter = RelativeDateTimeFormatter()
        formatter.unitsStyle = .abbreviated
        return formatter.localizedString(for: date, relativeTo: Date())
    }
}

// MARK: - Command Manager Extensions
// Extensions moved to CommandManager.swift

// MARK: - Command Group
// CompositeCommand moved to CommandManager.swift

// MARK: - View Model

@MainActor
class CommandHistoryViewModel: ObservableObject {
    @Published var undoHistory: [UndoRedoAPI.HistoryEntry] = []
    @Published var redoHistory: [UndoRedoAPI.HistoryEntry] = []
    @Published var undoCount: UInt32 = 0
    @Published var redoCount: UInt32 = 0
    @Published var maxStackSize: UInt32 = 100
    
    var canUndo: Bool { UndoRedoAPI.shared.canUndo() }
    var canRedo: Bool { UndoRedoAPI.shared.canRedo() }
    var undoDescription: String? { UndoRedoAPI.shared.getUndoDescription() }
    var redoDescription: String? { UndoRedoAPI.shared.getRedoDescription() }
    
    func undo() {
        UndoRedoAPI.shared.undo()
        updateHistory()
    }
    
    func redo() {
        UndoRedoAPI.shared.redo()
        updateHistory()
    }
    
    func clearHistory() {
        UndoRedoAPI.shared.clearHistory()
        updateHistory()
    }
    
    func updateHistory() {
        undoHistory = UndoRedoAPI.shared.getUndoHistory()
        redoHistory = UndoRedoAPI.shared.getRedoHistory()
        undoCount = UndoRedoAPI.shared.getUndoStackSize()
        redoCount = UndoRedoAPI.shared.getRedoStackSize()
        maxStackSize = UndoRedoAPI.shared.getMaxStackSize()
    }
}

// MARK: - Preview

#Preview {
    CommandHistoryPanel()
        .frame(width: 300, height: 500)
}
