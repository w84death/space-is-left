#include "engine.h"
#include <math.h>

// =====================================
// Camera System Implementation
// =====================================

void Camera_InitOrbit(OrbitCamera* cam, Vector3 target, float distance) {
    if (!cam) return;
    
    cam->target = target;
    cam->distance = distance;
    cam->rotationH = PI * 0.25f;
    cam->rotationV = PI * 0.15f;
}

void Camera_InitIsometric(IsometricCamera* cam, Vector3 target, float height) {
    if (!cam) return;
    
    cam->target = target;
    cam->targetTarget = target;
    cam->height = height;
    cam->angle = ISO_CAMERA_ANGLE;
    
    // Calculate position based on isometric angle
    float angleRad = cam->angle * DEG2RAD;
    cam->position.x = cam->target.x;
    cam->position.y = cam->height;
    cam->position.z = cam->target.z + cam->height / tanf(angleRad);
    cam->targetPosition = cam->position;
    
    cam->selecting = false;
}

void Camera_UpdateOrbit(EngineState* engine) {
    if (!engine) return;
    
    OrbitCamera* cam = &engine->orbitCamera;
    
    // Mouse rotation
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !IsKeyDown(KEY_LEFT_ALT)) {
        Vector2 mouseDelta = GetMouseDelta();
        cam->rotationH -= mouseDelta.x * CAMERA_MOUSE_SENSITIVITY;
        cam->rotationV -= mouseDelta.y * CAMERA_MOUSE_SENSITIVITY;
        
        // Clamp vertical rotation
        if (cam->rotationV < 0.01f) cam->rotationV = 0.01f;
        if (cam->rotationV > PI - 0.01f) cam->rotationV = PI - 0.01f;
    }
    
    // Mouse pan
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 mouseDelta = GetMouseDelta();
        
        Vector3 forward = Vector3Normalize(Vector3Subtract(engine->camera.target, engine->camera.position));
        Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, (Vector3){0, 1, 0}));
        Vector3 up = Vector3CrossProduct(right, forward);
        
        cam->target = Vector3Add(cam->target, Vector3Scale(right, -mouseDelta.x * 0.01f * cam->distance));
        cam->target = Vector3Add(cam->target, Vector3Scale(up, mouseDelta.y * 0.01f * cam->distance));
    }
    
    // Mouse zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        cam->distance -= wheel * cam->distance * CAMERA_ZOOM_SPEED;
        if (cam->distance < CAMERA_MIN_DISTANCE) cam->distance = CAMERA_MIN_DISTANCE;
        if (cam->distance > CAMERA_MAX_DISTANCE) cam->distance = CAMERA_MAX_DISTANCE;
    }
    
    // Reset camera
    if (IsKeyPressed(KEY_R)) {
        cam->rotationH = PI * 0.25f;
        cam->rotationV = PI * 0.15f;
        cam->distance = 10.0f;
        cam->target = (Vector3){0, 0, 0};
    }
    
    // Calculate camera position
    float x = cam->distance * sinf(cam->rotationV) * cosf(cam->rotationH);
    float y = cam->distance * cosf(cam->rotationV);
    float z = cam->distance * sinf(cam->rotationV) * sinf(cam->rotationH);
    
    engine->camera.position = Vector3Add(cam->target, (Vector3){x, y, z});
    engine->camera.target = cam->target;
}

void Camera_UpdateIsometric(EngineState* engine) {
    if (!engine) return;
    
    IsometricCamera* cam = &engine->isoCamera;
    float dt = engine->deltaTime;
    
    // Keyboard panning
    Vector3 moveDir = {0, 0, 0};
    
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) moveDir.z -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) moveDir.z += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) moveDir.x += 1.0f;
    
    // Edge scrolling
    Vector2 mousePos = GetMousePosition();
    if (mousePos.x < CAMERA_EDGE_SCROLL_ZONE) moveDir.x -= 1.0f;
    if (mousePos.x > engine->windowWidth - CAMERA_EDGE_SCROLL_ZONE) moveDir.x += 1.0f;
    if (mousePos.y < CAMERA_EDGE_SCROLL_ZONE) moveDir.z -= 1.0f;
    if (mousePos.y > engine->windowHeight - CAMERA_EDGE_SCROLL_ZONE) moveDir.z += 1.0f;
    
    // Normalize and apply movement
    if (Vector3Length(moveDir) > 0) {
        moveDir = Vector3Normalize(moveDir);
        float speed = IsKeyDown(KEY_LEFT_SHIFT) ? CAMERA_PAN_SPEED * 2.0f : CAMERA_PAN_SPEED;
        cam->targetTarget = Vector3Add(cam->targetTarget, Vector3Scale(moveDir, speed * dt));
    }
    
    // Middle mouse pan
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 mouseDelta = GetMouseDelta();
        cam->targetTarget.x -= mouseDelta.x * 0.02f * cam->height;
        cam->targetTarget.z -= mouseDelta.y * 0.02f * cam->height;
    }
    
    // Mouse zoom (adjust height)
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        cam->height -= wheel * ISO_CAMERA_ZOOM_SPEED;
        if (cam->height < ISO_CAMERA_MIN_HEIGHT) cam->height = ISO_CAMERA_MIN_HEIGHT;
        if (cam->height > ISO_CAMERA_MAX_HEIGHT) cam->height = ISO_CAMERA_MAX_HEIGHT;
    }
    
    // Selection box
    if (engine->viewMode == VIEW_MODE_ISOMETRIC) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            cam->selecting = true;
            cam->selectionStart = GetMousePosition();
            cam->selectionEnd = cam->selectionStart;
        }
        
        if (cam->selecting) {
            cam->selectionEnd = GetMousePosition();
            
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                // Perform selection
                Entity_SelectInBox(engine, cam->selectionStart, cam->selectionEnd);
                cam->selecting = false;
            }
        }
    }
    
    // Reset camera
    if (IsKeyPressed(KEY_R)) {
        cam->targetTarget = (Vector3){0, 0, 0};
        cam->height = 15.0f;
    }
    
    // Smooth camera movement
    cam->target = Vector3Lerp(cam->target, cam->targetTarget, CAMERA_SMOOTHING);
    
    // Update camera position based on isometric angle
    float angleRad = cam->angle * DEG2RAD;
    cam->targetPosition.x = cam->target.x;
    cam->targetPosition.y = cam->height;
    cam->targetPosition.z = cam->target.z + cam->height / tanf(angleRad);
    
    cam->position = Vector3Lerp(cam->position, cam->targetPosition, CAMERA_SMOOTHING);
    
    // Apply to engine camera
    engine->camera.position = cam->position;
    engine->camera.target = cam->target;
}

void Camera_SetMode(EngineState* engine, ViewMode mode) {
    if (!engine) return;
    
    engine->viewMode = mode;
    
    // Reset camera positions when switching modes
    switch (mode) {
        case VIEW_MODE_ORBIT:
            engine->orbitCamera.target = engine->isoCamera.target;
            break;
        case VIEW_MODE_ISOMETRIC:
            engine->isoCamera.target = engine->orbitCamera.target;
            engine->isoCamera.targetTarget = engine->orbitCamera.target;
            break;
        default:
            break;
    }
}

void Camera_Apply(EngineState* engine) {
    if (!engine) return;
    
    // Camera settings are already applied in the update functions
    // This function can be used for additional camera effects or post-processing
}