import UIKit
import Combine

struct SaveGame {
    let id: UUID
    let name: String
    let worldName: String
    let creationDate: Date
    let lastPlayed: Date
    let playtime: TimeInterval
    let screenshot: UIImage?
    let version: String
    let seed: Int64
}

class SaveLoadViewController: UIViewController {
    
    private let gameStateManager = GameStateManager.shared
    private var cancellables = Set<AnyCancellable>()
    private var saveGames: [SaveGame] = []
    
    private let tableView = UITableView()
    private let deleteButton = UIButton(type: .system)
    private let loadButton = UIButton(type: .system)
    private var selectedSaveIndex: Int? = nil
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = "Load Game"
        view.backgroundColor = .systemBackground
        
        setupUI()
        setupTableView()
        loadSaveGames()
    }
    
    private func setupUI() {
        // Setup navigation
        navigationItem.leftBarButtonItem = UIBarButtonItem(
            barButtonSystemItem: .cancel,
            target: self,
            action: #selector(cancelTapped)
        )
        
        // Setup buttons
        deleteButton.setTitle("Delete", for: .normal)
        deleteButton.setTitleColor(.systemRed, for: .normal)
        deleteButton.titleLabel?.font = .systemFont(ofSize: 18, weight: .semibold)
        deleteButton.addTarget(self, action: #selector(deleteTapped), for: .touchUpInside)
        deleteButton.isEnabled = false
        deleteButton.translatesAutoresizingMaskIntoConstraints = false
        
        loadButton.setTitle("Load", for: .normal)
        loadButton.setTitleColor(.systemBlue, for: .normal)
        loadButton.titleLabel?.font = .systemFont(ofSize: 18, weight: .semibold)
        loadButton.addTarget(self, action: #selector(loadTapped), for: .touchUpInside)
        loadButton.isEnabled = false
        loadButton.translatesAutoresizingMaskIntoConstraints = false
        
        view.addSubview(deleteButton)
        view.addSubview(loadButton)
        
        NSLayoutConstraint.activate([
            deleteButton.leadingAnchor.constraint(equalTo: view.safeAreaLayoutGuide.leadingAnchor, constant: 20),
            deleteButton.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -20),
            deleteButton.widthAnchor.constraint(equalToConstant: 100),
            deleteButton.heightAnchor.constraint(equalToConstant: 44),
            
            loadButton.trailingAnchor.constraint(equalTo: view.safeAreaLayoutGuide.trailingAnchor, constant: -20),
            loadButton.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -20),
            loadButton.widthAnchor.constraint(equalToConstant: 100),
            loadButton.heightAnchor.constraint(equalToConstant: 44)
        ])
    }
    
    private func setupTableView() {
        tableView.delegate = self
        tableView.dataSource = self
        tableView.register(SaveGameCell.self, forCellReuseIdentifier: "SaveGameCell")
        tableView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(tableView)
        
        NSLayoutConstraint.activate([
            tableView.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor),
            tableView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            tableView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            tableView.bottomAnchor.constraint(equalTo: deleteButton.topAnchor, constant: -20)
        ])
    }
    
    private func loadSaveGames() {
        // Load actual save games from file system using bridge
        saveGames = loadActualSaveGames()
        tableView.reloadData()
    }
    
    private func loadActualSaveGames() -> [SaveGame] {
        var worlds: [SaveGame] = []
        
        // Get world list from C engine via bridge
        var worldPtr: UnsafeMutablePointer<BridgeWorldMetadata>?
        var worldCount: UInt32 = 0
        
        let success = bridge_get_world_list(&worldPtr, &worldCount)
        
        if success, let worldPtr = worldPtr, worldCount > 0 {
            let buffer = UnsafeBufferPointer(start: worldPtr, count: Int(worldCount))
            
            for i in 0..<Int(worldCount) {
                let worldMetadata = buffer[i]
                
                // Convert BridgeWorldMetadata to SaveGame
                let name = String(cString: worldMetadata.name)
                let worldName = name // Use name as world name for now
                let creationDate = Date(timeIntervalSince1970: TimeInterval(worldMetadata.last_played - worldMetadata.play_time))
                let lastPlayed = Date(timeIntervalSince1970: TimeInterval(worldMetadata.last_played))
                let playtime = TimeInterval(worldMetadata.play_time)
                let version = String(cString: worldMetadata.version)
                let seed = Int64(worldMetadata.seed)
                
                let saveGame = SaveGame(
                    id: UUID(),
                    name: name,
                    worldName: worldName,
                    creationDate: creationDate,
                    lastPlayed: lastPlayed,
                    playtime: playtime,
                    screenshot: nil, // TODO: Load actual screenshot from save
                    version: version,
                    seed: seed
                )
                
                worlds.append(saveGame)
            }
            
            // Free the memory allocated by C function
            worldPtr.deallocate()
        }
        
        // If no worlds found, create sample ones for demonstration
        if worlds.isEmpty {
            worlds = createMockSaveGames()
        }
        
        return worlds
    }
    
    private func createMockSaveGames() -> [SaveGame] {
        let calendar = Calendar.current
        let now = Date()
        
        return [
            SaveGame(
                id: UUID(),
                name: "World 1",
                worldName: "Survival Island",
                creationDate: calendar.date(byAdding: .day, value: -7, to: now) ?? now,
                lastPlayed: calendar.date(byAdding: .hour, value: -2, to: now) ?? now,
                playtime: 3600 * 12.5, // 12.5 hours
                screenshot: nil,
                version: "1.0.0",
                seed: 123456789
            ),
            SaveGame(
                id: UUID(),
                name: "Creative Build",
                worldName: "Creative Paradise",
                creationDate: calendar.date(byAdding: .day, value: -3, to: now) ?? now,
                lastPlayed: calendar.date(byAdding: .hour, value: -24, to: now) ?? now,
                playtime: 3600 * 5.25, // 5.25 hours
                screenshot: nil,
                version: "1.0.0",
                seed: 987654321
            ),
            SaveGame(
                id: UUID(),
                name: "Hardcore",
                worldName: "Desert Survival",
                creationDate: calendar.date(byAdding: .day, value: -14, to: now) ?? now,
                lastPlayed: calendar.date(byAdding: .day, value: -1, to: now) ?? now,
                playtime: 3600 * 8.75, // 8.75 hours
                screenshot: nil,
                version: "1.0.0",
                seed: 555666777
            )
        ]
    }
    
    @objc private func cancelTapped() {
        navigationController?.popViewController(animated: true)
    }
    
    @objc private func deleteTapped() {
        guard let index = selectedSaveIndex else { return }
        
        let saveGame = saveGames[index]
        let alert = UIAlertController(
            title: "Delete Save Game",
            message: "Are you sure you want to delete '\(saveGame.name)'? This action cannot be undone.",
            preferredStyle: .alert
        )
        
        alert.addAction(UIAlertAction(title: "Cancel", style: .cancel))
        alert.addAction(UIAlertAction(title: "Delete", style: .destructive) { _ in
            self.deleteSaveGame(at: index)
        })
        
        present(alert, animated: true)
    }
    
    @objc private func loadTapped() {
        guard let index = selectedSaveIndex else { return }
        
        let saveGame = saveGames[index]
        
        // Implement actual game loading
        print("Loading save game: \(saveGame.name)")
        
        // Load the world using bridge function
        let success = bridge_load_world(saveGame.name)
        
        if success {
            // Transition to game state
            gameStateManager.transition(to: .loading)
            let loadingViewController = LoadingViewController()
            navigationController?.setViewControllers([loadingViewController], animated: true)
        } else {
            // Show error alert
            let alert = UIAlertController(
                title: "Load Failed",
                message: "Failed to load '\(saveGame.name)'. The save file may be corrupted.",
                preferredStyle: .alert
            )
            
            alert.addAction(UIAlertAction(title: "OK", style: .default))
            present(alert, animated: true)
        }
    }
    
    private func deleteSaveGame(at index: Int) {
        let saveGame = saveGames[index]
        
        // Actually delete the save game using bridge function
        let success = bridge_delete_world(saveGame.name)
        
        if success {
            saveGames.remove(at: index)
            selectedSaveIndex = nil
            updateButtonStates()
            
            tableView.deleteRows(at: [IndexPath(row: index, section: 0)], with: .automatic)
        } else {
            // Show error alert
            let alert = UIAlertController(
                title: "Delete Failed",
                message: "Failed to delete '\(saveGame.name)'. The file may be in use.",
                preferredStyle: .alert
            )
            
            alert.addAction(UIAlertAction(title: "OK", style: .default))
            present(alert, animated: true)
        }
    }
    
    private func updateButtonStates() {
        deleteButton.isEnabled = selectedSaveIndex != nil
        loadButton.isEnabled = selectedSaveIndex != nil
    }
}

extension SaveLoadViewController: UITableViewDataSource, UITableViewDelegate {
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return saveGames.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "SaveGameCell", for: indexPath) as! SaveGameCell
        cell.configure(with: saveGames[indexPath.row])
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        selectedSaveIndex = indexPath.row
        updateButtonStates()
    }
    
    func tableView(_ tableView: UITableView, didDeselectRowAt indexPath: IndexPath) {
        if selectedSaveIndex == indexPath.row {
            selectedSaveIndex = nil
            updateButtonStates()
        }
    }
    
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        return 80
    }
}

class SaveGameCell: UITableViewCell {
    
    private let nameLabel = UILabel()
    private let worldNameLabel = UILabel()
    private let lastPlayedLabel = UILabel()
    private let playtimeLabel = UILabel()
    private let stackView = UIStackView()
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        setupUI()
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    private func setupUI() {
        stackView.axis = .vertical
        stackView.spacing = 4
        stackView.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(stackView)
        
        nameLabel.font = .systemFont(ofSize: 18, weight: .semibold)
        nameLabel.textColor = .label
        stackView.addArrangedSubview(nameLabel)
        
        worldNameLabel.font = .systemFont(ofSize: 14, weight: .medium)
        worldNameLabel.textColor = .secondaryLabel
        stackView.addArrangedSubview(worldNameLabel)
        
        let infoStackView = UIStackView()
        infoStackView.axis = .horizontal
        infoStackView.spacing = 20
        infoStackView.distribution = .fillEqually
        
        lastPlayedLabel.font = .systemFont(ofSize: 12)
        lastPlayedLabel.textColor = .tertiaryLabel
        infoStackView.addArrangedSubview(lastPlayedLabel)
        
        playtimeLabel.font = .systemFont(ofSize: 12)
        playtimeLabel.textColor = .tertiaryLabel
        infoStackView.addArrangedSubview(playtimeLabel)
        
        stackView.addArrangedSubview(infoStackView)
        
        NSLayoutConstraint.activate([
            stackView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
            stackView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),
            stackView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 8),
            stackView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -8)
        ])
    }
    
    func configure(with saveGame: SaveGame) {
        nameLabel.text = saveGame.name
        worldNameLabel.text = saveGame.worldName
        lastPlayedLabel.text = "Last played: \(formatDate(saveGame.lastPlayed))"
        playtimeLabel.text = "Playtime: \(formatPlaytime(saveGame.playtime))"
    }
    
    private func formatDate(_ date: Date) -> String {
        let formatter = RelativeDateTimeFormatter()
        formatter.unitsStyle = .abbreviated
        return formatter.localizedString(for: date, relativeTo: Date())
    }
    
    private func formatPlaytime(_ timeInterval: TimeInterval) -> String {
        let hours = Int(timeInterval) / 3600
        let minutes = (Int(timeInterval) % 3600) / 60
        
        if hours > 0 {
            return "\(hours)h \(minutes)m"
        } else {
            return "\(minutes)m"
        }
    }
}
