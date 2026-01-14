import SwiftUI

// MARK: - Stat Row Component

/// Reusable component for displaying label-value statistics
struct StatRow: View {
    let label: String
    let value: String
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            Text(label + ":")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Text(value)
                .font(DesignSystem.Typography.mono)
                .foregroundColor(DesignSystem.Colors.textPrimary)
        }
    }
}

// MARK: - Preview

#Preview {
    VStack(alignment: .leading, spacing: 8) {
        StatRow(label: "FPS", value: "60.0")
        StatRow(label: "Frame Time", value: "16.67ms")
        StatRow(label: "Memory", value: "256MB")
    }
    .padding()
    .background(DesignSystem.Colors.backgroundSecondary)
}
