import Foundation

// MARK: - Fuzzy Search Utility for NodeGraphEditor

struct FuzzySearch {
    
    /// Performs fuzzy string matching using a simplified Levenshtein distance algorithm
    /// - Parameters:
    ///   - pattern: The search pattern (what user typed)
    ///   - text: The text to search against (node type name)
    /// - Returns: A score between 0.0 and 1.0, where 1.0 is a perfect match
    static func fuzzyMatch(_ pattern: String, _ text: String) -> Double {
        let patternLower = pattern.lowercased()
        let textLower = text.lowercased()
        
        // Empty pattern matches everything with low score
        if patternLower.isEmpty {
            return 0.1
        }
        
        // Exact match gets highest score
        if patternLower == textLower {
            return 1.0
        }
        
        // Prefix match gets high score
        if textLower.hasPrefix(patternLower) {
            return 0.9
        }
        
        // Contains match gets medium score
        if textLower.contains(patternLower) {
            return 0.7
        }
        
        // Calculate fuzzy score based on character positions
        return calculateFuzzyScore(pattern: patternLower, text: textLower)
    }
    
    /// Calculates fuzzy score based on character sequence matching
    private static func calculateFuzzyScore(pattern: String, text: String) -> Double {
        let patternChars = Array(pattern)
        let textChars = Array(text)
        
        var patternIndex = 0
        var lastMatchIndex = -1
        var consecutiveMatches = 0
        var totalDistance = 0
        
        for (index, char) in textChars.enumerated() {
            if patternIndex < patternChars.count && char == patternChars[patternIndex] {
                // Found a match
                let distance = index - lastMatchIndex
                totalDistance += distance
                
                // Bonus for consecutive matches
                if index == lastMatchIndex + 1 {
                    consecutiveMatches += 1
                }
                
                lastMatchIndex = index
                patternIndex += 1
            }
        }
        
        // If we didn't match all pattern characters, return low score
        guard patternIndex == patternChars.count else {
            return 0.1
        }
        
        // Calculate score based on:
        // 1. How close matches are to each other (lower distance = higher score)
        // 2. How many consecutive matches there are
        // 3. How much of the text is used (shorter text with full match = higher score)
        
        let distanceScore = Double(textChars.count - totalDistance) / Double(textChars.count)
        let consecutiveBonus = Double(consecutiveMatches) * 0.1
        let lengthEfficiency = Double(patternChars.count) / Double(textChars.count)
        
        let finalScore = (distanceScore + consecutiveBonus + lengthEfficiency) / 3.0
        
        return max(0.1, min(1.0, finalScore))
    }
    
    /// Searches through an array of items and returns sorted results with scores
    /// - Parameters:
    ///   - pattern: Search pattern
    ///   - items: Array of items to search
    ///   - keyPath: Key path to extract searchable string from items
    /// - Returns: Array of tuples containing item and score, sorted by score (highest first)
    static func search<T: Identifiable>(
        _ pattern: String,
        in items: [T],
        keyPath: KeyPath<T, String>
    ) -> [(item: T, score: Double)] {
        let scoredItems = items.map { item in
            let text = item[keyPath: keyPath]
            let score = fuzzyMatch(pattern, text)
            return (item: item, score: score)
        }
        
        return scoredItems
            .filter { $0.score >= 0.2 } // Filter out very low scores
            .sorted { $0.score > $1.score } // Sort by score (highest first)
    }
    
    /// Highlights matched characters in text for UI display
    /// - Parameters:
    ///   - pattern: Search pattern
    ///   - text: Text to highlight
    /// - Returns: Array of tuples containing character and whether it's matched
    static func highlightMatches(in pattern: String, text: String) -> [(character: Character, isMatch: Bool)] {
        let patternLower = pattern.lowercased()
        let textLower = text.lowercased()
        let textChars = Array(text)
        
        var patternIndex = 0
        var result: [(Character, Bool)] = []
        
        for (index, char) in textChars.enumerated() {
            let isMatch = patternIndex < patternLower.count && 
                         index < textLower.count &&
                         textLower[index] == patternLower[patternIndex]
            
            result.append((char, isMatch))
            
            if isMatch {
                patternIndex += 1
            }
        }
        
        return result
    }
    
    /// Suggests corrections for misspelled patterns
    /// - Parameters:
    ///   - pattern: Potentially misspelled pattern
    ///   - candidates: Array of possible correct strings
    ///   - maxSuggestions: Maximum number of suggestions to return
    /// - Returns: Array of suggested corrections with scores
    static func suggestCorrections(
        for pattern: String,
        in candidates: [String],
        maxSuggestions: Int = 5
    ) -> [(suggestion: String, score: Double)] {
        let scored = candidates.map { candidate in
            let score = fuzzyMatch(pattern, candidate)
            return (suggestion: candidate, score: score)
        }
        
        return scored
            .filter { $0.score >= 0.3 } // Only reasonable matches
            .sorted { $0.score > $1.score }
            .prefix(maxSuggestions)
            .map { $0 }
    }
}

// MARK: - Search History Manager

class SearchHistoryManager: ObservableObject {
    static let shared = SearchHistoryManager()
    
    @Published var recentSearches: [String] = []
    @Published var favoriteSearches: [String] = []
    
    private let maxRecentSearches = 10
    private let userDefaults = UserDefaults.standard
    private let recentSearchesKey = "NodeGraphEditor.recentSearches"
    private let favoriteSearchesKey = "NodeGraphEditor.favoriteSearches"
    
    private init() {
        loadSearchHistory()
    }
    
    func addRecentSearch(_ search: String) {
        guard !search.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else { return }
        
        // Remove if already exists
        recentSearches.removeAll { $0.lowercased() == search.lowercased() }
        
        // Add to beginning
        recentSearches.insert(search, at: 0)
        
        // Limit to max size
        if recentSearches.count > maxRecentSearches {
            recentSearches = Array(recentSearches.prefix(maxRecentSearches))
        }
        
        saveSearchHistory()
    }
    
    func addFavoriteSearch(_ search: String) {
        guard !search.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else { return }
        
        if !favoriteSearches.contains(search) {
            favoriteSearches.append(search)
            saveSearchHistory()
        }
    }
    
    func removeFavoriteSearch(_ search: String) {
        favoriteSearches.removeAll { $0 == search }
        saveSearchHistory()
    }
    
    func clearRecentSearches() {
        recentSearches.removeAll()
        saveSearchHistory()
    }
    
    private func loadSearchHistory() {
        if let recentData = userDefaults.stringArray(forKey: recentSearchesKey) {
            recentSearches = recentData
        }
        
        if let favoriteData = userDefaults.stringArray(forKey: favoriteSearchesKey) {
            favoriteSearches = favoriteData
        }
    }
    
    private func saveSearchHistory() {
        userDefaults.set(recentSearches, forKey: recentSearchesKey)
        userDefaults.set(favoriteSearches, forKey: favoriteSearchesKey)
    }
}

// MARK: - Node Type Categories for Better Search Organization

struct NodeTypeCategory {
    let name: String
    let icon: String
    let color: Color
    let description: String
    let nodeTypes: [NodeType]
}

extension NodeTypeCategory {
    static let allCategories: [NodeTypeCategory] = [
        NodeTypeCategory(
            name: "Events",
            icon: "bolt.fill",
            color: .red,
            description: "Event triggers and handlers",
            nodeTypes: [.event, .onStart, .onUpdate, .onDestroy, .customEvent]
        ),
        NodeTypeCategory(
            name: "Flow Control",
            icon: "arrow.branch",
            color: .gray,
            description: "Control flow and logic",
            nodeTypes: [.flowBranch, .flowSequence, .loopFor, .loopForEach, .gate, .latch]
        ),
        NodeTypeCategory(
            name: "Math",
            icon: "plus.forwardslash.minus",
            color: .teal,
            description: "Mathematical operations",
            nodeTypes: [.mathAdd, .mathSubtract, .mathMultiply, .mathDivide, .mathPow, .mathSqrt]
        ),
        NodeTypeCategory(
            name: "Logic",
            icon: "questionmark.diamond",
            color: .purple,
            description: "Logical and comparison operations",
            nodeTypes: [.logicAnd, .logicOr, .logicNot, .compareEqual, .compareLess, .compareGreater]
        ),
        NodeTypeCategory(
            name: "Variables",
            icon: "v.square",
            color: .orange,
            description: "Variable storage and access",
            nodeTypes: [.variable, .variableGet, .variableSet, .constant, .constantValue]
        ),
        NodeTypeCategory(
            name: "Data",
            icon: "doc.text",
            color: .blue,
            description: "Data structures and manipulation",
            nodeTypes: [.arrayLiteral, .dictionaryLiteral, .typeCast, .typeCheck, .nullCheck, .safeAccess]
        ),
        NodeTypeCategory(
            name: "Utility",
            icon: "wrench",
            color: .secondary,
            description: "Utility and helper nodes",
            nodeTypes: [.comment, .reroute]
        )
    ]
}

// MARK: - Search Performance Optimization

class SearchPerformanceManager {
    static let shared = SearchPerformanceManager()
    
    private var searchCache: [String: [(NodeType, Double)]] = [:]
    private let maxCacheSize = 100
    
    func getCachedResult(for pattern: String) -> [(NodeType, Double)]? {
        return searchCache[pattern]
    }
    
    func cacheResult(for pattern: String, result: [(NodeType, Double)]) {
        // Remove oldest entry if cache is full
        if searchCache.count >= maxCacheSize {
            let oldestKey = searchCache.keys.min { a, b in
                // Simple heuristic: remove shortest keys first
                return a.count < b.count
            }
            if let key = oldestKey {
                searchCache.removeValue(forKey: key)
            }
        }
        
        searchCache[pattern] = result
    }
    
    func clearCache() {
        searchCache.removeAll()
    }
}
