#include "engine.h"

// =====================================
// Input System Implementation
// =====================================

void Input_Update(EngineState* engine) {
    if (!engine) return;
    
    // Update mouse state
    engine->mousePosition = GetMousePosition();
    engine->mouseDelta = GetMouseDelta();
    engine->mouseWheel = GetMouseWheelMove();
    
    engine->mouseLeftPressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    engine->mouseRightPressed = IsMouseButtonPressed(MOUSE_RIGHT_BUTTON);
    engine->mouseMiddlePressed = IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON);
    
    // Update gamepad state
    Input_UpdateGamepads(engine);
    
    // Handle common input actions (keyboard or gamepad)
    bool tabPressed = IsKeyPressed(KEY_TAB);
    if (engine->activeGamepad >= 0) {
        tabPressed = tabPressed || IsGamepadButtonPressed(engine->activeGamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP);
    }
    
    if (tabPressed) {
        ViewMode newMode = engine->viewMode;
        switch (engine->viewMode) {
            case VIEW_MODE_ORBIT:
                newMode = VIEW_MODE_ISOMETRIC;
                break;
            case VIEW_MODE_ISOMETRIC:
                newMode = VIEW_MODE_ORBIT;
                break;
            default:
                break;
        }
        Camera_SetMode(engine, newMode);
    }
    
    // Toggle display options (keyboard or gamepad)
    if (IsKeyPressed(KEY_I) || 
        (engine->activeGamepad >= 0 && IsGamepadButtonPressed(engine->activeGamepad, GAMEPAD_BUTTON_MIDDLE_LEFT))) {
        engine->showDebugInfo = !engine->showDebugInfo;
    }
    if (IsKeyPressed(KEY_U) || 
        (engine->activeGamepad >= 0 && IsGamepadButtonPressed(engine->activeGamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT))) {
        engine->showUI = !engine->showUI;
    }
}

// Wrapper functions for consistency
bool Input_IsKeyPressed(int key) { return IsKeyPressed(key); }
bool Input_IsKeyDown(int key) { return IsKeyDown(key); }
bool Input_IsKeyReleased(int key) { return IsKeyReleased(key); }
bool Input_IsMouseButtonPressed(int button) { return IsMouseButtonPressed(button); }
bool Input_IsMouseButtonDown(int button) { return IsMouseButtonDown(button); }
bool Input_IsMouseButtonReleased(int button) { return IsMouseButtonReleased(button); }
Vector2 Input_GetMousePosition(void) { return GetMousePosition(); }
Vector2 Input_GetMouseDelta(void) { return GetMouseDelta(); }
float Input_GetMouseWheel(void) { return GetMouseWheelMove(); }

// Gamepad input functions
bool Input_IsGamepadAvailable(int gamepad) {
    return IsGamepadAvailable(gamepad);
}

bool Input_IsGamepadButtonPressed(int gamepad, int button) {
    return IsGamepadAvailable(gamepad) && IsGamepadButtonPressed(gamepad, button);
}

bool Input_IsGamepadButtonDown(int gamepad, int button) {
    return IsGamepadAvailable(gamepad) && IsGamepadButtonDown(gamepad, button);
}

bool Input_IsGamepadButtonReleased(int gamepad, int button) {
    return IsGamepadAvailable(gamepad) && IsGamepadButtonReleased(gamepad, button);
}

Vector2 Input_GetGamepadLeftStick(int gamepad) {
    if (!IsGamepadAvailable(gamepad)) return (Vector2){0, 0};
    
    float x = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
    float y = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);
    
    // Apply dead zone
    if (fabsf(x) < GAMEPAD_DEAD_ZONE) x = 0;
    if (fabsf(y) < GAMEPAD_DEAD_ZONE) y = 0;
    
    return (Vector2){x, y};
}

Vector2 Input_GetGamepadRightStick(int gamepad) {
    if (!IsGamepadAvailable(gamepad)) return (Vector2){0, 0};
    
    float x = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_X);
    float y = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_Y);
    
    // Apply dead zone
    if (fabsf(x) < GAMEPAD_DEAD_ZONE) x = 0;
    if (fabsf(y) < GAMEPAD_DEAD_ZONE) y = 0;
    
    return (Vector2){x, y};
}

float Input_GetGamepadLeftTrigger(int gamepad) {
    if (!IsGamepadAvailable(gamepad)) return 0;
    
    float trigger = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_TRIGGER);
    
    // Normalize trigger value (some controllers report -1 to 1, others 0 to 1)
    trigger = (trigger + 1.0f) * 0.5f;
    
    // Apply threshold
    if (trigger < GAMEPAD_TRIGGER_THRESHOLD) trigger = 0;
    
    return trigger;
}

float Input_GetGamepadRightTrigger(int gamepad) {
    if (!IsGamepadAvailable(gamepad)) return 0;
    
    float trigger = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_TRIGGER);
    
    // Normalize trigger value (some controllers report -1 to 1, others 0 to 1)
    trigger = (trigger + 1.0f) * 0.5f;
    
    // Apply threshold
    if (trigger < GAMEPAD_TRIGGER_THRESHOLD) trigger = 0;
    
    return trigger;
}

int Input_GetActiveGamepad(EngineState* engine) {
    if (!engine) return -1;
    return engine->activeGamepad;
}

void Input_UpdateGamepads(EngineState* engine) {
    if (!engine) return;
    
    // Find first connected gamepad or detect new connections
    engine->activeGamepad = -1;
    
    for (int i = 0; i < MAX_GAMEPADS; i++) {
        bool wasConnected = engine->gamepadConnected[i];
        engine->gamepadConnected[i] = IsGamepadAvailable(i);
        
        if (engine->gamepadConnected[i]) {
            // Set first available gamepad as active
            if (engine->activeGamepad < 0) {
                engine->activeGamepad = i;
            }
            
            // Update stick positions for all connected gamepads
            engine->gamepadLeftStick[i] = Input_GetGamepadLeftStick(i);
            engine->gamepadRightStick[i] = Input_GetGamepadRightStick(i);
            engine->gamepadLeftTrigger[i] = Input_GetGamepadLeftTrigger(i);
            engine->gamepadRightTrigger[i] = Input_GetGamepadRightTrigger(i);
            
            // Log new connections
            if (!wasConnected) {
                TraceLog(LOG_INFO, "Gamepad %d connected: %s", i, GetGamepadName(i));
            }
        } else if (wasConnected) {
            // Log disconnections
            TraceLog(LOG_INFO, "Gamepad %d disconnected", i);
            
            // Clear state for disconnected gamepad
            engine->gamepadLeftStick[i] = (Vector2){0, 0};
            engine->gamepadRightStick[i] = (Vector2){0, 0};
            engine->gamepadLeftTrigger[i] = 0;
            engine->gamepadRightTrigger[i] = 0;
        }
    }
}