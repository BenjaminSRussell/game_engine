import UIKit
import Combine

struct CraftingRecipe {
    let id: UUID
    let result: InventoryItem
    let ingredients: [InventoryItem]
    let category: RecipeCategory
    let unlockLevel: Int
    
    init(result: InventoryItem, ingredients: [InventoryItem], category: RecipeCategory = .basic, unlockLevel: Int = 1) {
        self.id = UUID()
        self.result = result
        self.ingredients = ingredients
        self.category = category
        self.unlockLevel = unlockLevel
    }
    
    func canCraft(with inventory: [InventoryItem]) -> Bool {
        for ingredient in ingredients {
            let available = inventory.filter { $0.name == ingredient.name }.reduce(0) { $0 + $1.count }
            if available < ingredient.count {
                return false
            }
        }
        return true
    }
}

enum RecipeCategory: String, CaseIterable {
    case basic = "Basic"
    case tools = "Tools"
    case weapons = "Weapons"
    case armor = "Armor"
    case building = "Building"
    case redstone = "Redstone"
    case food = "Food"
    case decoration = "Decoration"
}

class CraftingViewController: UIViewController {
    
    @Published var availableRecipes: [CraftingRecipe] = []
    @Published var playerInventory: [InventoryItem] = []
    @Published var craftingGrid: [InventoryItem?] = Array(repeating: nil, count: 9)
    @Published var selectedCategory: RecipeCategory = .basic
    @Published var resultItem: InventoryItem?
    
    private var cancellables = Set<AnyCancellable>()
    
    // UI Components
    private let categorySegmentedControl = UISegmentedControl(items: RecipeCategory.allCases.map { $0.rawValue })
    private let recipeCollectionView = UICollectionView(frame: .zero, collectionViewLayout: UICollectionViewFlowLayout())
    private let craftingGridView = UIView()
    private let resultSlot = InventorySlot(slotNumber: 0, slotType: "Result")
    private let craftButton = UIButton(type: .system)
    private let closeButton = UIButton(type: .system)
    
    // Crafting grid slots
    private var gridSlots: [InventorySlot] = []
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        setupUI()
        setupCraftingGrid()
        setupRecipes()
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
        closeButton.setTitle("✕", for: .normal)
        closeButton.setTitleColor(.white, for: .normal)
        closeButton.titleLabel?.font = .systemFont(ofSize: 24, weight: .bold)
        closeButton.backgroundColor = UIColor.red.withAlphaComponent(0.7)
        closeButton.layer.cornerRadius = 16
        closeButton.addTarget(self, action: #selector(closeCrafting), for: .touchUpInside)
        closeButton.translatesAutoresizingMaskIntoConstraints = false
        containerView.addSubview(closeButton)
        
        // Category selector
        categorySegmentedControl.selectedSegmentIndex = 0
        categorySegmentedControl.addTarget(self, action: #selector(categoryChanged), for: .valueChanged)
        categorySegmentedControl.translatesAutoresizingMaskIntoConstraints = false
        containerView.addSubview(categorySegmentedControl)
        
        // Recipe collection view
        let layout = UICollectionViewFlowLayout()
        layout.itemSize = CGSize(width: 80, height: 100)
        layout.minimumInteritemSpacing = 10
        layout.minimumLineSpacing = 10
        layout.sectionInset = UIEdgeInsets(top: 10, left: 10, bottom: 10, right: 10)
        
        recipeCollectionView.collectionViewLayout = layout
        recipeCollectionView.backgroundColor = UIColor.black.withAlphaComponent(0.3)
        recipeCollectionView.layer.cornerRadius = 8
        recipeCollectionView.delegate = self
        recipeCollectionView.dataSource = self
        recipeCollectionView.register(RecipeCell.self, forCellWithReuseIdentifier: "RecipeCell")
        recipeCollectionView.translatesAutoresizingMaskIntoConstraints = false
        containerView.addSubview(recipeCollectionView)
        
        // Crafting area
        let craftingArea = UIView()
        craftingArea.backgroundColor = UIColor.black.withAlphaComponent(0.3)
        craftingArea.layer.cornerRadius = 8
        craftingArea.translatesAutoresizingMaskIntoConstraints = false
        containerView.addSubview(craftingArea)
        
        // Crafting grid
        craftingGridView.translatesAutoresizingMaskIntoConstraints = false
        craftingArea.addSubview(craftingGridView)
        
        // Result slot
        resultSlot.delegate = self
        resultSlot.translatesAutoresizingMaskIntoConstraints = false
        craftingArea.addSubview(resultSlot)
        
        // Craft button
        craftButton.setTitle("Craft", for: .normal)
        craftButton.setTitleColor(.white, for: .normal)
        craftButton.titleLabel?.font = .systemFont(ofSize: 18, weight: .bold)
        craftButton.backgroundColor = UIColor.systemGreen.withAlphaComponent(0.8)
        craftButton.layer.cornerRadius = 8
        craftButton.addTarget(self, action: #selector(craftItem), for: .touchUpInside)
        craftButton.translatesAutoresizingMaskIntoConstraints = false
        craftingArea.addSubview(craftButton)
        
        NSLayoutConstraint.activate([
            containerView.centerXAnchor.constraint(equalTo: view.centerXAnchor),
            containerView.centerYAnchor.constraint(equalTo: view.centerYAnchor),
            containerView.widthAnchor.constraint(equalToConstant: 600),
            containerView.heightAnchor.constraint(equalToConstant: 500),
            
            closeButton.topAnchor.constraint(equalTo: containerView.topAnchor, constant: 8),
            closeButton.trailingAnchor.constraint(equalTo: containerView.trailingAnchor, constant: -8),
            closeButton.widthAnchor.constraint(equalToConstant: 32),
            closeButton.heightAnchor.constraint(equalToConstant: 32),
            
            categorySegmentedControl.topAnchor.constraint(equalTo: containerView.topAnchor, constant: 16),
            categorySegmentedControl.leadingAnchor.constraint(equalTo: containerView.leadingAnchor, constant: 16),
            categorySegmentedControl.trailingAnchor.constraint(equalTo: containerView.trailingAnchor, constant: -16),
            
            recipeCollectionView.topAnchor.constraint(equalTo: categorySegmentedControl.bottomAnchor, constant: 16),
            recipeCollectionView.leadingAnchor.constraint(equalTo: containerView.leadingAnchor, constant: 16),
            recipeCollectionView.widthAnchor.constraint(equalToConstant: 300),
            recipeCollectionView.bottomAnchor.constraint(equalTo: containerView.bottomAnchor, constant: -16),
            
            craftingArea.topAnchor.constraint(equalTo: categorySegmentedControl.bottomAnchor, constant: 16),
            craftingArea.trailingAnchor.constraint(equalTo: containerView.trailingAnchor, constant: -16),
            craftingArea.bottomAnchor.constraint(equalTo: containerView.bottomAnchor, constant: -16),
            craftingArea.leadingAnchor.constraint(equalTo: recipeCollectionView.trailingAnchor, constant: 16),
            
            craftingGridView.centerXAnchor.constraint(equalTo: craftingArea.centerXAnchor, constant: -40),
            craftingGridView.topAnchor.constraint(equalTo: craftingArea.topAnchor, constant: 20),
            craftingGridView.widthAnchor.constraint(equalToConstant: 120),
            craftingGridView.heightAnchor.constraint(equalToConstant: 120),
            
            resultSlot.trailingAnchor.constraint(equalTo: craftingArea.trailingAnchor, constant: -20),
            resultSlot.centerYAnchor.constraint(equalTo: craftingGridView.centerYAnchor),
            resultSlot.widthAnchor.constraint(equalToConstant: 40),
            resultSlot.heightAnchor.constraint(equalToConstant: 40),
            
            craftButton.bottomAnchor.constraint(equalTo: craftingArea.bottomAnchor, constant: -20),
            craftButton.centerXAnchor.constraint(equalTo: craftingGridView.centerXAnchor),
            craftButton.widthAnchor.constraint(equalToConstant: 100),
            craftButton.heightAnchor.constraint(equalToConstant: 40)
        ])
    }
    
    private func setupCraftingGrid() {
        let stackView = UIStackView()
        stackView.axis = .vertical
        stackView.spacing = 4
        stackView.distribution = .fillEqually
        stackView.translatesAutoresizingMaskIntoConstraints = false
        craftingGridView.addSubview(stackView)
        
        for _ in 0..<3 {
            let row = UIStackView()
            row.axis = .horizontal
            row.spacing = 4
            row.distribution = .fillEqually
            
            for i in 0..<3 {
                let slot = InventorySlot(slotNumber: i, slotType: "Crafting")
                slot.delegate = self
                gridSlots.append(slot)
                row.addArrangedSubview(slot)
            }
            stackView.addArrangedSubview(row)
        }
        
        NSLayoutConstraint.activate([
            stackView.leadingAnchor.constraint(equalTo: craftingGridView.leadingAnchor),
            stackView.trailingAnchor.constraint(equalTo: craftingGridView.trailingAnchor),
            stackView.topAnchor.constraint(equalTo: craftingGridView.topAnchor),
            stackView.bottomAnchor.constraint(equalTo: craftingGridView.bottomAnchor)
        ])
    }
    
    private func setupRecipes() {
        availableRecipes = createAllRecipes()
        updateFilteredRecipes()
    }
    
    private func createAllRecipes() -> [CraftingRecipe] {
        return [
            // Basic recipes
            CraftingRecipe(
                result: InventoryItem(name: "Wood Planks", count: 4, icon: "🪵"),
                ingredients: [InventoryItem(name: "Wood", count: 1, icon: "🌲")]
            ),
            CraftingRecipe(
                result: InventoryItem(name: "Sticks", count: 4, icon: "🏏"),
                ingredients: [InventoryItem(name: "Wood Planks", count: 2, icon: "🪵")]
            ),
            CraftingRecipe(
                result: InventoryItem(name: "Torch", count: 4, icon: "🔦"),
                ingredients: [
                    InventoryItem(name: "Coal", count: 1, icon: "⚫"),
                    InventoryItem(name: "Sticks", count: 1, icon: "🏏")
                ]
            ),
            
            // Tools
            CraftingRecipe(
                result: InventoryItem(name: "Wooden Pickaxe", count: 1, icon: "⛏️", durability: 60),
                ingredients: [
                    InventoryItem(name: "Wood Planks", count: 3, icon: "🪵"),
                    InventoryItem(name: "Sticks", count: 2, icon: "🏏")
                ],
                category: .tools
            ),
            CraftingRecipe(
                result: InventoryItem(name: "Stone Pickaxe", count: 1, icon: "⛏️", durability: 132),
                ingredients: [
                    InventoryItem(name: "Cobblestone", count: 3, icon: "🪨"),
                    InventoryItem(name: "Sticks", count: 2, icon: "🏏")
                ],
                category: .tools
            ),
            CraftingRecipe(
                result: InventoryItem(name: "Iron Pickaxe", count: 1, icon: "⛏️", durability: 251),
                ingredients: [
                    InventoryItem(name: "Iron Ingot", count: 3, icon: "🔩"),
                    InventoryItem(name: "Sticks", count: 2, icon: "🏏")
                ],
                category: .tools,
                unlockLevel: 2
            ),
            CraftingRecipe(
                result: InventoryItem(name: "Diamond Pickaxe", count: 1, icon: "⛏️", durability: 1561),
                ingredients: [
                    InventoryItem(name: "Diamond", count: 3, icon: "💎"),
                    InventoryItem(name: "Sticks", count: 2, icon: "🏏")
                ],
                category: .tools,
                unlockLevel: 3
            ),
            
            // Weapons
            CraftingRecipe(
                result: InventoryItem(name: "Wooden Sword", count: 1, icon: "⚔️", durability: 60),
                ingredients: [
                    InventoryItem(name: "Wood Planks", count: 2, icon: "🪵"),
                    InventoryItem(name: "Sticks", count: 1, icon: "🏏")
                ],
                category: .weapons
            ),
            CraftingRecipe(
                result: InventoryItem(name: "Bow", count: 1, icon: "🏹", durability: 384),
                ingredients: [
                    InventoryItem(name: "Sticks", count: 3, icon: "🏏"),
                    InventoryItem(name: "String", count: 3, icon: "🧵")
                ],
                category: .weapons
            ),
            
            // Building
            CraftingRecipe(
                result: InventoryItem(name: "Crafting Table", count: 1, icon: "🔨"),
                ingredients: [InventoryItem(name: "Wood Planks", count: 4, icon: "🪵")],
                category: .building
            ),
            CraftingRecipe(
                result: InventoryItem(name: "Furnace", count: 1, icon: "🔥"),
                ingredients: [
                    InventoryItem(name: "Cobblestone", count: 8, icon: "🪨")
                ],
                category: .building
            ),
            
            // Food
            CraftingRecipe(
                result: InventoryItem(name: "Bread", count: 1, icon: "🍞"),
                ingredients: [
                    InventoryItem(name: "Wheat", count: 3, icon: "🌾")
                ],
                category: .food
            ),
            
            // Armor
            CraftingRecipe(
                result: InventoryItem(name: "Iron Helmet", count: 1, icon: "⛑️", durability: 165),
                ingredients: [
                    InventoryItem(name: "Iron Ingot", count: 5, icon: "🔩")
                ],
                category: .armor,
                unlockLevel: 2
            ),
        ]
    }
    
    private func setupBindings() {
        $selectedCategory
            .receive(on: DispatchQueue.main)
            .sink { [weak self] _ in
                self?.updateFilteredRecipes()
            }
            .store(in: &cancellables)
        
        $craftingGrid
            .receive(on: DispatchQueue.main)
            .sink { [weak self] grid in
                self?.updateCraftingResult()
            }
            .store(in: &cancellables)
    }
    
    @objc private func categoryChanged() {
        selectedCategory = RecipeCategory.allCases[categorySegmentedControl.selectedSegmentIndex]
    }
    
    @objc private func closeCrafting() {
        dismiss(animated: true)
    }
    
    @objc private func craftItem() {
        guard let result = resultItem else { return }
        
        // Check if player has required materials
        if result.canCraft(with: playerInventory) {
            // Remove ingredients from inventory
            for ingredient in result.ingredients {
                removeItemsFromInventory(ingredient)
            }
            
            // Add result to inventory
            addItemsToInventory(result)
            
            // Clear crafting grid
            clearCraftingGrid()
            
            // Update UI
            updateCraftingResult()
            
            // Play success feedback
            playCraftSuccessFeedback()
        } else {
            // Show insufficient materials alert
            showInsufficientMaterialsAlert()
        }
    }
    
    private func updateFilteredRecipes() {
        recipeCollectionView.reloadData()
    }
    
    private func updateCraftingResult() {
        // Check if current grid matches any recipe
        for recipe in availableRecipes {
            if matchesRecipe(recipe) {
                resultItem = recipe.result
                resultSlot.item = recipe.result
                craftButton.isEnabled = recipe.canCraft(with: playerInventory)
                return
            }
        }
        
        // No recipe matches
        resultItem = nil
        resultSlot.item = nil
        craftButton.isEnabled = false
    }
    
    private func matchesRecipe(_ recipe: CraftingRecipe) -> Bool {
        // Simple pattern matching - in a real implementation, this would be more sophisticated
        let gridItems = craftingGrid.compactMap { $0 }
        
        // Check if grid has the right number of ingredients
        if gridItems.count != recipe.ingredients.count {
            return false
        }
        
        // Check if all required ingredients are present (ignoring position for now)
        for ingredient in recipe.ingredients {
            let found = gridItems.contains { $0.name == ingredient.name && $0.count >= ingredient.count }
            if !found {
                return false
            }
        }
        
        return true
    }
    
    private func removeItemsFromInventory(_ item: InventoryItem) {
        // Implementation would remove items from player inventory
        print("Removing \(item.count)x \(item.name) from inventory")
    }
    
    private func addItemsToInventory(_ item: InventoryItem) {
        // Implementation would add items to player inventory
        print("Adding \(item.count)x \(item.name) to inventory")
    }
    
    private func clearCraftingGrid() {
        for i in 0..<craftingGrid.count {
            craftingGrid[i] = nil
            gridSlots[i].item = nil
        }
    }
    
    private func playCraftSuccessFeedback() {
        // Play success sound and haptic feedback
        #if os(iOS)
        let impactFeedback = UIImpactFeedbackGenerator(style: .medium)
        impactFeedback.impactOccurred()
        #endif
    }
    
    private func showInsufficientMaterialsAlert() {
        let alert = UIAlertController(
            title: "Insufficient Materials",
            message: "You don't have the required materials to craft this item.",
            preferredStyle: .alert
        )
        alert.addAction(UIAlertAction(title: "OK", style: .default))
        present(alert, animated: true)
    }
}

// MARK: - Collection View

extension CraftingViewController: UICollectionViewDataSource, UICollectionViewDelegate {
    
    func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        return availableRecipes.filter { $0.category == selectedCategory }.count
    }
    
    func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        let cell = collectionView.dequeueReusableCell(withReuseIdentifier: "RecipeCell", for: indexPath) as! RecipeCell
        
        let filteredRecipes = availableRecipes.filter { $0.category == selectedCategory }
        let recipe = filteredRecipes[indexPath.item]
        cell.configure(with: recipe, canCraft: recipe.canCraft(with: playerInventory))
        
        return cell
    }
    
    func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        let filteredRecipes = availableRecipes.filter { $0.category == selectedCategory }
        let recipe = filteredRecipes[indexPath.item]
        
        // Auto-fill crafting grid with recipe ingredients
        autoFillCraftingGrid(with: recipe)
    }
    
    private func autoFillCraftingGrid(with recipe: CraftingRecipe) {
        clearCraftingGrid()
        
        // Fill grid with recipe ingredients (simplified - just fills sequentially)
        for (index, ingredient) in recipe.ingredients.enumerated() {
            if index < craftingGrid.count {
                craftingGrid[index] = ingredient
                gridSlots[index].item = ingredient
            }
        }
        
        updateCraftingResult()
    }
}

// MARK: - Recipe Cell

class RecipeCell: UICollectionViewCell {
    
    private let itemLabel = UILabel()
    private let countLabel = UILabel()
    private let backgroundView = UIView()
    private let lockOverlay = UIView()
    
    override init(frame: CGRect) {
        super.init(frame: frame)
        setupCell()
    }
    
    required init?(coder: NSCoder) {
        super.init(coder: coder)
        setupCell()
    }
    
    private func setupCell() {
        contentView.backgroundColor = UIColor.black.withAlphaComponent(0.6)
        contentView.layer.borderWidth = 1
        contentView.layer.borderColor = UIColor.white.withAlphaComponent(0.3).cgColor
        contentView.layer.cornerRadius = 8
        
        backgroundView.backgroundColor = UIColor.black.withAlphaComponent(0.4)
        backgroundView.layer.cornerRadius = 6
        backgroundView.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(backgroundView)
        
        itemLabel.font = .systemFont(ofSize: 20)
        itemLabel.textAlignment = .center
        itemLabel.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(itemLabel)
        
        countLabel.font = .systemFont(ofSize: 10, weight: .bold)
        countLabel.textColor = .white
        countLabel.textAlignment = .right
        countLabel.translatesAutoresizingMaskIntoConstraints = false
        contentView.addSubview(countLabel)
        
        lockOverlay.backgroundColor = UIColor.black.withAlphaComponent(0.7)
        lockOverlay.layer.cornerRadius = 6
        lockOverlay.translatesAutoresizingMaskIntoConstraints = false
        lockOverlay.isHidden = true
        contentView.addSubview(lockOverlay)
        
        let lockLabel = UILabel()
        lockLabel.text = "🔒"
        lockLabel.font = .systemFont(ofSize: 16)
        lockLabel.textAlignment = .center
        lockLabel.translatesAutoresizingMaskIntoConstraints = false
        lockOverlay.addSubview(lockLabel)
        
        NSLayoutConstraint.activate([
            backgroundView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 2),
            backgroundView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -2),
            backgroundView.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 2),
            backgroundView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -2),
            
            itemLabel.centerXAnchor.constraint(equalTo: contentView.centerXAnchor),
            itemLabel.centerYAnchor.constraint(equalTo: contentView.centerYAnchor, constant: -5),
            
            countLabel.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -4),
            countLabel.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -4),
            
            lockOverlay.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 2),
            lockOverlay.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -2),
            lockOverlay.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 2),
            lockOverlay.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -2),
            
            lockLabel.centerXAnchor.constraint(equalTo: lockOverlay.centerXAnchor),
            lockLabel.centerYAnchor.constraint(equalTo: lockOverlay.centerYAnchor)
        ])
    }
    
    func configure(with recipe: CraftingRecipe, canCraft: Bool) {
        itemLabel.text = recipe.result.icon ?? recipe.result.name.prefix(2).uppercased()
        countLabel.text = recipe.result.count > 1 ? "\(recipe.result.count)" : ""
        
        if canCraft {
            contentView.layer.borderColor = UIColor.systemGreen.cgColor
            contentView.alpha = 1.0
        } else {
            contentView.layer.borderColor = UIColor.systemRed.cgColor
            contentView.alpha = 0.6
        }
        
        // Show lock for higher level recipes
        lockOverlay.isHidden = true
    }
}

// MARK: - Inventory Slot Delegate

extension CraftingViewController: InventorySlotDelegate {
    func slotWasTapped(_ slot: InventorySlot) {
        // Handle slot taps for crafting grid
        if let index = gridSlots.firstIndex(of: slot) {
            // Toggle item in crafting grid
            if craftingGrid[index] != nil {
                craftingGrid[index] = nil
                slot.item = nil
            }
            updateCraftingResult()
        }
    }
}
