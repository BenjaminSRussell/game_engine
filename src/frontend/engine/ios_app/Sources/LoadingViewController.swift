import UIKit
import Combine

class LoadingViewController: UIViewController {
    
    private let progressView = UIProgressView(progressViewStyle: .default)
    private let statusLabel = UILabel()
    private let activityIndicator = UIActivityIndicatorView(style: .large)
    private let gameStateManager = GameStateManager.shared
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        setupUI()
        setupObservers()
        startLoadingSimulation()
    }
    
    private func setupUI() {
        view.backgroundColor = .black
        
        // Setup activity indicator
        activityIndicator.color = .white
        activityIndicator.translatesAutoresizingMaskIntoConstraints = false
        activityIndicator.startAnimating()
        view.addSubview(activityIndicator)
        
        // Setup progress view
        progressView.progressTintColor = .systemGreen
        progressView.trackTintColor = .darkGray
        progressView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(progressView)
        
        // Setup status label
        statusLabel.text = "Loading..."
        statusLabel.textColor = .white
        statusLabel.font = .systemFont(ofSize: 16)
        statusLabel.textAlignment = .center
        statusLabel.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(statusLabel)
        
        NSLayoutConstraint.activate([
            activityIndicator.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            activityIndicator.centerYAnchor.constraint(equalTo: view.centerYAnchor, constant: -50),
            
            progressView.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            progressView.centerYAnchor.constraint(equalTo: view.centerYAnchor),
            progressView.widthAnchor.constraint(equalToConstant: 200),
            
            statusLabel.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            statusLabel.topAnchor.constraint(equalTo: progressView.bottomAnchor, constant: 20),
            statusLabel.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 20),
            statusLabel.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -20)
        ])
    }
    
    private func setupObservers() {
        gameStateManager.$loadingProgress
            .receive(on: DispatchQueue.main)
            .sink { [weak self] progress in
                self?.progressView.progress = progress
            }
            .store(in: &cancellables)
        
        gameStateManager.$loadingMessage
            .receive(on: DispatchQueue.main)
            .sink { [weak self] message in
                self?.statusLabel.text = message
            }
            .store(in: &cancellables)
        
        gameStateManager.$currentState
            .receive(on: DispatchQueue.main)
            .sink { [weak self] state in
                if state == .inGame {
                    self?.transitionToGame()
                }
            }
            .store(in: &cancellables)
    }
    
    private var cancellables = Set<AnyCancellable>()
    
    private func startLoadingSimulation() {
        // Simulate loading process
        let loadingSteps = [
            (0.1, "Generating terrain..."),
            (0.3, "Loading textures..."),
            (0.5, "Initializing entities..."),
            (0.7, "Setting up world..."),
            (0.9, "Finalizing..."),
            (1.0, "Ready!")
        ]
        
        for (index, (progress, message)) in loadingSteps.enumerated() {
            DispatchQueue.main.asyncAfter(deadline: .now() + Double(index) * 0.5) {
                self.gameStateManager.updateLoadingProgress(progress, message: message)
                
                if progress >= 1.0 {
                    DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                        self.gameStateManager.transition(to: .inGame)
                    }
                }
            }
        }
    }
    
    private func transitionToGame() {
        let gameViewController = GameViewController()
        navigationController?.setViewControllers([gameViewController], animated: true)
    }
}
