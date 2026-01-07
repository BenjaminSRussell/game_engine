import UIKit
import Combine

class GameHUD: UIView, ThemeObserver {
    
    @Published var health: Float = 20.0
    @Published var hunger: Float = 20.0
    @Published var armor: Float = 0.0
    @Published var experience: Float = 0.0
    @Published var level: Int = 1
    @Published var selectedSlot: Int = 0
    @Published var hotbarItems: [HotbarItem] = []
    @Published var oxygen: Float = 20.0
    
    private var cancellables = Set<AnyCancellable>()
    private let assetManager = AssetManager.shared
    private let themeManager = ThemeManager.shared
    
    // Health and Hunger Bars
    private lazy var healthBar = ProgressBar(color: themeManager.currentTheme.healthColor, maxValue: 20)
    private lazy var hungerBar = ProgressBar(color: themeManager.currentTheme.hungerColor, maxValue: 20)
    private lazy var armorBar = ProgressBar(color: themeManager.currentTheme.armorColor, maxValue: 20)
    private lazy var oxygenBar = ProgressBar(color: themeManager.currentTheme.armorColor.withAlphaComponent(0.8), maxValue: 20)
    
    // Experience Bar
    private lazy var experienceBar = ProgressBar(color: themeManager.currentTheme.experienceColor, maxValue: 100)
    private let levelLabel = UILabel()
    
    // Hotbar
    private let hotbarStackView = UIStackView()
    private var hotbarSlots: [HotbarSlot] = []
    
    // Crosshair
    private let crosshairView = CrosshairView()
    
    // Debug info
    private let debugLabel = UILabel()
    private var showDebugInfo: Bool = false
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setupHUD()
        setupBindings()
        themeManager.addObserver(self)
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupHUD()
        setupBindings()
        themeManager.addObserver(self)
    }
    
    deinit {
        themeManager.removeObserver(self)
    }
    
    private func setupHUD() {
        backgroundColor = .clear
        isUserInteractionEnabled = false
        
        setupStatusBars()
        setupExperienceBar()
        setupHotbar()
        setupCrosshair()
        setupDebugInfo()
    }
    
    private func setupStatusBars() {
        let statusStackView = UIStackView()
        statusStackView.axis = .vertical
        statusStackView.spacing = 2
        statusStackView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(statusStackView)
        
        // Health bar with heart icon
        let healthContainer = createStatusBarContainer(iconName: "health", bar: healthBar)
        statusStackView.addArrangedSubview(healthContainer)
        
        // Hunger bar with food icon
        let hungerContainer = createStatusBarContainer(iconName: "hunger", bar: hungerBar)
        statusStackView.addArrangedSubview(hungerContainer)
        
        // Armor bar with shield icon
        let armorContainer = createStatusBarContainer(iconName: "armor", bar: armorBar)
        statusStackView.addArrangedSubview(armorContainer)
        
        // Oxygen bar (hidden by default)
        let oxygenContainer = createStatusBarContainer(iconName: "oxygen", bar: oxygenBar)
        oxygenContainer.isHidden = true
        statusStackView.addArrangedSubview(oxygenContainer)
        
        NSLayoutConstraint.activate([
            statusStackView.leadingAnchor.constraint(equalTo: leadingAnchor, constant: 16),
            statusStackView.topAnchor.constraint(equalTo: safeAreaLayoutGuide.topAnchor, constant: 16),
            statusStackView.widthAnchor.constraint(equalToConstant: 120)
        ])
    }
    
    private func createStatusBarContainer(iconName: String, bar: ProgressBar) -> UIView {
        let container = UIView()
        container.translatesAutoresizingMaskIntoConstraints = false
        
        let iconImageView = UIImageView()
        iconImageView.image = assetManager.loadIcon(named: "icon_status_\(iconName)")
        iconImageView.contentMode = .scaleAspectFit
        iconImageView.translatesAutoresizingMaskIntoConstraints = false
        container.addSubview(iconImageView)
        
        bar.translatesAutoresizingMaskIntoConstraints = false
        container.addSubview(bar)
        
        NSLayoutConstraint.activate([
            iconImageView.leadingAnchor.constraint(equalTo: container.leadingAnchor),
            iconImageView.centerYAnchor.constraint(equalTo: container.centerYAnchor),
            iconImageView.widthAnchor.constraint(equalToConstant: 16),
            iconImageView.heightAnchor.constraint(equalToConstant: 16),
            
            bar.leadingAnchor.constraint(equalTo: iconImageView.trailingAnchor, constant: 4),
            bar.trailingAnchor.constraint(equalTo: container.trailingAnchor),
            bar.centerYAnchor.constraint(equalTo: container.centerYAnchor),
            bar.heightAnchor.constraint(equalToConstant: 8)
        ])
        
        return container
    }
    
    private func setupExperienceBar() {
        let expContainer = UIView()
        expContainer.translatesAutoresizingMaskIntoConstraints = false
        addSubview(expContainer)
        
        levelLabel.text = "Lv \(level)"
        levelLabel.font = .systemFont(ofSize: 14, weight: .semibold)
        levelLabel.textColor = .white
        levelLabel.translatesAutoresizingMaskIntoConstraints = false
        expContainer.addSubview(levelLabel)
        
        experienceBar.translatesAutoresizingMaskIntoConstraints = false
        expContainer.addSubview(experienceBar)
        
        NSLayoutConstraint.activate([
            expContainer.leadingAnchor.constraint(equalTo: leadingAnchor, constant: 16),
            expContainer.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -16),
            expContainer.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor, constant: -60),
            expContainer.heightAnchor.constraint(equalToConstant: 20),
            
            levelLabel.leadingAnchor.constraint(equalTo: expContainer.leadingAnchor),
            levelLabel.centerYAnchor.constraint(equalTo: expContainer.centerYAnchor),
            levelLabel.widthAnchor.constraint(equalToConstant: 40),
            
            experienceBar.leadingAnchor.constraint(equalTo: levelLabel.trailingAnchor, constant: 8),
            experienceBar.trailingAnchor.constraint(equalTo: expContainer.trailingAnchor),
            experienceBar.centerYAnchor.constraint(equalTo: expContainer.centerYAnchor),
            experienceBar.heightAnchor.constraint(equalToConstant: 6)
        ])
    }
    
    private func setupHotbar() {
        hotbarStackView.axis = .horizontal
        hotbarStackView.spacing = 2
        hotbarStackView.distribution = .fillEqually
        hotbarStackView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(hotbarStackView)
        
        // Create 10 hotbar slots
        for i in 0..<10 {
            let slot = HotbarSlot(slotNumber: i)
            hotbarSlots.append(slot)
            hotbarStackView.addArrangedSubview(slot)
        }
        
        NSLayoutConstraint.activate([
            hotbarStackView.centerXAnchor.constraint(equalTo: centerXAnchor),
            hotbarStackView.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor, constant: -16),
            hotbarStackView.widthAnchor.constraint(equalToConstant: 400),
            hotbarStackView.heightAnchor.constraint(equalToConstant: 40)
        ])
    }
    
    private func setupCrosshair() {
        crosshairView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(crosshairView)
        
        NSLayoutConstraint.activate([
            crosshairView.centerXAnchor.constraint(equalTo: centerXAnchor),
            crosshairView.centerYAnchor.constraint(equalTo: centerYAnchor),
            crosshairView.widthAnchor.constraint(equalToConstant: 20),
            crosshairView.heightAnchor.constraint(equalToConstant: 20)
        ])
    }
    
    private func setupDebugInfo() {
        debugLabel.font = .monospacedSystemFont(ofSize: 12, weight: .regular)
        debugLabel.textColor = .white
        debugLabel.backgroundColor = UIColor.black.withAlphaComponent(0.7)
        debugLabel.numberOfLines = 0
        debugLabel.translatesAutoresizingMaskIntoConstraints = false
        debugLabel.isHidden = true
        addSubview(debugLabel)
        
        NSLayoutConstraint.activate([
            debugLabel.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -16),
            debugLabel.topAnchor.constraint(equalTo: safeAreaLayoutGuide.topAnchor, constant: 16),
            debugLabel.widthAnchor.constraint(equalToConstant: 200)
        ])
    }
    
    private func setupBindings() {
        $health
            .receive(on: DispatchQueue.main)
            .sink { [weak self] health in
                self?.healthBar.setValue(health)
            }
            .store(in: &cancellables)
        
        $hunger
            .receive(on: DispatchQueue.main)
            .sink { [weak self] hunger in
                self?.hungerBar.setValue(hunger)
            }
            .store(in: &cancellables)
        
        $armor
            .receive(on: DispatchQueue.main)
            .sink { [weak self] armor in
                self?.armorBar.setValue(armor)
            }
            .store(in: &cancellables)
        
        $experience
            .receive(on: DispatchQueue.main)
            .sink { [weak self] experience in
                self?.experienceBar.setValue(experience)
            }
            .store(in: &cancellables)
        
        $level
            .receive(on: DispatchQueue.main)
            .sink { [weak self] level in
                self?.levelLabel.text = "Lv \(level)"
            }
            .store(in: &cancellables)
        
        $selectedSlot
            .receive(on: DispatchQueue.main)
            .sink { [weak self] selectedSlot in
                self?.updateSelectedSlot(selectedSlot)
            }
            .store(in: &cancellables)
        
        $hotbarItems
            .receive(on: DispatchQueue.main)
            .sink { [weak self] items in
                self?.updateHotbarItems(items)
            }
            .store(in: &cancellables)
    }
    
    private func updateSelectedSlot(_ selected: Int) {
        for (index, slot) in hotbarSlots.enumerated() {
            slot.isSelected = (index == selected)
        }
    }
    
    private func updateHotbarItems(_ items: [HotbarItem]) {
        for (index, item) in items.enumerated() {
            if index < hotbarSlots.count {
                hotbarSlots[index].item = item
            }
        }
    }
    
    func updateDebugInfo(position: (x: Float, y: Float, z: Float), fps: Int, chunks: Int) {
        if showDebugInfo {
            let debugText = """
            Position: \(Int(position.x)), \(Int(position.y)), \(Int(position.z))
            FPS: \(fps)
            Chunks: \(chunks)
            Health: \(Int(health))/20
            Hunger: \(Int(hunger))/20
            """
            debugLabel.text = debugText
        }
    }
    
    func toggleDebugInfo() {
        showDebugInfo.toggle()
        debugLabel.isHidden = !showDebugInfo
    }
    
    func showOxygenBar(_ show: Bool) {
        oxygenBar.superview?.isHidden = !show
    }
    
    // MARK: - ThemeObserver
    
    func themeDidChange(_ theme: Theme, animated: Bool) {
        let duration = animated ? 0.3 : 0.0
        
        UIView.animate(withDuration: duration) { [weak self] in
            guard let self = self else { return }
            
            // Update bar colors
            self.healthBar.updateColor(theme.healthColor)
            self.hungerBar.updateColor(theme.hungerColor)
            self.armorBar.updateColor(theme.armorColor)
            self.oxygenBar.updateColor(theme.armorColor.withAlphaComponent(0.8))
            self.experienceBar.updateColor(theme.experienceColor)
            
            // Update text colors
            self.levelLabel.textColor = theme.textColor
            self.debugLabel.textColor = theme.textColor
        }
        
        // Reload icons with new theme
        // Icons will be reloaded automatically on next access due to cache clear
    }
}

struct HotbarItem {
    let id: UUID
    let name: String
    let count: Int
    let iconName: String? // Asset name instead of emoji
    let durability: Float?
    
    init(name: String, count: Int = 1, iconName: String? = nil, durability: Float? = nil) {
        self.id = UUID()
        self.name = name
        self.count = count
        self.iconName = iconName
        self.durability = durability
    }
    
    // Helper to get icon image
    func getIcon() -> UIImage? {
        if let iconName = iconName {
            return AssetManager.shared.loadIcon(named: iconName)
        }
        return AssetManager.shared.iconForItem(name)
    }
}

class ProgressBar: UIView {
    
    private let maxValue: Float
    private let backgroundColor: UIColor
    private let foregroundColor: UIColor
    
    private let backgroundView = UIView()
    private let foregroundView = UIView()
    
    init(color: UIColor, maxValue: Float) {
        self.maxValue = maxValue
        self.backgroundColor = UIColor.darkGray.withAlphaComponent(0.7)
        self.foregroundColor = color
        super.init(frame: .zero)
        setupBar()
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    private func setupBar() {
        backgroundView.backgroundColor = backgroundColor
        backgroundView.layer.cornerRadius = 2
        backgroundView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(backgroundView)
        
        foregroundView.backgroundColor = foregroundColor
        foregroundView.layer.cornerRadius = 2
        foregroundView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(foregroundView)
        
        NSLayoutConstraint.activate([
            backgroundView.leadingAnchor.constraint(equalTo: leadingAnchor),
            backgroundView.trailingAnchor.constraint(equalTo: trailingAnchor),
            backgroundView.topAnchor.constraint(equalTo: topAnchor),
            backgroundView.bottomAnchor.constraint(equalTo: bottomAnchor),
            
            foregroundView.leadingAnchor.constraint(equalTo: leadingAnchor),
            foregroundView.topAnchor.constraint(equalTo: topAnchor),
            foregroundView.bottomAnchor.constraint(equalTo: bottomAnchor),
            foregroundView.widthAnchor.constraint(equalTo: widthAnchor, multiplier: 0)
        ])
    }
    
    func setValue(_ value: Float) {
        let percentage = min(max(value / maxValue, 0), 1)
        
        UIView.animate(withDuration: 0.2) {
            self.foregroundView.transform = CGAffineTransform(scaleX: CGFloat(percentage), y: 1)
        }
    }
    
    func updateColor(_ color: UIColor) {
        foregroundView.backgroundColor = color
    }
}

class HotbarSlot: UIView {
    
    let slotNumber: Int
    var item: HotbarItem? {
        didSet {
            updateDisplay()
        }
    }
    
    var isSelected: Bool = false {
        didSet {
            updateSelection()
        }
    }
    
    private let backgroundView = UIView()
    private let itemImageView = UIImageView()
    private let countLabel = UILabel()
    private let slotNumberLabel = UILabel()
    private let durabilityBar = ProgressBar(color: .systemGreen, maxValue: 100)
    
    init(slotNumber: Int) {
        self.slotNumber = slotNumber
        super.init(frame: .zero)
        setupSlot()
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    private func setupSlot() {
        backgroundView.backgroundColor = UIColor.black.withAlphaComponent(0.6)
        backgroundView.layer.borderWidth = 2
        backgroundView.layer.borderColor = UIColor.white.withAlphaComponent(0.3).cgColor
        backgroundView.layer.cornerRadius = 4
        backgroundView.translatesAutoresizingMaskIntoConstraints = false
        addSubview(backgroundView)
        
        itemImageView.contentMode = .scaleAspectFit
        itemImageView.translatesAutoresizingMaskIntoConstraints = false
        backgroundView.addSubview(itemImageView)
        
        countLabel.font = .systemFont(ofSize: 10, weight: .bold)
        countLabel.textColor = .white
        countLabel.textAlignment = .right
        countLabel.translatesAutoresizingMaskIntoConstraints = false
        backgroundView.addSubview(countLabel)
        
        slotNumberLabel.text = "\(slotNumber + 1)"
        slotNumberLabel.font = .systemFont(ofSize: 8, weight: .bold)
        slotNumberLabel.textColor = .white
        slotNumberLabel.textAlignment = .center
        slotNumberLabel.translatesAutoresizingMaskIntoConstraints = false
        backgroundView.addSubview(slotNumberLabel)
        
        durabilityBar.translatesAutoresizingMaskIntoConstraints = false
        backgroundView.addSubview(durabilityBar)
        durabilityBar.isHidden = true
        
        NSLayoutConstraint.activate([
            backgroundView.leadingAnchor.constraint(equalTo: leadingAnchor),
            backgroundView.trailingAnchor.constraint(equalTo: trailingAnchor),
            backgroundView.topAnchor.constraint(equalTo: topAnchor),
            backgroundView.bottomAnchor.constraint(equalTo: bottomAnchor),
            
            itemImageView.centerXAnchor.constraint(equalTo: backgroundView.centerXAnchor),
            itemImageView.centerYAnchor.constraint(equalTo: backgroundView.centerYAnchor),
            itemImageView.widthAnchor.constraint(equalToConstant: 24),
            itemImageView.heightAnchor.constraint(equalToConstant: 24),
            
            countLabel.bottomAnchor.constraint(equalTo: backgroundView.bottomAnchor, constant: -2),
            countLabel.trailingAnchor.constraint(equalTo: backgroundView.trailingAnchor, constant: -2),
            
            slotNumberLabel.topAnchor.constraint(equalTo: backgroundView.topAnchor, constant: 2),
            slotNumberLabel.leadingAnchor.constraint(equalTo: backgroundView.leadingAnchor, constant: 2),
            
            durabilityBar.leadingAnchor.constraint(equalTo: backgroundView.leadingAnchor, constant: 2),
            durabilityBar.trailingAnchor.constraint(equalTo: backgroundView.trailingAnchor, constant: -2),
            durabilityBar.bottomAnchor.constraint(equalTo: backgroundView.bottomAnchor, constant: -2),
            durabilityBar.heightAnchor.constraint(equalToConstant: 2)
        ])
    }
    
    private func updateDisplay() {
        if let item = item {
            itemImageView.image = item.getIcon()
            countLabel.text = item.count > 1 ? "\(item.count)" : ""
            
            if let durability = item.durability {
                durabilityBar.isHidden = false
                durabilityBar.setValue(durability)
            } else {
                durabilityBar.isHidden = true
            }
        } else {
            itemImageView.image = nil
            countLabel.text = ""
            durabilityBar.isHidden = true
        }
    }
    
    private func updateSelection() {
        if isSelected {
            backgroundView.layer.borderColor = UIColor.white.cgColor
            backgroundView.layer.borderWidth = 3
        } else {
            backgroundView.layer.borderColor = UIColor.white.withAlphaComponent(0.3).cgColor
            backgroundView.layer.borderWidth = 2
        }
    }
}

class CrosshairView: UIView {
    
    override func draw(_ rect: CGRect) {
        guard let context = UIGraphicsGetCurrentContext() else { return }
        
        context.setStrokeColor(UIColor.white.cgColor)
        context.setLineWidth(2)
        
        let center = CGPoint(x: rect.width / 2, y: rect.height / 2)
        let size: CGFloat = 10
        
        // Draw horizontal line
        context.move(to: CGPoint(x: center.x - size, y: center.y))
        context.addLine(to: CGPoint(x: center.x + size, y: center.y))
        
        // Draw vertical line
        context.move(to: CGPoint(x: center.x, y: center.y - size))
        context.addLine(to: CGPoint(x: center.x, y: center.y + size))
        
        context.strokePath()
    }
}
