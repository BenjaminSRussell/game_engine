import UIKit
import Combine

struct Server {
    let id: UUID
    let name: String
    let address: String
    let port: Int
    let playerCount: Int
    let maxPlayers: Int
    let version: String
    let ping: Int
    let description: String
    let isOnline: Bool
    let hasPassword: Bool
}

class MultiplayerViewController: UIViewController {
    
    private let gameStateManager = GameStateManager.shared
    private var cancellables = Set<AnyCancellable>()
    private var servers: [Server] = []
    
    private let segmentedControl = UISegmentedControl(items: ["Browse", "Direct Connect", "Create"])
    private let tableView = UITableView()
    private let directConnectView = UIView()
    private let createServerView = UIView()
    
    // Direct connect fields
    private let addressTextField = UITextField()
    private let portTextField = UITextField()
    private let connectButton = UIButton(type: .system)
    
    // Create server fields
    private let serverNameTextField = UITextField()
    private let maxPlayersSlider = UISlider()
    private let maxPlayersLabel = UILabel()
    private let passwordTextField = UITextField()
    private let createButton = UIButton(type: .system)
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = "Multiplayer"
        view.backgroundColor = .systemBackground
        
        setupUI()
        setupTableView()
        setupDirectConnect()
        setupCreateServer()
        loadServers()
    }
    
    private func setupUI() {
        navigationItem.leftBarButtonItem = UIBarButtonItem(
            barButtonSystemItem: .cancel,
            target: self,
            action: #selector(cancelTapped)
        )
        
        // Setup segmented control
        segmentedControl.selectedSegmentIndex = 0
        segmentedControl.addTarget(self, action: #selector(segmentChanged), for: .valueChanged)
        segmentedControl.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(segmentedControl)
        
        NSLayoutConstraint.activate([
            segmentedControl.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor, constant: 16),
            segmentedControl.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
            segmentedControl.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16)
        ])
    }
    
    private func setupTableView() {
        tableView.delegate = self
        tableView.dataSource = self
        tableView.register(ServerCell.self, forCellReuseIdentifier: "ServerCell")
        tableView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(tableView)
        
        NSLayoutConstraint.activate([
            tableView.topAnchor.constraint(equalTo: segmentedControl.bottomAnchor, constant: 16),
            tableView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            tableView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            tableView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])
    }
    
    private func setupDirectConnect() {
        directConnectView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(directConnectView)
        
        // Address field
        addressTextField.placeholder = "Server Address"
        addressTextField.borderStyle = .roundedRect
        addressTextField.translatesAutoresizingMaskIntoConstraints = false
        directConnectView.addSubview(addressTextField)
        
        // Port field
        portTextField.placeholder = "Port (default: 25565)"
        portTextField.borderStyle = .roundedRect
        portTextField.text = "25565"
        portTextField.translatesAutoresizingMaskIntoConstraints = false
        directConnectView.addSubview(portTextField)
        
        // Connect button
        connectButton.setTitle("Connect", for: .normal)
        connectButton.setTitleColor(.white, for: .normal)
        connectButton.backgroundColor = .systemBlue
        connectButton.layer.cornerRadius = 8
        connectButton.addTarget(self, action: #selector(connectToServer), for: .touchUpInside)
        connectButton.translatesAutoresizingMaskIntoConstraints = false
        directConnectView.addSubview(connectButton)
        
        NSLayoutConstraint.activate([
            directConnectView.topAnchor.constraint(equalTo: segmentedControl.bottomAnchor, constant: 16),
            directConnectView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
            directConnectView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16),
            directConnectView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
            
            addressTextField.topAnchor.constraint(equalTo: directConnectView.topAnchor, constant: 40),
            addressTextField.leadingAnchor.constraint(equalTo: directConnectView.leadingAnchor),
            addressTextField.trailingAnchor.constraint(equalTo: directConnectView.trailingAnchor),
            addressTextField.heightAnchor.constraint(equalToConstant: 44),
            
            portTextField.topAnchor.constraint(equalTo: addressTextField.bottomAnchor, constant: 16),
            portTextField.leadingAnchor.constraint(equalTo: directConnectView.leadingAnchor),
            portTextField.trailingAnchor.constraint(equalTo: directConnectView.trailingAnchor),
            portTextField.heightAnchor.constraint(equalToConstant: 44),
            
            connectButton.topAnchor.constraint(equalTo: portTextField.bottomAnchor, constant: 24),
            connectButton.centerXAnchor.constraint(equalTo: directConnectView.centerXAnchor),
            connectButton.widthAnchor.constraint(equalToConstant: 120),
            connectButton.heightAnchor.constraint(equalToConstant: 44)
        ])
        
        directConnectView.isHidden = true
    }
    
    private func setupCreateServer() {
        createServerView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(createServerView)
        
        // Server name field
        serverNameTextField.placeholder = "Server Name"
        serverNameTextField.borderStyle = .roundedRect
        serverNameTextField.translatesAutoresizingMaskIntoConstraints = false
        createServerView.addSubview(serverNameTextField)
        
        // Max players
        maxPlayersLabel.text = "Max Players: 10"
        maxPlayersLabel.font = .systemFont(ofSize: 16, weight: .medium)
        maxPlayersLabel.translatesAutoresizingMaskIntoConstraints = false
        createServerView.addSubview(maxPlayersLabel)
        
        maxPlayersSlider.minimumValue = 2
        maxPlayersSlider.maximumValue = 20
        maxPlayersSlider.value = 10
        maxPlayersSlider.addTarget(self, action: #selector(maxPlayersChanged), for: .valueChanged)
        maxPlayersSlider.translatesAutoresizingMaskIntoConstraints = false
        createServerView.addSubview(maxPlayersSlider)
        
        // Password field
        passwordTextField.placeholder = "Password (optional)"
        passwordTextField.borderStyle = .roundedRect
        passwordTextField.isSecureTextEntry = true
        passwordTextField.translatesAutoresizingMaskIntoConstraints = false
        createServerView.addSubview(passwordTextField)
        
        // Create button
        createButton.setTitle("Create Server", for: .normal)
        createButton.setTitleColor(.white, for: .normal)
        createButton.backgroundColor = .systemGreen
        createButton.layer.cornerRadius = 8
        createButton.addTarget(self, action: #selector(createServer), for: .touchUpInside)
        createButton.translatesAutoresizingMaskIntoConstraints = false
        createServerView.addSubview(createButton)
        
        NSLayoutConstraint.activate([
            createServerView.topAnchor.constraint(equalTo: segmentedControl.bottomAnchor, constant: 16),
            createServerView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
            createServerView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16),
            createServerView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
            
            serverNameTextField.topAnchor.constraint(equalTo: createServerView.topAnchor, constant: 40),
            serverNameTextField.leadingAnchor.constraint(equalTo: createServerView.leadingAnchor),
            serverNameTextField.trailingAnchor.constraint(equalTo: createServerView.trailingAnchor),
            serverNameTextField.heightAnchor.constraint(equalToConstant: 44),
            
            maxPlayersLabel.topAnchor.constraint(equalTo: serverNameTextField.bottomAnchor, constant: 24),
            maxPlayersLabel.leadingAnchor.constraint(equalTo: createServerView.leadingAnchor),
            maxPlayersLabel.trailingAnchor.constraint(equalTo: createServerView.trailingAnchor),
            
            maxPlayersSlider.topAnchor.constraint(equalTo: maxPlayersLabel.bottomAnchor, constant: 8),
            maxPlayersSlider.leadingAnchor.constraint(equalTo: createServerView.leadingAnchor),
            maxPlayersSlider.trailingAnchor.constraint(equalTo: createServerView.trailingAnchor),
            
            passwordTextField.topAnchor.constraint(equalTo: maxPlayersSlider.bottomAnchor, constant: 24),
            passwordTextField.leadingAnchor.constraint(equalTo: createServerView.leadingAnchor),
            passwordTextField.trailingAnchor.constraint(equalTo: createServerView.trailingAnchor),
            passwordTextField.heightAnchor.constraint(equalToConstant: 44),
            
            createButton.topAnchor.constraint(equalTo: passwordTextField.bottomAnchor, constant: 24),
            createButton.centerXAnchor.constraint(equalTo: createServerView.centerXAnchor),
            createButton.widthAnchor.constraint(equalToConstant: 140),
            createButton.heightAnchor.constraint(equalToConstant: 44)
        ])
        
        createServerView.isHidden = true
    }
    
    private func loadServers() {
        // Load actual servers from master server or LAN discovery
        var serverPtr: UnsafeMutablePointer<BridgeServerInfo>?
        var serverCount: UInt32 = 0
        
        let success = bridge_get_server_list(&serverPtr, &serverCount)
        
        if success, let serverPtr = serverPtr, serverCount > 0 {
            let buffer = UnsafeBufferPointer(start: serverPtr, count: Int(serverCount))
            
            servers.removeAll()
            
            for i in 0..<Int(serverCount) {
                let serverInfo = buffer[i]
                
                let server = Server(
                    id: UUID(),
                    name: String(cString: serverInfo.name),
                    address: String(cString: serverInfo.address),
                    port: Int(serverInfo.port),
                    playerCount: Int(serverInfo.player_count),
                    maxPlayers: Int(serverInfo.max_players),
                    version: String(cString: serverInfo.version),
                    ping: Int(serverInfo.ping),
                    description: String(cString: serverInfo.description),
                    isOnline: serverInfo.is_online,
                    hasPassword: serverInfo.has_password
                )
                
                servers.append(server)
            }
            
            // Free memory allocated by C function
            serverPtr.deallocate()
        } else {
            // Fallback to mock servers if bridge fails
            servers = createMockServers()
        }
        
        tableView.reloadData()
    }
    
    private func createMockServers() -> [Server] {
        return [
            Server(
                id: UUID(),
                name: "Crafting Paradise",
                address: "mc.craftingparadise.com",
                port: 25565,
                playerCount: 12,
                maxPlayers: 20,
                version: "1.0.0",
                ping: 45,
                description: "A friendly survival server with economy and minigames!",
                isOnline: true,
                hasPassword: false
            ),
            Server(
                id: UUID(),
                name: "Hardcore Survival",
                address: "192.168.1.100",
                port: 25565,
                playerCount: 3,
                maxPlayers: 8,
                version: "1.0.0",
                ping: 12,
                description: "Hard mode survival server for experienced players.",
                isOnline: true,
                hasPassword: true
            ),
            Server(
                id: UUID(),
                name: "Creative Build",
                address: "mc.creative.net",
                port: 25565,
                playerCount: 0,
                maxPlayers: 16,
                version: "1.0.0",
                ping: 89,
                description: "Free building server with WorldEdit and custom plots.",
                isOnline: false,
                hasPassword: false
            )
        ]
    }
    
    @objc private func segmentChanged() {
        switch segmentedControl.selectedSegmentIndex {
        case 0: // Browse
            tableView.isHidden = false
            directConnectView.isHidden = true
            createServerView.isHidden = true
        case 1: // Direct Connect
            tableView.isHidden = true
            directConnectView.isHidden = false
            createServerView.isHidden = true
        case 2: // Create Server
            tableView.isHidden = true
            directConnectView.isHidden = true
            createServerView.isHidden = false
        default:
            break
        }
    }
    
    @objc private func maxPlayersChanged() {
        maxPlayersLabel.text = "Max Players: \(Int(maxPlayersSlider.value))"
    }
    
    @objc private func cancelTapped() {
        navigationController?.popViewController(animated: true)
    }
    
    @objc private func connectToServer() {
        guard let address = addressTextField.text, !address.isEmpty else {
            showAlert(title: "Error", message: "Please enter a server address")
            return
        }
        
        let port = Int(portTextField.text ?? "25565") ?? 25565
        
        // Implement actual server connection
        let success = bridge_connect_to_server(address, UInt32(port))
        
        if success {
            print("Successfully connected to server: \(address):\(port)")
            gameStateManager.transition(to: .loading)
            let loadingViewController = LoadingViewController()
            navigationController?.setViewControllers([loadingViewController], animated: true)
        } else {
            showAlert(title: "Connection Failed", message: "Failed to connect to \(address):\(port). Please check the address and try again.")
        }
    }
    
    @objc private func createServer() {
        guard let serverName = serverNameTextField.text, !serverName.isEmpty else {
            showAlert(title: "Error", message: "Please enter a server name")
            return
        }
        
        let maxPlayers = Int(maxPlayersSlider.value)
        let password = passwordTextField.text ?? ""
        
        // Implement actual server creation
        let success = bridge_create_server(serverName, UInt32(maxPlayers), password)
        
        if success {
            print("Successfully created server: \(serverName) with \(maxPlayers) players")
            gameStateManager.transition(to: .loading)
            let loadingViewController = LoadingViewController()
            navigationController?.setViewControllers([loadingViewController], animated: true)
        } else {
            showAlert(title: "Server Creation Failed", message: "Failed to create server '\(serverName)'. Please check your network settings and try again.")
        }
    }
    
    private func showAlert(title: String, message: String) {
        let alert = UIAlertController(title: title, message: message, preferredStyle: .alert)
        alert.addAction(UIAlertAction(title: "OK", style: .default))
        present(alert, animated: true)
    }
}

extension MultiplayerViewController: UITableViewDataSource, UITableViewDelegate {
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return servers.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "ServerCell", for: indexPath) as! ServerCell
        cell.configure(with: servers[indexPath.row])
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        let server = servers[indexPath.row]
        if server.isOnline {
            // Implement server connection
            let success = bridge_connect_to_server(server.address, UInt32(server.port))
            
            if success {
                print("Successfully connected to server: \(server.name)")
                gameStateManager.transition(to: .loading)
                let loadingViewController = LoadingViewController()
                navigationController?.setViewControllers([loadingViewController], animated: true)
            } else {
                showAlert(title: "Connection Failed", message: "Failed to connect to \(server.name). Please try again later.")
            }
        } else {
            showAlert(title: "Server Offline", message: "This server is currently offline.")
        }
        tableView.deselectRow(at: indexPath, animated: true)
    }
    
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        return 100
    }
}

class ServerCell: UITableViewCell {
    
    private let nameLabel = UILabel()
    private let addressLabel = UILabel()
    private let playersLabel = UILabel()
    private let pingLabel = UILabel()
    private let descriptionLabel = UILabel()
    private let lockIcon = UIImageView()
    private let statusView = UIView()
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        setupUI()
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    private func setupUI() {
        let stackView = UIStackView()
        stackView.axis = .vertical
        stackView.spacing = 4
        stackView.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(stackView)
        
        // Top row with name, players, ping, and lock
        let topRow = UIStackView()
        topRow.axis = .horizontal
        topRow.spacing = 8
        topRow.alignment = .center
        
        nameLabel.font = .systemFont(ofSize: 16, weight: .semibold)
        nameLabel.textColor = .label
        topRow.addArrangedSubview(nameLabel)
        
        let spacer = UIView()
        spacer.setContentHuggingPriority(.defaultLow, for: .horizontal)
        topRow.addArrangedSubview(spacer)
        
        playersLabel.font = .systemFont(ofSize: 14, weight: .medium)
        playersLabel.textColor = .secondaryLabel
        topRow.addArrangedSubview(playersLabel)
        
        pingLabel.font = .systemFont(ofSize: 14, weight: .medium)
        pingLabel.textColor = .secondaryLabel
        pingLabel.setContentHuggingPriority(.defaultHigh, for: .horizontal)
        topRow.addArrangedSubview(pingLabel)
        
        lockIcon.image = UIImage(systemName: "lock.fill")
        lockIcon.tintColor = .systemOrange
        lockIcon.contentMode = .scaleAspectFit
        lockIcon.setContentHuggingPriority(.defaultHigh, for: .horizontal)
        topRow.addArrangedSubview(lockIcon)
        
        stackView.addArrangedSubview(topRow)
        
        // Address label
        addressLabel.font = .systemFont(ofSize: 12)
        addressLabel.textColor = .tertiaryLabel
        stackView.addArrangedSubview(addressLabel)
        
        // Description label
        descriptionLabel.font = .systemFont(ofSize: 14)
        descriptionLabel.textColor = .secondaryLabel
        descriptionLabel.numberOfLines = 2
        stackView.addArrangedSubview(descriptionLabel)
        
        // Status indicator
        statusView.layer.cornerRadius = 4
        statusView.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(statusView)
        
        NSLayoutConstraint.activate([
            stackView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
            stackView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),
            stackView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 8),
            stackView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -8),
            
            statusView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 8),
            statusView.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
            statusView.widthAnchor.constraint(equalToConstant: 8),
            statusView.heightAnchor.constraint(equalToConstant: 8)
        ])
    }
    
    func configure(with server: Server) {
        nameLabel.text = server.name
        addressLabel.text = "\(server.address):\(server.port)"
        playersLabel.text = "\(server.playerCount)/\(server.maxPlayers) players"
        pingLabel.text = "\(server.ping)ms"
        descriptionLabel.text = server.description
        
        lockIcon.isHidden = !server.hasPassword
        
        if server.isOnline {
            statusView.backgroundColor = .systemGreen
        } else {
            statusView.backgroundColor = .systemRed
        }
    }
}
