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
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector2 mouseDelta = GetMouseDelta();
        cam->rotationH += mouseDelta.x * CAMERA_MOUSE_SENSITIVITY;
        cam->rotationV -= mouseDelta.y * CAMERA_MOUSE_SENSITIVITY;
        
        // Clamp vertical rotation
        if (cam->rotationV < 0.1f) cam->rotationV = 0.1f;
        if (cam->rotationV > PI - 0.1f) cam->rotationV = PI - 0.1f;
    }
    
    // Gamepad rotation (right stick)
    if (engine->activeGamepad >= 0) {
        Vector2 rightStick = engine->gamepadRightStick[engine->activeGamepad];
        if (fabsf(rightStick.x) > 0.1f || fabsf(rightStick.y) > 0.1f) {
            cam->rotationH += rightStick.x * CAMERA_MOUSE_SENSITIVITY * 60.0f * engine->deltaTime;
            cam->rotationV += rightStick.y * CAMERA_MOUSE_SENSITIVITY * 60.0f * engine->deltaTime;
            
            // Clamp vertical rotation
            if (cam->rotationV < 0.1f) cam->rotationV = 0.1f;
            if (cam->rotationV > PI - 0.1f) cam->rotationV = PI - 0.1f;
        }
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
    
    // Gamepad pan (left stick or D-pad)
    if (engine->activeGamepad >= 0) {
        Vector2 leftStick = engine->gamepadLeftStick[engine->activeGamepad];
        Vector2 panInput = {0, 0};
        
        // Left stick
        if (fabsf(leftStick.x) > 0.1f || fabsf(leftStick.y) > 0.1f) {
            panInput.x = leftStick.x;
            panInput.y = leftStick.y;
        }
        
        // D-pad
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) panInput.x -= 1.0f;
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) panInput.x += 1.0f;
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) panInput.y -= 1.0f;
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) panInput.y += 1.0f;
        
        if (panInput.x != 0 || panInput.y != 0) {
            Vector3 forward = Vector3Normalize(Vector3Subtract(engine->camera.target, engine->camera.position));
            Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, (Vector3){0, 1, 0}));
            Vector3 up = Vector3CrossProduct(right, forward);
            
            float panSpeed = cam->distance * 0.5f * engine->deltaTime;
            cam->target = Vector3Add(cam->target, Vector3Scale(right, panInput.x * panSpeed));
            cam->target = Vector3Add(cam->target, Vector3Scale(up, -panInput.y * panSpeed));
        }
    }
    
    // Mouse zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        cam->distance -= wheel * cam->distance * CAMERA_ZOOM_SPEED;
        if (cam->distance < CAMERA_MIN_DISTANCE) cam->distance = CAMERA_MIN_DISTANCE;
        if (cam->distance > CAMERA_MAX_DISTANCE) cam->distance = CAMERA_MAX_DISTANCE;
    }
    
    // Gamepad zoom (left bumper and left trigger only, or stick buttons)
    if (engine->activeGamepad >= 0) {
        float zoomInput = 0;
        
        // Use left bumper for zoom in, left trigger for zoom out (avoid right side which is used for steering)
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) zoomInput += 1.0f;
        
        // Use left trigger for zoom out (analog)
        float leftTrigger = engine->gamepadLeftTrigger[engine->activeGamepad];
        if (leftTrigger > 0.1f) zoomInput -= leftTrigger;
        
        // Alternative: Use stick buttons for zoom
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_THUMB)) zoomInput += 1.0f;   // L3 zoom in
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_RIGHT_THUMB)) zoomInput -= 1.0f;  // R3 zoom out
        
        if (zoomInput != 0) {
            cam->distance -= zoomInput * cam->distance * CAMERA_ZOOM_SPEED * 3.0f * engine->deltaTime;
            if (cam->distance < CAMERA_MIN_DISTANCE) cam->distance = CAMERA_MIN_DISTANCE;
            if (cam->distance > CAMERA_MAX_DISTANCE) cam->distance = CAMERA_MAX_DISTANCE;
        }
    }
    
    // Reset camera (R key or gamepad Select button)
    if (IsKeyPressed(KEY_R) || 
        (engine->activeGamepad >= 0 && IsGamepadButtonPressed(engine->activeGamepad, GAMEPAD_BUTTON_MIDDLE_LEFT))) {
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
    
    // Gamepad panning (left stick and D-pad)
    if (engine->activeGamepad >= 0) {
        Vector2 leftStick = engine->gamepadLeftStick[engine->activeGamepad];
        
        // Left stick
        if (fabsf(leftStick.x) > 0.1f) moveDir.x += leftStick.x;
        if (fabsf(leftStick.y) > 0.1f) moveDir.z += leftStick.y;
        
        // D-pad
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) moveDir.x -= 1.0f;
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) moveDir.x += 1.0f;
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) moveDir.z -= 1.0f;
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) moveDir.z += 1.0f;
    }
    
    // Edge scrolling
    Vector2 mousePos = GetMousePosition();
    if (mousePos.x < CAMERA_EDGE_SCROLL_ZONE) moveDir.x -= 1.0f;
    if (mousePos.x > engine->windowWidth - CAMERA_EDGE_SCROLL_ZONE) moveDir.x += 1.0f;
    if (mousePos.y < CAMERA_EDGE_SCROLL_ZONE) moveDir.z -= 1.0f;
    if (mousePos.y > engine->windowHeight - CAMERA_EDGE_SCROLL_ZONE) moveDir.z += 1.0f;
    
    // Normalize and apply movement
    if (Vector3Length(moveDir) > 0) {
        moveDir = Vector3Normalize(moveDir);
        // Speed boost with shift key or gamepad left bumper
        bool speedBoost = IsKeyDown(KEY_LEFT_SHIFT) || 
                         (engine->activeGamepad >= 0 && IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1));
        float speed = speedBoost ? CAMERA_PAN_SPEED * 2.0f : CAMERA_PAN_SPEED;
        cam->targetTarget = Vector3Add(cam->targetTarget, Vector3Scale(moveDir, speed * dt));
    }
    
    // Gamepad pan with right stick (alternative control scheme)
    if (engine->activeGamepad >= 0) {
        Vector2 rightStick = engine->gamepadRightStick[engine->activeGamepad];
        if (fabsf(rightStick.x) > 0.1f || fabsf(rightStick.y) > 0.1f) {
            float panSpeed = cam->height * 0.5f * dt;
            cam->targetTarget.x += rightStick.x * panSpeed;
            cam->targetTarget.z += rightStick.y * panSpeed;
        }
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
    
    // Gamepad zoom (left bumper and left trigger only, or stick buttons)
    if (engine->activeGamepad >= 0) {
        float zoomInput = 0;
        
        // Use left bumper for zoom in (decrease height)
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) zoomInput -= 1.0f;
        
        // Use left trigger for zoom out (increase height) - analog
        float leftTrigger = engine->gamepadLeftTrigger[engine->activeGamepad];
        if (leftTrigger > 0.1f) zoomInput += leftTrigger;
        
        // Alternative: Use stick buttons for zoom
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_THUMB)) zoomInput -= 1.0f;   // L3 zoom in
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_RIGHT_THUMB)) zoomInput += 1.0f;  // R3 zoom out
        
        if (zoomInput != 0) {
            cam->height += zoomInput * ISO_CAMERA_ZOOM_SPEED * 10.0f * dt;
            if (cam->height < ISO_CAMERA_MIN_HEIGHT) cam->height = ISO_CAMERA_MIN_HEIGHT;
            if (cam->height > ISO_CAMERA_MAX_HEIGHT) cam->height = ISO_CAMERA_MAX_HEIGHT;
        }
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
    
    // Reset camera (R key or gamepad Select button)
    if (IsKeyPressed(KEY_R) || 
        (engine->activeGamepad >= 0 && IsGamepadButtonPressed(engine->activeGamepad, GAMEPAD_BUTTON_MIDDLE_LEFT))) {
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