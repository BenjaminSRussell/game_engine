import SwiftUI

/// Health bar HUD component
struct HealthBarView: View {
    let data: HUDElementData
    
    var body: some View {
        HStack(spacing: 2) {
            ForEach(0..<10) { index in
                heartIcon(for: index)
            }
        }
    }
    
    private func heartIcon(for index: Int) -> some View {
        let heartsToShow = Int(ceil(data.health / 10))
        let isFilled = index < heartsToShow
        
        return Image(systemName: isFilled ? "heart.fill" : "heart")
            .foregroundColor(.red)
            .font(.system(size: 16))
    }
}

/// Hunger bar HUD component
struct HungerBarView: View {
    let data: HUDElementData
    
    var body: some View {
        HStack(spacing: 2) {
            ForEach(0..<10) { index in
                hungerIcon(for: index)
            }
        }
    }
    
    private func hungerIcon(for index: Int) -> some View {
        let iconsToShow = Int(ceil(data.hunger / 10))
        let isFilled = index < iconsToShow
        
        return Image(systemName: isFilled ? "circle.fill" : "circle")
            .foregroundColor(.orange)
            .font(.system(size: 16))
    }
}

/// Hotbar HUD component
struct HotbarView: View {
    let data: HUDElementData
    
    var body: some View {
        HStack(spacing: 4) {
            ForEach(0..<9) { index in
                hotbarSlot(index: index)
            }
        }
        .padding(4)
        .background(Color.black.opacity(0.5))
        .cornerRadius(8)
    }
    
    private func hotbarSlot(index: Int) -> some View {
        RoundedRectangle(cornerRadius: 4)
            .strokeBorder(
                data.selectedSlot == index ? Color.white : Color.gray,
                lineWidth: data.selectedSlot == index ? 3 : 1
            )
            .frame(width: 36, height: 36)
            .background(
                RoundedRectangle(cornerRadius: 4)
                    .fill(Color.gray.opacity(0.3))
            )
    }
}

/// Crosshair HUD component
struct CrosshairView: View {
    var body: some View {
        ZStack {
            // Horizontal line
            Rectangle()
                .fill(Color.white)
                .frame(width: 16, height: 2)
            
            // Vertical line
            Rectangle()
                .fill(Color.white)
                .frame(width: 2, height: 16)
            
            // Center dot
            Circle()
                .fill(Color.white)
                .frame(width: 3, height: 3)
        }
        .shadow(color: .black, radius: 1, x: 1, y: 1)
    }
}
