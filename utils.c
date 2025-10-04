#include "engine.h"
#include <math.h>

// =====================================
// Control Groups Implementation
// =====================================

void ControlGroup_Assign(EngineState* engine, int groupId) {
    if (!engine || groupId < 0 || groupId >= MAX_CONTROL_GROUPS) return;
    
    ControlGroup* group = &engine->controlGroups[groupId];
    group->entityCount = 0;
    group->active = false;
    
    Vector3 centerSum = {0, 0, 0};
    int count = 0;
    
    // Add selected entities to group
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (engine->entities[i].active && engine->entities[i].selected) {
            if (group->entityCount < MAX_ENTITIES) {
                // Remove from other groups
                engine->entities[i].groupId = groupId;
                group->entityIds[group->entityCount++] = engine->entities[i].id;
                
                centerSum = Vector3Add(centerSum, engine->entities[i].position);
                count++;
            }
        }
    }
    
    if (count > 0) {
        group->active = true;
        group->center = Vector3Scale(centerSum, 1.0f / count);
    }
}

void ControlGroup_Select(EngineState* engine, int groupId) {
    if (!engine || groupId < 0 || groupId >= MAX_CONTROL_GROUPS) return;
    
    ControlGroup* group = &engine->controlGroups[groupId];
    if (!group->active) return;
    
    // Clear current selection
    Entity_ClearSelection(engine);
    
    Vector3 centerSum = {0, 0, 0};
    int validCount = 0;
    
    // Select all entities in group
    for (int i = 0; i < group->entityCount; i++) {
        Entity* entity = Entity_GetById(engine, group->entityIds[i]);
        if (entity && entity->active) {
            entity->selected = true;
            centerSum = Vector3Add(centerSum, entity->position);
            validCount++;
        }
    }
    
    // Update group center and focus camera
    if (validCount > 0) {
        group->center = Vector3Scale(centerSum, 1.0f / validCount);
        
        // Move camera to group center
        if (engine->viewMode == VIEW_MODE_ISOMETRIC) {
            engine->isoCamera.targetTarget = group->center;
        } else if (engine->viewMode == VIEW_MODE_ORBIT) {
            engine->orbitCamera.target = group->center;
        }
    }
}

void ControlGroup_Clear(EngineState* engine, int groupId) {
    if (!engine || groupId < 0 || groupId >= MAX_CONTROL_GROUPS) return;
    
    ControlGroup* group = &engine->controlGroups[groupId];
    
    // Clear group assignment from entities
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (engine->entities[i].active && engine->entities[i].groupId == groupId) {
            engine->entities[i].groupId = 0;
        }
    }
    
    group->active = false;
    group->entityCount = 0;
}

Vector3 ControlGroup_GetCenter(EngineState* engine, int groupId) {
    if (!engine || groupId < 0 || groupId >= MAX_CONTROL_GROUPS) {
        return (Vector3){0, 0, 0};
    }
    
    return engine->controlGroups[groupId].center;
}

// =====================================
// Utility Functions Implementation
// =====================================

Vector3 Utils_GetGroundPosition(Vector3 worldPos) {
    // Simple ground plane at Y=0
    // This can be extended for terrain heightmaps
    return (Vector3){worldPos.x, 0, worldPos.z};
}

Vector3 Utils_ScreenToWorld(EngineState* engine, Vector2 screenPos) {
    if (!engine) return (Vector3){0, 0, 0};
    
    // Create a ray from screen position
    Ray ray = GetMouseRay(screenPos, engine->camera);
    
    // Intersect with ground plane (Y=0)
    float t = -ray.position.y / ray.direction.y;
    
    if (t > 0) {
        return Vector3Add(ray.position, Vector3Scale(ray.direction, t));
    }
    
    return (Vector3){0, 0, 0};
}

Vector2 Utils_WorldToScreen(EngineState* engine, Vector3 worldPos) {
    if (!engine) return (Vector2){0, 0};
    return GetWorldToScreen(worldPos, engine->camera);
}

bool Utils_IsPointInBox(Vector2 point, Vector2 boxStart, Vector2 boxEnd) {
    float minX = fminf(boxStart.x, boxEnd.x);
    float maxX = fmaxf(boxStart.x, boxEnd.x);
    float minY = fminf(boxStart.y, boxEnd.y);
    float maxY = fmaxf(boxStart.y, boxEnd.y);
    
    return (point.x >= minX && point.x <= maxX && 
            point.y >= minY && point.y <= maxY);
}

bool Utils_CheckCollisionSpheres(Vector3 pos1, float radius1, Vector3 pos2, float radius2) {
    float distance = Vector3Distance(pos1, pos2);
    return distance <= (radius1 + radius2);
}

bool Utils_CheckCollisionBoxes(BoundingBox box1, BoundingBox box2) {
    return (box1.min.x <= box2.max.x && box1.max.x >= box2.min.x) &&
           (box1.min.y <= box2.max.y && box1.max.y >= box2.min.y) &&
           (box1.min.z <= box2.max.z && box1.max.z >= box2.min.z);
}