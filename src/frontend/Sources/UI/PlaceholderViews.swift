import SwiftUI

// MARK: - Placeholder Views

struct AssetLibraryView: View {
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Image(systemName: "square.grid.2x2.fill")
                .font(.system(size: 48, weight: .light))
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            Text("Asset Library")
                .font(DesignSystem.Typography.title2)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Text("Coming soon - Browse and manage project assets")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

struct SettingsView: View {
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            Image(systemName: "gearshape.fill")
                .font(.system(size: 48, weight: .light))
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            Text("Settings")
                .font(DesignSystem.Typography.title2)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Text("Coming soon - Configure editor preferences")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}
