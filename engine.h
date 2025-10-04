#ifndef SPACE_IS_LEFT_ENGINE_H
#define SPACE_IS_LEFT_ENGINE_H

#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>

// =====================================
// Engine Configuration
// =====================================

#define ENGINE_VERSION "1.0.0"
#define ENGINE_NAME "Space is Left Engine"

// Window settings
#define DEFAULT_WINDOW_WIDTH 1920
#define DEFAULT_WINDOW_HEIGHT 1080
#define DEFAULT_FPS 60

// Internal rendering resolution options for different aspect ratios
// 16:9 aspect ratio (most common for modern monitors)
#define INTERNAL_RENDER_WIDTH_16_9 640
#define INTERNAL_RENDER_HEIGHT_16_9 360

// 16:10 aspect ratio
#define INTERNAL_RENDER_WIDTH_16_10 640
#define INTERNAL_RENDER_HEIGHT_16_10 400

// 4:3 aspect ratio (older monitors)
#define INTERNAL_RENDER_WIDTH_4_3 640
#define INTERNAL_RENDER_HEIGHT_4_3 480

// 21:9 ultrawide
#define INTERNAL_RENDER_WIDTH_21_9 840
#define INTERNAL_RENDER_HEIGHT_21_9 360

// Default internal resolution (will be set dynamically)
#define INTERNAL_RENDER_WIDTH 640
#define INTERNAL_RENDER_HEIGHT 360

// Camera settings
#define CAMERA_MOUSE_SENSITIVITY 0.003f
#define CAMERA_ZOOM_SPEED 0.1f
#define CAMERA_MIN_DISTANCE 1.0f
#define CAMERA_MAX_DISTANCE 100.0f
#define CAMERA_PAN_SPEED 10.0f
#define CAMERA_EDGE_SCROLL_ZONE 20
#define CAMERA_EDGE_SCROLL_SPEED 8.0f
#define CAMERA_SMOOTHING 0.15f

// Isometric camera
#define ISO_CAMERA_ANGLE 45.0f
#define ISO_CAMERA_MIN_HEIGHT 10.0f
#define ISO_CAMERA_MAX_HEIGHT 100.0f
#define ISO_CAMERA_ZOOM_SPEED 3.0f

// Entity system
#define MAX_ENTITIES 1000
#define MAX_CONTROL_GROUPS 10

// Gamepad settings
#define MAX_GAMEPADS 4
#define GAMEPAD_DEAD_ZONE 0.15f
#define GAMEPAD_TRIGGER_THRESHOLD 0.1f

// =====================================
// Type Definitions
// =====================================

// Camera view modes
typedef enum {
    VIEW_MODE_ORBIT,
    VIEW_MODE_ISOMETRIC,
    VIEW_MODE_FIRST_PERSON,
    VIEW_MODE_THIRD_PERSON
} ViewMode;

// Entity types for game objects
typedef enum {
    ENTITY_TYPE_NONE,
    ENTITY_TYPE_UNIT,
    ENTITY_TYPE_BUILDING,
    ENTITY_TYPE_PROJECTILE,
    ENTITY_TYPE_EFFECT,
    ENTITY_TYPE_TERRAIN,
    ENTITY_TYPE_CUSTOM
} EntityType;

// Orbit camera for 3D viewing
typedef struct {
    float distance;
    float rotationH;  // Horizontal rotation (around Y axis)
    float rotationV;  // Vertical rotation (pitch)
    Vector3 target;
} OrbitCamera;

// Isometric camera for strategy games
typedef struct {
    Vector3 position;
    Vector3 target;
    float height;
    float angle;
    Vector3 targetPosition;  // For smooth movement
    Vector3 targetTarget;    // For smooth panning
    bool selecting;          // Selection box state
    Vector2 selectionStart;
    Vector2 selectionEnd;
} IsometricCamera;

// Universal entity structure
typedef struct Entity {
    int id;
    EntityType type;
    bool active;
    bool selected;

    // Transform
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;

    // Physics
    Vector3 velocity;
    Vector3 acceleration;
    float mass;

    // Visual
    Color color;
    Model* model;  // Optional 3D model
    Texture2D* texture;  // Optional texture

    // Gameplay
    float health;
    float maxHealth;
    int team;
    int groupId;  // Control group

    // Custom data pointer for game-specific data
    void* customData;
} Entity;

// Control group for RTS-style games
typedef struct {
    int entityIds[MAX_ENTITIES];
    int entityCount;
    bool active;
    Vector3 center;
} ControlGroup;

// Engine state
typedef struct {
    // Window
    int windowWidth;
    int windowHeight;
    const char* windowTitle;

    // Dynamic resolution settings
    int internalWidth;     // Actual internal render width based on aspect ratio
    int internalHeight;    // Actual internal render height based on aspect ratio
    float monitorAspectRatio;  // Detected monitor aspect ratio

    // Camera
    Camera3D camera;
    ViewMode viewMode;
    OrbitCamera orbitCamera;
    IsometricCamera isoCamera;

    // Entities
    Entity entities[MAX_ENTITIES];
    int entityCount;
    int nextEntityId;

    // Control groups
    ControlGroup controlGroups[MAX_CONTROL_GROUPS];

    // Input state
    bool mouseLeftPressed;
    bool mouseRightPressed;
    bool mouseMiddlePressed;
    Vector2 mousePosition;
    Vector2 mouseDelta;
    float mouseWheel;

    // Gamepad state
    int activeGamepad;  // Currently active gamepad ID (-1 if none)
    bool gamepadConnected[MAX_GAMEPADS];
    Vector2 gamepadLeftStick[MAX_GAMEPADS];
    Vector2 gamepadRightStick[MAX_GAMEPADS];
    float gamepadLeftTrigger[MAX_GAMEPADS];
    float gamepadRightTrigger[MAX_GAMEPADS];

    // Engine state
    bool running;
    float deltaTime;
    float totalTime;

    // Debug/display options
    bool showDebugInfo;
    bool showUI;

    // Low resolution rendering
    RenderTexture2D renderTarget;  // Internal render texture at low resolution
    bool useInternalResolution;    // Whether to use internal resolution rendering
    bool showScanlines;            // Whether to show CRT scanline effect
    bool maintainAspectRatio;      // Whether to maintain aspect ratio (letterbox) or stretch to fill
    Rectangle sourceRect;           // Source rectangle for render texture
    Rectangle destRect;             // Destination rectangle for fullscreen
} EngineState;

// =====================================
// Engine Core Functions
// =====================================

// Initialize and shutdown
EngineState* Engine_Init(int width, int height, const char* title);
void Engine_Shutdown(EngineState* engine);

// Main loop
void Engine_BeginFrame(EngineState* engine);
void Engine_End3D(EngineState* engine);  // End 3D mode, begin 2D UI rendering
void Engine_EndFrame(EngineState* engine);
bool Engine_ShouldClose(EngineState* engine);

// =====================================
// Camera Functions
// =====================================

void Camera_InitOrbit(OrbitCamera* cam, Vector3 target, float distance);
void Camera_InitIsometric(IsometricCamera* cam, Vector3 target, float height);
void Camera_UpdateOrbit(EngineState* engine);
void Camera_UpdateIsometric(EngineState* engine);
void Camera_SetMode(EngineState* engine, ViewMode mode);
void Camera_Apply(EngineState* engine);

// =====================================
// Entity Management
// =====================================

Entity* Entity_Create(EngineState* engine, EntityType type);
void Entity_Destroy(EngineState* engine, int entityId);
Entity* Entity_GetById(EngineState* engine, int entityId);
void Entity_Update(EngineState* engine, Entity* entity);
void Entity_Render(Entity* entity);

// Selection and control
void Entity_Select(Entity* entity, bool selected);
void Entity_SelectInBox(EngineState* engine, Vector2 start, Vector2 end);
void Entity_ClearSelection(EngineState* engine);
int Entity_GetSelectedCount(EngineState* engine);

// Control groups
void ControlGroup_Assign(EngineState* engine, int groupId);
void ControlGroup_Select(EngineState* engine, int groupId);
void ControlGroup_Clear(EngineState* engine, int groupId);
Vector3 ControlGroup_GetCenter(EngineState* engine, int groupId);

// =====================================
// Input Functions
// =====================================

void Input_Update(EngineState* engine);
bool Input_IsKeyPressed(int key);
bool Input_IsKeyDown(int key);
bool Input_IsKeyReleased(int key);
bool Input_IsMouseButtonPressed(int button);
bool Input_IsMouseButtonDown(int button);
bool Input_IsMouseButtonReleased(int button);
Vector2 Input_GetMousePosition(void);
Vector2 Input_GetMouseDelta(void);
float Input_GetMouseWheel(void);

// Gamepad functions
bool Input_IsGamepadAvailable(int gamepad);
bool Input_IsGamepadButtonPressed(int gamepad, int button);
bool Input_IsGamepadButtonDown(int gamepad, int button);
bool Input_IsGamepadButtonReleased(int gamepad, int button);
Vector2 Input_GetGamepadLeftStick(int gamepad);
Vector2 Input_GetGamepadRightStick(int gamepad);
float Input_GetGamepadLeftTrigger(int gamepad);
float Input_GetGamepadRightTrigger(int gamepad);
int Input_GetActiveGamepad(EngineState* engine);
void Input_UpdateGamepads(EngineState* engine);

// =====================================
// Rendering Functions
// =====================================

void Render_SelectionBox(Vector2 start, Vector2 end);
void Render_DebugInfo(EngineState* engine);

// =====================================
// Utility Functions
// =====================================

// 3D math utilities
Vector3 Utils_GetGroundPosition(Vector3 worldPos);
Vector3 Utils_ScreenToWorld(EngineState* engine, Vector2 screenPos);
Vector2 Utils_WorldToScreen(EngineState* engine, Vector3 worldPos);
bool Utils_IsPointInBox(Vector2 point, Vector2 boxStart, Vector2 boxEnd);

// Collision detection
bool Utils_CheckCollisionSpheres(Vector3 pos1, float radius1, Vector3 pos2, float radius2);
bool Utils_CheckCollisionBoxes(BoundingBox box1, BoundingBox box2);

// Resolution utilities
void Utils_SelectInternalResolution(EngineState* engine, int monitorWidth, int monitorHeight);

#endif // SPACE_IS_LEFT_ENGINE_H
