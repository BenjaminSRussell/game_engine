import SwiftUI

// MARK: - Collaboration & Version Control

class VersionControlManager: ObservableObject {
    static let shared = VersionControlManager()
    
    @Published var currentBranch: String = "main"
    @Published var modifiedFiles: [String] = ["Sources/Panels/CollaborationPanel.swift", "Assets/Materials/NewMat.mat"]
    @Published var isConnected: Bool = true
    @Published var commitMessage: String = ""
    
    func pull() {
        print("Pulling changes...")
    }
    
    func push() {
        print("Pushing changes...")
    }
    
    func commit() {
        print("Committing: \(commitMessage)")
        commitMessage = ""
    }
}

class CollaborationManager: ObservableObject {
    static let shared = CollaborationManager()
    
    @Published var connectedUsers: [CollabUser] = [
        CollabUser(name: "You", color: .blue, isOnline: true),
        CollabUser(name: "Alice", color: .red, isOnline: true),
        CollabUser(name: "Bob", color: .green, isOnline: false)
    ]
}

struct CollabUser: Identifiable {
    let id = UUID()
    var name: String
    var color: Color
    var isOnline: Bool
}

struct CollaborationPanel: View {
    @ObservedObject var vcsManager = VersionControlManager.shared
    @ObservedObject var collabManager = CollaborationManager.shared
    @State private var selectedTab = 0
    
    var body: some View {
        VStack(spacing: 0) {
            // Tab Bar
            Picker("", selection: $selectedTab) {
                Text("Version Control").tag(0)
                Text("Team").tag(1)
            }
            .pickerStyle(.segmented)
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            if selectedTab == 0 {
                VersionControlView(manager: vcsManager)
            } else {
                TeamView(manager: collabManager)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

struct VersionControlView: View {
    @ObservedObject var manager: VersionControlManager
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Image(systemName: "git.branch")
                Text(manager.currentBranch)
                    .font(DesignSystem.Typography.mono)
                Spacer()
                Circle()
                    .fill(manager.isConnected ? Color.green : Color.red)
                    .frame(width: 8, height: 8)
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Actions
            HStack(spacing: DesignSystem.Spacing.md) {
                Button(action: { manager.pull() }) {
                    Label("Pull", systemImage: "arrow.down")
                }
                Button(action: { manager.push() }) {
                    Label("Push", systemImage: "arrow.up")
                }
            }
            .padding(DesignSystem.Spacing.sm)
            
            EditorDivider()
            
            // Modified Files
            List {
                Section("Changes") {
                    ForEach(manager.modifiedFiles, id: \.self) { file in
                        HStack {
                            Image(systemName: "doc.text")
                            Text(file)
                            Spacer()
                            Text("M")
                                .font(DesignSystem.Typography.mono)
                                .foregroundColor(.orange)
                        }
                    }
                }
            }
            .listStyle(.plain)
            
            EditorDivider()
            
            // Commit Area
            VStack(spacing: 8) {
                TextField("Commit message...", text: $manager.commitMessage)
                    .textFieldStyle(.roundedBorder)
                
                Button(action: { manager.commit() }) {
                    Text("Commit Changes")
                        .frame(maxWidth: .infinity)
                }
                .disabled(manager.commitMessage.isEmpty)
            }
            .padding(DesignSystem.Spacing.md)
        }
    }
}

struct TeamView: View {
    @ObservedObject var manager: CollaborationManager
    
    var body: some View {
        List {
            Section("Team Members") {
                ForEach(manager.connectedUsers) { user in
                    HStack {
                        Circle()
                            .fill(user.color)
                            .frame(width: 8, height: 8)
                        Text(user.name)
                        Spacer()
                        if user.isOnline {
                            Text("Online")
                                .font(.caption)
                                .foregroundColor(.green)
                        } else {
                            Text("Offline")
                                .font(.caption)
                                .foregroundColor(.gray)
                        }
                    }
                }
            }
        }
        .listStyle(.plain)
    }
}
