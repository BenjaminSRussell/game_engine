// SettingsViewController.swift
//
// Purpose: This view controller provides comprehensive settings management
// for the Minecraft v2 game, including graphics, audio, controls, and game options.
//
// Role: Manages all user-configurable settings with a modern, organized interface.
// Settings are persisted and applied immediately when changed.
//
// Lifecycle:
// - `viewDidLoad()`: Sets up the segmented control interface and loads current settings
// - Settings changes are saved immediately to UserDefaults
//
// Inputs: User interactions with toggles, sliders, and selection controls
// Outputs: Updated game settings that affect gameplay and presentation
//
import UIKit
import Combine

class SettingsViewController: UIViewController {
    
    @Published var settings = GameSettings()
    private var cancellables = Set<AnyCancellable>()
    
    // UI Components
    private let tableView = UITableView(frame: .zero, style: .insetGrouped)
    private let segmentedControl = UISegmentedControl(items: ["Graphics", "Audio", "Controls", "Game", "Appearance"])
    private let resetButton = UIButton(type: .system)
    
    // Settings sections
    private enum Section: String, CaseIterable {
        case graphics = "Graphics"
        case audio = "Audio"
        case controls = "Controls"
        case game = "Game"
        case appearance = "Appearance"
        
        var rows: [Row] {
            switch self {
            case .graphics:
                return [.renderDistance, .graphicsQuality, .vsync, .fullscreen, .shadows, .particles]
            case .audio:
                return [.masterVolume, .musicVolume, .sfxVolume, .ambientSounds, .hapticFeedback]
            case .controls:
                return [.sensitivity, .invertY, .deadzone, .controllerVibration, .touchControls]
            case .game:
                return [.difficulty, .gamemode, .autosave, .debugInfo, .language]
            case .appearance:
                return [.uiTheme]
            }
        }
    }
    
    private enum Row: String, CaseIterable {
        // Graphics
        case renderDistance = "Render Distance"
        case graphicsQuality = "Graphics Quality"
        case vsync = "VSync"
        case fullscreen = "Fullscreen"
        case shadows = "Shadows"
        case particles = "Particles"
        
        // Audio
        case masterVolume = "Master Volume"
        case musicVolume = "Music Volume"
        case sfxVolume = "SFX Volume"
        case ambientSounds = "Ambient Sounds"
        case hapticFeedback = "Haptic Feedback"
        
        // Controls
        case sensitivity = "Sensitivity"
        case invertY = "Invert Y-Axis"
        case deadzone = "Controller Deadzone"
        case controllerVibration = "Controller Vibration"
        case touchControls = "Touch Controls"
        
        // Game
        case difficulty = "Difficulty"
        case gamemode = "Game Mode"
        case autosave = "Autosave"
        case debugInfo = "Debug Info"
        case language = "Language"
        
        // Appearance
        case uiTheme = "UI Theme"
        
        var type: SettingType {
            switch self {
            case .vsync, .fullscreen, .shadows, .particles, .ambientSounds, .hapticFeedback, .invertY, .controllerVibration, .touchControls, .autosave, .debugInfo:
                return .toggle
            case .graphicsQuality, .difficulty, .gamemode, .language, .uiTheme:
                return .selection
            case .renderDistance, .masterVolume, .musicVolume, .sfxVolume, .sensitivity, .deadzone:
                return .slider
            }
        }
    }
    
    private enum SettingType {
        case toggle
        case slider
        case selection
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        title = "Settings"
        view.backgroundColor = .systemBackground
        
        setupUI()
        setupTableView()
        loadSettings()
    }
    
    private func setupUI() {
        // Segmented control for categories
        segmentedControl.selectedSegmentIndex = 0
        segmentedControl.addTarget(self, action: #selector(segmentChanged), for: .valueChanged)
        segmentedControl.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(segmentedControl)
        
        // Reset button
        resetButton.setTitle("Reset to Defaults", for: .normal)
        resetButton.setTitleColor(.systemRed, for: .normal)
        resetButton.titleLabel?.font = .systemFont(ofSize: 16, weight: .medium)
        resetButton.addTarget(self, action: #selector(resetSettings), for: .touchUpInside)
        resetButton.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(resetButton)
        
        NSLayoutConstraint.activate([
            segmentedControl.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor, constant: 16),
            segmentedControl.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
            segmentedControl.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16),
            
            resetButton.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -16),
            resetButton.centerXAnchor.constraint(equalTo: view.centerXAnchor)
        ])
    }
    
    private func setupTableView() {
        tableView.delegate = self
        tableView.dataSource = self
        tableView.register(SettingsToggleCell.self, forCellReuseIdentifier: "SettingsToggleCell")
        tableView.register(SettingsSliderCell.self, forCellReuseIdentifier: "SettingsSliderCell")
        tableView.register(SettingsSelectionCell.self, forCellReuseIdentifier: "SettingsSelectionCell")
        tableView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(tableView)
        
        NSLayoutConstraint.activate([
            tableView.topAnchor.constraint(equalTo: segmentedControl.bottomAnchor, constant: 16),
            tableView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            tableView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            tableView.bottomAnchor.constraint(equalTo: resetButton.topAnchor, constant: -16)
        ])
    }
    
    private func loadSettings() {
        // Load settings from UserDefaults or game config
        settings = GameSettings.load()
        tableView.reloadData()
    }
    
    private func saveSettings() {
        settings.save()
    }
    
    @objc private func segmentChanged() {
        tableView.reloadData()
    }
    
    @objc private func resetSettings() {
        let alert = UIAlertController(
            title: "Reset Settings",
            message: "Are you sure you want to reset all settings to their default values?",
            preferredStyle: .alert
        )
        
        alert.addAction(UIAlertAction(title: "Cancel", style: .cancel))
        alert.addAction(UIAlertAction(title: "Reset", style: .destructive) { _ in
            self.settings = GameSettings.default
            self.saveSettings()
            self.tableView.reloadData()
        })
        
        present(alert, animated: true)
    }
}

// MARK: - Game Settings Model

struct GameSettings {
    // Graphics
    var renderDistance: Int = 8
    var graphicsQuality: GraphicsQuality = .medium
    var vsync: Bool = true
    var fullscreen: Bool = false
    var shadows: Bool = true
    var particles: Bool = true
    
    // Audio
    var masterVolume: Float = 0.8
    var musicVolume: Float = 0.7
    var sfxVolume: Float = 0.8
    var ambientSounds: Bool = true
    var hapticFeedback: Bool = true
    
    // Controls
    var sensitivity: Float = 1.0
    var invertY: Bool = false
    var deadzone: Float = 0.15
    var controllerVibration: Bool = true
    var touchControls: Bool = true
    
    // Game
    var difficulty: Difficulty = .normal
    var gamemode: Gamemode = .survival
    var autosave: Bool = true
    var debugInfo: Bool = false
    var language: String = "English"
    
    // Appearance
    var uiTheme: String = "medieval"
    
    static let `default` = GameSettings()
    
    func save() {
        let defaults = UserDefaults.standard
        defaults.set(renderDistance, forKey: "renderDistance")
        defaults.set(graphicsQuality.rawValue, forKey: "graphicsQuality")
        defaults.set(vsync, forKey: "vsync")
        defaults.set(fullscreen, forKey: "fullscreen")
        defaults.set(shadows, forKey: "shadows")
        defaults.set(particles, forKey: "particles")
        
        defaults.set(masterVolume, forKey: "masterVolume")
        defaults.set(musicVolume, forKey: "musicVolume")
        defaults.set(sfxVolume, forKey: "sfxVolume")
        defaults.set(ambientSounds, forKey: "ambientSounds")
        defaults.set(hapticFeedback, forKey: "hapticFeedback")
        
        defaults.set(sensitivity, forKey: "sensitivity")
        defaults.set(invertY, forKey: "invertY")
        defaults.set(deadzone, forKey: "deadzone")
        defaults.set(controllerVibration, forKey: "controllerVibration")
        defaults.set(touchControls, forKey: "touchControls")
        
        defaults.set(difficulty.rawValue, forKey: "difficulty")
        defaults.set(gamemode.rawValue, forKey: "gamemode")
        defaults.set(autosave, forKey: "autosave")
        defaults.set(debugInfo, forKey: "debugInfo")
        defaults.set(language, forKey: "language")
        
        defaults.set(uiTheme, forKey: "uiTheme")
    }
    
    static func load() -> GameSettings {
        var settings = GameSettings()
        let defaults = UserDefaults.standard
        
        settings.renderDistance = defaults.integer(forKey: "renderDistance") != 0 ? defaults.integer(forKey: "renderDistance") : 8
        if let qualityRaw = defaults.string(forKey: "graphicsQuality"),
           let quality = GraphicsQuality(rawValue: qualityRaw) {
            settings.graphicsQuality = quality
        }
        settings.vsync = defaults.object(forKey: "vsync") as? Bool ?? true
        settings.fullscreen = defaults.bool(forKey: "fullscreen")
        settings.shadows = defaults.object(forKey: "shadows") as? Bool ?? true
        settings.particles = defaults.object(forKey: "particles") as? Bool ?? true
        
        settings.masterVolume = defaults.object(forKey: "masterVolume") as? Float ?? 0.8
        settings.musicVolume = defaults.object(forKey: "musicVolume") as? Float ?? 0.7
        settings.sfxVolume = defaults.object(forKey: "sfxVolume") as? Float ?? 0.8
        settings.ambientSounds = defaults.object(forKey: "ambientSounds") as? Bool ?? true
        settings.hapticFeedback = defaults.object(forKey: "hapticFeedback") as? Bool ?? true
        
        settings.sensitivity = defaults.object(forKey: "sensitivity") as? Float ?? 1.0
        settings.invertY = defaults.bool(forKey: "invertY")
        settings.deadzone = defaults.object(forKey: "deadzone") as? Float ?? 0.15
        settings.controllerVibration = defaults.object(forKey: "controllerVibration") as? Bool ?? true
        settings.touchControls = defaults.object(forKey: "touchControls") as? Bool ?? true
        
        if let difficultyRaw = defaults.string(forKey: "difficulty"),
           let difficulty = Difficulty(rawValue: difficultyRaw) {
            settings.difficulty = difficulty
        }
        if let gamemodeRaw = defaults.string(forKey: "gamemode"),
           let gamemode = Gamemode(rawValue: gamemodeRaw) {
            settings.gamemode = gamemode
        }
        settings.autosave = defaults.object(forKey: "autosave") as? Bool ?? true
        settings.debugInfo = defaults.bool(forKey: "debugInfo")
        settings.language = defaults.string(forKey: "language") ?? "English"
        
        settings.uiTheme = defaults.string(forKey: "uiTheme") ?? "medieval"
        
        return settings
    }
}

enum GraphicsQuality: String, CaseIterable {
    case low = "Low"
    case medium = "Medium"
    case high = "High"
    case ultra = "Ultra"
}

enum Difficulty: String, CaseIterable {
    case peaceful = "Peaceful"
    case easy = "Easy"
    case normal = "Normal"
    case hard = "Hard"
}

enum Gamemode: String, CaseIterable {
    case survival = "Survival"
    case creative = "Creative"
    case adventure = "Adventure"
    case spectator = "Spectator"
}

// MARK: - Table View

extension SettingsViewController: UITableViewDataSource, UITableViewDelegate {
    
    func numberOfSections(in tableView: UITableView) -> Int {
        return 1
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        let selectedSection = Section.allCases[segmentedControl.selectedSegmentIndex]
        return selectedSection.rows.count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let selectedSection = Section.allCases[segmentedControl.selectedSegmentIndex]
        let row = selectedSection.rows[indexPath.row]
        
        switch row.type {
        case .toggle:
            let cell = tableView.dequeueReusableCell(withIdentifier: "SettingsToggleCell", for: indexPath) as! SettingsToggleCell
            cell.configure(with: row.rawValue, settings: settings)
            cell.delegate = self
            return cell
            
        case .slider:
            let cell = tableView.dequeueReusableCell(withIdentifier: "SettingsSliderCell", for: indexPath) as! SettingsSliderCell
            cell.configure(with: row.rawValue, settings: settings)
            cell.delegate = self
            return cell
            
        case .selection:
            let cell = tableView.dequeueReusableCell(withIdentifier: "SettingsSelectionCell", for: indexPath) as! SettingsSelectionCell
            cell.configure(with: row.rawValue, settings: settings)
            cell.delegate = self
            return cell
        }
    }
    
    func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
        return Section.allCases[segmentedControl.selectedSegmentIndex].rawValue
    }
    
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        let selectedSection = Section.allCases[segmentedControl.selectedSegmentIndex]
        let row = selectedSection.rows[indexPath.row]
        
        switch row.type {
        case .slider:
            return 80
        default:
            return 50
        }
    }
}

// MARK: - Settings Cell Protocols

protocol SettingsCellDelegate: AnyObject {
    func settingChanged(_ setting: String, value: Any)
}

// MARK: - Toggle Cell

class SettingsToggleCell: UITableViewCell {
    
    weak var delegate: SettingsCellDelegate?
    private let titleLabel = UILabel()
    private let toggleSwitch = UISwitch()
    private var currentSetting: String = ""
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        setupCell()
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupCell()
    }
    
    private func setupCell() {
        titleLabel.font = .systemFont(ofSize: 16)
        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(titleLabel)
        
        toggleSwitch.addTarget(self, action: #selector(toggleChanged), for: .valueChanged)
        toggleSwitch.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(toggleSwitch)
        
        NSLayoutConstraint.activate([
            titleLabel.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
            titleLabel.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
            
            toggleSwitch.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),
            toggleSwitch.centerYAnchor.constraint(equalTo: contentView.centerYAnchor)
        ])
    }
    
    func configure(with setting: String, settings: GameSettings) {
        currentSetting = setting
        titleLabel.text = setting
        
        switch setting {
        case "VSync":
            toggleSwitch.isOn = settings.vsync
        case "Fullscreen":
            toggleSwitch.isOn = settings.fullscreen
        case "Shadows":
            toggleSwitch.isOn = settings.shadows
        case "Particles":
            toggleSwitch.isOn = settings.particles
        case "Ambient Sounds":
            toggleSwitch.isOn = settings.ambientSounds
        case "Haptic Feedback":
            toggleSwitch.isOn = settings.hapticFeedback
        case "Invert Y-Axis":
            toggleSwitch.isOn = settings.invertY
        case "Controller Vibration":
            toggleSwitch.isOn = settings.controllerVibration
        case "Touch Controls":
            toggleSwitch.isOn = settings.touchControls
        case "Autosave":
            toggleSwitch.isOn = settings.autosave
        case "Debug Info":
            toggleSwitch.isOn = settings.debugInfo
        default:
            toggleSwitch.isOn = false
        }
    }
    
    @objc private func toggleChanged() {
        delegate?.settingChanged(currentSetting, value: toggleSwitch.isOn)
    }
}

// MARK: - Slider Cell

class SettingsSliderCell: UITableViewCell {
    
    weak var delegate: SettingsCellDelegate?
    private let titleLabel = UILabel()
    private let slider = UISlider()
    private let valueLabel = UILabel()
    private var currentSetting: String = ""
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        setupCell()
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupCell()
    }
    
    private func setupCell() {
        titleLabel.font = .systemFont(ofSize: 16)
        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(titleLabel)
        
        slider.minimumValue = 0
        slider.maximumValue = 1
        slider.addTarget(self, action: #selector(sliderChanged), for: .valueChanged)
        slider.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(slider)
        
        valueLabel.font = .systemFont(ofSize: 14, weight: .medium)
        valueLabel.textColor = .secondaryLabel
        valueLabel.textAlignment = .right
        valueLabel.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(valueLabel)
        
        NSLayoutConstraint.activate([
            titleLabel.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
            titleLabel.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 8),
            
            slider.leadingAnchor.constraint(equalTo: titleLabel.trailingAnchor, constant: 16),
            slider.trailingAnchor.constraint(equalTo: valueLabel.leadingAnchor, constant: -8),
            slider.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
            
            valueLabel.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),
            valueLabel.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
            valueLabel.widthAnchor.constraint(equalToConstant: 40)
        ])
    }
    
    func configure(with setting: String, settings: GameSettings) {
        currentSetting = setting
        titleLabel.text = setting
        
        switch setting {
        case "Render Distance":
            slider.minimumValue = 2
            slider.maximumValue = 32
            slider.value = Float(settings.renderDistance)
            valueLabel.text = "\(settings.renderDistance)"
        case "Master Volume":
            slider.value = settings.masterVolume
            valueLabel.text = "\(Int(settings.masterVolume * 100))%"
        case "Music Volume":
            slider.value = settings.musicVolume
            valueLabel.text = "\(Int(settings.musicVolume * 100))%"
        case "SFX Volume":
            slider.value = settings.sfxVolume
            valueLabel.text = "\(Int(settings.sfxVolume * 100))%"
        case "Sensitivity":
            slider.minimumValue = 0.1
            slider.maximumValue = 3.0
            slider.value = settings.sensitivity
            valueLabel.text = String(format: "%.1f", settings.sensitivity)
        case "Controller Deadzone":
            slider.minimumValue = 0.0
            slider.maximumValue = 0.5
            slider.value = settings.deadzone
            valueLabel.text = String(format: "%.2f", settings.deadzone)
        default:
            slider.value = 0
            valueLabel.text = "0"
        }
    }
    
    @objc private func sliderChanged() {
        let value = slider.value
        
        switch currentSetting {
        case "Render Distance":
            valueLabel.text = "\(Int(value))"
        case "Master Volume", "Music Volume", "SFX Volume":
            valueLabel.text = "\(Int(value * 100))%"
        case "Sensitivity":
            valueLabel.text = String(format: "%.1f", value)
        case "Controller Deadzone":
            valueLabel.text = String(format: "%.2f", value)
        default:
            valueLabel.text = "\(Int(value))"
        }
        
        delegate?.settingChanged(currentSetting, value: value)
    }
}

// MARK: - Selection Cell

class SettingsSelectionCell: UITableViewCell {
    
    weak var delegate: SettingsCellDelegate?
    private let titleLabel = UILabel()
    private let valueLabel = UILabel()
    private var currentSetting: String = ""
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        setupCell()
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupCell()
    }
    
    private func setupCell() {
        titleLabel.font = .systemFont(ofSize: 16)
        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(titleLabel)
        
        valueLabel.font = .systemFont(ofSize: 16)
        valueLabel.textColor = .systemBlue
        valueLabel.textAlignment = .right
        valueLabel.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(valueLabel)
        
        accessoryType = .disclosureIndicator
        
        NSLayoutConstraint.activate([
            titleLabel.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
            titleLabel.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
            
            valueLabel.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16),
            valueLabel.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
            valueLabel.widthAnchor.constraint(equalToConstant: 100)
        ])
    }
    
    func configure(with setting: String, settings: GameSettings) {
        currentSetting = setting
        titleLabel.text = setting
        
        switch setting {
        case "Graphics Quality":
            valueLabel.text = settings.graphicsQuality.rawValue
        case "Difficulty":
            valueLabel.text = settings.difficulty.rawValue
        case "Game Mode":
            valueLabel.text = settings.gamemode.rawValue
        case "Language":
            valueLabel.text = settings.language
        case "UI Theme":
            // Display theme name from ThemeManager
            if let theme = ThemeManager.shared.availableThemes.first(where: { $0.identifier == settings.uiTheme }) {
                valueLabel.text = theme.displayName
            } else {
                valueLabel.text = "Medieval Fantasy"
            }
        default:
            valueLabel.text = "Unknown"
        }
    }
}

// MARK: - Settings Delegate

extension SettingsViewController: SettingsCellDelegate {
    func settingChanged(_ setting: String, value: Any) {
        switch setting {
        case "VSync":
            settings.vsync = value as? Bool ?? false
        case "Fullscreen":
            settings.fullscreen = value as? Bool ?? false
        case "Shadows":
            settings.shadows = value as? Bool ?? false
        case "Particles":
            settings.particles = value as? Bool ?? false
        case "Ambient Sounds":
            settings.ambientSounds = value as? Bool ?? false
        case "Haptic Feedback":
            settings.hapticFeedback = value as? Bool ?? false
        case "Invert Y-Axis":
            settings.invertY = value as? Bool ?? false
        case "Controller Vibration":
            settings.controllerVibration = value as? Bool ?? false
        case "Touch Controls":
            settings.touchControls = value as? Bool ?? false
        case "Autosave":
            settings.autosave = value as? Bool ?? false
        case "Debug Info":
            settings.debugInfo = value as? Bool ?? false
        case "Render Distance":
            settings.renderDistance = Int(value as? Float ?? 8)
        case "Master Volume":
            settings.masterVolume = value as? Float ?? 0.8
        case "Music Volume":
            settings.musicVolume = value as? Float ?? 0.7
        case "SFX Volume":
            settings.sfxVolume = value as? Float ?? 0.8
        case "Sensitivity":
            settings.sensitivity = value as? Float ?? 1.0
        case "Controller Deadzone":
            settings.deadzone = value as? Float ?? 0.15
        case "UI Theme":
            // Handle theme selection - this will be triggered by tapping the cell
            // For now, just save the value. We'll add a picker in the next step
            if let themeId = value as? String {
                settings.uiTheme = themeId
                ThemeManager.shared.applyTheme(withIdentifier: themeId, animated: true)
            }
        default:
            break
        }
        
        saveSettings()
    }
}
