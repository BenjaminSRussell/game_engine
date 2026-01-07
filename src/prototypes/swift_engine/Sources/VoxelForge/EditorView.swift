import SwiftUI

struct EditorView: View {
    var body: some View {
        HSplitView {
            // Left Sidebar: Inspector / Hierarchy
            VStack(alignment: .leading) {
                Text("Scene Hierarchy")
                    .font(.headline)
                    .padding()
                Divider()
                List {
                    Label("World", systemImage: "globe")
                    Label("Player", systemImage: "person.fill")
                    Label("Sun Light", systemImage: "sun.max.fill")
                }
                .listStyle(.sidebar)
            }
            .frame(minWidth: 200, maxWidth: 300)
            
            // Center: Game View
            MetalView()
                .frame(minWidth: 400, minHeight: 300)
            
            // Right Sidebar: Properties
            VStack(alignment: .leading) {
                Text("Inspector")
                    .font(.headline)
                    .padding()
                Divider()
                Form {
                    Section(header: Text("Transform")) {
                        TextField("Position X", text: .constant("0.0"))
                        TextField("Position Y", text: .constant("0.0"))
                        TextField("Position Z", text: .constant("0.0"))
                    }
                    Section(header: Text("Voxel Settings")) {
                        Toggle("Show Wireframe", isOn: .constant(false))
                        Slider(value: .constant(0.5)) {
                            Text("Sun Intensity")
                        }
                    }
                }
                .formStyle(.grouped)
            }
            .frame(minWidth: 250, maxWidth: 350)
        }
        .frame(width: 1200, height: 800)
    }
}
