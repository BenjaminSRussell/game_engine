import SwiftUI

// MARK: - Complete NodeType Enumeration

extension NodeType {
    // MARK: - Event Nodes
    
    /// Basic event trigger
    case event
    
    /// Lifecycle events
    case onStart, onUpdate, onDestroy
    
    /// Input events
    case inputKeyboard, inputMouse, inputTouch, inputGamepad
    
    /// Collision events
    case collisionEnter, collisionExit, collisionStay
    
    /// Trigger events
    case triggerEnter, triggerExit, triggerStay
    
    /// Custom events
    case customEvent
    
    // MARK: - Flow Control Nodes
    
    /// Basic flow control
    case flowBranch, flowSequence, flowSelect, flowSwitch
    
    /// Loop nodes
    case loopFor, loopForEach, loopWhile, loopDoWhile
    
    /// Control nodes
    case gate, latch, delay, timer, asyncAwait
    
    // MARK: - Mathematical Operation Nodes
    
    /// Arithmetic operations
    case mathAdd, mathSubtract, mathMultiply, mathDivide, mathModulo, mathPower
    
    /// Trigonometric functions
    case mathSin, mathCos, mathTan, mathAsin, mathAcos, mathAtan
    
    /// Utility functions
    case mathSqrt, mathAbs, mathMin, mathMax, mathClamp, mathFloor, mathCeil, mathRound
    
    /// Interpolation functions
    case mathLerp, mathInverseLerp, mathSmoothStep, mathMoveTowards
    
    // MARK: - Vector Math Nodes
    
    /// Basic vector operations
    case vectorAdd, vectorSubtract, vectorMultiply, vectorDivide, vectorScale
    
    /// Advanced vector operations
    case vectorDot, vectorCross, vectorMagnitude, vectorNormalize, vectorDistance
    
    /// Vector utilities
    case vectorReflect, vectorProject, vectorAngle, vectorLerp, vectorSlerp
    
    /// Vector constructors
    case vector2Make, vector3Make, vector4Make, quaternionMake
    
    /// Vector decomposition
    case vectorGetX, vectorGetY, vectorGetZ, vectorGetW
    case vectorSetX, vectorSetY, vectorSetZ, vectorSetW
    
    // MARK: - Matrix Math Nodes
    
    /// Matrix operations
    case matrixMake, matrixIdentity, matrixTranspose, matrixInverse, matrixDeterminant
    
    /// Matrix arithmetic
    case matrixMultiply, matrixAdd, matrixSubtract, matrixScale
    
    /// Matrix transformations
    case matrixTranslation, matrixRotation, matrixScale, matrixTRS, matrixLookAt
    
    /// Matrix decomposition
    case matrixDecompose, matrixGetTranslation, matrixGetRotation, matrixGetScale
    
    // MARK: - Logic Operation Nodes
    
    /// Basic logic
    case logicAnd, logicOr, logicNot, logicXor, logicNand, logicNor
    
    /// Advanced logic
    case logicImplies, logicEquivalent, logicBitwiseAnd, logicBitwiseOr, logicBitwiseXor, logicBitwiseNot
    
    /// Bit operations
    case logicShiftLeft, logicShiftRight, logicRotateLeft, logicRotateRight
    
    // MARK: - Comparison Nodes
    
    /// Basic comparisons
    case compareEqual, compareNotEqual, compareLess, compareLessEqual
    case compareGreater, compareGreaterEqual, compareApproximately
    
    /// Type checking
    case compareIsNone, compareIsNotNull, compareIsTrue, compareIsFalse
    case compareIsType, compareIsNotType
    
    // MARK: - String Manipulation Nodes
    
    /// Basic string operations
    case stringConcat, stringSubstring, stringLength, stringUpper, stringLower
    
    /// String searching and manipulation
    case stringReplace, stringSplit, stringJoin, stringTrim, stringPadLeft, stringPadRight
    
    /// String formatting
    case stringFormat, stringContains, stringStartsWith, stringEndsWith
    
    // MARK: - Data Structure Nodes
    
    /// Array operations
    case arrayMake, arrayGet, arraySet, arrayAdd, arrayInsert, arrayRemove
    case arrayLength, arrayClear, arrayContains, arrayIndexOf, arraySort, arrayReverse
    
    /// Dictionary operations
    case dictMake, dictGet, dictSet, dictRemove, dictHasKey, dictGetKeys, dictGetValues
    case dictMerge, dictClear, dictSize, dictFilter, dictMap
    
    // MARK: - Type System Nodes
    
    /// Type casting and checking
    case typeCast, typeCheck, typeConvert, typeAs, typeIs
    
    /// Safety operations
    case nullCheck, safeAccess, optionalUnwrap, optionalCoalesce
    
    /// Generic operations
    case genericMake, genericGet, genericSet
    
    // MARK: - Asset and Resource Nodes
    
    /// Asset loading
    case assetLoad, assetUnload, assetReload, assetExists, assetGetPath
    
    /// Asset types
    case assetTexture, assetModel, assetAudio, assetAnimation, assetMaterial
    
    // MARK: - Physics Nodes
    
    /// Physics operations
    case physicsAddForce, physicsAddImpulse, physicsSetVelocity, physicsGetVelocity
    case physicsSetMass, physicsGetMass, physicsEnableGravity, physicsDisableGravity
    
    /// Physics queries
    case physicsRaycast, physicsSphereCast, physicsBoxCast, physicsOverlapSphere
    case physicsOverlapBox, physicsCheckSphere, physicsCheckBox
    
    /// Physics properties
    case physicsSetLinearDamping, physicsSetAngularDamping, physicsSetFriction, physicsSetRestitution
    
    // MARK: - Audio Nodes
    
    /// Audio playback
    case audioPlay, audioStop, audioPause, audioResume, audioPlayOneShot, audioPlayLooped
    
    /// Audio properties
    case audioSetVolume, audioGetVolume, audioSetPitch, audioGetPitch, audioSetPan, audioGetPan
    
    /// Audio effects
    case audioFadeIn, audioFadeOut, audioCrossFade, audioMute, audioUnmute
    
    // MARK: - Animation Nodes
    
    /// Animation playback
    case animPlay, animStop, animPause, animResume, animRestart
    
    /// Animation properties
    case animSetSpeed, animGetSpeed, animSetTime, animGetTime, animGetDuration
    
    /// Animation blending
    case animBlend, animCrossFade, animAdditiveBlend, animSetWeight, animGetWeight
    
    /// Animation events
    case animHasFinished, animIsPlaying, animGetNormalizedTime
    
    // MARK: - UI Nodes
    
    /// UI visibility
    case uiShow, uiHide, uiSetVisible, uiIsVisible, uiSetEnabled, uiIsEnabled
    
    /// UI properties
    case uiSetText, uiGetText, uiSetPosition, uiGetPosition, uiSetSize, uiGetSize
    
    /// UI interaction
    case uiButtonClick, uiSliderChange, uiTextFieldChange, uiToggleChange
    
    // MARK: - File I/O Nodes
    
    /// File operations
    case fileRead, fileWrite, fileExists, fileDelete, fileCopy, fileMove
    
    /// File information
    case fileGetSize, fileGetModified, fileGetCreated, fileGetExtension, fileGetName
    
    /// Directory operations
    case dirCreate, dirDelete, dirExists, dirList, dirGetSize
    
    // MARK: - Network Nodes
    
    /// Network requests
    case netGet, netPost, netPut, netDelete, netRequest
    
    /// Network utilities
    case netEncodeURL, netDecodeURL, netJSONEncode, netJSONDecode
    
    // MARK: - Debug Nodes
    
    /// Debug operations
    case debugPrint, debugLog, debugAssert, debugBreak, debugWatch
    
    /// Performance
    case debugTimeStart, debugTimeEnd, debugMemoryUsage, debugProfile
    
    // MARK: - Utility Nodes
    
    /// Utility functions
    case random, randomRange, noise, perlinNoise, hash
    
    /// Conversion utilities
    case toString, toInt, toFloat, toBool, toVector3, toColor
    
    /// Math utilities
    case degToRad, radToDeg, min, max, clamp, abs, sign
    
    // MARK: - Advanced Features
    
    /// Multi-threading
    case threadStart, threadJoin, threadSleep, asyncStart, asyncWait
    
    /// Memory management
    case memoryAlloc, memoryFree, memoryCopy, memorySize
    
    /// Profiling
    case profileStart, profileEnd, profileSample, profileReport
    
    // MARK: - Legacy Compatibility
    
    /// Legacy node types (for backward compatibility)
    case getter, setter, function, math, logic, variable, constant, comment
    case branch, sequence, forLoop, forEach, macro, graphInstance, graphInput, graphOutput
    case literal, enumValue, structConstructor, objectInstance
    case variableGet, variableSet, constantValue
    case arrayLiteral, dictionaryLiteral, nullCheck, safeAccess
    case reroute
}

// MARK: - Node Type Categories

extension NodeType {
    var category: NodeCategory {
        switch self {
        // Events
        case .event, .onStart, .onUpdate, .onDestroy, .customEvent:
            return .events
            
        case .inputKeyboard, .inputMouse, .inputTouch, .inputGamepad:
            return .input
            
        case .collisionEnter, .collisionExit, .collisionStay,
             .triggerEnter, .triggerExit, .triggerStay:
            return .physics
            
        // Flow Control
        case .flowBranch, .flowSequence, .flowSelect, .flowSwitch,
             .loopFor, .loopForEach, .loopWhile, .loopDoWhile,
             .gate, .latch, .delay, .timer, .asyncAwait:
            return .flowControl
            
        // Math
        case .mathAdd, .mathSubtract, .mathMultiply, .mathDivide, .mathModulo, .mathPower,
             .mathSin, .mathCos, .mathTan, .mathAsin, .mathAcos, .mathAtan,
             .mathSqrt, .mathAbs, .mathMin, .mathMax, .mathClamp, .mathFloor, .mathCeil, .mathRound,
             .mathLerp, .mathInverseLerp, .mathSmoothStep, .mathMoveTowards:
            return .math
            
        // Vector
        case .vectorAdd, .vectorSubtract, .vectorMultiply, .vectorDivide, .vectorScale,
             .vectorDot, .vectorCross, .vectorMagnitude, .vectorNormalize, .vectorDistance,
             .vectorReflect, .vectorProject, .vectorAngle, .vectorLerp, .vectorSlerp,
             .vector2Make, .vector3Make, .vector4Make, .quaternionMake,
             .vectorGetX, .vectorGetY, .vectorGetZ, .vectorGetW,
             .vectorSetX, .vectorSetY, .vectorSetZ, .vectorSetW:
            return .vector
            
        // Matrix
        case .matrixMake, .matrixIdentity, .matrixTranspose, .matrixInverse, .matrixDeterminant,
             .matrixMultiply, .matrixAdd, .matrixSubtract, .matrixScale,
             .matrixTranslation, .matrixRotation, .matrixScale, .matrixTRS, .matrixLookAt,
             .matrixDecompose, .matrixGetTranslation, .matrixGetRotation, .matrixGetScale:
            return .matrix
            
        // Logic
        case .logicAnd, .logicOr, .logicNot, .logicXor, .logicNand, .logicNor,
             .logicImplies, .logicEquivalent, .logicBitwiseAnd, .logicBitwiseOr, .logicBitwiseXor, .logicBitwiseNot,
             .logicShiftLeft, .logicShiftRight, .logicRotateLeft, .logicRotateRight:
            return .logic
            
        // Comparison
        case .compareEqual, .compareNotEqual, .compareLess, .compareLessEqual,
             .compareGreater, .compareGreaterEqual, .compareApproximately,
             .compareIsNone, .compareIsNotNull, .compareIsTrue, .compareIsFalse,
             .compareIsType, .compareIsNotType:
            return .comparison
            
        // String
        case .stringConcat, .stringSubstring, .stringLength, .stringUpper, .stringLower,
             .stringReplace, .stringSplit, .stringJoin, .stringTrim, .stringPadLeft, .stringPadRight,
             .stringFormat, .stringContains, .stringStartsWith, .stringEndsWith:
            return .string
            
        // Data Structures
        case .arrayMake, .arrayGet, .arraySet, .arrayAdd, .arrayInsert, .arrayRemove,
             .arrayLength, .arrayClear, .arrayContains, .arrayIndexOf, .arraySort, .arrayReverse,
             .dictMake, .dictGet, .dictSet, .dictRemove, .dictHasKey, .dictGetKeys, .dictGetValues,
             .dictMerge, .dictClear, .dictSize, .dictFilter, .dictMap:
            return .data
            
        // Type System
        case .typeCast, .typeCheck, .typeConvert, .typeAs, .typeIs,
             .nullCheck, .safeAccess, .optionalUnwrap, .optionalCoalesce,
             .genericMake, .genericGet, .genericSet:
            return .typeSystem
            
        // Assets
        case .assetLoad, .assetUnload, .assetReload, .assetExists, .assetGetPath,
             .assetTexture, .assetModel, .assetAudio, .assetAnimation, .assetMaterial:
            return .assets
            
        // Physics
        case .physicsAddForce, .physicsAddImpulse, .physicsSetVelocity, .physicsGetVelocity,
             .physicsSetMass, .physicsGetMass, .physicsEnableGravity, .physicsDisableGravity,
             .physicsRaycast, .physicsSphereCast, .physicsBoxCast, .physicsOverlapSphere,
             .physicsOverlapBox, .physicsCheckSphere, .physicsCheckBox,
             .physicsSetLinearDamping, .physicsSetAngularDamping, .physicsSetFriction, .physicsSetRestitution:
            return .physics
            
        // Audio
        case .audioPlay, .audioStop, .audioPause, .audioResume, .audioPlayOneShot, .audioPlayLooped,
             .audioSetVolume, .audioGetVolume, .audioSetPitch, .audioGetPitch, .audioSetPan, .audioGetPan,
             .audioFadeIn, .audioFadeOut, .audioCrossFade, .audioMute, .audioUnmute:
            return .audio
            
        // Animation
        case .animPlay, .animStop, .animPause, .animResume, .animRestart,
             .animSetSpeed, .animGetSpeed, .animSetTime, .animGetTime, .animGetDuration,
             .animBlend, .animCrossFade, .animAdditiveBlend, .animSetWeight, .animGetWeight,
             .animHasFinished, .animIsPlaying, .animGetNormalizedTime:
            return .animation
            
        // UI
        case .uiShow, .uiHide, .uiSetVisible, .uiIsVisible, .uiSetEnabled, .uiIsEnabled,
             .uiSetText, .uiGetText, .uiSetPosition, .uiGetPosition, .uiSetSize, .uiGetSize,
             .uiButtonClick, .uiSliderChange, .uiTextFieldChange, .uiToggleChange:
            return .ui
            
        // File I/O
        case .fileRead, .fileWrite, .fileExists, .fileDelete, .fileCopy, .fileMove,
             .fileGetSize, .fileGetModified, .fileGetCreated, .fileGetExtension, .fileGetName,
             .dirCreate, .dirDelete, .dirExists, .dirList, .dirGetSize:
            return .fileIO
            
        // Network
        case .netGet, .netPost, .netPut, .netDelete, .netRequest,
             .netEncodeURL, .netDecodeURL, .netJSONEncode, .netJSONDecode:
            return .network
            
        // Debug
        case .debugPrint, .debugLog, .debugAssert, .debugBreak, .debugWatch,
             .debugTimeStart, .debugTimeEnd, .debugMemoryUsage, .debugProfile:
            return .debug
            
        // Utility
        case .random, .randomRange, .noise, .perlinNoise, .hash,
             .toString, .toInt, .toFloat, .toBool, .toVector3, .toColor,
             .degToRad, .radToDeg, .min, .max, .clamp, .abs, .sign:
            return .utility
            
        // Advanced
        case .threadStart, .threadJoin, .threadSleep, .asyncStart, .asyncWait,
             .memoryAlloc, .memoryFree, .memoryCopy, .memorySize,
             .profileStart, .profileEnd, .profileSample, .profileReport:
            return .advanced
            
        // Legacy
        case .getter, .setter, .function, .math, .logic, .variable, .constant, .comment,
             .branch, .sequence, .forLoop, .forEach, .macro, .graphInstance, .graphInput, .graphOutput,
             .literal, .enumValue, .structConstructor, .objectInstance,
             .variableGet, .variableSet, .constantValue,
             .arrayLiteral, .dictionaryLiteral, .nullCheck, .safeAccess, .reroute:
            return .legacy
        }
    }
    
    var isDeprecated: Bool {
        switch self {
        case .getter, .setter, .function, .math, .logic, .branch, .sequence, .forLoop, .forEach:
            return true
        default:
            return false
        }
    }
    
    var description: String {
        switch self {
        // Events
        case .event: return "Basic event trigger"
        case .onStart: return "Called when graph starts"
        case .onUpdate: return "Called every frame"
        case .onDestroy: return "Called when graph stops"
        case .customEvent: return "Custom user-defined event"
        
        // Input
        case .inputKeyboard: return "Keyboard input event"
        case .inputMouse: return "Mouse input event"
        case .inputTouch: return "Touch input event"
        case .inputGamepad: return "Gamepad input event"
        
        // Physics
        case .collisionEnter: return "Collision start event"
        case .collisionExit: return "Collision end event"
        case .collisionStay: return "Collision event"
        case .triggerEnter: return "Trigger start event"
        case .triggerExit: return "Trigger end event"
        case .triggerStay: return "Trigger event"
        
        // Flow Control
        case .flowBranch: return "Conditional branch"
        case .flowSequence: return "Execute in sequence"
        case .flowSelect: return "Select value"
        case .flowSwitch: return "Switch statement"
        case .loopFor: return "For loop"
        case .loopForEach: return "For each loop"
        case .loopWhile: return "While loop"
        case .loopDoWhile: return "Do while loop"
        case .gate: return "Gate control"
        case .latch: return "Latch control"
        case .delay: return "Delay execution"
        case .timer: return "Timer event"
        case .asyncAwait: return "Async/await"
        
        // Math
        case .mathAdd: return "Addition (A + B)"
        case .mathSubtract: return "Subtraction (A - B)"
        case .mathMultiply: return "Multiplication (A * B)"
        case .mathDivide: return "Division (A / B)"
        case .mathModulo: return "Modulo (A % B)"
        case .mathPower: return "Power (A^B)"
        case .mathSin: return "Sine function"
        case .mathCos: return "Cosine function"
        case .mathTan: return "Tangent function"
        case .mathAsin: return "Arc sine"
        case .mathAcos: return "Arc cosine"
        case .mathAtan: return "Arc tangent"
        case .mathSqrt: return "Square root"
        case .mathAbs: return "Absolute value"
        case .mathMin: return "Minimum value"
        case .mathMax: return "Maximum value"
        case .mathClamp: return "Clamp value"
        case .mathFloor: return "Floor function"
        case .mathCeil: return "Ceiling function"
        case .mathRound: return "Round function"
        case .mathLerp: return "Linear interpolation"
        case .mathInverseLerp: return "Inverse linear interpolation"
        case .mathSmoothStep: return "Smooth step function"
        case .mathMoveTowards: return "Move towards value"
        
        // Vector
        case .vectorAdd: return "Vector addition"
        case .vectorSubtract: return "Vector subtraction"
        case .vectorMultiply: return "Vector multiplication"
        case .vectorDivide: return "Vector division"
        case .vectorScale: return "Vector scaling"
        case .vectorDot: return "Vector dot product"
        case .vectorCross: return "Vector cross product"
        case .vectorMagnitude: return "Vector magnitude"
        case .vectorNormalize: return "Vector normalization"
        case .vectorDistance: return "Vector distance"
        case .vectorReflect: return "Vector reflection"
        case .vectorProject: return "Vector projection"
        case .vectorAngle: return "Vector angle"
        case .vectorLerp: return "Vector interpolation"
        case .vectorSlerp: return "Spherical interpolation"
        case .vector2Make: return "Create Vector2"
        case .vector3Make: return "Create Vector3"
        case .vector4Make: return "Create Vector4"
        case .quaternionMake: return "Create Quaternion"
        
        // Matrix
        case .matrixMake: return "Create matrix"
        case .matrixIdentity: return "Identity matrix"
        case .matrixTranspose: return "Matrix transpose"
        case .matrixInverse: return "Matrix inverse"
        case .matrixDeterminant: return "Matrix determinant"
        case .matrixMultiply: return "Matrix multiplication"
        case .matrixTranslation: return "Translation matrix"
        case .matrixRotation: return "Rotation matrix"
        case .matrixScale: return "Scale matrix"
        case .matrixTRS: return "TRS matrix"
        
        // Logic
        case .logicAnd: return "Logical AND"
        case .logicOr: return "Logical OR"
        case .logicNot: return "Logical NOT"
        case .logicXor: return "Logical XOR"
        case .logicNand: return "Logical NAND"
        case .logicNor: return "Logical NOR"
        case .logicImplies: return "Logical implies"
        case .logicEquivalent: return "Logical equivalent"
        case .logicBitwiseAnd: return "Bitwise AND"
        case .logicBitwiseOr: return "Bitwise OR"
        case .logicBitwiseXor: return "Bitwise XOR"
        case .logicBitwiseNot: return "Bitwise NOT"
        case .logicShiftLeft: return "Shift left"
        case .logicShiftRight: return "Shift right"
        
        // Comparison
        case .compareEqual: return "Equal (==)"
        case .compareNotEqual: return "Not equal (!=)"
        case .compareLess: return "Less than (<)"
        case .compareLessEqual: return "Less or equal (<=)"
        case .compareGreater: return "Greater than (>)"
        case .compareGreaterEqual: return "Greater or equal (>=)"
        case .compareApproximately: return "Approximately equal"
        case .compareIsNone: return "Is null check"
        case .compareIsNotNull: return "Is not null check"
        case .compareIsTrue: return "Is true check"
        case .compareIsFalse: return "Is false check"
        
        // String
        case .stringConcat: return "String concatenation"
        case .stringSubstring: return "Get substring"
        case .stringLength: return "String length"
        case .stringUpper: return "To uppercase"
        case .stringLower: return "To lowercase"
        case .stringReplace: return "Replace text"
        case .stringSplit: return "Split string"
        case .stringJoin: return "Join strings"
        case .stringTrim: return "Trim whitespace"
        case .stringFormat: return "Format string"
        
        // Data Structures
        case .arrayMake: return "Create array"
        case .arrayGet: return "Get array element"
        case .arraySet: return "Set array element"
        case .arrayAdd: return "Add to array"
        case .arrayLength: return "Array length"
        case .dictMake: return "Create dictionary"
        case .dictGet: return "Get dictionary value"
        case .dictSet: return "Set dictionary value"
        
        // Type System
        case .typeCast: return "Type casting"
        case .typeCheck: return "Type checking"
        case .typeConvert: return "Type conversion"
        case .nullCheck: return "Null check"
        case .safeAccess: return "Safe access"
        
        // Assets
        case .assetLoad: return "Load asset"
        case .assetTexture: return "Texture asset"
        case .assetModel: return "Model asset"
        case .assetAudio: return "Audio asset"
        
        // Physics
        case .physicsAddForce: return "Add force"
        case .physicsRaycast: return "Raycast"
        case .physicsSetVelocity: return "Set velocity"
        
        // Audio
        case .audioPlay: return "Play audio"
        case .audioSetVolume: return "Set volume"
        case .audioStop: return "Stop audio"
        
        // Animation
        case .animPlay: return "Play animation"
        case .animSetSpeed: return "Set animation speed"
        case .animStop: return "Stop animation"
        
        // UI
        case .uiShow: return "Show UI element"
        case .uiHide: return "Hide UI element"
        case .uiSetText: return "Set UI text"
        
        // File I/O
        case .fileRead: return "Read file"
        case .fileWrite: return "Write file"
        case .fileExists: return "Check file exists"
        
        // Network
        case .netGet: return "HTTP GET request"
        case .netPost: return "HTTP POST request"
        
        // Debug
        case .debugPrint: return "Print debug message"
        case .debugLog: return "Log debug message"
        case .debugBreak: return "Debug breakpoint"
        
        // Utility
        case .random: return "Random number"
        case .toString: return "Convert to string"
        case .toInt: return "Convert to integer"
        case .toFloat: return "Convert to float"
        
        // Legacy
        case .getter: return "Get property (legacy)"
        case .setter: return "Set property (legacy)"
        case .function: return "Function call (legacy)"
        case .variable: return "Variable (legacy)"
        case .constant: return "Constant (legacy)"
        case .comment: return "Comment"
        case .reroute: return "Reroute connection"
        
        default:
            return String(describing: self)
        }
    }
}

// MARK: - Node Category Enumeration

enum NodeCategory: String, CaseIterable {
    case events = "Events"
    case input = "Input"
    case flowControl = "Flow Control"
    case math = "Math"
    case vector = "Vector"
    case matrix = "Matrix"
    case logic = "Logic"
    case comparison = "Comparison"
    case string = "String"
    case data = "Data"
    case typeSystem = "Type System"
    case assets = "Assets"
    case physics = "Physics"
    case audio = "Audio"
    case animation = "Animation"
    case ui = "UI"
    case fileIO = "File I/O"
    case network = "Network"
    case debug = "Debug"
    case utility = "Utility"
    case advanced = "Advanced"
    case legacy = "Legacy"
    
    var icon: String {
        switch self {
        case .events: return "bolt.fill"
        case .input: return "gamecontroller"
        case .flowControl: return "arrow.branch"
        case .math: return "plus.forwardslash.minus"
        case .vector: return "arrow.up.right.and.arrow.down.left"
        case .matrix: return "square.3x3"
        case .logic: return "questionmark.diamond"
        case .comparison: return "equal"
        case .string: return "textformat"
        case .data: return "doc.text"
        case .typeSystem: return "gear"
        case .assets: return "photo"
        case .physics: return "atom"
        case .audio: return "speaker.wave.3"
        case .animation: return "film"
        case .ui: return "rectangle.3d"
        case .fileIO: return "doc"
        case .network: return "network"
        case .debug: return "ladybug"
        case .utility: return "wrench"
        case .advanced: return "gearshape.2"
        case .legacy: return "clock"
        }
    }
    
    var color: Color {
        switch self {
        case .events: return .red
        case .input: return .orange
        case .flowControl: return .gray
        case .math: return .teal
        case .vector: return .yellow
        case .matrix: return .purple
        case .logic: return .teal
        case .comparison: return .orange
        case .string: return .pink
        case .data: return .blue
        case .typeSystem: return .purple
        case .assets: return .orange
        case .physics: return .red
        case .audio: return .brown
        case .animation: return .cyan
        case .ui: return .green
        case .fileIO: return .secondary
        case .network: return .blue
        case .debug: return .red
        case .utility: return .secondary
        case .advanced: return .purple
        case .legacy: return .gray
        }
    }
    
    var description: String {
        switch self {
        case .events: return "Event triggers and handlers"
        case .input: return "Input device events"
        case .flowControl: return "Control flow and logic"
        case .math: return "Mathematical operations"
        case .vector: return "Vector mathematics"
        case .matrix: return "Matrix operations"
        case .logic: return "Logical operations"
        case .comparison: return "Value comparisons"
        case .string: return "String manipulation"
        case .data: return "Data structures"
        case .typeSystem: return "Type operations"
        case .assets: return "Asset management"
        case .physics: return "Physics operations"
        case .audio: return "Audio operations"
        case .animation: return "Animation control"
        case .ui: return "User interface"
        case .fileIO: return "File operations"
        case .network: return "Network operations"
        case .debug: return "Debug utilities"
        case .utility: return "Utility functions"
        case .advanced: return "Advanced features"
        case .legacy: return "Legacy nodes"
        }
    }
}
