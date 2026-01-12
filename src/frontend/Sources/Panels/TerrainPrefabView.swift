import SwiftUI

// MARK: - Terrain Prefab UI

struct TerrainPrefabView: View {
    @StateObject private var prefabSystem = TerrainPrefabSystem.shared
    @StateObject private var terrainAPI = TerrainRuntimeAPI.shared
    @State private var selectedPrefab: TerrainPrefabSystem.TerrainPrefab?
    @State private var showingSaveSheet = false
    @State private var showingLoadSheet = false
    @State private var searchText = ""
    @State private var selectedTags: Set<String> = []
    @State private var showingTagFilter = false
    
    var filteredPrefabs: [TerrainPrefabSystem.TerrainPrefab] {
        let prefabs = prefabSystem.searchPrefabs(
            query: searchText.isEmpty ? nil : searchText,
            tags: Array(selectedTags)
        )
        
        return prefabs.sorted { $0.modifiedAt > $1.modifiedAt }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Terrain Prefabs")
                    .font(DesignSystem.Typography.h2)
                
                Spacer()
                
                Button("Save Current") {
                    showingSaveSheet = true
                }
                .buttonStyle(.borderedProminent)
                .disabled(!terrainAPI.isInitialized)
                
                Button("Load") {
                    showingLoadSheet = true
                }
                .buttonStyle(.bordered)
                
                Button("Refresh") {
                    Task {
                        await prefabSystem.refreshPrefabs()
                    }
                }
                .buttonStyle(.bordered)
            }
            .padding()
            
            Divider()
            
            // Search and Filter
            VStack(spacing: 8) {
                HStack {
                    HStack {
                        Image(systemName: "magnifyingglass")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        TextField("Search prefabs...", text: $searchText)
                            .textFieldStyle(.plain)
                    }
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(6)
                    
                    Button(action: { showingTagFilter.toggle() }) {
                        HStack {
                            Image(systemName: "tag.fill")
                            if !selectedTags.isEmpty {
                                Text("\(selectedTags.count)")
                                    .font(.caption)
                            }
                        }
                    }
                    .buttonStyle(.bordered)
                    .tint(selectedTags.isEmpty ? DesignSystem.Colors.textSecondary : DesignSystem.Colors.accentPrimary)
                }
                
                if showingTagFilter {
                    TagFilterView(
                        availableTags: getAllTags(),
                        selectedTags: $selectedTags
                    )
                    .transition(.opacity.combined(with: .scale(scale: 0.95)))
                }
            }
            .padding(.horizontal)
            .padding(.vertical, 8)
            
            Divider()
            
            // Prefab Grid
            if prefabSystem.isLoadingPrefabs {
                VStack(spacing: 20) {
                    ProgressView()
                        .scaleEffect(1.5)
                    
                    Text("Loading prefabs...")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            } else if filteredPrefabs.isEmpty {
                VStack(spacing: 20) {
                    Image(systemName: "cube.box")
                        .font(.system(size: 60))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Text("No Prefabs Found")
                        .font(DesignSystem.Typography.h3)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Create your first terrain prefab to get started")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            } else {
                ScrollView {
                    LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 3), spacing: 16) {
                        ForEach(filteredPrefabs) { prefab in
                            PrefabCardView(
                                prefab: prefab,
                                isSelected: selectedPrefab?.id == prefab.id,
                                onSelect: { selectedPrefab = prefab },
                                onLoad: { loadPrefab(prefab) },
                                onDelete: { deletePrefab(prefab) },
                                onDuplicate: { duplicatePrefab(prefab) }
                            )
                        }
                    }
                    .padding()
                }
            }
        }
        .sheet(isPresented: $showingSaveSheet) {
            SavePrefabSheet()
        }
        .sheet(isPresented: $showingLoadSheet) {
            LoadPrefabSheet(prefabs: filteredPrefabs) { prefab in
                loadPrefab(prefab)
            }
        }
        .alert("Error", isPresented: .constant(prefabSystem.lastSaveError != nil)) {
            Button("OK") {
                prefabSystem.lastSaveError = nil
            }
        } message: {
            if let error = prefabSystem.lastSaveError {
                Text(error.localizedDescription)
            }
        }
    }
    
    private func getAllTags() -> [String] {
        let allTags = prefabSystem.availablePrefabs.flatMap { $0.tags }
        return Array(Set(allTags)).sorted()
    }
    
    private func loadPrefab(_ prefab: TerrainPrefabSystem.TerrainPrefab) {
        Task {
            let result = await prefabSystem.loadTerrainData(from: prefab)
            
            switch result {
            case .success(let terrainData):
                do {
                    try terrainAPI.initialize(with: terrainData)
                    
                    // Apply modifications if any
                    if !prefab.modifications.isEmpty {
                        _ = await prefabSystem.applyModifications(from: prefab, to: terrainAPI)
                    }
                    
                    print("Successfully loaded prefab: \(prefab.name)")
                    
                } catch {
                    print("Failed to initialize terrain with prefab: \(error)")
                }
                
            case .failure(let error):
                print("Failed to load prefab: \(error)")
            }
        }
    }
    
    private func deletePrefab(_ prefab: TerrainPrefabSystem.TerrainPrefab) {
        Task {
            let result = await prefabSystem.deletePrefab(prefab)
            
            switch result {
            case .success:
                if selectedPrefab?.id == prefab.id {
                    selectedPrefab = nil
                }
                print("Successfully deleted prefab: \(prefab.name)")
                
            case .failure(let error):
                print("Failed to delete prefab: \(error)")
            }
        }
    }
    
    private func duplicatePrefab(_ prefab: TerrainPrefabSystem.TerrainPrefab) {
        Task {
            let newName = "\(prefab.name) Copy"
            let result = await prefabSystem.duplicatePrefab(prefab, newName: newName)
            
            switch result {
            case .success:
                print("Successfully duplicated prefab: \(prefab.name)")
                
            case .failure(let error):
                print("Failed to duplicate prefab: \(error)")
            }
        }
    }
}

// MARK: - Prefab Card View

struct PrefabCardView: View {
    let prefab: TerrainPrefabSystem.TerrainPrefab
    let isSelected: Bool
    let onSelect: () -> Void
    let onLoad: () -> Void
    let onDelete: () -> Void
    let onDuplicate: () -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Preview Image
            ZStack {
                RoundedRectangle(cornerRadius: 8)
                    .fill(DesignSystem.Colors.backgroundTertiary)
                    .aspectRatio(1, contentMode: .fit)
                
                if let previewData = prefab.previewImage,
                   let image = NSImage(data: previewData) {
                    Image(nsImage: image)
                        .resizable()
                        .aspectRatio(contentMode: .fill)
                        .frame(width: 120, height: 120)
                        .clipShape(RoundedRectangle(cornerRadius: 8))
                } else {
                    VStack(spacing: 8) {
                        Image(systemName: "cube.box")
                            .font(.system(size: 40))
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text("No Preview")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                }
                
                if isSelected {
                    RoundedRectangle(cornerRadius: 8)
                        .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
                }
            }
            
            // Info
            VStack(alignment: .leading, spacing: 4) {
                Text(prefab.name)
                    .font(DesignSystem.Typography.bodyBold)
                    .lineLimit(1)
                
                Text(prefab.description)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .lineLimit(2)
                
                HStack {
                    Text("by \(prefab.author)")
                        .font(DesignSystem.Typography.micro)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Spacer()
                    
                    Text(prefab.modifiedAt, style: .relative)
                        .font(DesignSystem.Typography.micro)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                // Tags
                if !prefab.tags.isEmpty {
                    ScrollView(.horizontal, showsIndicators: false) {
                        HStack(spacing: 4) {
                            ForEach(prefab.tags.prefix(3), id: \.self) { tag in
                                Text(tag)
                                    .font(DesignSystem.Typography.micro)
                                    .padding(.horizontal, 6)
                                    .padding(.vertical, 2)
                                    .background(DesignSystem.Colors.backgroundTertiary)
                                    .clipShape(Capsule())
                            }
                        }
                    }
                }
                
                // Metadata
                HStack {
                    Label("\(prefab.metadata.totalVertices)", systemImage: "grid")
                        .font(DesignSystem.Typography.micro)
                    
                    Spacer()
                    
                    Label(ByteCountFormatter.string(fromByteCount: prefab.metadata.estimatedMemoryUsage, countStyle: .memory), systemImage: "memorychip")
                        .font(DesignSystem.Typography.micro)
                }
                .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            // Actions
            HStack(spacing: 8) {
                Button("Load") {
                    onLoad()
                }
                .buttonStyle(.borderedProminent)
                .controlSize(.small)
                
                Button("Duplicate") {
                    onDuplicate()
                }
                .buttonStyle(.bordered)
                .controlSize(.small)
                
                Spacer()
                
                Button(action: onDelete) {
                    Image(systemName: "trash")
                        .foregroundColor(.red)
                }
                .buttonStyle(.plain)
                .controlSize(.small)
            }
        }
        .padding()
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(12)
        .shadow(color: .black.opacity(0.1), radius: 4, x: 0, y: 2)
        .onTapGesture {
            onSelect()
        }
    }
}

// MARK: - Tag Filter View

struct TagFilterView: View {
    let availableTags: [String]
    @Binding var selectedTags: Set<String>
    
    var body: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 8) {
                ForEach(availableTags, id: \.self) { tag in
                    Button(action: {
                        if selectedTags.contains(tag) {
                            selectedTags.remove(tag)
                        } else {
                            selectedTags.insert(tag)
                        }
                    }) {
                        Text(tag)
                            .font(DesignSystem.Typography.small)
                            .padding(.horizontal, 8)
                            .padding(.vertical, 4)
                            .background(
                                selectedTags.contains(tag) 
                                ? DesignSystem.Colors.accentPrimary 
                                : DesignSystem.Colors.backgroundTertiary
                            )
                            .foregroundColor(
                                selectedTags.contains(tag) 
                                ? .white 
                                : DesignSystem.Colors.textPrimary
                            )
                            .clipShape(Capsule())
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(.horizontal)
        }
        .frame(height: 40)
    }
}

// MARK: - Save Prefab Sheet

struct SavePrefabSheet: View {
    @Environment(\.dismiss) var dismiss
    @StateObject private var prefabSystem = TerrainPrefabSystem.shared
    @StateObject private var terrainAPI = TerrainRuntimeAPI.shared
    
    @State private var prefabName = ""
    @State private var prefabDescription = ""
    @State private var prefabAuthor = ""
    @State private var prefabTags: [String] = []
    @State private var newTag = ""
    @State private var includeModifications = true
    @State private var generatePreview = true
    @State private var isSaving = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Save Terrain Prefab")
                    .font(DesignSystem.Typography.h2)
                
                Spacer()
                
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Button("Save") {
                    savePrefab()
                }
                .buttonStyle(.borderedProminent)
                .disabled(prefabName.isEmpty || isSaving)
            }
            .padding()
            
            Divider()
            
            ScrollView {
                VStack(alignment: .leading, spacing: 16) {
                    Group {
                        TextField("Prefab Name", text: $prefabName)
                            .textFieldStyle(.roundedBorder)
                        
                        TextField("Description", text: $prefabDescription, axis: .vertical)
                            .textFieldStyle(.roundedBorder)
                            .lineLimit(3...6)
                        
                        TextField("Author", text: $prefabAuthor)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    // Tags
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Tags")
                            .font(DesignSystem.Typography.bodyBold)
                        
                        HStack {
                            TextField("Add tag...", text: $newTag)
                                .textFieldStyle(.roundedBorder)
                            
                            Button("Add") {
                                addTag()
                            }
                            .disabled(newTag.isEmpty)
                            .buttonStyle(.bordered)
                        }
                        
                        if !prefabTags.isEmpty {
                            LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 3), spacing: 8) {
                                ForEach(prefabTags, id: \.self) { tag in
                                    HStack {
                                        Text(tag)
                                            .font(DesignSystem.Typography.small)
                                        
                                        Button(action: { removeTag(tag) }) {
                                            Image(systemName: "xmark.circle.fill")
                                                .font(.caption)
                                        }
                                        .buttonStyle(.plain)
                                    }
                                    .padding(.horizontal, 8)
                                    .padding(.vertical, 4)
                                    .background(DesignSystem.Colors.backgroundTertiary)
                                    .clipShape(Capsule())
                                }
                            }
                        }
                    }
                    
                    // Options
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Options")
                            .font(DesignSystem.Typography.bodyBold)
                        
                        Toggle("Include modification history", isOn: $includeModifications)
                        Toggle("Generate preview image", isOn: $generatePreview)
                    }
                    
                    // Preview
                    if generatePreview {
                        VStack(alignment: .leading, spacing: 8) {
                            Text("Preview")
                                .font(DesignSystem.Typography.bodyBold)
                            
                            RoundedRectangle(cornerRadius: 8)
                                .fill(DesignSystem.Colors.backgroundTertiary)
                                .frame(height: 200)
                                .overlay(
                                    Text("Preview will be generated")
                                        .foregroundColor(DesignSystem.Colors.textTertiary)
                                )
                        }
                    }
                }
                .padding()
            }
            
            if isSaving {
                HStack {
                    ProgressView()
                        .scaleEffect(0.8)
                    
                    Text("Saving prefab...")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .padding()
            }
        }
        .frame(width: 600, height: 700)
    }
    
    private func addTag() {
        let trimmedTag = newTag.trimmingCharacters(in: .whitespacesAndNewlines)
        if !trimmedTag.isEmpty && !prefabTags.contains(trimmedTag) {
            prefabTags.append(trimmedTag)
            newTag = ""
        }
    }
    
    private func removeTag(_ tag: String) {
        prefabTags.removeAll { $0 == tag }
    }
    
    private func savePrefab() {
        guard !prefabName.isEmpty else { return }
        
        isSaving = true
        
        Task {
            // Create mock terrain data (in real implementation, this would come from the current terrain)
            let terrainData = TerrainData(size: SIMD2<Int>(256, 256))
            
            // Generate preview if requested
            let previewImage = generatePreview ? prefabSystem.generatePreviewImage(for: terrainData) : nil
            
            let result = await prefabSystem.savePrefab(
                name: prefabName,
                description: prefabDescription,
                author: prefabAuthor.isEmpty ? "Unknown" : prefabAuthor,
                tags: prefabTags,
                previewImage: previewImage,
                terrainData: terrainData,
                modifications: includeModifications ? [] : [] // Would get from terrain API
            )
            
            await MainActor.run {
                isSaving = false
                
                switch result {
                case .success:
                    dismiss()
                case .failure(let error):
                    print("Failed to save prefab: \(error)")
                }
            }
        }
    }
}

// MARK: - Load Prefab Sheet

struct LoadPrefabSheet: View {
    @Environment(\.dismiss) var dismiss
    let prefabs: [TerrainPrefabSystem.TerrainPrefab]
    let onLoad: (TerrainPrefabSystem.TerrainPrefab) -> Void
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Load Terrain Prefab")
                    .font(DesignSystem.Typography.h2)
                
                Spacer()
                
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
            }
            .padding()
            
            Divider()
            
            if prefabs.isEmpty {
                VStack(spacing: 20) {
                    Image(systemName: "cube.box")
                        .font(.system(size: 60))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Text("No Prefabs Available")
                        .font(DesignSystem.Typography.h3)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            } else {
                List(prefabs, id: \.id) { prefab in
                    HStack {
                        // Preview
                        if let previewData = prefab.previewImage,
                           let image = NSImage(data: previewData) {
                            Image(nsImage: image)
                                .resizable()
                                .aspectRatio(contentMode: .fill)
                                .frame(width: 60, height: 60)
                                .clipShape(RoundedRectangle(cornerRadius: 6))
                        } else {
                            RoundedRectangle(cornerRadius: 6)
                                .fill(DesignSystem.Colors.backgroundTertiary)
                                .frame(width: 60, height: 60)
                                .overlay(
                                    Image(systemName: "cube.box")
                                        .foregroundColor(DesignSystem.Colors.textTertiary)
                                )
                        }
                        
                        VStack(alignment: .leading, spacing: 4) {
                            Text(prefab.name)
                                .font(DesignSystem.Typography.bodyBold)
                            
                            Text(prefab.description)
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                                .lineLimit(2)
                            
                            HStack {
                                Text("by \(prefab.author)")
                                    .font(DesignSystem.Typography.micro)
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                                
                                Spacer()
                                
                                Text(prefab.modifiedAt, style: .relative)
                                    .font(DesignSystem.Typography.micro)
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                            }
                        }
                        
                        Spacer()
                        
                        Button("Load") {
                            onLoad(prefab)
                            dismiss()
                        }
                        .buttonStyle(.borderedProminent)
                        .controlSize(.small)
                    }
                    .padding(.vertical, 4)
                }
                .listStyle(.plain)
            }
        }
        .frame(width: 700, height: 500)
    }
}
