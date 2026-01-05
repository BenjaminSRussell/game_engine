import UIKit
import Combine

class TouchControlsView: UIView {
    
    @Published var movementVector: CGPoint = .zero
    @Published var lookVector: CGPoint = .zero
    @Published var jumpPressed: Bool = false
    @Published var attackPressed: Bool = false
    @Published var interactPressed: Bool = false
    @Published var sprintPressed: Bool = false
    @Published var sneakPressed: Bool = false
    
    private var cancellables = Set<AnyCancellable>()
    
    // Joystick components
    private let movementJoystick = VirtualJoystick()
    private let lookJoystick = VirtualJoystick()
    
    // Action buttons
    private let jumpButton = VirtualButton(title: "Jump", color: .systemGreen)
    private let attackButton = VirtualButton(title: "⚔️", color: .systemRed)
    private let interactButton = VirtualButton(title: "👆", color: .systemBlue)
    private let sprintButton = VirtualButton(title: "🏃", color: .systemOrange)
    private let sneakButton = VirtualButton(title: "👣", color: .systemGray)
    private let inventoryButton = VirtualButton(title: "🎒", color: .systemPurple)
    
    // Control visibility
    private var isControlsVisible: Bool = true
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setupControls()
        setupBindings()
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupControls()
        setupBindings()
    }
    
    private func setupControls() {
        backgroundColor = .clear
        isUserInteractionEnabled = true
        
        setupMovementJoystick()
        setupLookJoystick()
        setupActionButtons()
        setupToggleGesture()
    }
    
    private func setupMovementJoystick() {
        movementJoystick.translatesAutoresizingMaskIntoConstraints = false
        movementJoystick.delegate = self
        addSubview(movementJoystick)
        
        NSLayoutConstraint.activate([
            movementJoystick.leadingAnchor.constraint(equalTo: leadingAnchor, constant: 20),
            movementJoystick.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor, constant: -20),
            movementJoystick.widthAnchor.constraint(equalToConstant: 120),
            movementJoystick.heightAnchor.constraint(equalToConstant: 120)
        ])
    }
    
    private func setupLookJoystick() {
        lookJoystick.translatesAutoresizingMaskIntoConstraints = false
        lookJoystick.delegate = self
        addSubview(lookJoystick)
        
        NSLayoutConstraint.activate([
            lookJoystick.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -20),
            lookJoystick.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor, constant: -20),
            lookJoystick.widthAnchor.constraint(equalToConstant: 120),
            lookJoystick.heightAnchor.constraint(equalToConstant: 120)
        ])
    }
    
    private func setupActionButtons() {
        let buttonStackView = UIStackView()
        buttonStackView.axis = .vertical
        buttonStackView.spacing = 12
        buttonStackView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(buttonStackView)
        
        // Right side action buttons
        let rightStackView = UIStackView()
        rightStackView.axis = .vertical
        rightStackView.spacing = 8
        rightStackView.translatesAutoresizingMaskIntoConstraints = false
        buttonStackView.addArrangedSubview(rightStackView)
        
        attackButton.delegate = self
        rightStackView.addArrangedSubview(attackButton)
        
        interactButton.delegate = self
        rightStackView.addArrangedSubview(interactButton)
        
        // Left side modifier buttons
        let leftStackView = UIStackView()
        leftStackView.axis = .horizontal
        leftStackView.spacing = 8
        leftStackView.translatesAutoresizingMaskIntoConstraints = false
        buttonStackView.addArrangedSubview(leftStackView)
        
        jumpButton.delegate = self
        leftStackView.addArrangedSubview(jumpButton)
        
        sprintButton.delegate = self
        leftStackView.addArrangedSubview(sprintButton)
        
        sneakButton.delegate = self
        leftStackView.addArrangedSubview(sneakButton)
        
        inventoryButton.delegate = self
        addSubview(inventoryButton)
        
        NSLayoutConstraint.activate([
            buttonStackView.trailingAnchor.constraint(equalTo: lookJoystick.leadingAnchor, constant: -20),
            buttonStackView.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor, constant: -20),
            
            attackButton.widthAnchor.constraint(equalToConstant: 60),
            attackButton.heightAnchor.constraint(equalToConstant: 60),
            
            interactButton.widthAnchor.constraint(equalToConstant: 60),
            interactButton.heightAnchor.constraint(equalToConstant: 60),
            
            jumpButton.widthAnchor.constraint(equalToConstant: 50),
            jumpButton.heightAnchor.constraint(equalToConstant: 50),
            
            sprintButton.widthAnchor.constraint(equalToConstant: 50),
            sprintButton.heightAnchor.constraint(equalToConstant: 50),
            
            sneakButton.widthAnchor.constraint(equalToConstant: 50),
            sneakButton.heightAnchor.constraint(equalToConstant: 50),
            
            inventoryButton.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -20),
            inventoryButton.topAnchor.constraint(equalTo: safeAreaLayoutGuide.topAnchor, constant: 20),
            inventoryButton.widthAnchor.constraint(equalToConstant: 50),
            inventoryButton.heightAnchor.constraint(equalToConstant: 50)
        ])
    }
    
    private func setupToggleGesture() {
        let tripleTapGesture = UITapGestureRecognizer(target: self, action: #selector(toggleControls))
        tripleTapGesture.numberOfTapsRequired = 3
        addGestureRecognizer(tripleTapGesture)
    }
    
    private func setupBindings() {
        movementJoystick.$vector
            .receive(on: DispatchQueue.main)
            .sink { [weak self] vector in
                self?.movementVector = vector
            }
            .store(in: &cancellables)
        
        lookJoystick.$vector
            .receive(on: DispatchQueue.main)
            .sink { [weak self] vector in
                self?.lookVector = vector
            }
            .store(in: &cancellables)
    }
    
    @objc private func toggleControls() {
        isControlsVisible.toggle()
        
        UIView.animate(withDuration: 0.3) {
            self.movementJoystick.alpha = self.isControlsVisible ? 1.0 : 0.2
            self.lookJoystick.alpha = self.isControlsVisible ? 1.0 : 0.2
            self.jumpButton.alpha = self.isControlsVisible ? 1.0 : 0.2
            self.attackButton.alpha = self.isControlsVisible ? 1.0 : 0.2
            self.interactButton.alpha = self.isControlsVisible ? 1.0 : 0.2
            self.sprintButton.alpha = self.isControlsVisible ? 1.0 : 0.2
            self.sneakButton.alpha = self.isControlsVisible ? 1.0 : 0.2
            self.inventoryButton.alpha = self.isControlsVisible ? 1.0 : 0.2
        }
    }
    
    func resetControls() {
        movementVector = .zero
        lookVector = .zero
        jumpPressed = false
        attackPressed = false
        interactPressed = false
        sprintPressed = false
        sneakPressed = false
        
        movementJoystick.reset()
        lookJoystick.reset()
    }
}

// MARK: - Virtual Joystick

class VirtualJoystick: UIView {
    
    @Published var vector: CGPoint = .zero
    weak var delegate: VirtualJoystickDelegate?
    
    private let backgroundView = UIView()
    private let handleView = UIView()
    private let centerPoint = CGPoint()
    
    private var isDragging: Bool = false
    private var maxRadius: CGFloat = 50
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setupJoystick()
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupJoystick()
    }
    
    private func setupJoystick() {
        backgroundColor = .clear
        
        // Background
        backgroundView.backgroundColor = UIColor.white.withAlphaComponent(0.3)
        backgroundView.layer.cornerRadius = frame.width / 2
        backgroundView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(backgroundView)
        
        // Handle
        handleView.backgroundColor = UIColor.white.withAlphaComponent(0.8)
        handleView.layer.cornerRadius = 20
        handleView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(handleView)
        
        NSLayoutConstraint.activate([
            backgroundView.centerXAnchor.constraint(equalTo: centerXAnchor),
            backgroundView.centerYAnchor.constraint(equalTo: centerYAnchor),
            backgroundView.widthAnchor.constraint(equalToConstant: 100),
            backgroundView.heightAnchor.constraint(equalToConstant: 100),
            
            handleView.centerXAnchor.constraint(equalTo: centerXAnchor),
            handleView.centerYAnchor.constraint(equalTo: centerYAnchor),
            handleView.widthAnchor.constraint(equalToConstant: 40),
            handleView.heightAnchor.constraint(equalToConstant: 40)
        ])
        
        maxRadius = 50
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let touch = touches.first else { return }
        let location = touch.location(in: self)
        
        if backgroundView.frame.contains(location) {
            isDragging = true
            updateJoystick(position: location)
        }
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard isDragging, let touch = touches.first else { return }
        let location = touch.location(in: self)
        updateJoystick(position: location)
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        isDragging = false
        reset()
    }
    
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        isDragging = false
        reset()
    }
    
    private func updateJoystick(position: CGPoint) {
        let center = CGPoint(x: bounds.midX, y: bounds.midY)
        let distance = sqrt(pow(position.x - center.x, 2) + pow(position.y - center.y, 2))
        let angle = atan2(position.y - center.y, position.x - center.x)
        
        let clampedDistance = min(distance, maxRadius)
        let clampedX = center.x + cos(angle) * clampedDistance
        let clampedY = center.y + sin(angle) * clampedDistance
        
        handleView.center = CGPoint(x: clampedX, y: clampedY)
        
        // Update vector (normalized)
        vector = CGPoint(
            x: (clampedX - center.x) / maxRadius,
            y: (clampedY - center.y) / maxRadius
        )
        
        delegate?.joystickDidMove(self, vector: vector)
    }
    
    func reset() {
        UIView.animate(withDuration: 0.2) {
            self.handleView.center = CGPoint(x: self.bounds.midX, y: self.bounds.midY)
        }
        
        vector = .zero
        delegate?.joystickDidMove(self, vector: vector)
    }
}

// MARK: - Virtual Button

class VirtualButton: UIView {
    
    weak var delegate: VirtualButtonDelegate?
    private let titleLabel = UILabel()
    private let backgroundView = UIView()
    
    var isPressed: Bool = false {
        didSet {
            updateAppearance()
        }
    }
    
    init(title: String, color: UIColor) {
        super.init(frame: .zero)
        setupButton(title: title, color: color)
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    private func setupButton(title: String, color: UIColor) {
        backgroundView.backgroundColor = color.withAlphaComponent(0.7)
        backgroundView.layer.cornerRadius = 25
        backgroundView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(backgroundView)
        
        titleLabel.text = title
        titleLabel.font = .systemFont(ofSize: 20, weight: .bold)
        titleLabel.textColor = .white
        titleLabel.textAlignment = .center
        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        addSubview(titleLabel)
        
        NSLayoutConstraint.activate([
            backgroundView.leadingAnchor.constraint(equalTo: leadingAnchor),
            backgroundView.trailingAnchor.constraint(equalTo: trailingAnchor),
            backgroundView.topAnchor.constraint(equalTo: topAnchor),
            backgroundView.bottomAnchor.constraint(equalTo: bottomAnchor),
            
            titleLabel.centerXAnchor.constraint(equalTo: centerXAnchor),
            titleLabel.centerYAnchor.constraint(equalTo: centerYAnchor)
        ])
        
        isUserInteractionEnabled = true
    }
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        isPressed = true
        delegate?.buttonPressed(self)
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        isPressed = false
        delegate?.buttonReleased(self)
    }
    
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        isPressed = false
        delegate?.buttonReleased(self)
    }
    
    private func updateAppearance() {
        UIView.animate(withDuration: 0.1) {
            if self.isPressed {
                self.backgroundView.transform = CGAffineTransform(scaleX: 0.9, y: 0.9)
                self.backgroundView.alpha = 1.0
            } else {
                self.backgroundView.transform = .identity
                self.backgroundView.alpha = 0.7
            }
        }
    }
}

// MARK: - Delegates

protocol VirtualJoystickDelegate: AnyObject {
    func joystickDidMove(_ joystick: VirtualJoystick, vector: CGPoint)
}

protocol VirtualButtonDelegate: AnyObject {
    func buttonPressed(_ button: VirtualButton)
    func buttonReleased(_ button: VirtualButton)
}

// MARK: - TouchControlsView Delegate Implementation

extension TouchControlsView: VirtualJoystickDelegate {
    func joystickDidMove(_ joystick: VirtualJoystick, vector: CGPoint) {
        // Movement and look vectors are already bound via Combine
    }
}

extension TouchControlsView: VirtualButtonDelegate {
    func buttonPressed(_ button: VirtualButton) {
        switch button {
        case jumpButton:
            jumpPressed = true
        case attackButton:
            attackPressed = true
        case interactButton:
            interactPressed = true
        case sprintButton:
            sprintPressed = true
        case sneakButton:
            sneakPressed = true
        case inventoryButton:
            // Handle inventory opening through delegate
            break
        default:
            break
        }
    }
    
    func buttonReleased(_ button: VirtualButton) {
        switch button {
        case jumpButton:
            jumpPressed = false
        case attackButton:
            attackPressed = false
        case interactButton:
            interactPressed = false
        case sprintButton:
            sprintPressed = false
        case sneakButton:
            sneakPressed = false
        case inventoryButton:
            // Handle inventory opening through delegate
            break
        default:
            break
        }
    }
}
