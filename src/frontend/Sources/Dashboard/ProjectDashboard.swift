import SwiftUI

struct ProjectDashboard: View {
    
    struct Project: Identifiable {
        let id = UUID()
        let name: String
        let path: String
        let lastModified: Date
        let icon: String // SF Symbol name
    }
    
    @State private var projects: [Project] = [
        Project(name: "New World", path: "/Game/Worlds/NewWorld", lastModified: Date(), icon: "globe"),
        Project(name: "Test Level", path: "/Game/Worlds/Test", lastModified: Date().addingTimeInterval(-86400), icon: "cube.fill"),
        Project(name: "Physics Demo", path: "/Game/Worlds/Physics", lastModified: Date().addingTimeInterval(-172800), icon: "atom")
    ]
    
    var body: some View {
        HSplitView {
            // Sidebar
            VStack(alignment: .leading, spacing: 10) {
                Text("Dashboard")
                    .font(.title2)
                    .fontWeight(.bold)
                    .padding(.horizontal)
                    .padding(.top)
                
                List {
                    Label("Projects", systemImage: "folder")
                    Label("Documentation", systemImage: "book")
                    Label("Analytics", systemImage: "chart.bar")
                    Label("Settings", systemImage: "gear")
                }
                .listStyle(SidebarListStyle())
            }
            .frame(minWidth: 200, maxWidth: 300)
            
            // Main Content
            VStack(alignment: .leading) {
                HStack {
                    Text("Recent Projects")
                        .font(.headline)
                    Spacer()
                    Button(action: createNewProject) {
                        Label("New Project", systemImage: "plus")
                    }
                }
                .padding()
                
                ScrollView {
                    LazyVGrid(columns: [GridItem(.adaptive(minimum: 250))], spacing: 20) {
                        ForEach(projects) { project in
                            ProjectCard(project: project)
                        }
                    }
                    .padding()
                }
            }
        }
        .sheet(isPresented: $showNewProjectSheet) {
            NewProjectView()
        }
    }
    
    @State private var showNewProjectSheet = false

    private func createNewProject() {
        showNewProjectSheet = true
    }
}

struct ProjectCard: View {
    let project: ProjectDashboard.Project
    
    var body: some View {
        VStack(alignment: .leading) {
            HStack {
                Image(systemName: project.icon)
                    .font(.largeTitle)
                    .foregroundColor(.accentColor)
                Spacer()
                Menu {
                    Button("Open") {}
                    Button("Rename") {}
                    Divider()
                    Button("Delete", role: .destructive) {}
                } label: {
                    Image(systemName: "ellipsis")
                }
                .menuStyle(BorderlessButtonMenuStyle())
            }
            
            Spacer().frame(height: 12)
            
            Text(project.name)
                .font(.headline)
            Text(project.path)
                .font(.caption)
                .foregroundColor(.secondary)
                .lineLimit(1)
            
            Spacer().frame(height: 8)
            
            Text("Modified \(project.lastModified.formatted())")
                .font(.caption2)
                .foregroundColor(Color(NSColor.tertiaryLabelColor))
        }
        .padding()
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(12)
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(Color.gray.opacity(0.2), lineWidth: 1)
        )
    }
}

struct ProjectDashboard_Previews: PreviewProvider {
    static var previews: some View {
        ProjectDashboard()
    }
}
