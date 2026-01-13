// GameViewController.swift
//
// Purpose: This view controller is responsible for hosting the MetalKit view,
// initializing and managing the game engine, and handling user input for gameplay.
// It bridges the iOS UI events with the underlying C game engine.
//
// Role: Acts as the primary display and interaction hub for the game. It sets up
// the rendering surface, orchestrates the game loop through `CADisplayLink`,
// and translates touch/pan gestures into game engine commands.
//
// Lifecycle:
// - `viewDidLoad()`: Initializes the Metal view, sets up the game engine,
//   configures the display link for updates, and registers gesture recognizers.
// - `viewWillDisappear(_:)`: Invalidates the display link and shuts down the game engine
//   when the view controller is about to be removed.
//
// Inputs: User touch and pan gestures, system display link events.
// Outputs: Renders game content to the `MTKView`, passes input events to the C game engine.
//
import UIKit
import MetalKit
import Combine

// TODO: High - Implement a system for handling game state changes (e.g., main menu, loading screen, in-game).
// TODO: High - Implement a system for pausing and resuming the game.
class GameViewController: UIViewController {
    
    var metalView: MTKView?
    var gameEngine: GameEngine?
    var displayLink: CADisplayLink?
    private let gameStateManager = GameStateManager.shared
    private var cancellables = Set<AnyCancellable>()
    
    private var isPaused: Bool = false {
        didSet {
            displayLink?.isPaused = isPaused
        }
    }
    
    // Pause menu overlay
    private let pauseOverlay = UIView()
    private let pauseStackView = UIStackView()
    
    // Game HUD
    private let gameHUD = GameHUD()
    
    // Touch controls
    private let touchControls = TouchControlsView()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = "Game"
        view.backgroundColor = .black
        navigationItem.hidesBackButton = true
        
        setupPauseOverlay()
        setupGameStateObservers()
        setupHUD()
        setupTouchControls()
        
        // Initialize external controllers
        bridge_init_controllers()
        
        // Create Metal view
        let metalView = MTKView(frame: view.bounds)
        metalView.device = MTLCreateSystemDefaultDevice()
        metalView.delegate = self
        self.metalView = metalView
        view.addSubview(metalView)
        
        metalView.translatesAutoresizingMaskIntoConstraints = false
        NSLayoutConstraint.activate([
            metalView.topAnchor.constraint(equalTo: view.topAnchor),
            metalView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
            metalView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            metalView.trailingAnchor.constraint(equalTo: view.trailingAnchor)
        ])
        
        // Initialize game engine
        let width = UInt32(view.bounds.width)
        let height = UInt32(view.bounds.height)
        game_engine_init(width, height)
        self.gameEngine = GameEngine()
        
        game_engine_start_new_game()
        
        // Setup display link for continuous rendering
        setupDisplayLink()
        
        // Add gestures for input
        setupGestures()
        
        // Add pause gesture
        setupPauseGesture()
        
        // Setup keyboard input
        setupKeyboardInput()
        
        // Setup controller update timer
        setupControllerUpdates()
    }
    
    private func setupPauseOverlay() {
        pauseOverlay.backgroundColor = UIColor.black.withAlphaComponent(0.7)
        pauseOverlay.isHidden = true
        pauseOverlay.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(pauseOverlay)
        
        pauseStackView.axis = .vertical
        pauseStackView.spacing = 20
        pauseStackView.alignment = .center
        pauseStackView.translatesAutoresizingMaskIntoConstraints = false
        pauseOverlay.addSubview(pauseStackView)
        
        let resumeButton = createPauseButton(title: "Resume", action: #selector(resumeGame))
        let settingsButton = createPauseButton(title: "Settings", action: #selector(goToSettings))
        let mainMenuButton = createPauseButton(title: "Main Menu", action: #selector(goToMainMenu))
        
        pauseStackView.addArrangedSubview(resumeButton)
        pauseStackView.addArrangedSubview(settingsButton)
        pauseStackView.addArrangedSubview(mainMenuButton)
        
        NSLayoutConstraint.activate([
            pauseOverlay.topAnchor.constraint(equalTo: view.topAnchor),
            pauseOverlay.bottomAnchor.constraint(equalTo: view.bottomAnchor),
            pauseOverlay.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            pauseOverlay.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            
            pauseStackView.centerXAnchor.constraint(equalTo: pauseOverlay.centerXAnchor),
            pauseStackView.centerYAnchor.constraint(equalTo: pauseOverlay.centerYAnchor)
        ])
    }
    
    private func createPauseButton(title: String, action: Selector) -> UIButton {
        let button = UIButton(type: .system)
        button.setTitle(title, for: .normal)
        button.setTitleColor(.white, for: .normal)
        button.titleLabel?.font = .systemFont(ofSize: 24, weight: .semibold)
        button.backgroundColor = UIColor.systemBlue.withAlphaComponent(0.8)
        button.layer.cornerRadius = 12
        button.addTarget(self, action: action, for: .touchUpInside)
        button.widthAnchor.constraint(equalToConstant: 200).isActive = true
        button.heightAnchor.constraint(equalToConstant: 50).isActive = true
        return button
    }
    
    private func setupGameStateObservers() {
        gameStateManager.$currentState
            .receive(on: DispatchQueue.main)
            .sink { [weak self] state in
                switch state {
                case .paused:
                    self?.showPauseMenu()
                case .inGame:
                    self?.hidePauseMenu()
                default:
                    break
                }
            }
            .store(in: &cancellables)
    }
    
    private func setupPauseGesture() {
        // Add tap gesture to handle pause
        let tapGesture = UITapGestureRecognizer(target: self, action: #selector(handlePauseGesture))
        tapGesture.numberOfTapsRequired = 2
        view.addGestureRecognizer(tapGesture)
    }
    
    @objc private func handlePauseGesture() {
        if gameStateManager.currentState == .inGame {
            gameStateManager.transition(to: .paused)
        }
    }
    
    private func showPauseMenu() {
        isPaused = true
        pauseOverlay.isHidden = false
        pauseOverlay.alpha = 0
        UIView.animate(withDuration: 0.3) {
            self.pauseOverlay.alpha = 1
        }
    }
    
    private func hidePauseMenu() {
        UIView.animate(withDuration: 0.3, animations: {
            self.pauseOverlay.alpha = 0
        }) { _ in
            self.pauseOverlay.isHidden = true
            self.isPaused = false
        }
    }
    
    @objc private func resumeGame() {
        gameStateManager.transition(to: .inGame)
    }
    
    @objc private func goToSettings() {
        let settingsViewController = SettingsViewController()
        navigationController?.pushViewController(settingsViewController, animated: true)
    }
    
    @objc private func goToMainMenu() {
        gameStateManager.transition(to: .mainMenu)
        navigationController?.popToRootViewController(animated: true)
    }
    
    private func setupHUD() {
        gameHUD.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(gameHUD)
        
        NSLayoutConstraint.activate([
            gameHUD.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            gameHUD.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            gameHUD.topAnchor.constraint(equalTo: view.topAnchor),
            gameHUD.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])
        
        // Initialize with sample data
        initializeHUDData()
    }
    
    private func initializeHUDData() {
        // Sample hotbar items
        let sampleItems = [
            HotbarItem(name: "Sword", icon: "", durability: 85),
            HotbarItem(name: "Pickaxe", icon: "", durability: 60),
            HotbarItem(name: "Wood", count: 64),
            HotbarItem(name: "Stone", count: 32),
            HotbarItem(name: "Torch", count: 16),
            HotbarItem(name: "Apple", count: 5),
            HotbarItem(name: "Bow", icon: "", durability: 100),
            HotbarItem(name: "Arrow", count: 24),
            HotbarItem(name: "Dirt", count: 64),
            HotbarItem(name: "Water Bucket", icon: "")
        ]
        
        gameHUD.hotbarItems = sampleItems
        gameHUD.health = 18.0
        gameHUD.hunger = 15.5
        gameHUD.armor = 10.0
        gameHUD.experience = 45.0
        gameHUD.level = 3
        gameHUD.selectedSlot = 0
    }
    
    private func updateHUD() {
        // Update HUD with actual game data from C engine
        var playerData = BridgePlayerData()
        let success = bridge_get_player_data(&playerData)
        
        if success {
            // Update player stats
            gameHUD.health = playerData.health
            gameHUD.hunger = playerData.hunger
            gameHUD.armor = playerData.armor
            gameHUD.experience = playerData.experience
            gameHUD.level = Int(playerData.level)
            gameHUD.selectedSlot = Int(playerData.hotbar_selected_slot)
            gameHUD.oxygen = playerData.oxygen
            
            // Update hotbar items
            var hotbarItems: [HotbarItem] = []
            for i in 0..<10 {
                let itemName = UnsafeMutablePointer<CChar>.allocate(capacity: 64)
                var count: UInt32 = 0
                var maxStack: UInt32 = 0
                
                bridge_get_hotbar_items(UInt32(i), itemName, 64, &count, &maxStack)
                
                let name = String(cString: itemName)
                itemName.deallocate()
                
                if count > 0 {
                    let hotbarItem = HotbarItem(
                        name: name,
                        icon: "", // TODO: Load actual icon
                        durability: maxStack > 1 ? Int(count) : nil,
                        count: maxStack > 1 ? nil : Int(count)
                    )
                    hotbarItems.append(hotbarItem)
                } else {
                    hotbarItems.append(HotbarItem(name: "Empty", icon: "", durability: nil, count: nil))
                }
            }
            gameHUD.hotbarItems = hotbarItems
            
            // Update debug info
            gameHUD.updateDebugInfo(
                position: (x: playerData.position_x, y: playerData.position_y, z: playerData.position_z),
                fps: 60, // TODO: Get actual FPS from engine
                chunks: 49 // TODO: Get actual chunk count from engine
            )
        }
    }
    
    private func setupTouchControls() {
        touchControls.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(touchControls)
        
        NSLayoutConstraint.activate([
            touchControls.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            touchControls.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            touchControls.topAnchor.constraint(equalTo: view.topAnchor),
            touchControls.bottomAnchor.constraint(equalTo: view.bottomAnchor)
        ])
        
        setupTouchControlsBindings()
    }
    
    private func setupTouchControlsBindings() {
        // Movement controls
        touchControls.$movementVector
            .receive(on: DispatchQueue.main)
            .sink { [weak self] vector in
                // Send movement to game engine
                let forward = vector.y
                let strafe = vector.x
                game_engine_handle_movement(forward, strafe)
            }
            .store(in: &cancellables)
        
        // Look controls
        touchControls.$lookVector
            .receive(on: DispatchQueue.main)
            .sink { [weak self] vector in
                // Send look to game engine
                let pitch = Float(vector.y) * 0.02
                let yaw = Float(vector.x) * 0.02
                game_engine_handle_motion(pitch, yaw)
            }
            .store(in: &cancellables)
        
        // Action buttons
        touchControls.$jumpPressed
            .receive(on: DispatchQueue.main)
            .sink { [weak self] pressed in
                if pressed {
                    game_engine_handle_jump()
                }
            }
            .store(in: &cancellables)
        
        touchControls.$attackPressed
            .receive(on: DispatchQueue.main)
            .sink { [weak self] pressed in
                if pressed {
                    game_engine_handle_attack()
                }
            }
            .store(in: &cancellables)
        
        touchControls.$interactPressed
            .receive(on: DispatchQueue.main)
            .sink { [weak self] pressed in
                if pressed {
                    game_engine_handle_interact()
                }
            }
            .store(in: &cancellables)
        
        touchControls.$sprintPressed
            .receive(on: DispatchQueue.main)
            .sink { [weak self] pressed in
                game_engine_handle_sprint(pressed)
            }
            .store(in: &cancellables)
        
        touchControls.$sneakPressed
            .receive(on: DispatchQueue.main)
            .sink { [weak self] pressed in
                game_engine_handle_sneak(pressed)
            }
            .store(in: &cancellables)
    }
    
    private func setupKeyboardInput() {
        // Add keyboard shortcuts
        let tapGesture = UITapGestureRecognizer(target: self, action: #selector(handleKeyPress(_:)))
        tapGesture.numberOfTapsRequired = 1
        view.addGestureRecognizer(tapGesture)
    }
    
    private func setupControllerUpdates() {
        // Setup timer to update controller states
        Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { _ in
            self.updateControllerInput()
        }
    }
    
    private func updateControllerInput() {
        // Update controller states from bridge
        bridge_update_controllers()
        
        let controllerCount = bridge_get_connected_controller_count()
        
        for i in 0..<controllerCount {
            var controllerState = BridgeControllerState()
            let isConnected = bridge_get_controller_state(UInt32(i), &controllerState)
            
            if isConnected {
                // Map controller input to game actions
                handleControllerInput(controllerState, playerIndex: i)
            }
        }
    }
    
    private func handleControllerInput(_ state: BridgeControllerState, playerIndex: Int) {
        // Movement
        let moveForward = state.left_stick_y < -0.5
        let moveBackward = state.left_stick_y > 0.5
        let moveLeft = state.left_stick_x < -0.5
        let moveRight = state.left_stick_x > 0.5
        
        // Look
        let lookUp = state.right_stick_y < -0.5
        let lookDown = state.right_stick_y > 0.5
        let lookLeft = state.right_stick_x < -0.5
        let lookRight = state.right_stick_x > 0.5
        
        // Actions
        let jump = state.button_a
        let attack = state.button_x
        let interact = state.button_b
        let sprint = state.right_trigger > 0.5
        let sneak = state.left_trigger > 0.5
        
        // Send input to game engine
        if moveForward {
            game_engine_handle_movement(1.0, 0.0)
        } else if moveBackward {
            game_engine_handle_movement(-1.0, 0.0)
        } else {
            game_engine_handle_movement(0.0, 0.0)
        }
        
        if moveLeft {
            game_engine_handle_movement(0.0, -1.0)
        } else if moveRight {
            game_engine_handle_movement(0.0, 1.0)
        } else {
            game_engine_handle_movement(0.0, 0.0)
        }
        
        if jump {
            game_engine_handle_jump()
        }
        
        if attack {
            game_engine_handle_attack()
        }
        
        if interact {
            game_engine_handle_interact()
        }
        
        game_engine_handle_sprint(sprint)
        game_engine_handle_sneak(sneak)
        
        // Camera look (simplified - would need proper mapping)
        if lookUp || lookDown || lookLeft || lookRight {
            let pitch = lookUp ? -0.1f : (lookDown ? 0.1f : 0.0f)
            let yaw = lookRight ? 0.1f : (lookLeft ? -0.1f : 0.0f)
            game_engine_handle_motion(pitch, yaw)
        }
        
        // Haptic feedback
        if jump || attack {
            bridge_set_controller_vibration(UInt32(playerIndex), 0.8, 0.2, 200)
        }
    }
    
    @objc private func handleKeyPress(_ gesture: UITapGestureRecognizer) {
        // This is a simplified version - in a real implementation,
        // you'd want to use proper keyboard event handling
        // For now, we'll simulate hotbar selection with taps in different areas
        
        let location = gesture.location(in: view)
        let hotbarArea = view.bounds.height - 100
        
        if location.y > hotbarArea {
            // Calculate which hotbar slot based on x position
            let slotWidth = view.bounds.width / 10
            let slotIndex = Int(location.x / slotWidth)
            
            if slotIndex >= 0 && slotIndex < 10 {
                gameHUD.selectedSlot = slotIndex
            }
        } else if location.y < 100 {
            // Tap in upper area to open inventory
            openInventory()
        }
    }
    
    private func openInventory() {
        let inventoryViewController = InventoryViewController()
        inventoryViewController.modalPresentationStyle = .overFullScreen
        present(inventoryViewController, animated: true)
    }
    
    private func setupDisplayLink() {
        displayLink = CADisplayLink(
            target: self,
            selector: #selector(update)
        )
        displayLink?.preferredFramesPerSecond = 60
        displayLink?.add(to: .main, forMode: .common)
    }
    
    private func setupGestures() {
        // TODO: Medium - Add support for external controllers (e.g., gamepads, keyboards).
        let panGesture = UIPanGestureRecognizer(target: self, action: #selector(handlePan(_:)))
        view.addGestureRecognizer(panGesture)
        
        let touchGesture = UITouchGestureRecognizer(target: self, action: #selector(handleTouch(_:)))
        view.addGestureRecognizer(touchGesture)
    }
    
    @objc private func update() {
        let deltaTime: Float = 1.0 / 60.0
        game_engine_update(deltaTime)
        updateHUD()
        metalView?.setNeedsDisplay()
    }
    
    @objc private func handlePan(_ gesture: UIPanGestureRecognizer) {
        let translation = gesture.translation(in: view)
        let pitch = Float(translation.y) * 0.01
        let yaw = Float(translation.x) * 0.01
        game_engine_handle_motion(pitch, yaw)
    }
    
    @objc private func handleTouch(_ gesture: UITouchGestureRecognizer) {
        guard let touch = gesture.touches(first: view) else { return }
        let location = touch.location(in: view)
        let pressed = gesture.state == .began
        game_engine_handle_touch(Float(location.x), Float(location.y), pressed)
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        
        displayLink?.invalidate()
        displayLink = nil
        game_engine_shutdown()
    }
}

extension GameViewController: MTKViewDelegate {
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
    }
    
    func draw(in view: MTKView) {
        game_engine_render()
    }
}

// Wrapper for UITouchGestureRecognizer since it's not available
class UITouchGestureRecognizer: UIGestureRecognizer {
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent) {
        state = .began
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent) {
        state = .changed
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent) {
        state = .ended
    }
}

// Wrapper class for game engine
class GameEngine {
    init() {
    }
}
