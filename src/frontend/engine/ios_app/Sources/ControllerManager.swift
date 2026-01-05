import UIKit
import Combine
import GameController

class ControllerManager: NSObject {
    
    static let shared = ControllerManager()
    
    @Published var isConnected: Bool = false
    @Published var controllerType: String = "None"
    @Published var leftStick: CGPoint = .zero
    @Published var rightStick: CGPoint = .zero
    @Published var leftTrigger: Float = 0
    @Published var rightTrigger: Float = 0
    @Published var buttonStates: [String: Bool] = [:]
    
    private var gameController: GCController?
    private var cancellables = Set<AnyCancellable>()
    
    // Button mappings
    private let buttonMappings: [GCControllerButtonInput: String] = [
        .buttonA: "A",
        .buttonB: "B",
        .buttonX: "X",
        .buttonY: "Y",
        .leftShoulder: "LB",
        .rightShoulder: "RB",
        .leftTrigger: "LT",
        .rightTrigger: "RT",
        .dpad: "DPad",
        .buttonMenu: "Menu",
        .buttonOptions: "Options"
    ]
    
    override init() {
        super.init()
        setupControllerObservers()
        setupKeyboardObservers()
    }
    
    private func setupControllerObservers() {
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(controllerDidConnect),
            name: .GCControllerDidConnect,
            object: nil
        )
        
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(controllerDidDisconnect),
            name: .GCControllerDidDisconnect,
            object: nil
        )
        
        // Check for already connected controllers
        GCController.controllers().forEach { controller in
            setupController(controller)
        }
    }
    
    private func setupKeyboardObservers() {
        // Setup keyboard input for desktop platforms
        NSEvent.addLocalMonitorForEvents(matching: .keyDown) { [weak self] event in
            self?.handleKeyPress(event)
            return nil
        }
        
        NSEvent.addLocalMonitorForEvents(matching: .keyUp) { [weak self] event in
            self?.handleKeyRelease(event)
            return nil
        }
    }
    
    @objc private func controllerDidConnect(_ notification: Notification) {
        guard let controller = notification.object as? GCController else { return }
        setupController(controller)
    }
    
    @objc private func controllerDidDisconnect(_ notification: Notification) {
        guard let controller = notification.object as? GCController else { return }
        if controller == gameController {
            gameController = nil
            isConnected = false
            controllerType = "None"
            resetInputs()
        }
    }
    
    private func setupController(_ controller: GCController) {
        gameController = controller
        isConnected = true
        
        // Determine controller type
        if controller.extendedGamepad != nil {
            controllerType = "Extended Gamepad"
        } else if controller.microGamepad != nil {
            controllerType = "Micro Gamepad"
        } else {
            controllerType = "Unknown"
        }
        
        // Setup extended gamepad if available
        if let extendedGamepad = controller.extendedGamepad {
            setupExtendedGamepad(extendedGamepad)
        }
        
        // Setup micro gamepad if available
        if let microGamepad = controller.microGamepad {
            setupMicroGamepad(microGamepad)
        }
        
        print("Controller connected: \(controllerType)")
    }
    
    private func setupExtendedGamepad(_ gamepad: GCExtendedGamepad) {
        // Left stick
        gamepad.dpad.xAxis.valueChangedHandler = { [weak self] (axis, value) in
            self?.leftStick.x = CGFloat(value)
        }
        
        gamepad.dpad.yAxis.valueChangedHandler = { [weak self] (axis, value) in
            self?.leftStick.y = CGFloat(value)
        }
        
        // Right stick
        gamepad.leftThumbstick.xAxis.valueChangedHandler = { [weak self] (axis, value) in
            self?.leftStick.x = CGFloat(value)
        }
        
        gamepad.leftThumbstick.yAxis.valueChangedHandler = { [weak self] (axis, value) in
            self?.leftStick.y = CGFloat(value)
        }
        
        gamepad.rightThumbstick.xAxis.valueChangedHandler = { [weak self] (axis, value) in
            self?.rightStick.x = CGFloat(value)
        }
        
        gamepad.rightThumbstick.yAxis.valueChangedHandler = { [weak self] (axis, value) in
            self?.rightStick.y = CGFloat(value)
        }
        
        // Triggers
        gamepad.leftTrigger.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.leftTrigger = value
            self?.buttonStates["LT"] = pressed
        }
        
        gamepad.rightTrigger.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.rightTrigger = value
            self?.buttonStates["RT"] = pressed
        }
        
        // Shoulder buttons
        gamepad.leftShoulder.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["LB"] = pressed
        }
        
        gamepad.rightShoulder.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["RB"] = pressed
        }
        
        // Face buttons
        gamepad.buttonA.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["A"] = pressed
        }
        
        gamepad.buttonB.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["B"] = pressed
        }
        
        gamepad.buttonX.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["X"] = pressed
        }
        
        gamepad.buttonY.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["Y"] = pressed
        }
        
        // D-pad
        gamepad.dpad.xAxis.valueChangedHandler = { [weak self] (axis, value) in
            self?.buttonStates["DPad_Left"] = value < -0.5
            self?.buttonStates["DPad_Right"] = value > 0.5
        }
        
        gamepad.dpad.yAxis.valueChangedHandler = { [weak self] (axis, value) in
            self?.buttonStates["DPad_Up"] = value > 0.5
            self?.buttonStates["DPad_Down"] = value < -0.5
        }
        
        // Menu buttons
        gamepad.buttonMenu.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["Menu"] = pressed
        }
        
        gamepad.buttonOptions.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["Options"] = pressed
        }
    }
    
    private func setupMicroGamepad(_ gamepad: GCMicroGamepad) {
        // D-pad
        gamepad.dpad.xAxis.valueChangedHandler = { [weak self] (axis, value) in
            self?.leftStick.x = CGFloat(value)
            self?.buttonStates["DPad_Left"] = value < -0.5
            self?.buttonStates["DPad_Right"] = value > 0.5
        }
        
        gamepad.dpad.yAxis.valueChangedHandler = { [weak self] (axis, value) in
            self?.leftStick.y = CGFloat(value)
            self?.buttonStates["DPad_Up"] = value > 0.5
            self?.buttonStates["DPad_Down"] = value < -0.5
        }
        
        // Face buttons
        gamepad.buttonA.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["A"] = pressed
        }
        
        gamepad.buttonX.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["X"] = pressed
        }
        
        // Shoulder buttons
        gamepad.buttonL1.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["LB"] = pressed
        }
        
        gamepad.buttonR1.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["RB"] = pressed
        }
        
        // Menu button
        gamepad.buttonMenu.valueChangedHandler = { [weak self] (button, value, pressed) in
            self?.buttonStates["Menu"] = pressed
        }
    }
    
    private func handleKeyPress(_ event: NSEvent) {
        let keyCode = event.keyCode
        
        switch keyCode {
        case 13: // W
            leftStick.y = 1.0
        case 1:  // S
            leftStick.y = -1.0
        case 0:  // A
            leftStick.x = -1.0
        case 2:  // D
            leftStick.x = 1.0
        case 49: // Space
            buttonStates["Jump"] = true
        case 53: // Esc
            buttonStates["Menu"] = true
        case 45: // E
            buttonStates["Interact"] = true
        case 43: // Q
            buttonStates["Drop"] = true
        case 18: // 1
            buttonStates["Hotbar1"] = true
        case 19: // 2
            buttonStates["Hotbar2"] = true
        case 20: // 3
            buttonStates["Hotbar3"] = true
        case 21: // 4
            buttonStates["Hotbar4"] = true
        case 23: // 5
            buttonStates["Hotbar5"] = true
        case 22: // 6
            buttonStates["Hotbar6"] = true
        case 26: // 7
            buttonStates["Hotbar7"] = true
        case 28: // 8
            buttonStates["Hotbar8"] = true
        case 25: // 9
            buttonStates["Hotbar9"] = true
        case 29: // 0
            buttonStates["Hotbar0"] = true
        case 46: // I (Inventory)
            buttonStates["Inventory"] = true
        case 8:  // C (Crafting)
            buttonStates["Crafting"] = true
        default:
            break
        }
    }
    
    private func handleKeyRelease(_ event: NSEvent) {
        let keyCode = event.keyCode
        
        switch keyCode {
        case 13, 1: // W, S
            leftStick.y = 0.0
        case 0, 2:  // A, D
            leftStick.x = 0.0
        case 49: // Space
            buttonStates["Jump"] = false
        case 53: // Esc
            buttonStates["Menu"] = false
        case 45: // E
            buttonStates["Interact"] = false
        case 43: // Q
            buttonStates["Drop"] = false
        case 18: // 1
            buttonStates["Hotbar1"] = false
        case 19: // 2
            buttonStates["Hotbar2"] = false
        case 20: // 3
            buttonStates["Hotbar3"] = false
        case 21: // 4
            buttonStates["Hotbar4"] = false
        case 23: // 5
            buttonStates["Hotbar5"] = false
        case 22: // 6
            buttonStates["Hotbar6"] = false
        case 26: // 7
            buttonStates["Hotbar7"] = false
        case 28: // 8
            buttonStates["Hotbar8"] = false
        case 25: // 9
            buttonStates["Hotbar9"] = false
        case 29: // 0
            buttonStates["Hotbar0"] = false
        case 46: // I
            buttonStates["Inventory"] = false
        case 8:  // C
            buttonStates["Crafting"] = false
        default:
            break
        }
    }
    
    private func resetInputs() {
        leftStick = .zero
        rightStick = .zero
        leftTrigger = 0
        rightTrigger = 0
        buttonStates.removeAll()
    }
    
    // Public methods for game integration
    func isButtonPressed(_ button: String) -> Bool {
        return buttonStates[button] ?? false
    }
    
    func getMovementVector() -> CGPoint {
        return leftStick
    }
    
    func getLookVector() -> CGPoint {
        return rightStick
    }
    
    func getTriggerValue(_ trigger: String) -> Float {
        switch trigger.lowercased() {
        case "left", "lt":
            return leftTrigger
        case "right", "rt":
            return rightTrigger
        default:
            return 0
        }
    }
    
    func vibrate(intensity: Float, duration: TimeInterval) {
        // Haptic feedback for controllers that support it
        if let gameController = gameController {
            #if os(iOS)
            if #available(iOS 13.0, *) {
                gameController.haptics?.createEngine(withCompletionHandler: { hapticEngine in
                    hapticEngine?.start(completionHandler: nil)
                    
                    let hapticEvent = CHHapticEvent(
                        eventType: .hapticTransient,
                        parameters: [
                            CHHapticEventParameter(parameterID: .hapticIntensity, value: intensity),
                            CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.5)
                        ],
                        relativeTime: 0
                    )
                    
                    do {
                        let pattern = try CHHapticPattern(events: [hapticEvent], parameters: [])
                        let player = try hapticEngine?.makePlayer(with: pattern)
                        player?.start(atTime: 0)
                        
                        DispatchQueue.main.asyncAfter(deadline: .now() + duration) {
                            hapticEngine?.stop(completionHandler: nil)
                        }
                    } catch {
                        print("Failed to play haptic pattern: \(error)")
                    }
                })
            }
            #endif
        }
    }
}

// MARK: - Controller Configuration

struct ControllerConfig {
    var sensitivity: Float = 1.0
    var invertY: Bool = false
    var deadzone: Float = 0.1
    var vibrationEnabled: Bool = true
    var buttonMappings: [String: String] = [:]
    
    static let `default` = ControllerConfig()
}

class ControllerSettingsViewController: UIViewController {
    
    @Published var config = ControllerConfig.default
    private var cancellables = Set<AnyCancellable>()
    
    private let sensitivitySlider = UISlider()
    private let invertYSwitch = UISwitch()
    private let deadzoneSlider = UISlider()
    private let vibrationSwitch = UISwitch()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = "Controller Settings"
        view.backgroundColor = .systemBackground
        
        setupUI()
        setupBindings()
    }
    
    private func setupUI() {
        let stackView = UIStackView()
        stackView.axis = .vertical
        stackView.spacing = 20
        stackView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(stackView)
        
        // Sensitivity
        let sensitivityLabel = UILabel()
        sensitivityLabel.text = "Sensitivity"
        sensitivityStackView.addArrangedSubview(sensitivityLabel)
        
        sensitivitySlider.minimumValue = 0.1
        sensitivitySlider.maximumValue = 3.0
        sensitivitySlider.value = config.sensitivity
        stackView.addArrangedSubview(sensitivitySlider)
        
        // Invert Y
        let invertYLabel = UILabel()
        invertYLabel.text = "Invert Y-Axis"
        stackView.addArrangedSubview(invertYLabel)
        
        invertYSwitch.isOn = config.invertY
        stackView.addArrangedSubview(invertYSwitch)
        
        // Deadzone
        let deadzoneLabel = UILabel()
        deadzoneLabel.text = "Deadzone"
        stackView.addArrangedSubview(deadzoneLabel)
        
        deadzoneSlider.minimumValue = 0.0
        deadzoneSlider.maximumValue = 0.5
        deadzoneSlider.value = config.deadzone
        stackView.addArrangedSubview(deadzoneSlider)
        
        // Vibration
        let vibrationLabel = UILabel()
        vibrationLabel.text = "Vibration"
        stackView.addArrangedSubview(vibrationLabel)
        
        vibrationSwitch.isOn = config.vibrationEnabled
        stackView.addArrangedSubview(vibrationSwitch)
        
        // Test vibration button
        let testButton = UIButton(type: .system)
        testButton.setTitle("Test Vibration", for: .normal)
        testButton.addTarget(self, action: #selector(testVibration), for: .touchUpInside)
        stackView.addArrangedSubview(testButton)
        
        NSLayoutConstraint.activate([
            stackView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 20),
            stackView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -20),
            stackView.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor, constant: 20),
            stackView.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -20)
        ])
    }
    
    private func setupBindings() {
        sensitivitySlider.addTarget(self, action: #selector(sensitivityChanged), for: .valueChanged)
        invertYSwitch.addTarget(self, action: #selector(invertYChanged), for: .valueChanged)
        deadzoneSlider.addTarget(self, action: #selector(deadzoneChanged), for: .valueChanged)
        vibrationSwitch.addTarget(self, action: #selector(vibrationChanged), for: .valueChanged)
    }
    
    @objc private func sensitivityChanged() {
        config.sensitivity = sensitivitySlider.value
    }
    
    @objc private func invertYChanged() {
        config.invertY = invertYSwitch.isOn
    }
    
    @objc private func deadzoneChanged() {
        config.deadzone = deadzoneSlider.value
    }
    
    @objc private func vibrationChanged() {
        config.vibrationEnabled = vibrationSwitch.isOn
    }
    
    @objc private func testVibration() {
        if config.vibrationEnabled {
            ControllerManager.shared.vibrate(intensity: 0.8, duration: 0.5)
        }
    }
}
