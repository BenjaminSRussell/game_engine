import SwiftUI

// MARK: - Texture Inspector Panel

struct TextureInspectorPanel: View {
    @StateObject private var viewModel = TextureInspectorViewModel()
    @State private var selectedTexture: TextureAsset?
    
    var body: some View {
        HSplitView {
            // Left: Texture List
            TextureListView(
                textures: viewModel.textures,
                selectedTexture: $selectedTexture
            )
            .frame(minWidth: 200, maxWidth: 280)
            
            EditorDivider()
            
            // Center: Preview
            if let texture = selectedTexture {
                VStack(spacing: 0) {
                    TexturePreviewView(
                        texture: texture,
                        zoom: $viewModel.zoomLevel,
                        offset: $viewModel.panOffset,
                        selectedChannel: $viewModel.selectedChannel
                    )
                    
                    EditorDivider()
                    
                    // Channel Selector
                    ChannelSelectorView(selectedChannel: $viewModel.selectedChannel)
                        .padding(DesignSystem.Spacing.sm)
                        .background(DesignSystem.Colors.backgroundTertiary)
                }
            } else {
                ZStack {
                    DesignSystem.Colors.backgroundPrimary
                    VStack(spacing: DesignSystem.Spacing.md) {
                        Image(systemName: "photo")
                            .font(.system(size: 64))
                            .foregroundColor(DesignSystem.Colors.textDisabled)
                        Text("Select a texture to inspect")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                }
            }
            
            EditorDivider()
            
            // Right: Properties
            if let texture = selectedTexture {
                TexturePropertiesView(texture: texture)
                    .frame(minWidth: 250, maxWidth: 320)
            } else {
                Color.clear
                    .frame(width: 250)
            }
        }
    }
}

// MARK: - Texture List View

private struct TextureListView: View {
    let textures: [TextureAsset]
    @Binding var selectedTexture: TextureAsset?
    @State private var searchText = ""
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Textures")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            EditorSearchBar(text: $searchText, placeholder: "Search textures...")
                .padding(DesignSystem.Spacing.sm)
            
            EditorDivider()
            
            ScrollView {
                LazyVStack(spacing: 1) {
                    ForEach(filteredTextures) { texture in
                        TextureListRow(
                            texture: texture,
                            isSelected: selectedTexture?.id == texture.id
                        )
                        .onTapGesture {
                            selectedTexture = texture
                        }
                    }
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
    
    private var filteredTextures: [TextureAsset] {
        if searchText.isEmpty {
            return textures
        }
        return textures.filter { $0.name.lowercased().contains(searchText.lowercased()) }
    }
}

private struct TextureListRow: View {
    let texture: TextureAsset
    let isSelected: Bool
    @State private var isHovering = false
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            // Thumbnail
            RoundedRectangle(cornerRadius: 4)
                .fill(DesignSystem.Colors.backgroundTertiary)
                .frame(width: 40, height: 40)
                .overlay(
                    Image(systemName: "photo.fill")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                )
            
            VStack(alignment: .leading, spacing: 2) {
                Text(texture.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .lineLimit(1)
                
                Text("\(texture.width)×\(texture.height)")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
        }
        .padding(DesignSystem.Spacing.sm)
        .background(isSelected ? DesignSystem.Colors.selection : (isHovering ? DesignSystem.Colors.hover : Color.clear))
        .onHover { isHovering = $0 }
    }
}

// MARK: - Texture Preview View

private struct TexturePreviewView: View {
    let texture: TextureAsset
    @Binding var zoom: CGFloat
    @Binding var offset: CGSize
    @Binding var selectedChannel: TextureChannel
    
    @State private var isDragging = false
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Checkerboard background
                CheckerboardPattern()
                
                // Texture preview (placeholder)
                RoundedRectangle(cornerRadius: 8)
                    .fill(
                        LinearGradient(
                            colors: channelGradientColors,
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        )
                    )
                    .frame(
                        width: CGFloat(texture.width) * zoom,
                        height: CGFloat(texture.height) * zoom
                    )
                    .offset(offset)
                
                // Zoom controls overlay
                VStack {
                    Spacer()
                    HStack {
                        Spacer()
                        ZoomControlsView(zoom: $zoom, offset: $offset)
                            .padding(DesignSystem.Spacing.md)
                    }
                }
            }
            .gesture(
                DragGesture()
                    .onChanged { value in
                        offset = CGSize(
                            width: offset.width + value.translation.width,
                            height: offset.height + value.translation.height
                        )
                    }
            )
            .gesture(
                MagnificationGesture()
                    .onChanged { value in
                        zoom = max(0.1, min(10.0, zoom * value))
                    }
            )
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
    
    private var channelGradientColors: [Color] {
        switch selectedChannel {
        case .rgba:
            return [.white, .gray]
        case .red:
            return [.red, .black]
        case .green:
            return [.green, .black]
        case .blue:
            return [.blue, .black]
        case .alpha:
            return [.white, .black]
        }
    }
}

private struct CheckerboardPattern: View {
    var body: some View {
        GeometryReader { geometry in
            Canvas { context, size in
                let squareSize: CGFloat = 10
                let rows = Int(size.height / squareSize) + 1
                let cols = Int(size.width / squareSize) + 1
                
                for row in 0..<rows {
                    for col in 0..<cols {
                        let isEven = (row + col) % 2 == 0
                        let rect = CGRect(
                            x: CGFloat(col) * squareSize,
                            y: CGFloat(row) * squareSize,
                            width: squareSize,
                            height: squareSize
                        )
                        context.fill(
                            Path(rect),
                            with: .color(isEven ? Color.gray.opacity(0.2) : Color.gray.opacity(0.1))
                        )
                    }
                }
            }
        }
    }
}

private struct ZoomControlsView: View {
    @Binding var zoom: CGFloat
    @Binding var offset: CGSize
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            EditorIconButton(icon: "minus.magnifyingglass", tooltip: "Zoom Out") {
                zoom = max(0.1, zoom / 1.5)
            }
            
            Text(String(format: "%.0f%%", zoom * 100))
                .font(DesignSystem.Typography.smallMono)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .frame(width: 60)
            
            EditorIconButton(icon: "plus.magnifyingglass", tooltip: "Zoom In") {
                zoom = min(10.0, zoom * 1.5)
            }
            
            EditorIconButton(icon: "arrow.counterclockwise", tooltip: "Reset View") {
                zoom = 1.0
                offset = .zero
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary.opacity(0.95))
        .cornerRadius(DesignSystem.CornerRadius.md)
    }
}

// MARK: - Channel Selector View

private struct ChannelSelectorView: View {
    @Binding var selectedChannel: TextureChannel
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            Text("Channel:")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            ForEach(TextureChannel.allCases, id: \.self) { channel in
                Button(action: { selectedChannel = channel }) {
                    Text(channel.rawValue)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(selectedChannel == channel ? .white : DesignSystem.Colors.textPrimary)
                        .padding(.horizontal, DesignSystem.Spacing.md)
                        .padding(.vertical, DesignSystem.Spacing.xs)
                        .background(selectedChannel == channel ? channel.color : DesignSystem.Colors.backgroundSecondary)
                        .cornerRadius(4)
                }
                .buttonStyle(.plain)
            }
            
            Spacer()
        }
    }
}

// MARK: - Texture Properties View

private struct TexturePropertiesView: View {
    let texture: TextureAsset
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                EditorCollapsibleSection("Information", isExpanded: true) {
                    PropertyRow(label: "Name", value: texture.name)
                    PropertyRow(label: "Format", value: texture.format)
                    PropertyRow(label: "Dimensions", value: "\(texture.width) × \(texture.height)")
                    PropertyRow(label: "Size", value: texture.formattedSize)
                    PropertyRow(label: "Mip Levels", value: "\(texture.mipLevels)")
                }
                
                EditorCollapsibleSection("Import Settings", isExpanded: true) {
                    Toggle("sRGB", isOn: .constant(texture.isSRGB))
                        .toggleStyle(EditorToggleStyle())
                    Toggle("Generate Mipmaps", isOn: .constant(true))
                        .toggleStyle(EditorToggleStyle())
                    
                    Picker("Wrap Mode", selection: .constant("Repeat")) {
                        Text("Repeat").tag("Repeat")
                        Text("Clamp").tag("Clamp")
                        Text("Mirror").tag("Mirror")
                    }
                    
                    Picker("Filter Mode", selection: .constant("Bilinear")) {
                        Text("Point").tag("Point")
                        Text("Bilinear").tag("Bilinear")
                        Text("Trilinear").tag("Trilinear")
                    }
                }
                
                EditorCollapsibleSection("Statistics", isExpanded: true) {
                    PropertyRow(label: "References", value: "\(texture.referenceCount)")
                    PropertyRow(label: "GPU Memory", value: texture.formattedGPUMemory)
                }
            }
            .padding(DesignSystem.Spacing.md)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

private struct PropertyRow: View {
    let label: String
    let value: String
    
    var body: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            Spacer()
            Text(value)
                .font(DesignSystem.Typography.smallMono)
                .foregroundColor(DesignSystem.Colors.textPrimary)
        }
    }
}

// MARK: - Data Models

enum TextureChannel: String, CaseIterable {
    case rgba = "RGBA"
    case red = "R"
    case green = "G"
    case blue = "B"
    case alpha = "A"
    
    var color: Color {
        switch self {
        case .rgba: return DesignSystem.Colors.accentPrimary
        case .red: return .red
        case .green: return .green
        case .blue: return .blue
        case .alpha: return .gray
        }
    }
}

struct TextureAsset: Identifiable {
    let id = UUID()
    let name: String
    let format: String
    let width: Int
    let height: Int
    let size: Int64
    let mipLevels: Int
    let isSRGB: Bool
    let referenceCount: Int
    
    var formattedSize: String {
        ByteCountFormatter.string(fromByteCount: size, countStyle: .file)
    }
    
    var formattedGPUMemory: String {
        let gpuSize = Int64(width * height * 4 * mipLevels)
        return ByteCountFormatter.string(fromByteCount: gpuSize, countStyle: .memory)
    }
}

// MARK: - View Model

class TextureInspectorViewModel: ObservableObject {
    @Published var textures: [TextureAsset] = []
    @Published var zoomLevel: CGFloat = 1.0
    @Published var panOffset: CGSize = .zero
    @Published var selectedChannel: TextureChannel = .rgba
    
    init() {
        loadDemoTextures()
    }
    
    func loadDemoTextures() {
        textures = [
            TextureAsset(name: "grass_diffuse.png", format: "RGBA8", width: 2048, height: 2048, size: 16777216, mipLevels: 11, isSRGB: true, referenceCount: 3),
            TextureAsset(name: "stone_normal.png", format: "RGBA8", width: 1024, height: 1024, size: 4194304, mipLevels: 10, isSRGB: false, referenceCount: 2),
            TextureAsset(name: "metal_roughness.png", format: "R8", width: 512, height: 512, size: 262144, mipLevels: 9, isSRGB: false, referenceCount: 1),
            TextureAsset(name: "skybox_hdr.exr", format: "RGBA16F", width: 4096, height: 2048, size: 67108864, mipLevels: 12, isSRGB: false, referenceCount: 1),
        ]
    }
}
