/**
 * =================================================================================================
 *                          GAME CONTROLLER & HAPTICS
 * =================================================================================================
 *
 * Handles gamepad input (Xbox, PS5, Switch) and advanced haptic feedback.
 *
 * =================================================================================================
 */

#import <GameController/GameController.h>
#import <CoreHaptics/CoreHaptics.h>

// Controller management
static NSMutableArray<GCController *> *g_connectedControllers = nil;
static NSMutableDictionary<NSNumber *, CHHapticEngine *> *g_hapticEngines = nil;
static dispatch_queue_t g_controllerQueue = nil;

// Input mapping structure
typedef struct {
    int engineInputCode;
    GCControllerButtonInput *gcButton;
    GCControllerDirectionPad *gcDPad;
    GCControllerAxisInput *gcAxis;
    float deadzone;
    float sensitivity;
    bool isPressed;
    float value;
} InputMapping;

static InputMapping g_inputMappings[256];
static int g_mappingCount = 0;

// Initialize controller system
void gamecontroller_init() {
    if (g_connectedControllers == nil) {
        g_connectedControllers = [[NSMutableArray alloc] init];
    }
    if (g_hapticEngines == nil) {
        g_hapticEngines = [[NSMutableDictionary alloc] init];
    }
    if (g_controllerQueue == nil) {
        g_controllerQueue = dispatch_queue_create("com.game.controllers", DISPATCH_QUEUE_SERIAL);
    }
    
    // Register for controller connection/disconnection
    [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification
                                                      object:nil
                                                       queue:g_controllerQueue
                                                  usingBlock:^(NSNotification *notification) {
        GCController *controller = notification.object;
        [g_connectedControllers addObject:controller];
        [self setupHapticEngineForController:controller];
        [self setupInputMappingForController:controller];
    }];
    
    [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidDisconnectNotification
                                                      object:nil
                                                       queue:g_controllerQueue
                                                  usingBlock:^(NSNotification *notification) {
        GCController *controller = notification.object;
        [g_connectedControllers removeObject:controller];
        [self cleanupHapticEngineForController:controller];
    }];
    
    // Discover existing controllers
    [GCController startWirelessControllerDiscovery];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.0 * NSEC_PER_SEC)), g_controllerQueue, ^{
        [GCController stopWirelessControllerDiscovery];
    });
}

// Setup haptic engine for controller
void setupHapticEngineForController(GCController *controller) {
    if (@available(macOS 11.0, *)) {
        CHHapticEngine *engine = [[CHHapticEngine alloc] initAndReturnError:nil];
        if (engine) {
            [engine startAndReturnError:nil];
            g_hapticEngines[@(controller.hash)] = engine;
        }
    }
}

// Cleanup haptic engine
void cleanupHapticEngineForController(GCController *controller) {
    CHHapticEngine *engine = g_hapticEngines[@(controller.hash)];
    if (engine) {
        [engine stopWithCompletionHandler:nil];
        [g_hapticEngines removeObjectForKey:@(controller.hash)];
    }
}

// Setup unified input mapping
void setupInputMappingForController(GCController *controller) {
    // Map Xbox/PlayStation buttons to engine codes
    if (controller.extendedGamepad) {
        GCExtendedGamepad *gamepad = controller.extendedGamepad;
        
        // A/Cross button
        g_inputMappings[g_mappingCount++] = (InputMapping){
            .engineInputCode = INPUT_ACTION,
            .gcButton = gamepad.buttonA,
            .deadzone = 0.1f,
            .sensitivity = 1.0f
        };
        
        // B/Circle button
        g_inputMappings[g_mappingCount++] = (InputMapping){
            .engineInputCode = INPUT_CANCEL,
            .gcButton = gamepad.buttonB,
            .deadzone = 0.1f,
            .sensitivity = 1.0f
        };
        
        // Left stick
        g_inputMappings[g_mappingCount++] = (InputMapping){
            .engineInputCode = INPUT_MOVE_X,
            .gcAxis = gamepad.leftThumbstick.xAxis,
            .deadzone = 0.15f,
            .sensitivity = 1.0f
        };
        
        g_inputMappings[g_mappingCount++] = (InputMapping){
            .engineInputCode = INPUT_MOVE_Y,
            .gcAxis = gamepad.leftThumbstick.yAxis,
            .deadzone = 0.15f,
            .sensitivity = 1.0f
        };
        
        // Right stick
        g_inputMappings[g_mappingCount++] = (InputMapping){
            .engineInputCode = INPUT_LOOK_X,
            .gcAxis = gamepad.rightThumbstick.xAxis,
            .deadzone = 0.15f,
            .sensitivity = 1.0f
        };
        
        g_inputMappings[g_mappingCount++] = (InputMapping){
            .engineInputCode = INPUT_LOOK_Y,
            .gcAxis = gamepad.rightThumbstick.yAxis,
            .deadzone = 0.15f,
            .sensitivity = 1.0f
        };
    }
}

// Update input state
void gamecontroller_update() {
    for (int i = 0; i < g_mappingCount; i++) {
        InputMapping *mapping = &g_inputMappings[i];
        
        if (mapping->gcButton) {
            mapping->isPressed = mapping->gcButton.isPressed;
            mapping->value = mapping->gcButton.value;
        } else if (mapping->gcAxis) {
            float rawValue = mapping->gcAxis.value;
            if (fabs(rawValue) > mapping->deadzone) {
                mapping->value = rawValue * mapping->sensitivity;
            } else {
                mapping->value = 0.0f;
            }
        }
    }
}

// Get input value
float gamecontroller_get_input(int engineCode) {
    for (int i = 0; i < g_mappingCount; i++) {
        if (g_inputMappings[i].engineInputCode == engineCode) {
            return g_inputMappings[i].value;
        }
    }
    return 0.0f;
}

bool gamecontroller_is_pressed(int engineCode) {
    for (int i = 0; i < g_mappingCount; i++) {
        if (g_inputMappings[i].engineInputCode == engineCode) {
            return g_inputMappings[i].isPressed;
        }
    }
    return false;
}

// Advanced haptics (DualSense)
void gamecontroller_play_haptic(GCController *controller, float intensity, float sharpness, float duration) {
    CHHapticEngine *engine = g_hapticEngines[@(controller.hash)];
    if (@available(macOS 11.0, *) && engine) {
        CHHapticEventParameter *intensityParam = [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticIntensity value:intensity];
        CHHapticEventParameter *sharpnessParam = [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticSharpness value:sharpness];
        
        CHHapticEvent *event = [[CHHapticEvent alloc] initWithEventType:CHHapticEventTypeHapticTransient
                                                               parameters:@[intensityParam, sharpnessParam]
                                                                   relativeTime:0];
        
        CHHapticPattern *pattern = [[CHHapticPattern alloc] initWithEvents:@[event] parameters:@[] error:nil];
        
        CHHapticPatternPlayer *player = [engine createPlayerWithPattern:pattern error:nil];
        [player startAtTime:0 error:nil];
    }
}

// Adaptive trigger support (if available)
void gamecontroller_set_adaptive_trigger(GCController *controller, GCControllerButtonInput *trigger, float resistance) {
    if (@available(macOS 12.0, *)) {
        // This would use GCControllerDirectionPad's advanced features
        // Implementation depends on Apple's API availability
        if ([trigger respondsToSelector:@selector(setActuators:)]) {
            // Set trigger resistance/feedback
        }
    }
}

// Lightbar control (DualShock/DualSense)
void gamecontroller_set_lightbar_color(GCController *controller, float r, float g, float b) {
    if (@available(macOS 11.0, *)) {
        GCDeviceLight *light = controller.light;
        if (light) {
            [light setColor:[[UIColor alloc] initWithRed:r green:g blue:b alpha:1.0]];
        }
    }
}

// Motion sensor support
void gamecontroller_get_motion_data(GCController *controller, float *accelX, float *accelY, float *accelZ, 
                                   float *gyroX, float *gyroY, float *gyroZ) {
    if (@available(macOS 11.0, *)) {
        GCMotion *motion = controller.motion;
        if (motion) {
            if (accelX) *accelX = motion.acceleration.x;
            if (accelY) *accelY = motion.acceleration.y;
            if (accelZ) *accelZ = motion.acceleration.z;
            if (gyroX) *gyroX = motion.rotationRate.x;
            if (gyroY) *gyroY = motion.rotationRate.y;
            if (gyroZ) *gyroZ = motion.rotationRate.z;
        }
    }
}

// Get connected controllers count
int gamecontroller_get_controller_count() {
    return (int)g_connectedControllers.count;
}

// Get controller by index
GCController *gamecontroller_get_controller(int index) {
    if (index >= 0 && index < g_connectedControllers.count) {
        return g_connectedControllers[index];
    }
    return nil;
}

// Cleanup
void gamecontroller_shutdown() {
    // Stop discovery
    [GCController stopWirelessControllerDiscovery];
    
    // Cleanup haptic engines
    for (CHHapticEngine *engine in g_hapticEngines.allValues) {
        [engine stopWithCompletionHandler:nil];
    }
    [g_hapticEngines removeAllObjects];
    
    // Clear controllers
    [g_connectedControllers removeAllObjects];
    
    // Remove notification observers
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}
