#include "engine.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// =====================================
// Engine Core Implementation
// =====================================

EngineState* Engine_Init(int width, int height, const char* title) {
    // Allocate engine state
    EngineState* engine = (EngineState*)calloc(1, sizeof(EngineState));
    if (!engine) {
        fprintf(stderr, "Failed to allocate engine state\n");
        return NULL;
    }
    
    // Initialize window
    engine->windowWidth = width > 0 ? width : DEFAULT_WINDOW_WIDTH;
    engine->windowHeight = height > 0 ? height : DEFAULT_WINDOW_HEIGHT;
    engine->windowTitle = title ? title : ENGINE_NAME;
    
    InitWindow(engine->windowWidth, engine->windowHeight, engine->windowTitle);
    SetTargetFPS(DEFAULT_FPS);
    
    // Initialize camera
    engine->camera.position = (Vector3){10.0f, 10.0f, 10.0f};
    engine->camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    engine->camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    engine->camera.fovy = 45.0f;
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
    
    engine->running = true;
    
    return engine;
}

void Engine_Shutdown(EngineState* engine) {
    if (!engine) return;
    
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
    BeginDrawing();
    ClearBackground((Color){32, 32, 32, 255});
    
    BeginMode3D(engine->camera);
}

void Engine_EndFrame(EngineState* engine) {
    if (!engine) return;
    
    // Render debug visuals
    
    EndMode3D();
    
    // Render 2D UI elements
    if (engine->isoCamera.selecting) {
        Render_SelectionBox(engine->isoCamera.selectionStart, engine->isoCamera.selectionEnd);
    }
    
    if (engine->showDebugInfo) {
        Render_DebugInfo(engine);
    }
    
    EndDrawing();
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