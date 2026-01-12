import SwiftUI

// MARK: - Project Tasks Panel

struct TaskPanel: View {
    @StateObject private var viewModel = TaskViewModel()
    @State private var searchText = ""
    
    var body: some View {
        EditorPanel("Project Tasks") {
            VStack(spacing: 0) {
                // Toolbar
                HStack {
                    EditorSearchField(text: $searchText, placeholder: "Filter tasks...")
                    
                    EditorIconButton(icon: "arrow.clockwise", tooltip: "Reload Tasks") {
                        viewModel.loadTasks()
                    }
                }
                .padding(DesignSystem.Spacing.sm)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                // Content
                if viewModel.isLoading {
                    CenterSpinner()
                } else {
                    List {
                        ForEach(viewModel.filteredTasks(searchText: searchText)) { item in
                            TaskRow(item: item, viewModel: viewModel)
                                .listRowInsets(EdgeInsets())
                                .listRowSeparator(.hidden)
                                .listRowBackground(Color.clear)
                        }
                    }
                    .listStyle(.plain)
                }
            }
        }
        .task {
            viewModel.loadTasks()
        }
    }
}

// MARK: - Task Row

private struct TaskRow: View {
    let item: TaskItem
    @ObservedObject var viewModel: TaskViewModel
    @State private var isHovering = false
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Indentation
            Color.clear
                .frame(width: CGFloat(item.indentParams.level * 20))
            
            if item.indentParams.type == .header {
                // Header Row
                HStack(spacing: 4) {
                    Image(systemName: "number")
                        .font(.system(size: 10))
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                        .opacity(0.7)
                    
                    Text(item.title)
                        .font(DesignSystem.Typography.smallBold)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                }
                .padding(.vertical, 4)
            } else {
                // Task Row
                Button(action: {
                    viewModel.toggleTask(id: item.id)
                }) {
                    HStack(spacing: DesignSystem.Spacing.sm) {
                        Image(systemName: item.isCompleted ? "checkmark.square.fill" : "square")
                            .foregroundColor(item.isCompleted ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.textTertiary)
                        
                        Text(item.title)
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(item.isCompleted ? DesignSystem.Colors.textSecondary : DesignSystem.Colors.textPrimary)
                            .strikethrough(item.isCompleted)
                        
                        Spacer()
                    }
                }
                .buttonStyle(.plain)
            }
            
            Spacer()
        }
        .padding(.horizontal, DesignSystem.Spacing.md)
        .padding(.vertical, 2)
        .background(isHovering ? DesignSystem.Colors.backgroundTertiary : Color.clear)
        .onHover { isHovering = $0 }
    }
}

// MARK: - Models and ViewModel

struct TaskItem: Identifiable {
    let id: UUID = UUID()
    let originalLineIndex: Int
    var title: String
    var isCompleted: Bool
    var indentParams: IndentationParams
    
    struct IndentationParams {
        let level: Int
        let type: RowType
    }
    
    enum RowType {
        case header
        case task
    }
}

@MainActor
class TaskViewModel: ObservableObject {
    @Published var tasks: [TaskItem] = []
    @Published var isLoading = false
    
    // Path to the file
    private let filePath = "/Users/benjaminrussell/Desktop/Minecraft v2/REAL_TODOS_FINAL.md"
    private var fileLines: [String] = []
    
    func loadTasks() {
        isLoading = true
        
        Task.detached(priority: .userInitiated) {
            do {
                let content = try String(contentsOfFile: self.filePath, encoding: .utf8)
                let lines = content.components(separatedBy: .newlines)
                
                var newTasks: [TaskItem] = []
                
                for (index, line) in lines.enumerated() {
                    let trimmed = line.trimmingCharacters(in: .whitespaces)
                    
                    // Check for headers (Phase 1, etc)
                    if trimmed.hasPrefix("#") {
                        let level = line.prefix(while: { $0 == "#" }).count
                        let title = trimmed.drop(while: { $0 == "#" }).trimmingCharacters(in: .whitespaces)
                        
                        newTasks.append(TaskItem(
                            originalLineIndex: index,
                            title: title,
                            isCompleted: false,
                            indentParams: .init(level: level - 1, type: .header)
                        ))
                    }
                    // Check for tasks (- [ ])
                    else if trimmed.hasPrefix("- [ ]") || trimmed.hasPrefix("- [x]") {
                        let isCompleted = trimmed.hasPrefix("- [x]")
                        let title = trimmed.dropFirst(5).trimmingCharacters(in: .whitespaces)
                        
                        // Calculate indentation level vaguely based on leading spaces
                        let leadingSpaces = line.prefix(while: { $0 == " " }).count
                        let indentLevel = max(0, (leadingSpaces / 2) + 1)
                        
                        newTasks.append(TaskItem(
                            originalLineIndex: index,
                            title: title,
                            isCompleted: isCompleted,
                            indentParams: .init(level: indentLevel, type: .task)
                        ))
                    }
                }
                
                await MainActor.run {
                    self.fileLines = lines
                    self.tasks = newTasks
                    self.isLoading = false
                }
            } catch {
                print("Error loading tasks: \(error)")
                await MainActor.run {
                    self.isLoading = false
                }
            }
        }
    }
    
    func toggleTask(id: UUID) {
        guard let index = tasks.firstIndex(where: { $0.id == id }) else { return }
        
        // Toggle in memory
        tasks[index].isCompleted.toggle()
        
        // Update file line
        let item = tasks[index]
        let originalLine = fileLines[item.originalLineIndex]
        
        // Replace [ ] with [x] or vice versa
        if item.isCompleted {
            fileLines[item.originalLineIndex] = originalLine.replacingOccurrences(of: "- [ ]", with: "- [x]")
        } else {
            fileLines[item.originalLineIndex] = originalLine.replacingOccurrences(of: "- [x]", with: "- [ ]")
        }
        
        saveFile()
    }
    
    private func saveFile() {
        let content = fileLines.joined(separator: "\n")
        do {
            try content.write(toFile: filePath, atomically: true, encoding: .utf8)
        } catch {
            print("Error saving tasks: \(error)")
        }
    }
    
    func filteredTasks(searchText: String) -> [TaskItem] {
        if searchText.isEmpty {
            return tasks
        }
        return tasks.filter { $0.title.localizedCaseInsensitiveContains(searchText) }
    }
}
