// UndoRedoAPI.swift
// Swift wrapper for undo/redo command history

import Foundation

/// Undo/Redo command history API
public class UndoRedoAPI {
    public static let shared = UndoRedoAPI()
    private init() {}
    
    // MARK: - Undo/Redo Operations
    
    /// Check if undo is available
    public func canUndo() -> Bool {
        return undo_can_undo()
    }
    
    /// Check if redo is available
    public func canRedo() -> Bool {
        return undo_can_redo()
    }
    
    /// Execute undo operation
    public func undo() {
        undo_execute()
    }
    
    /// Execute redo operation
    public func redo() {
        redo_execute()
    }
    
    /// Get description of next undo operation
    public func getUndoDescription() -> String? {
        guard let ptr = undo_get_description() else { return nil }
        return String(cString: ptr)
    }
    
    /// Get description of next redo operation
    public func getRedoDescription() -> String? {
        guard let ptr = redo_get_description() else { return nil }
        return String(cString: ptr)
    }
    
    // MARK: - Command Grouping
    
    /// Begin a command group
    public func beginGroup(_ description: String) {
        undo_begin_group(description)
    }
    
    /// End current command group
    public func endGroup() {
        undo_end_group()
    }
    
    /// Execute multiple commands as a group
    public func executeAsGroup(_ description: String, _ block: () throws -> Void) rethrows {
        beginGroup(description)
        defer { endGroup() }
        try block()
    }
    
    // MARK: - Stack Management
    
    /// Get number of commands in undo stack
    public func getUndoStackSize() -> UInt32 {
        return undo_get_stack_size()
    }
    
    /// Get number of commands in redo stack
    public func getRedoStackSize() -> UInt32 {
        return undo_get_stack_size()
    }
    
    /// Clear all history
    public func clearHistory() {
        undo_clear_history()
    }
    
    /// Set maximum stack size
    public func setMaxStackSize(_ size: UInt32) {
        undo_set_max_stack_size(size)
    }
    
    /// Get maximum stack size
    public func getMaxStackSize() -> UInt32 {
        return undo_get_max_stack_size()
    }
    
    // MARK: - History
    
    /// History entry
    public struct HistoryEntry {
        public let description: String
        public let timestamp: Date
        public let isGroup: Bool
        public let groupSize: UInt32
    }
    
    /// Get undo history
    public func getUndoHistory(maxCount: Int = 50) -> [HistoryEntry] {
        let entries = UnsafeMutablePointer<UndoHistoryEntry_C>.allocate(capacity: maxCount)
        defer { entries.deallocate() }
        
        let count = undo_get_history(entries, UInt32(maxCount))
        
        return (0..<Int(count)).map { i in
            let entry = entries[i]
            var descriptionTuple = entry.description
            let description = withUnsafePointer(to: &descriptionTuple) { ptr in
                ptr.withMemoryRebound(to: CChar.self, capacity: 256) { charPtr in
                    String(cString: charPtr)
                }
            }
            
            return HistoryEntry(
                description: description,
                timestamp: Date(timeIntervalSince1970: TimeInterval(entry.timestamp)),
                isGroup: entry.is_group,
                groupSize: entry.group_size
            )
        }
    }
    
    /// Get redo history
    public func getRedoHistory(maxCount: Int = 50) -> [HistoryEntry] {
        let entries = UnsafeMutablePointer<UndoHistoryEntry_C>.allocate(capacity: maxCount)
        defer { entries.deallocate() }
        
        let count = redo_get_history(entries, UInt32(maxCount))
        
        return (0..<Int(count)).map { i in
            let entry = entries[i]
            var descriptionTuple = entry.description
            let description = withUnsafePointer(to: &descriptionTuple) { ptr in
                ptr.withMemoryRebound(to: CChar.self, capacity: 256) { charPtr in
                    String(cString: charPtr)
                }
            }
            
            return HistoryEntry(
                description: description,
                timestamp: Date(timeIntervalSince1970: TimeInterval(entry.timestamp)),
                isGroup: entry.is_group,
                groupSize: entry.group_size
            )
        }
    }
}

// MARK: - C Bridging

private typealias UndoHistoryEntry_C = (
    description: (CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar),
    timestamp: UInt64,
    is_group: Bool,
    group_size: UInt32
)

@_silgen_name("undo_can_undo")
private func undo_can_undo() -> Bool

@_silgen_name("undo_can_redo")
private func undo_can_redo() -> Bool

@_silgen_name("undo_execute")
private func undo_execute()

@_silgen_name("redo_execute")
private func redo_execute()

@_silgen_name("undo_get_description")
private func undo_get_description() -> UnsafePointer<CChar>?

@_silgen_name("redo_get_description")
private func redo_get_description() -> UnsafePointer<CChar>?

@_silgen_name("undo_begin_group")
private func undo_begin_group(_ description: String)

@_silgen_name("undo_end_group")
private func undo_end_group()

@_silgen_name("undo_get_stack_size")
private func undo_get_stack_size() -> UInt32

@_silgen_name("redo_get_stack_size")
private func redo_get_stack_size() -> UInt32

@_silgen_name("undo_clear_history")
private func undo_clear_history()

@_silgen_name("undo_set_max_stack_size")
private func undo_set_max_stack_size(_ size: UInt32)

@_silgen_name("undo_get_max_stack_size")
private func undo_get_max_stack_size() -> UInt32

@_silgen_name("undo_get_history")
private func undo_get_history(_ entries: UnsafeMutablePointer<UndoHistoryEntry_C>, _ maxCount: UInt32) -> UInt32

@_silgen_name("redo_get_history")
private func redo_get_history(_ entries: UnsafeMutablePointer<UndoHistoryEntry_C>, _ maxCount: UInt32) -> UInt32
