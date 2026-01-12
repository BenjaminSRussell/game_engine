import UIKit
import Combine

class InventoryViewController: UIViewController {
    
    @Published var inventoryItems: [InventoryItem] = []
    @Published var hotbarItems: [InventoryItem] = []
    @Published var armorItems: [InventoryItem?] = [nil, nil, nil, nil] // helmet, chest, legs, boots
    
    private var cancellables = Set<AnyCancellable>()
    private var draggedItem: InventoryItem?
    private var draggedSource: InventorySource?
    private var draggedIndex: Int?
    
    // UI Components
    private let inventoryGridView = UIView()
    private let hotbarView = UIView()
    private let armorView = UIView()
    private let craftingView = UIView()
    private let closeButton = UIButton(type: .system)
    
    // Grid layout
    private var inventorySlots: [InventorySlot] = []
    private var hotbarSlots: [InventorySlot] = []
    private var armorSlots: [InventorySlot] = []
    
    enum InventorySource {
        case inventory
        case hotbar
        case armor
        case crafting
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        setupUI()
        setupInventory()
        setupDragAndDrop()
        setupBindings()
    }
    
    private func setupUI() {
        view.backgroundColor = UIColor.black.withAlphaComponent(0.8)
        
        // Main container
        let containerView = UIView()
        containerView.backgroundColor = UIColor.darkGray.withAlphaComponent(0.9)
        containerView.layer.cornerRadius = 12
        containerView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(containerView)
        
        // Close button
        closeButton.setTitle("", for: .normal)
        closeButton.setTitleColor(.white, for: .normal)
        closeButton.titleLabel?.font = .systemFont(ofSize: 24, weight: .bold)
        closeButton.backgroundColor = UIColor.red.withAlphaComponent(0.7)
        closeButton.layer.cornerRadius = 16
        closeButton.addTarget(self, action: #selector(closeInventory), for: .touchUpInside)
        closeButton.translatesAutoresizingMaskIntoConstraints = false
        containerView.addSubview(closeButton)
        
        // Setup sections
        setupInventoryGrid(containerView)
        setupHotbar(containerView)
        setupArmorSection(containerView)
        setupCraftingSection(containerView)
        
        NSLayoutConstraint.activate([
            containerView.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            containerView.centerYAnchor.constraint(equalTo: view.centerYAnchor),
            containerView.widthAnchor.constraint(equalToConstant: 500),
            containerView.heightAnchor.constraint(equalToConstant: 400),
            
            closeButton.topAnchor.constraint(equalTo: containerView.topAnchor, constant: 8),
            closeButton.trailingAnchor.constraint(equalTo: containerView.trailingAnchor, constant: -8),
            closeButton.widthAnchor.constraint(equalToConstant: 32),
            closeButton.heightAnchor.constraint(equalToConstant: 32)
        ])
    }
    
    private func setupInventoryGrid(_ containerView: UIView) {
        inventoryGridView.backgroundColor = UIColor.black.withAlphaComponent(0.3)
        inventoryGridView.layer.cornerRadius = 8
        inventoryGridView.translatesAutoresizingMaskIntoConstraints = false
        containerView.addSubview(inventoryGridView)
        
        let gridLayout = UICollectionViewFlowLayout()
        gridLayout.itemSize = CGSize(width: 40, height: 40)
        gridLayout.minimumInteritemSpacing = 2
        gridLayout.minimumLineSpacing = 2
        gridLayout.sectionInset = UIEdgeInsets(top: 8, left: 8, bottom: 8, right: 8)
        
        let collectionView = UICollectionView(frame: .zero, collectionViewLayout: gridLayout)
        collectionView.backgroundColor = .clear
        collectionView.delegate = self
        collectionView.dataSource = self
        collectionView.register(InventorySlotCell.self, forCellWithReuseIdentifier: "InventorySlotCell")
        collectionView.translatesAutoresizingMaskIntoConstraints = false
        inventoryGridView.addSubview(collectionView)
        
        NSLayoutConstraint.activate([
            inventoryGridView.leadingAnchor.constraint(equalTo: containerView.leadingAnchor, constant: 16),
            inventoryGridView.topAnchor.constraint(equalTo: containerView.topAnchor, constant: 16),
            inventoryGridView.widthAnchor.constraint(equalToConstant: 276),
            inventoryGridView.heightAnchor.constraint(equalToConstant: 276),
            
            collectionView.leadingAnchor.constraint(equalTo: inventoryGridView.leadingAnchor),
            collectionView.trailingAnchor.constraint(equalTo: inventoryGridView.trailingAnchor),
            collectionView.topAnchor.constraint(equalTo: inventoryGridView.topAnchor),
            collectionView.bottomAnchor.constraint(equalTo: inventoryGridView.bottomAnchor)
        ])
    }
    
    private func setupHotbar(_ containerView: UIView) {
        hotbarView.backgroundColor = UIColor.black.withAlphaComponent(0.3)
        hotbarView.layer.cornerRadius = 8
        hotbarView.translatesAutoresizingMaskIntoConstraints = false
        containerView.addSubview(hotbarView)
        
        let stackView = UIStackView()
        stackView.axis = .horizontal
        stackView.spacing = 2
        stackView.distribution = .fillEqually
        stackView.translatesAutoresizingMaskIntoConstraints = false
        hotbarView.addSubview(stackView)
        
        for i in 0..<10 {
            let slot = InventorySlot(slotNumber: i)
            slot.delegate = self
            hotbarSlots.append(slot)
            stackView.addArrangedSubview(slot)
        }
        
        NSLayoutConstraint.activate([
            hotbarView.leadingAnchor.constraint(equalTo: containerView.leadingAnchor, constant: 16),
            hotbarView.bottomAnchor.constraint(equalTo: containerView.bottomAnchor, constant: -16),
            hotbarView.widthAnchor.constraint(equalToConstant: 402),
            hotbarView.heightAnchor.constraint(equalToConstant: 44),
            
            stackView.leadingAnchor.constraint(equalTo: hotbarView.leadingAnchor, constant: 8),
            stackView.trailingAnchor.constraint(equalTo: hotbarView.trailingAnchor, constant: -8),
            stackView.topAnchor.constraint(equalTo: hotbarView.topAnchor, constant: 2),
            stackView.bottomAnchor.constraint(equalTo: hotbarView.bottomAnchor, constant: -2)
        ])
    }
    
    private func setupArmorSection(_ containerView: UIView) {
        armorView.backgroundColor = UIColor.black.withAlphaComponent(0.3)
        armorView.layer.cornerRadius = 8
        armorView.translatesAutoresizingMaskIntoConstraints = false
        containerView.addSubview(armorView)
        
        let stackView = UIStackView()
        stackView.axis = .vertical
        stackView.spacing = 4
        stackView.translatesAutoresizingMaskIntoConstraints = false
        armorView.addSubview(stackView)
        
        let armorTypes = ["Helmet", "Chest", "Legs", "Boots"]
        for (index, type) in armorTypes.enumerated() {
            let slot = InventorySlot(slotNumber: index, slotType: type)
            slot.delegate = self
            armorSlots.append(slot)
            stackView.addArrangedSubview(slot)
        }
        
        NSLayoutConstraint.activate([
            armorView.trailingAnchor.constraint(equalTo: containerView.trailingAnchor, constant: -16),
            armorView.topAnchor.constraint(equalTo: containerView.topAnchor, constant: 16),
            armorView.widthAnchor.constraint(equalToConstant: 44),
            armorView.heightAnchor.constraint(equalToConstant: 180),
            
            stackView.leadingAnchor.constraint(equalTo: armorView.leadingAnchor, constant: 2),
            stackView.trailingAnchor.constraint(equalTo: armorView.trailingAnchor, constant: -2),
            stackView.topAnchor.constraint(equalTo: armorView.topAnchor, constant: 2),
            stackView.bottomAnchor.constraint(equalTo: armorView.bottomAnchor, constant: -2)
        ])
    }
    
    private func setupCraftingSection(_ containerView: UIView) {
        craftingView.backgroundColor = UIColor.black.withAlphaComponent(0.3)
        craftingView.layer.cornerRadius = 8
        craftingView.translatesAutoresizingMaskIntoConstraints = false
        containerView.addSubview(craftingView)
        
        let titleLabel = UILabel()
        titleLabel.text = "Crafting"
        titleLabel.textColor = .white
        titleLabel.font = .systemFont(ofSize: 14, weight: .semibold)
        titleLabel.textAlignment = .center
        titleLabel.translatesAutoresizingMaskIntoConstraints = false
        craftingView.addSubview(titleLabel)
        
        let craftingGrid = UIStackView()
        craftingGrid.axis = .vertical
        craftingGrid.spacing = 2
        craftingGrid.translatesAutoresizingMaskIntoConstraints = false
        craftingView.addSubview(craftingGrid)
        
        // 2x2 crafting grid
        for _ in 0..<2 {
            let row = UIStackView()
            row.axis = .horizontal
            row.spacing = 2
            row.distribution = .fillEqually
            
            for _ in 0..<2 {
                let slot = InventorySlot(slotNumber: 0, slotType: "Crafting")
                slot.delegate = self
                row.addArrangedSubview(slot)
            }
            craftingGrid.addArrangedSubview(row)
        }
        
        // Result slot
        let resultSlot = InventorySlot(slotNumber: 0, slotType: "Result")
        resultSlot.delegate = self
        craftingView.addSubview(resultSlot)
        
        NSLayoutConstraint.activate([
            craftingView.trailingAnchor.constraint(equalTo: containerView.trailingAnchor, constant: -16),
            craftingView.bottomAnchor.constraint(equalTo: containerView.bottomAnchor, constant: -16),
            craftingView.widthAnchor.constraint(equalToConstant: 120),
            craftingView.heightAnchor.constraint(equalToConstant: 120),
            
            titleLabel.topAnchor.constraint(equalTo: craftingView.topAnchor, constant: 4),
            titleLabel.leadingAnchor.constraint(equalTo: craftingView.leadingAnchor, constant: 4),
            titleLabel.trailingAnchor.constraint(equalTo: craftingView.trailingAnchor, constant: -4),
            
            craftingGrid.topAnchor.constraint(equalTo: titleLabel.bottomAnchor, constant: 8),
            craftingGrid.leadingAnchor.constraint(equalTo: craftingView.leadingAnchor, constant: 8),
            craftingGrid.widthAnchor.constraint(equalToConstant: 84),
            craftingGrid.heightAnchor.constraint(equalToConstant: 84),
            
            resultSlot.trailingAnchor.constraint(equalTo: craftingView.trailingAnchor, constant: -8),
            resultSlot.centerYAnchor.constraint(equalTo: craftingGrid.centerYAnchor),
            resultSlot.widthAnchor.constraint(equalToConstant: 28),
            resultSlot.heightAnchor.constraint(equalToConstant: 28)
        ])
    }
    
    private func setupInventory() {
        // Initialize with sample items
        inventoryItems = createSampleInventory()
        hotbarItems = Array(inventoryItems.prefix(10))
        
        // Update slots
        updateInventorySlots()
        updateHotbarSlots()
        updateArmorSlots()
    }
    
    private func createSampleInventory() -> [InventoryItem] {
        return [
            InventoryItem(name: "Diamond Sword", count: 1, icon: "", durability: 850),
            InventoryItem(name: "Diamond Pickaxe", count: 1, icon: "", durability: 1200),
            InventoryItem(name: "Wood", count: 64, icon: ""),
            InventoryItem(name: "Stone", count: 32, icon: ""),
            InventoryItem(name: "Iron Ingot", count: 16, icon: ""),
            InventoryItem(name: "Gold Ingot", count: 8, icon: ""),
            InventoryItem(name: "Diamond", count: 4, icon: ""),
            InventoryItem(name: "Apple", count: 12, icon: ""),
            InventoryItem(name: "Bread", count: 8, icon: ""),
            InventoryItem(name: "Torch", count: 24, icon: ""),
            InventoryItem(name: "Bow", count: 1, icon: "", durability: 300),
            InventoryItem(name: "Arrow", count: 64, icon: ""),
            InventoryItem(name: "Coal", count: 32, icon: ""),
            InventoryItem(name: "Redstone", count: 16, icon: ""),
            InventoryItem(name: "Emerald", count: 3, icon: ""),
            InventoryItem(name: "Leather", count: 8, icon: ""),
            InventoryItem(name: "Feather", count: 12, icon: ""),
            InventoryItem(name: "String", count: 16, icon: ""),
            InventoryItem(name: "Paper", count: 24, icon: ""),
            InventoryItem(name: "Book", count: 3, icon: ""),
            InventoryItem(name: "Compass", count: 1, icon: ""),
            InventoryItem(name: "Clock", count: 1, icon: ""),
            InventoryItem(name: "Fishing Rod", count: 1, icon: "", durability: 64),
            InventoryItem(name: "Flint and Steel", count: 1, icon: "", durability: 64),
            InventoryItem(name: "Bucket", count: 3, icon: ""),
            InventoryItem(name: "Water Bucket", count: 1, icon: ""),
            InventoryItem(name: "Lava Bucket", count: 1, icon: ""),
            InventoryItem(name: "Saddle", count: 1, icon: ""),
            InventoryItem(name: "Name Tag", count: 3, icon: ""),
            InventoryItem(name: "Ender Pearl", count: 8, icon: ""),
            InventoryItem(name: "Blaze Rod", count: 4, icon: ""),
            InventoryItem(name: "Ghast Tear", count: 2, icon: ""),
            InventoryItem(name: "Nether Star", count: 1, icon: "")
        ]
    }
    
    private func updateInventorySlots() {
        // Update collection view cells
        if let collectionView = inventoryGridView.subviews.first as? UICollectionView {
            collectionView.reloadData()
        }
    }
    
    private func updateHotbarSlots() {
        for (index, slot) in hotbarSlots.enumerated() {
            if index < hotbarItems.count {
                slot.item = hotbarItems[index]
            } else {
                slot.item = nil
            }
        }
    }
    
    private func updateArmorSlots() {
        for (index, slot) in armorSlots.enumerated() {
            if index < armorItems.count {
                slot.item = armorItems[index]
            } else {
                slot.item = nil
            }
        }
    }
    
    private func setupDragAndDrop() {
        // Setup long press gesture for drag and drop
        let longPressGesture = UILongPressGestureRecognizer(target: self, action: #selector(handleLongPress(_:)))
        longPressGesture.minimumPressDuration = 0.3
        view.addGestureRecognizer(longPressGesture)
    }
    
    private func setupBindings() {
        // Bind data updates
        $inventoryItems
            .receive(on: DispatchQueue.main)
            .sink { [weak self] _ in
                self?.updateInventorySlots()
            }
            .store(in: &cancellables)
        
        $hotbarItems
            .receive(on: DispatchQueue.main)
            .sink { [weak self] _ in
                self?.updateHotbarSlots()
            }
            .store(in: &cancellables)
        
        $armorItems
            .receive(on: DispatchQueue.main)
            .sink { [weak self] _ in
                self?.updateArmorSlots()
            }
            .store(in: &cancellables)
    }
    
    @objc private func closeInventory() {
        dismiss(animated: true)
    }
    
    @objc private func handleLongPress(_ gesture: UILongPressGestureRecognizer) {
        let location = gesture.location(in: view)
        
        switch gesture.state {
        case .began:
            startDrag(at: location)
        case .changed:
            updateDrag(at: location)
        case .ended, .cancelled:
            endDrag(at: location)
        default:
            break
        }
    }
    
    private func startDrag(at location: CGPoint) {
        // Find which slot was pressed
        if let (source, index) = findSlot(at: location) {
            draggedItem = getItemFromSource(source, index: index)
            draggedSource = source
            draggedIndex = index
            
            if let item = draggedItem {
                createDragView(for: item, at: location)
            }
        }
    }
    
    private func updateDrag(at location: CGPoint) {
        // Update drag view position
        if let dragView = view.viewWithTag(999) {
            dragView.center = location
        }
    }
    
    private func endDrag(at location: CGPoint) {
        // Find target slot
        if let (targetSource, targetIndex) = findSlot(at: location) {
            if let source = draggedSource, let sourceIndex = draggedIndex, let item = draggedItem {
                moveItem(item, from: source, fromIndex: sourceIndex, to: targetSource, toIndex: targetIndex)
            }
        }
        
        // Clean up
        if let dragView = view.viewWithTag(999) {
            dragView.removeFromSuperview()
        }
        draggedItem = nil
        draggedSource = nil
        draggedIndex = nil
    }
    
    private func findSlot(at location: CGPoint) -> (InventorySource, Int)? {
        // Check inventory grid
        if inventoryGridView.frame.contains(location) {
            let relativeLocation = view.convert(location, to: inventoryGridView)
            let x = Int(relativeLocation.x / 42) // 40px slot + 2px spacing
            let y = Int(relativeLocation.y / 42)
            let index = y * 6 + x
            
            if index < 36 {
                return (.inventory, index)
            }
        }
        
        // Check hotbar
        if hotbarView.frame.contains(location) {
            let relativeLocation = view.convert(location, to: hotbarView)
            let x = Int(relativeLocation.x / 42)
            if x < 10 {
                return (.hotbar, x)
            }
        }
        
        // Check armor slots
        if armorView.frame.contains(location) {
            let relativeLocation = view.convert(location, to: armorView)
            let y = Int(relativeLocation.y / 44)
            if y < 4 {
                return (.armor, y)
            }
        }
        
        return nil
    }
    
    private func getItemFromSource(_ source: InventorySource, index: Int) -> InventoryItem? {
        switch source {
        case .inventory:
            return index < inventoryItems.count ? inventoryItems[index] : nil
        case .hotbar:
            return index < hotbarItems.count ? hotbarItems[index] : nil
        case .armor:
            return index < armorItems.count ? armorItems[index] : nil
        case .crafting:
            return nil // Crafting slots don't store items permanently
        }
    }
    
    private func moveItem(_ item: InventoryItem, from source: InventorySource, fromIndex: Int, to targetSource: InventorySource, toIndex: Int) {
        // Remove from source
        removeItemFromSource(source, index: fromIndex)
        
        // Add to target (handle stacking)
        addItemToTarget(item, targetSource: targetSource, index: toIndex)
        
        // Update UI
        updateInventorySlots()
        updateHotbarSlots()
        updateArmorSlots()
    }
    
    private func removeItemFromSource(_ source: InventorySource, index: Int) {
        switch source {
        case .inventory:
            if index < inventoryItems.count {
                inventoryItems.remove(at: index)
            }
        case .hotbar:
            if index < hotbarItems.count {
                hotbarItems.remove(at: index)
            }
        case .armor:
            if index < armorItems.count {
                armorItems[index] = nil
            }
        case .crafting:
            break
        }
    }
    
    private func addItemToTarget(_ item: InventoryItem, targetSource: InventorySource, index: Int) {
        switch targetSource {
        case .inventory:
            if index >= inventoryItems.count {
                inventoryItems.append(item)
            } else {
                // Try to stack with existing item
                if let existingItem = inventoryItems[index], existingItem.name == item.name {
                    existingItem.count += item.count
                } else {
                    inventoryItems.insert(item, at: index)
                }
            }
        case .hotbar:
            if index >= hotbarItems.count {
                hotbarItems.append(item)
            } else {
                if let existingItem = hotbarItems[index], existingItem.name == item.name {
                    existingItem.count += item.count
                } else {
                    hotbarItems.insert(item, at: index)
                }
            }
        case .armor:
            if index < armorItems.count {
                armorItems[index] = item
            }
        case .crafting:
            break // Crafting handled separately
        }
    }
    
    private func createDragView(for item: InventoryItem, at location: CGPoint) {
        let dragView = UIView(frame: CGRect(x: 0, y: 0, width: 40, height: 40))
        dragView.backgroundColor = UIColor.black.withAlphaComponent(0.8)
        dragView.layer.cornerRadius = 4
        dragView.layer.borderWidth = 2
        dragView.layer.borderColor = UIColor.white.cgColor
        dragView.tag = 999
        dragView.center = location
        
        let label = UILabel()
        label.text = item.icon ?? item.name.prefix(2).uppercased()
        label.font = .systemFont(ofSize: 16)
        label.textAlignment = .center
        label.textColor = .white
        label.translatesAutoresizingMaskIntoConstraints = false
        dragView.addSubview(label)
        
        if item.count > 1 {
            let countLabel = UILabel()
            countLabel.text = "\(item.count)"
            countLabel.font = .systemFont(ofSize: 10, weight: .bold)
            countLabel.textColor = .white
            countLabel.textAlignment = .right
            countLabel.translatesAutoresizingMaskIntoConstraints = false
            dragView.addSubview(countLabel)
            
            NSLayoutConstraint.activate([
                countLabel.bottomAnchor.constraint(equalTo: dragView.bottomAnchor, constant: -2),
                countLabel.trailingAnchor.constraint(equalTo: dragView.trailingAnchor, constant: -2)
            ])
        }
        
        NSLayoutConstraint.activate([
            label.centerXAnchor.constraint(equalTo: dragView.centerXAnchor),
            label.centerYAnchor.constraint(equalTo: dragView.centerYAnchor)
        ])
        
        view.addSubview(dragView)
    }
}

// MARK: - Inventory Models

struct InventoryItem {
    let id: UUID
    var name: String
    var count: Int
    var icon: String?
    var durability: Float?
    var maxDurability: Float = 1000
    
    init(name: String, count: Int = 1, icon: String? = nil, durability: Float? = nil) {
        self.id = UUID()
        self.name = name
        self.count = count
        self.icon = icon
        self.durability = durability
    }
}

// MARK: - Collection View

extension InventoryViewController: UICollectionViewDataSource, UICollectionViewDelegate {
    
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return 36 // 6x6 inventory grid
    }
    
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        let cell = collectionView.dequeueReusableCell(withReuseIdentifier: "InventorySlotCell", for: indexPath) as! InventorySlotCell
        
        if indexPath.item < inventoryItems.count {
            cell.configure(with: inventoryItems[indexPath.item])
        } else {
            cell.configure(with: nil)
        }
        
        return cell
    }
}

class InventorySlotCell: UICollectionViewCell {
    
    private let itemLabel = UILabel()
    private let countLabel = UILabel()
    private let durabilityBar = UIProgressView()
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setupCell()
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupCell()
    }
    
    private func setupCell() {
        backgroundColor = UIColor.black.withAlphaComponent(0.6)
        layer.borderWidth = 1
        layer.borderColor = UIColor.white.withAlphaComponent(0.3).cgColor
        layer.cornerRadius = 4
        
        itemLabel.font = .systemFont(ofSize: 16)
        itemLabel.textAlignment = .center
        itemLabel.textColor = .white
        itemLabel.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(itemLabel)
        
        countLabel.font = .systemFont(ofSize: 10, weight: .bold)
        countLabel.textColor = .white
        countLabel.textAlignment = .right
        countLabel.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(countLabel)
        
        durabilityBar.progressTintColor = .systemGreen
        durabilityBar.trackTintColor = .systemRed
        durabilityBar.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(durabilityBar)
        
        NSLayoutConstraint.activate([
            itemLabel.centerXAnchor.constraint(equalTo: contentView.centerXAnchor),
            itemLabel.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
            
            countLabel.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -2),
            countLabel.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -2),
            
            durabilityBar.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 2),
            durabilityBar.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -2),
            durabilityBar.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -2),
            durabilityBar.heightAnchor.constraint(equalToConstant: 2)
        ])
    }
    
    func configure(with item: InventoryItem?) {
        if let item = item {
            itemLabel.text = item.icon ?? item.name.prefix(2).uppercased()
            countLabel.text = item.count > 1 ? "\(item.count)" : ""
            
            if let durability = item.durability {
                durabilityBar.isHidden = false
                durabilityBar.progress = durability / item.maxDurability
            } else {
                durabilityBar.isHidden = true
            }
        } else {
            itemLabel.text = ""
            countLabel.text = ""
            durabilityBar.isHidden = true
        }
    }
}

// MARK: - Inventory Slot

class InventorySlot: UIView {
    
    let slotNumber: Int
    let slotType: String
    weak var delegate: InventorySlotDelegate?
    
    var item: InventoryItem? {
        didSet {
            updateDisplay()
        }
    }
    
    private let itemLabel = UILabel()
    private let countLabel = UILabel()
    private let durabilityBar = UIProgressView()
    private let typeLabel = UILabel()
    
    init(slotNumber: Int, slotType: String = "Normal") {
        self.slotNumber = slotNumber
        self.slotType = slotType
        super.init(frame: CGRect(x: 0, y: 0, width: 40, height: 40))
        setupSlot()
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    private func setupSlot() {
        backgroundColor = UIColor.black.withAlphaComponent(0.6)
        layer.borderWidth = 1
        layer.borderColor = UIColor.white.withAlphaComponent(0.3).cgColor
        layer.cornerRadius = 4
        
        itemLabel.font = .systemFont(ofSize: 16)
        itemLabel.textAlignment = .center
        itemLabel.textColor = .white
        itemLabel.translatesAutoresizingMaskIntoConstraints = false
        addSubview(itemLabel)
        
        countLabel.font = .systemFont(ofSize: 10, weight: .bold)
        countLabel.textColor = .white
        countLabel.textAlignment = .right
        countLabel.translatesAutoresizingMaskIntoConstraints = false
        addSubview(countLabel)
        
        durabilityBar.progressTintColor = .systemGreen
        durabilityBar.trackTintColor = .systemRed
        durabilityBar.translatesAutoresizingMaskIntoConstraints = false
        addSubview(durabilityBar)
        
        if slotType != "Normal" {
            typeLabel.text = slotType.prefix(1).uppercased()
            typeLabel.font = .systemFont(ofSize: 8)
            typeLabel.textColor = .lightGray
            typeLabel.textAlignment = .center
            typeLabel.translatesAutoresizingMaskIntoConstraints = false
            addSubview(typeLabel)
            
            NSLayoutConstraint.activate([
                typeLabel.topAnchor.constraint(equalTo: topAnchor, constant: 2),
                typeLabel.leadingAnchor.constraint(equalTo: leadingAnchor, constant: 2),
                typeLabel.widthAnchor.constraint(equalToConstant: 8),
                typeLabel.heightAnchor.constraint(equalToConstant: 8)
            ])
        }
        
        NSLayoutConstraint.activate([
            itemLabel.centerXAnchor.constraint(equalTo: centerXAnchor),
            itemLabel.centerYAnchor.constraint(equalTo: centerYAnchor),
            
            countLabel.bottomAnchor.constraint(equalTo: bottomAnchor, constant: -2),
            countLabel.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -2),
            
            durabilityBar.leadingAnchor.constraint(equalTo: leadingAnchor, constant: 2),
            durabilityBar.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -2),
            durabilityBar.bottomAnchor.constraint(equalTo: bottomAnchor, constant: -2),
            durabilityBar.heightAnchor.constraint(equalToConstant: 2)
        ])
    }
    
    private func updateDisplay() {
        if let item = item {
            itemLabel.text = item.icon ?? item.name.prefix(2).uppercased()
            countLabel.text = item.count > 1 ? "\(item.count)" : ""
            
            if let durability = item.durability {
                durabilityBar.isHidden = false
                durabilityBar.progress = durability / item.maxDurability
            } else {
                durabilityBar.isHidden = true
            }
        } else {
            itemLabel.text = ""
            countLabel.text = ""
            durabilityBar.isHidden = true
        }
    }
}

protocol InventorySlotDelegate: AnyObject {
    func slotWasTapped(_ slot: InventorySlot)
}
