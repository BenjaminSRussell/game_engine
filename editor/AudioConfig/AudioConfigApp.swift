import SwiftUI

@main
struct AudioConfigApp: App {
    var body: some Scene {
        WindowGroup {
            SpatialAudioView()
                .frame(minWidth: 1000, minHeight: 700)
        }
        .windowStyle(.hiddenTitleBar)
        .windowToolbarStyle(.unified)
    }
}
