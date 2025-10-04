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
    
    // Handle common input actions
    if (IsKeyPressed(KEY_TAB)) {
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
    
    // Toggle display options
    if (IsKeyPressed(KEY_G)) engine->showGrid = !engine->showGrid;
    if (IsKeyPressed(KEY_X)) engine->showAxes = !engine->showAxes;
    if (IsKeyPressed(KEY_I)) engine->showDebugInfo = !engine->showDebugInfo;
    if (IsKeyPressed(KEY_U)) engine->showUI = !engine->showUI;
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