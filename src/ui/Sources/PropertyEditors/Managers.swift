import SwiftUI
// MARK: - Undo/Redo Manager (TODO-1462)
class PropertyUndoManager: ObservableObject {
    @Published private var undoStack: [Any] = []
    @Published private var redoStack: [Any] = []
    @Published var canUndo: Bool = false
    @Published var canRedo: Bool = false
    @Published var undoCount: Int = 0
    @Published var redoCount: Int = 0
    
    private let maxUndoLevels: Int = 50
    private var lastActionTime: Date = Date()
    private let actionCooldown: TimeInterval = 0.1 // 100ms between actions
    
    func recordAction<T>(_ oldValue: T, newValue: T, property: String) {
        // Check cooldown to prevent too frequent actions
        let now = Date()
        guard now.timeIntervalSince(lastActionTime) >= actionCooldown else { return }
        lastActionTime = now
        
        let action = PropertyAction(
            property: property,
            oldValue: oldValue,
            newValue: newValue,
            timestamp: now
        )
        
        undoStack.append(action)
        
        // Limit stack size
        if undoStack.count > maxUndoLevels {
            undoStack.removeFirst()
        }
        
        // Clear redo stack when new action is recorded
        redoStack.removeAll()
        
        updatePublisheds()
    }
    
    func undo() -> Bool {
        guard !undoStack.isEmpty else { return false }
        
        let action = undoStack.removeLast()
        redoStack.append(action)
        
        // Apply the undo action
        if let undoHandler = action.undoHandler {
            undoHandler()
        }
        
        updatePublisheds()
        return true
    }
    
    func redo() -> Bool {
        guard !redoStack.isEmpty else { return false }
        
        let action = redoStack.removeLast()
        undoStack.append(action)
        
        // Apply the redo action
        if let redoHandler = action.redoHandler {
            redoHandler()
        }
        
        updatePublisheds()
        return true
    }
    
    func clear() {
        undoStack.removeAll()
        redoStack.removeAll()
        updatePublisheds()
    }
    
    private func updatePublisheds() {
        canUndo = !undoStack.isEmpty
        canRedo = !redoStack.isEmpty
        undoCount = undoStack.count
        redoCount = redoStack.count
    }
    
    // Batch operations
    func beginBatch() {
        // Begin grouping multiple actions as one
    }
    
    func endBatch() {
        // End grouping and commit batch
    }
}

// Enhanced action structure
struct PropertyAction {
    let property: String
    let oldValue: Any
    let newValue: Any
    let timestamp: Date
    var undoHandler: (() -> Void)?
    var redoHandler: (() -> Void)?
}

// MARK: - Copy/Paste Manager (TODO-1463)
class PropertyClipboardManager: ObservableObject {
    @Published private var clipboard: Any?
    @Published private var clipboardType: String?
    @Published private var clipboardTimestamp: Date?
    @Published var canPaste: Bool = false
    @Published var clipboardHistory: [ClipboardItem] = []
    
    private let maxHistorySize: Int = 10
    private let clipboardTimeout: TimeInterval = 300 // 5 minutes
    
    func copy<T>(_ value: T, type: String) {
        let item = ClipboardItem(
            value: value,
            type: type,
            timestamp: Date(),
            stringValue: stringifyValue(value)
        )
        
        // Add to history
        clipboardHistory.insert(item, at: 0)
        
        // Limit history size
        if clipboardHistory.count > maxHistorySize {
            clipboardHistory = Array(clipboardHistory.prefix(maxHistorySize))
        }
        
        // Update current clipboard
        clipboard = value
        clipboardType = type
        clipboardTimestamp = Date()
        
        // Update system clipboard
        updateSystemClipboard(item.stringValue)
        
        updatePublisheds()
    }
    
    func paste<T>(as type: T.Type) -> T? {
        guard let clipboardValue = clipboard,
              let clipboardTypeValue = clipboardType,
              clipboardTypeValue == String(describing: type) else {
            return nil
        }
        
        // Check if clipboard is still valid
        if let timestamp = clipboardTimestamp {
            let now = Date()
            if now.timeIntervalSince(timestamp) > clipboardTimeout {
                clear()
                return nil
            }
        }
        
        return clipboardValue as? T
    }
    
    func clear() {
        clipboard = nil
        clipboardType = nil
        clipboardTimestamp = nil
        clipboardHistory.removeAll()
        updatePublisheds()
    }

