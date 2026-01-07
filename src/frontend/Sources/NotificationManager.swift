import SwiftUI
import Combine

// MARK: - Notification Manager
class NotificationManager: ObservableObject {
    static let shared = NotificationManager()
    
    @Published var activeNotifications: [EditorNotification] = []
    @Published var notificationHistory: [EditorNotification] = []
    
    func notify(_ message: String, type: NotificationType = .info, duration: Double = 3.0) {
        let notification = EditorNotification(message: message, type: type)
        
        DispatchQueue.main.async {
            self.activeNotifications.append(notification)
            self.notificationHistory.append(notification)
            
            // Auto-dismiss logic
            if duration > 0 {
                DispatchQueue.main.asyncAfter(deadline: .now() + duration) {
                    // Only dismiss if it's still active (haven't been manually dismissed)
                    if self.activeNotifications.contains(where: { $0.id == notification.id }) {
                        withAnimation {
                            self.dismiss(notification.id)
                        }
                    }
                }
            }
        }
    }
    
    func dismiss(_ id: UUID) {
        activeNotifications.removeAll { $0.id == id }
    }
    
    func clearHistory() {
        notificationHistory.removeAll()
    }
}

// MARK: - Notification Models
enum NotificationType {
    case info, success, warning, error
    
    var color: Color {
        switch self {
        case .info: return .blue
        case .success: return .green
        case .warning: return .orange
        case .error: return .red
        }
    }
    
    var icon: String {
        switch self {
        case .info: return "info.circle"
        case .success: return "checkmark.circle"
        case .warning: return "exclamationmark.triangle"
        case .error: return "xmark.octagon"
        }
    }
}

struct EditorNotification: Identifiable {
    let id = UUID()
    let timestamp = Date()
    let message: String
    let type: NotificationType
}

// MARK: - Notification Toast View
struct NotificationToastView: View {
    let notification: EditorNotification
    let onDismiss: () -> Void
    
    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: notification.type.icon)
                .foregroundColor(notification.type.color)
            
            Text(notification.message)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Spacer()
            
            Button(action: onDismiss) {
                Image(systemName: "xmark")
                    .font(.system(size: 10, weight: .bold))
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 12)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(8)
        .shadow(radius: 4)
        .frame(width: 300)
    }
}

// MARK: - Notification Overlay
struct NotificationOverlay: View {
    @ObservedObject var manager = NotificationManager.shared
    
    var body: some View {
        VStack {
            Spacer()
            HStack {
                Spacer()
                VStack(spacing: 8) {
                    ForEach(manager.activeNotifications) { notification in
                        NotificationToastView(notification: notification) {
                            manager.dismiss(notification.id)
                        }
                        .transition(.move(edge: .trailing).combined(with: .opacity))
                    }
                }
                .padding()
            }
        }
    }
}
