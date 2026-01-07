import SwiftUI

struct NewProjectView: View {
    @Environment(\.dismiss) var dismiss
    @State private var projectName = "My Project"
    @State private var location = "~/Documents/VoxelForge"
    @State private var selectedTemplate = "Empty 3D"
    
    let templates = ["Empty 3D", "First Person Shooter", "RPG Base", "2D Platformer"]
    
    var body: some View {
        VStack(spacing: 20) {
            Text("Create New Project")
                .font(.title2)
                .bold()
            
            Form {
                TextField("Project Name", text: $projectName)
                
                HStack {
                    TextField("Location", text: $location)
                    Button("Browse...") {
                        // File picker stub
                    }
                }
                
                Picker("Template", selection: $selectedTemplate) {
                    ForEach(templates, id: \.self) { template in
                        Text(template).tag(template)
                    }
                }
                .pickerStyle(.menu)
            }
            .padding()
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .keyboardShortcut(.cancelAction)
                
                Button("Create Project") {
                    // Logic to create project
                    print("Creating project: \(projectName) at \(location) with template \(selectedTemplate)")
                    dismiss()
                }
                .keyboardShortcut(.defaultAction)
                .buttonStyle(.borderedProminent)
            }
            .padding(.bottom)
        }
        .frame(width: 450, height: 300)
        .padding()
    }
}

struct NewProjectView_Previews: PreviewProvider {
    static var previews: some View {
        NewProjectView()
    }
}
