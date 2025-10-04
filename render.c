#include "engine.h"
#include <stdio.h>

// =====================================
// Rendering Utilities Implementation
// =====================================

void Render_Grid(float size, int divisions) {
    float spacing = size / divisions;
    float halfSize = size * 0.5f;
    
    // Draw grid lines
    for (int i = 0; i <= divisions; i++) {
        float pos = -halfSize + i * spacing;
        
        // X-axis lines (red tint for center)
        Color color = (i == divisions / 2) ? (Color){100, 50, 50, 255} : (Color){50, 50, 50, 255};
        DrawLine3D((Vector3){pos, 0, -halfSize}, (Vector3){pos, 0, halfSize}, color);
        
        // Z-axis lines (blue tint for center)
        color = (i == divisions / 2) ? (Color){50, 50, 100, 255} : (Color){50, 50, 50, 255};
        DrawLine3D((Vector3){-halfSize, 0, pos}, (Vector3){halfSize, 0, pos}, color);
    }
}

void Render_Axes(float size) {
    // X-axis (Red)
    DrawLine3D((Vector3){0, 0, 0}, (Vector3){size, 0, 0}, RED);
    DrawCube((Vector3){size, 0, 0}, 0.1f, 0.1f, 0.1f, RED);
    
    // Y-axis (Green)
    DrawLine3D((Vector3){0, 0, 0}, (Vector3){0, size, 0}, GREEN);
    DrawCube((Vector3){0, size, 0}, 0.1f, 0.1f, 0.1f, GREEN);
    
    // Z-axis (Blue)
    DrawLine3D((Vector3){0, 0, 0}, (Vector3){0, 0, size}, BLUE);
    DrawCube((Vector3){0, 0, size}, 0.1f, 0.1f, 0.1f, BLUE);
}

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
    
    int y = 10;
    int lineHeight = 20;
    Color textColor = LIGHTGRAY;
    int fontSize = 10;
    
    // Engine info
    DrawText(ENGINE_NAME, 10, y, fontSize + 2, WHITE);
    y += lineHeight + 5;
    
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, y, fontSize, textColor);
    y += lineHeight;
    
    DrawText(TextFormat("Delta: %.3fms", engine->deltaTime * 1000.0f), 10, y, fontSize, textColor);
    y += lineHeight;
    
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
    DrawText(TextFormat("Entities: %d/%d", engine->entityCount, MAX_ENTITIES), 10, y, fontSize, textColor);
    y += lineHeight;
    
    int selectedCount = Entity_GetSelectedCount(engine);
    if (selectedCount > 0) {
        DrawText(TextFormat("Selected: %d", selectedCount), 10, y, fontSize, LIME);
        y += lineHeight;
    }
    
    // Control groups with active entities
    y += 5;
    bool hasGroups = false;
    for (int i = 1; i < MAX_CONTROL_GROUPS; i++) {
        if (engine->controlGroups[i].active && engine->controlGroups[i].entityCount > 0) {
            if (!hasGroups) {
                DrawText("Groups:", 10, y, fontSize, textColor);
                y += lineHeight;
                hasGroups = true;
            }
            DrawText(TextFormat("  [%d]: %d units", i, engine->controlGroups[i].entityCount), 
                    10, y, fontSize, SKYBLUE);
            y += lineHeight;
        }
    }
    
    // Controls hint
    y = engine->windowHeight - 80;
    if (engine->activeGamepad >= 0) {
        DrawText("Gamepad Camera: L-Stick/D-Pad: Move | R-Stick: Rotate | LB/LT/L3/R3: Zoom", 10, y, fontSize, DARKGRAY);
        y += lineHeight;
        DrawText("Select: Reset | Y: Switch Camera | Start: Pause", 10, y, fontSize, DARKGRAY);
    } else {
        DrawText("TAB: Switch Camera | WASD: Move | Mouse Wheel: Zoom", 10, y, fontSize, DARKGRAY);
        y += lineHeight;
        DrawText("G: Grid | X: Axes | I: Info | ESC: Exit", 10, y, fontSize, DARKGRAY);
    }
    y += lineHeight;
    
    // Show gamepad connection status
    if (engine->activeGamepad >= 0) {
        DrawText(TextFormat("Gamepad %d Connected", engine->activeGamepad + 1), 10, y, fontSize, GREEN);
    }
}