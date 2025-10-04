#include "engine.h"
#include <stdio.h>

// =====================================
// Rendering Utilities Implementation
// =====================================





void Render_SelectionBox(Vector2 start, Vector2 end) {
    // Calculate box dimensions
    float x = fminf(start.x, end.x);
    float y = fminf(start.y, end.y);
    float width = fabsf(end.x - start.x);
    float height = fabsf(end.y - start.y);
    
    // Draw selection box
    DrawRectangleLines((int)x, (int)y, (int)width, (int)height, GREEN);
    DrawRectangle((int)x, (int)y, (int)width, (int)height, (Color){0, 255, 0, 30});
}

void Render_DebugInfo(EngineState* engine) {
    if (!engine) return;
    
    int y = 5;
    int lineHeight = 12;
    Color textColor = LIGHTGRAY;
    int fontSize = 8;
    
    // Engine info
    DrawText(ENGINE_NAME, 5, y, fontSize + 2, WHITE);
    y += lineHeight + 3;
    
    DrawText(TextFormat("FPS: %d", GetFPS()), 5, y, fontSize, textColor);
    y += lineHeight;
    
    DrawText(TextFormat("Delta: %.3fms", engine->deltaTime * 1000.0f), 5, y, fontSize, textColor);
    y += lineHeight;
    
    // Resolution mode
    if (engine->useInternalResolution) {
        DrawText(TextFormat("Resolution: %dx%d (Internal)", INTERNAL_RENDER_WIDTH, INTERNAL_RENDER_HEIGHT), 5, y, fontSize, YELLOW);
    } else {
        DrawText(TextFormat("Resolution: %dx%d (Native)", engine->windowWidth, engine->windowHeight), 5, y, fontSize, textColor);
    }
    y += lineHeight;
    
    // Fullscreen status
    DrawText(TextFormat("Window: %dx%d (%s)", engine->windowWidth, engine->windowHeight, 
            IsWindowFullscreen() ? "Fullscreen" : "Windowed"), 5, y, fontSize, textColor);
    y += lineHeight;
    
    DrawText("F1: Toggle resolution | F11/Alt+Enter: Fullscreen", 5, y, fontSize, DARKGRAY);
    y += lineHeight;
    
    // Aspect ratio mode
    DrawText(TextFormat("Aspect: %s (F3 to toggle)", 
            engine->maintainAspectRatio ? "Maintain 16:9" : "Stretch to Fill"), 
            5, y, fontSize, engine->maintainAspectRatio ? LIME : YELLOW);
    y += lineHeight;
    
    // Scanline effect status (only show when using internal resolution)
    if (engine->useInternalResolution) {
        DrawText(TextFormat("Scanlines: %s (F2 to toggle)", engine->showScanlines ? "ON" : "OFF"), 
                5, y, fontSize, engine->showScanlines ? GREEN : DARKGRAY);
        y += lineHeight;
    }
    
    // Camera info
    const char* modeStr = "";
    switch (engine->viewMode) {
        case VIEW_MODE_ORBIT: modeStr = "ORBIT"; break;
        case VIEW_MODE_ISOMETRIC: modeStr = "ISOMETRIC"; break;
        case VIEW_MODE_FIRST_PERSON: modeStr = "FIRST PERSON"; break;
        case VIEW_MODE_THIRD_PERSON: modeStr = "THIRD PERSON"; break;
    }
    DrawText(TextFormat("Camera: %s", modeStr), 10, y, fontSize, textColor);
    y += lineHeight;
    
    // Entity info
    DrawText(TextFormat("Entities: %d/%d", engine->entityCount, MAX_ENTITIES), 5, y, fontSize, textColor);
    y += lineHeight;
    
    int selectedCount = Entity_GetSelectedCount(engine);
    if (selectedCount > 0) {
        DrawText(TextFormat("Selected: %d", selectedCount), 5, y, fontSize, LIME);
        y += lineHeight;
    }
    
    // Control groups with active entities
    y += 3;
    bool hasGroups = false;
    for (int i = 1; i < MAX_CONTROL_GROUPS; i++) {
        if (engine->controlGroups[i].active && engine->controlGroups[i].entityCount > 0) {
            if (!hasGroups) {
                DrawText("Groups:", 5, y, fontSize, textColor);
                y += lineHeight;
                hasGroups = true;
            }
            DrawText(TextFormat("  [%d]: %d units", i, engine->controlGroups[i].entityCount), 
                    5, y, fontSize, SKYBLUE);
            y += lineHeight;
        }
    }
    
    // Controls hint
    y = (engine->useInternalResolution ? INTERNAL_RENDER_HEIGHT : engine->windowHeight) - 50;
    if (engine->activeGamepad >= 0) {
        DrawText("Gamepad Camera: L-Stick/D-Pad: Move | R-Stick: Rotate | LB/LT/L3/R3: Zoom", 5, y, fontSize, DARKGRAY);
        y += lineHeight;
        DrawText("Select: Reset | Y: Switch Camera | Start: Pause", 5, y, fontSize, DARKGRAY);
    } else {
        DrawText("TAB: Switch Camera | WASD: Move | Mouse Wheel: Zoom", 5, y, fontSize, DARKGRAY);
        y += lineHeight;
        DrawText("I: Info | ESC: Exit", 5, y, fontSize, DARKGRAY);
    }
    y += lineHeight;
    
    // Show gamepad connection status
    if (engine->activeGamepad >= 0) {
        DrawText(TextFormat("Gamepad %d Connected", engine->activeGamepad + 1), 5, y, fontSize, GREEN);
    }
}