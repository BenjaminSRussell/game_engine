// MainMenuViewController.swift
//
// Purpose: This view controller presents the main menu of the game,
// allowing users to start a new game, access settings, or quit the application.
//
// Role: Manages the main menu's user interface, including buttons for navigation
// to `GameViewController` and `SettingsViewController`. It handles user interaction
// with these menu options.
//
// Lifecycle:
// - `viewDidLoad()`: Configures the main menu UI elements and sets up action handlers
//   for the New Game, Settings, and Quit buttons.
//
// Inputs: User taps on "New Game", "Settings", or "Quit" buttons.
// Outputs: Navigates to `GameViewController`, `SettingsViewController`, or terminates the application.
//
// TODO: Low - Add a background image to the main menu.
// TODO: Low - Add some animations to the main menu, such as fading in the buttons one by one.
import UIKit
import Combine

class MainMenuViewController: UIViewController {
    
    private let gameStateManager = GameStateManager.shared
    private var cancellables = Set<AnyCancellable>()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = "BlockGame"
        view.backgroundColor = .systemBackground
        
        setupGameStateObservers()
        setupUI()
    }
    
    private func setupGameStateObservers() {
        gameStateManager.$currentState
            .receive(on: DispatchQueue.main)
            .sink { [weak self] state in
                if state == .mainMenu {
                    self?.resetToMainMenu()
                }
            }
            .store(in: &cancellables)
    }
    
    private func resetToMainMenu() {
        // Reset any UI state when returning to main menu
        navigationController?.popToRootViewController(animated: false)
    }
    
    private func setupUI() {
        let stackView = UIStackView()
        stackView.axis = .vertical
        stackView.spacing = 12
        stackView.alignment = .center
        stackView.distribution = .equalSpacing
        stackView.translatesAutoresizingMaskIntoConstraints = false
        
        view.addSubview(stackView)
        
        NSLayoutConstraint.activate([
            stackView.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            stackView.centerYAnchor.constraint(equalTo: view.centerYAnchor),
            stackView.widthAnchor.constraint(equalToConstant: 200)
        ])
        
        let titleLabel = UILabel()
        titleLabel.text = "BlockGame"
        titleLabel.font = .systemFont(ofSize: 32, weight: .bold)
        titleLabel.textAlignment = .center
        stackView.addArrangedSubview(titleLabel)
        
        let spacer1 = UIView()
        spacer1.heightAnchor.constraint(equalToConstant: 40).isActive = true
        stackView.addArrangedSubview(spacer1)
        
        let newGameButton = createMenuButton(title: "New Game", action: #selector(newGameTapped))
        stackView.addArrangedSubview(newGameButton)
        
        let loadGameButton = createMenuButton(title: "Load Game", action: #selector(loadGameTapped))
        stackView.addArrangedSubview(loadGameButton)
        
        let multiplayerButton = createMenuButton(title: "Multiplayer", action: #selector(multiplayerTapped))
        stackView.addArrangedSubview(multiplayerButton)
        
        let settingsButton = createMenuButton(title: "Settings", action: #selector(settingsTapped))
        stackView.addArrangedSubview(settingsButton)
        
        let editorButton = createMenuButton(title: "Asset Editor", action: #selector(editorTapped))
        editorButton.setTitleColor(.cyan, for: .normal)
        stackView.addArrangedSubview(editorButton)
        
        let quitButton = createMenuButton(title: "Quit", action: #selector(quitTapped))
        stackView.addArrangedSubview(quitButton)
    }
    
    private func createMenuButton(title: String, action: Selector) -> UIButton {
        let button = UIButton(type: .system)
        button.setTitle(title, for: .normal)
        button.titleLabel?.font = .systemFont(ofSize: 18, weight: .semibold)
        button.addTarget(self, action: action, for: .touchUpInside)
        button.widthAnchor.constraint(equalToConstant: 150).isActive = true
        button.heightAnchor.constraint(equalToConstant: 44).isActive = true
        return button
    }
    
    @objc private func newGameTapped() {
        gameStateManager.transition(to: .loading)
        let loadingViewController = LoadingViewController()
        navigationController?.pushViewController(loadingViewController, animated: true)
    }
    
    @objc private func loadGameTapped() {
        let saveLoadViewController = SaveLoadViewController()
        navigationController?.pushViewController(saveLoadViewController, animated: true)
    }
    
    @objc private func multiplayerTapped() {
        let multiplayerViewController = MultiplayerViewController()
        navigationController?.pushViewController(multiplayerViewController, animated: true)
    }
    
    @objc private func settingsTapped() {
        let settingsViewController = SettingsViewController()
        navigationController?.pushViewController(settingsViewController, animated: true)
    }
    
    @objc private func editorTapped() {
        let editorViewController = AssetEditorViewController()
        navigationController?.pushViewController(editorViewController, animated: true)
    }
    
    @objc private func quitTapped() {
        exit(0)
    }
}
