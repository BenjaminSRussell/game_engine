import UIKit

enum GameState {
    case mainMenu
    case loading
    case inGame
    case paused
    case settings
    case multiplayer
}

class GameStateManager: ObservableObject {
    @Published var currentState: GameState = .mainMenu
    @Published var isLoading: Bool = false
    @Published var loadingProgress: Float = 0.0
    @Published var loadingMessage: String = "Loading..."
    
    static let shared = GameStateManager()
    
    private init() {}
    
    func transition(to newState: GameState) {
        // Handle state transitions with proper cleanup/setup
        switch (currentState, newState) {
        case (.mainMenu, .loading):
            prepareForGameLoad()
        case (.loading, .inGame):
            finalizeGameLoad()
        case (.inGame, .paused):
            pauseGame()
        case (.paused, .inGame):
            resumeGame()
        case (.inGame, .mainMenu), (.paused, .mainMenu):
            returnToMainMenu()
        case (_, .settings):
            // Settings can be accessed from any state
            break
        default:
            break
        }
        
        currentState = newState
    }
    
    private func prepareForGameLoad() {
        isLoading = true
        loadingProgress = 0.0
        loadingMessage = "Initializing world..."
    }
    
    private func finalizeGameLoad() {
        isLoading = false
        loadingProgress = 1.0
    }
    
    private func pauseGame() {
        // Pause game engine updates
        game_engine_pause()
    }
    
    private func resumeGame() {
        // Resume game engine updates
        game_engine_resume()
    }
    
    private func returnToMainMenu() {
        // Cleanup game session
        game_engine_shutdown()
        isLoading = false
        loadingProgress = 0.0
    }
    
    func updateLoadingProgress(_ progress: Float, message: String = "") {
        loadingProgress = min(max(progress, 0.0), 1.0)
        if !message.isEmpty {
            loadingMessage = message
        }
    }
}

// C game engine interface extensions
func game_engine_pause() {
    // TODO: Implement pause functionality in C engine
}

func game_engine_resume() {
    // TODO: Implement resume functionality in C engine
}
