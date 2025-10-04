#include "engine.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// =====================================
// Engine Core Implementation
// =====================================

EngineState* Engine_Init(int width, int height, const char* title) {
    // Suppress unused parameter warnings (using fullscreen mode instead)
    (void)width;
    (void)height;
    
    // Allocate engine state
    EngineState* engine = (EngineState*)calloc(1, sizeof(EngineState));
    if (!engine) {
        fprintf(stderr, "Failed to allocate engine state\n");
        return NULL;
    }
    
    // Initialize window first, then go fullscreen
    engine->windowTitle = title ? title : ENGINE_NAME;
    engine->windowWidth = width > 0 ? width : DEFAULT_WINDOW_WIDTH;
    engine->windowHeight = height > 0 ? height : DEFAULT_WINDOW_HEIGHT;
    
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(engine->windowWidth, engine->windowHeight, engine->windowTitle);
    SetTargetFPS(DEFAULT_FPS);
    
    // Now toggle to fullscreen after window is created
    ToggleFullscreen();
    
    // Get actual fullscreen dimensions
    if (IsWindowFullscreen()) {
        int monitor = GetCurrentMonitor();
        engine->windowWidth = GetMonitorWidth(monitor);
        engine->windowHeight = GetMonitorHeight(monitor);
    } else {
        engine->windowWidth = GetScreenWidth();
        engine->windowHeight = GetScreenHeight();
    }
    
    // Initialize low-resolution render texture
    engine->renderTarget = LoadRenderTexture(INTERNAL_RENDER_WIDTH, INTERNAL_RENDER_HEIGHT);
    SetTextureFilter(engine->renderTarget.texture, TEXTURE_FILTER_POINT); // Pixelated look
    SetTextureWrap(engine->renderTarget.texture, TEXTURE_WRAP_CLAMP);  // Prevent edge bleeding
    engine->useInternalResolution = true;  // Enable internal resolution by default
    engine->maintainAspectRatio = false;  // Start with stretched full screen
    
    // Set up source and destination rectangles for scaling
    engine->sourceRect = (Rectangle){ 0, 0, INTERNAL_RENDER_WIDTH, INTERNAL_RENDER_HEIGHT };
    
    // Set destination rectangle to fill entire screen
    engine->destRect = (Rectangle){
        0,
        0,
        (float)engine->windowWidth,
        (float)engine->windowHeight
    };
    
    // Initialize camera
    engine->camera.position = (Vector3){10.0f, 10.0f, 10.0f};
    engine->camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    engine->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    engine->camera.fovy = 60.0f;  // Adjusted for wide-screen aspect ratio
    engine->camera.projection = CAMERA_PERSPECTIVE;
    
    // Initialize camera controllers
    Camera_InitOrbit(&engine->orbitCamera, (Vector3){0, 0, 0}, 10.0f);
    Camera_InitIsometric(&engine->isoCamera, (Vector3){0, 0, 0}, 45.0f);
    
    // Set default view mode
    engine->viewMode = VIEW_MODE_ISOMETRIC;
    
    // Initialize entities
    engine->entityCount = 0;
    engine->nextEntityId = 1;
    for (int i = 0; i < MAX_ENTITIES; i++) {
        engine->entities[i].active = false;
        engine->entities[i].id = 0;
    }
    
    // Initialize control groups
    for (int i = 0; i < MAX_CONTROL_GROUPS; i++) {
        engine->controlGroups[i].active = false;
        engine->controlGroups[i].entityCount = 0;
    }
    
    // Initialize gamepad state
    engine->activeGamepad = -1;
    for (int i = 0; i < MAX_GAMEPADS; i++) {
        engine->gamepadConnected[i] = false;
        engine->gamepadLeftStick[i] = (Vector2){0, 0};
        engine->gamepadRightStick[i] = (Vector2){0, 0};
        engine->gamepadLeftTrigger[i] = 0;
        engine->gamepadRightTrigger[i] = 0;
    }
    
    // Set default display options
    engine->showDebugInfo = true;
    engine->showUI = true;
    engine->showScanlines = false;  // Scanlines off by default
    
    engine->running = true;
    
    return engine;
}

void Engine_Shutdown(EngineState* engine) {
    if (!engine) return;
    
    // Unload render texture
    if (engine->useInternalResolution) {
        UnloadRenderTexture(engine->renderTarget);
    }
    
    // Clean up entities with custom data
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (engine->entities[i].active && engine->entities[i].customData) {
            free(engine->entities[i].customData);
        }
    }
    
    CloseWindow();
    free(engine);
}

void Engine_BeginFrame(EngineState* engine) {
    if (!engine) return;
    
    // Update timing
    engine->deltaTime = GetFrameTime();
    engine->totalTime += engine->deltaTime;
    
    // Update input state
    Input_Update(engine);
    
    // Toggle fullscreen with Alt+Enter or just F11
    if ((IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER)) || IsKeyPressed(KEY_F11)) {
        ToggleFullscreen();
        
        // Wait a frame for the window to resize
        if (IsWindowFullscreen()) {
            int monitor = GetCurrentMonitor();
            engine->windowWidth = GetMonitorWidth(monitor);
            engine->windowHeight = GetMonitorHeight(monitor);
        } else {
            engine->windowWidth = DEFAULT_WINDOW_WIDTH;
            engine->windowHeight = DEFAULT_WINDOW_HEIGHT;
        }
        
        // Recalculate destination rectangle based on aspect ratio mode
        if (engine->maintainAspectRatio) {
            float scale = fminf((float)engine->windowWidth / INTERNAL_RENDER_WIDTH,
                               (float)engine->windowHeight / INTERNAL_RENDER_HEIGHT);
            float scaledWidth = INTERNAL_RENDER_WIDTH * scale;
            float scaledHeight = INTERNAL_RENDER_HEIGHT * scale;
            engine->destRect = (Rectangle){
                (engine->windowWidth - scaledWidth) / 2,
                (engine->windowHeight - scaledHeight) / 2,
                scaledWidth,
                scaledHeight
            };
        } else {
            engine->destRect = (Rectangle){
                0,
                0,
                (float)engine->windowWidth,
                (float)engine->windowHeight
            };
        }
    }
    
    // Toggle internal resolution with F1
    if (IsKeyPressed(KEY_F1)) {
        engine->useInternalResolution = !engine->useInternalResolution;
        
        // Update destination rectangle for new window size in case it changed
        if (engine->useInternalResolution) {
            // Get current screen dimensions
            int currentWidth = GetScreenWidth();
            int currentHeight = GetScreenHeight();
            
            // Recalculate destination rectangle based on aspect ratio mode
            if (engine->maintainAspectRatio) {
                float scale = fminf((float)currentWidth / INTERNAL_RENDER_WIDTH,
                                   (float)currentHeight / INTERNAL_RENDER_HEIGHT);
                float scaledWidth = INTERNAL_RENDER_WIDTH * scale;
                float scaledHeight = INTERNAL_RENDER_HEIGHT * scale;
                engine->destRect = (Rectangle){
                    (currentWidth - scaledWidth) / 2,
                    (currentHeight - scaledHeight) / 2,
                    scaledWidth,
                    scaledHeight
                };
            } else {
                engine->destRect = (Rectangle){
                    0,
                    0,
                    (float)currentWidth,
                    (float)currentHeight
                };
            }
            
            engine->windowWidth = currentWidth;
            engine->windowHeight = currentHeight;
        }
    }
    
    // Toggle scanline effect with F2
    if (IsKeyPressed(KEY_F2)) {
        engine->showScanlines = !engine->showScanlines;
    }
    
    // Toggle aspect ratio mode with F3
    if (IsKeyPressed(KEY_F3)) {
        engine->maintainAspectRatio = !engine->maintainAspectRatio;
        
        // Recalculate destination rectangle based on aspect ratio mode
        if (engine->maintainAspectRatio) {
            // Calculate destination rectangle to maintain aspect ratio
            float scale = fminf((float)engine->windowWidth / INTERNAL_RENDER_WIDTH,
                               (float)engine->windowHeight / INTERNAL_RENDER_HEIGHT);
            float scaledWidth = INTERNAL_RENDER_WIDTH * scale;
            float scaledHeight = INTERNAL_RENDER_HEIGHT * scale;
            engine->destRect = (Rectangle){
                (engine->windowWidth - scaledWidth) / 2,
                (engine->windowHeight - scaledHeight) / 2,
                scaledWidth,
                scaledHeight
            };
        } else {
            // Stretch to fill entire screen
            engine->destRect = (Rectangle){
                0,
                0,
                (float)engine->windowWidth,
                (float)engine->windowHeight
            };
        }
    }
    
    // Update camera based on current mode
    switch (engine->viewMode) {
        case VIEW_MODE_ORBIT:
            Camera_UpdateOrbit(engine);
            break;
        case VIEW_MODE_ISOMETRIC:
            Camera_UpdateIsometric(engine);
            break;
        default:
            break;
    }
    
    // Apply camera settings
    Camera_Apply(engine);
    
    // Begin drawing
    if (engine->useInternalResolution) {
        // Begin drawing to render texture
        BeginTextureMode(engine->renderTarget);
        ClearBackground((Color){32, 32, 32, 255});
        BeginMode3D(engine->camera);
    } else {
        // Normal drawing
        BeginDrawing();
        ClearBackground((Color){32, 32, 32, 255});
        BeginMode3D(engine->camera);
    }
}

void Engine_End3D(EngineState* engine) {
    if (!engine) return;
    
    // End 3D mode
    EndMode3D();
}

void Engine_EndFrame(EngineState* engine) {
    if (!engine) return;
    
    // Render 2D UI elements (to render texture if using internal resolution)
    if (engine->isoCamera.selecting) {
        Render_SelectionBox(engine->isoCamera.selectionStart, engine->isoCamera.selectionEnd);
    }
    
    if (engine->showDebugInfo) {
        Render_DebugInfo(engine);
    }
    
    if (engine->useInternalResolution) {
        // End render texture mode
        EndTextureMode();
        
        // Now draw the render texture to the actual screen
        BeginDrawing();
        ClearBackground(BLACK);  // Black letterboxing
        
        // Draw the render texture scaled up (flip Y coordinate for correct orientation)
        Rectangle flippedSource = { 0, 0, INTERNAL_RENDER_WIDTH, -INTERNAL_RENDER_HEIGHT };
        DrawTexturePro(engine->renderTarget.texture,
                      flippedSource,
                      engine->destRect,
                      (Vector2){0, 0},
                      0.0f,
                      WHITE);
        
        // Optional: Add scanline effect for retro CRT look
        if (engine->useInternalResolution && engine->showScanlines) {
            for (int y = 0; y < engine->windowHeight; y += 2) {
                DrawRectangle(0, y, engine->windowWidth, 1, (Color){0, 0, 0, 30});
            }
        }
        
        EndDrawing();
    } else {
        // Normal ending
        EndDrawing();
    }
}

bool Engine_ShouldClose(EngineState* engine) {
    return !engine || !engine->running || WindowShouldClose();
}

// =====================================
// Entity Management
// =====================================

Entity* Entity_Create(EngineState* engine, EntityType type) {
    if (!engine || engine->entityCount >= MAX_ENTITIES) {
        return NULL;
    }
    
    // Find first inactive entity slot
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!engine->entities[i].active) {
            Entity* entity = &engine->entities[i];
            
            // Initialize entity
            memset(entity, 0, sizeof(Entity));
            entity->id = engine->nextEntityId++;
            entity->type = type;
            entity->active = true;
            entity->scale = (Vector3){1.0f, 1.0f, 1.0f};
            entity->color = WHITE;
            entity->health = 100.0f;
            entity->maxHealth = 100.0f;
            entity->mass = 1.0f;
            
            engine->entityCount++;
            return entity;
        }
    }
    
    return NULL;
}

void Entity_Destroy(EngineState* engine, int entityId) {
    if (!engine) return;
    
    Entity* entity = Entity_GetById(engine, entityId);
    if (entity) {
        if (entity->customData) {
            free(entity->customData);
        }
        entity->active = false;
        entity->id = 0;
        engine->entityCount--;
    }
}

Entity* Entity_GetById(EngineState* engine, int entityId) {
    if (!engine || entityId <= 0) return NULL;
    
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (engine->entities[i].active && engine->entities[i].id == entityId) {
            return &engine->entities[i];
        }
    }
    
    return NULL;
}

void Entity_Update(EngineState* engine, Entity* entity) {
    if (!engine || !entity || !entity->active) return;
    
    float dt = engine->deltaTime;
    
    // Basic physics integration
    entity->velocity = Vector3Add(entity->velocity, Vector3Scale(entity->acceleration, dt));
    entity->position = Vector3Add(entity->position, Vector3Scale(entity->velocity, dt));
    
    // Apply damping
    entity->velocity = Vector3Scale(entity->velocity, 0.98f);
}

void Entity_Render(Entity* entity) {
    if (!entity || !entity->active) return;
    
    // Default rendering as a cube if no model
    if (!entity->model) {
        DrawCube(entity->position, entity->scale.x, entity->scale.y, entity->scale.z, entity->color);
        
        if (entity->selected) {
            DrawCubeWires(entity->position, 
                         entity->scale.x * 1.1f, 
                         entity->scale.y * 1.1f, 
                         entity->scale.z * 1.1f, 
                         GREEN);
        }
    } else {
        // Draw the model if available
        DrawModel(*entity->model, entity->position, entity->scale.x, entity->color);
    }
}

void Entity_Select(Entity* entity, bool selected) {
    if (entity) {
        entity->selected = selected;
    }
}

void Entity_ClearSelection(EngineState* engine) {
    if (!engine) return;
    
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (engine->entities[i].active) {
            engine->entities[i].selected = false;
        }
    }
}

int Entity_GetSelectedCount(EngineState* engine) {
    if (!engine) return 0;
    
    int count = 0;
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (engine->entities[i].active && engine->entities[i].selected) {
            count++;
        }
    }
    return count;
}

// Entity selection in screen space
void Entity_SelectInBox(EngineState* engine, Vector2 start, Vector2 end) {
    if (!engine) return;
    
    // Clear previous selection
    Entity_ClearSelection(engine);
    
    // Normalize box coordinates
    float minX = fminf(start.x, end.x);
    float maxX = fmaxf(start.x, end.x);
    float minY = fminf(start.y, end.y);
    float maxY = fmaxf(start.y, end.y);
    
    // Select entities within box
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!engine->entities[i].active) continue;
        
        Entity* entity = &engine->entities[i];
        Vector2 screenPos = GetWorldToScreen(entity->position, engine->camera);
        
        if (screenPos.x >= minX && screenPos.x <= maxX &&
            screenPos.y >= minY && screenPos.y <= maxY) {
            entity->selected = true;
        }
    }
}