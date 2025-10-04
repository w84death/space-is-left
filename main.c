#include "engine.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

// =====================================
// Space is Left - Game Constants
// =====================================

#define GAME_TITLE "Space is Left"
#define GAME_VERSION "1.0.0"

// Game settings
#define ARENA_SIZE 100.0f
#define INITIAL_SEGMENTS 5
#define MAX_SEGMENTS 500
#define SEGMENT_SIZE 0.8f
#define SEGMENT_SPACING 1.0f
#define LINE_RIDER_SPEED 12.0f
#define TURN_SPEED 2.8f  // Radians per second (left only!)
#define ENERGY_DRAIN_RATE 1.5f  // Energy per second
#define MAX_ENERGY 100.0f
#define ENERGY_BAR_VALUE 20.0f
#define POWERUP_LIFETIME 30.0f

// Visual settings
#define TRAIL_GLOW_SIZE 1.2f
#define SEGMENT_HEIGHT 0.5f
#define ENERGY_BAR_SIZE 1.0f
#define POWERUP_SIZE 0.8f
#define PARTICLE_COUNT 100
#define STAR_COUNT 200
#define HARDCORE_SPEED_MULTI 2.0f

// Powerup types
typedef enum {
    POWERUP_ENERGY,
    POWERUP_SPEED_BOOST,
    POWERUP_SLOW_TIME,
    POWERUP_SHIELD,
    POWERUP_SHRINK,
    POWERUP_BONUS_POINTS,
    POWERUP_TYPE_COUNT
} PowerupType;

// Difficulty levels
typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_HARDCORE
} DifficultyLevel;

// =====================================
// Game Structures
// =====================================

typedef struct {
    Vector3 position;
    Vector3 previousPos;  // For smooth interpolation
    float angle;          // Current rotation angle
    Color color;
    float glowIntensity;
    bool isHead;
} LineSegment;

typedef struct {
    LineSegment segments[MAX_SEGMENTS];
    int segmentCount;
    float direction;      // Current direction in radians
    float speed;
    float energy;
    float score;
    bool alive;
    bool boosted;
    float boostTimer;
    float shieldTimer;
    int turnsCompleted;   // Track full rotations
    float totalRotation;  // Total rotation accumulated
} LineRider;

typedef struct {
    Vector3 position;
    PowerupType type;
    float lifetime;
    float rotation;
    float bobOffset;
    bool active;
    Color color;
} Powerup;

typedef struct {
    Vector3 position;
    Vector3 velocity;
    Color color;
    float lifetime;
    float size;
} Particle;

typedef struct {
    Vector3 position;
    float brightness;
    float twinkle;
} Star;

typedef struct {
    LineRider rider;
    Powerup powerups[20];
    Particle particles[PARTICLE_COUNT];
    Star stars[STAR_COUNT];
    float gameTime;
    float slowTimeMultiplier;
    int level;
    bool paused;
    bool gameOver;
    bool inMenu;
    DifficultyLevel difficulty;
    float difficultyMultiplier;
    float cameraShake;
    Vector3 arenaCenter;
    float powerupSpawnTimer;
    int highScore;
    int highScoreHardcore;
} GameState;

// =====================================
// Game Functions
// =====================================

Color GetPowerupColor(PowerupType type) {
    switch (type) {
        case POWERUP_ENERGY: return SKYBLUE;
        case POWERUP_SPEED_BOOST: return YELLOW;
        case POWERUP_SLOW_TIME: return PURPLE;
        case POWERUP_SHIELD: return GREEN;
        case POWERUP_SHRINK: return ORANGE;
        case POWERUP_BONUS_POINTS: return GOLD;
        default: return WHITE;
    }
}

void InitStars(GameState* game) {
    for (int i = 0; i < STAR_COUNT; i++) {
        game->stars[i].position = (Vector3){
            (float)(rand() % (int)(ARENA_SIZE * 2) - ARENA_SIZE),
            (float)(rand() % 20 - 10),
            (float)(rand() % (int)(ARENA_SIZE * 2) - ARENA_SIZE)
        };
        game->stars[i].brightness = 0.3f + (float)(rand() % 70) / 100.0f;
        game->stars[i].twinkle = (float)(rand() % 100) / 100.0f;
    }
}

void SpawnParticles(GameState* game, Vector3 position, Color color, int count) {
    for (int i = 0; i < count && i < PARTICLE_COUNT; i++) {
        for (int j = 0; j < PARTICLE_COUNT; j++) {
            if (game->particles[j].lifetime <= 0) {
                game->particles[j].position = position;
                game->particles[j].velocity = (Vector3){
                    (float)(rand() % 100 - 50) * 0.1f,
                    (float)(rand() % 100) * 0.1f,
                    (float)(rand() % 100 - 50) * 0.1f
                };
                game->particles[j].color = color;
                game->particles[j].lifetime = 1.0f + (float)(rand() % 100) * 0.01f;
                game->particles[j].size = 0.1f + (float)(rand() % 30) * 0.01f;
                break;
            }
        }
    }
}

void UpdateParticles(GameState* game, float deltaTime) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (game->particles[i].lifetime > 0) {
            game->particles[i].lifetime -= deltaTime;
            game->particles[i].position = Vector3Add(
                game->particles[i].position,
                Vector3Scale(game->particles[i].velocity, deltaTime)
            );
            game->particles[i].velocity.y -= 5.0f * deltaTime; // Gravity

            // Fade out
            int alpha = (int)(255 * game->particles[i].lifetime);
            if (alpha < 0) alpha = 0;
            if (alpha > 255) alpha = 255;
            game->particles[i].color.a = alpha;
        }
    }
}

void SpawnPowerup(GameState* game) {
    // Find inactive powerup slot
    for (int i = 0; i < 20; i++) {
        if (!game->powerups[i].active) {
            game->powerups[i].active = true;
            game->powerups[i].type = rand() % POWERUP_TYPE_COUNT;

            // Weight energy powerups more heavily
            if (rand() % 100 < 40) {
                game->powerups[i].type = POWERUP_ENERGY;
            }

            // Random position in arena
            float angle = (float)(rand() % 360) * DEG2RAD;
            float distance = 10.0f + (float)(rand() % (int)(ARENA_SIZE * 0.4f));
            game->powerups[i].position = (Vector3){
                cosf(angle) * distance,
                1.0f,
                sinf(angle) * distance
            };

            game->powerups[i].lifetime = POWERUP_LIFETIME;
            game->powerups[i].rotation = 0;
            game->powerups[i].bobOffset = (float)(rand() % 100) * 0.1f;
            game->powerups[i].color = GetPowerupColor(game->powerups[i].type);
            break;
        }
    }
}

void InitLineRider(GameState* game) {
    LineRider* rider = &game->rider;

    rider->segmentCount = INITIAL_SEGMENTS;
    rider->direction = 0;
    rider->speed = LINE_RIDER_SPEED * game->difficultyMultiplier;
    rider->energy = MAX_ENERGY;
    rider->score = 0;
    rider->alive = true;
    rider->boosted = false;
    rider->boostTimer = 0;
    rider->shieldTimer = 0;
    rider->turnsCompleted = 0;
    rider->totalRotation = 0;

    // Initialize segments in a line
    for (int i = 0; i < INITIAL_SEGMENTS; i++) {
        rider->segments[i].position = (Vector3){
            0, 0.5f, -i * SEGMENT_SPACING
        };
        rider->segments[i].previousPos = rider->segments[i].position;
        rider->segments[i].angle = 0;
        rider->segments[i].isHead = (i == 0);

        // Gradient color from bright to dark
        float t = (float)i / INITIAL_SEGMENTS;
        rider->segments[i].color = (Color){
            (int)(100 + 155 * (1.0f - t)),  // Cyan to blue gradient
            (int)(200 + 55 * (1.0f - t)),
            (int)(255),
            255
        };
        rider->segments[i].glowIntensity = 1.0f - t * 0.5f;
    }
}

void UpdateLineRider(GameState* game, EngineState* engine) {
    LineRider* rider = &game->rider;
    if (!rider->alive || game->paused) return;

    float deltaTime = engine->deltaTime * game->slowTimeMultiplier;

    // MAIN MECHANIC: Can only turn left!
    if (IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        rider->direction += TURN_SPEED * deltaTime * game->difficultyMultiplier;
        rider->totalRotation += TURN_SPEED * deltaTime * game->difficultyMultiplier;

        // Check for complete turns
        if (rider->totalRotation >= 2 * PI) {
            rider->turnsCompleted++;
            rider->totalRotation -= 2 * PI;
            rider->score += 100 * rider->turnsCompleted;  // Bonus for completing circles
            SpawnParticles(game, rider->segments[0].position, GOLD, 20);
        }
    }

    // Calculate speed with boost (already includes difficulty multiplier from init)
    float currentSpeed = rider->speed;
    if (rider->boosted && rider->boostTimer > 0) {
        currentSpeed *= 1.5f;
        rider->boostTimer -= deltaTime;
        if (rider->boostTimer <= 0) {
            rider->boosted = false;
        }
    }

    // Move head
    LineSegment* head = &rider->segments[0];
    head->previousPos = head->position;

    Vector3 moveDir = {
        sinf(rider->direction) * currentSpeed * deltaTime,
        0,
        cosf(rider->direction) * currentSpeed * deltaTime
    };

    head->position = Vector3Add(head->position, moveDir);
    head->angle = rider->direction;

    // Follow behavior for body segments
    for (int i = 1; i < rider->segmentCount; i++) {
        LineSegment* segment = &rider->segments[i];
        LineSegment* prevSegment = &rider->segments[i - 1];

        segment->previousPos = segment->position;

        // Calculate direction to previous segment
        Vector3 toTarget = Vector3Subtract(prevSegment->position, segment->position);
        float distance = Vector3Length(toTarget);

        // Only move if too far from previous segment
        if (distance > SEGMENT_SPACING) {
            toTarget = Vector3Normalize(toTarget);
            Vector3 targetPos = Vector3Add(
                prevSegment->position,
                Vector3Scale(toTarget, -SEGMENT_SPACING)
            );

            // Smooth following
            segment->position = Vector3Lerp(segment->position, targetPos, 0.5f);

            // Update angle to face previous segment
            segment->angle = atan2f(toTarget.x, toTarget.z);
        }
    }

    // Check arena boundaries (wrap around)
    if (fabsf(head->position.x) > ARENA_SIZE / 2) {
        head->position.x = -head->position.x * 0.95f;
        SpawnParticles(game, head->position, SKYBLUE, 10);
    }
    if (fabsf(head->position.z) > ARENA_SIZE / 2) {
        head->position.z = -head->position.z * 0.95f;
        SpawnParticles(game, head->position, SKYBLUE, 10);
    }

    // Energy drain (scales with difficulty)
    rider->energy -= ENERGY_DRAIN_RATE * deltaTime * game->difficultyMultiplier;
    if (rider->energy <= 0) {
        rider->energy = 0;
        rider->alive = false;
        game->gameOver = true;

        // Death particles
        for (int i = 0; i < rider->segmentCount; i++) {
            SpawnParticles(game, rider->segments[i].position, RED, 5);
        }
    }

    // Self-collision check (only with segments far from head)
    for (int i = 4; i < rider->segmentCount; i++) {
        if (Vector3Distance(head->position, rider->segments[i].position) < SEGMENT_SIZE) {
            if (rider->shieldTimer <= 0) {
                rider->alive = false;
                game->gameOver = true;
                SpawnParticles(game, head->position, RED, 30);
            }
        }
    }

    // Shield timer
    if (rider->shieldTimer > 0) {
        rider->shieldTimer -= deltaTime;
    }

    // Add score over time
    rider->score += deltaTime * 10;
}

void CollectPowerup(GameState* game, Powerup* powerup) {
    LineRider* rider = &game->rider;

    switch (powerup->type) {
        case POWERUP_ENERGY:
            rider->energy += ENERGY_BAR_VALUE;
            if (rider->energy > MAX_ENERGY) rider->energy = MAX_ENERGY;
            break;

        case POWERUP_SPEED_BOOST:
            rider->boosted = true;
            rider->boostTimer = 5.0f;
            break;

        case POWERUP_SLOW_TIME:
            game->slowTimeMultiplier = 0.5f;
            break;

        case POWERUP_SHIELD:
            rider->shieldTimer = 10.0f;
            break;

        case POWERUP_SHRINK:
            // Remove last few segments if possible
            if (rider->segmentCount > INITIAL_SEGMENTS) {
                rider->segmentCount -= 3;
                if (rider->segmentCount < INITIAL_SEGMENTS) {
                    rider->segmentCount = INITIAL_SEGMENTS;
                }
            }
            break;

        case POWERUP_BONUS_POINTS:
            rider->score += 500;
            break;

        default:
            break;
    }

    // Visual feedback
    SpawnParticles(game, powerup->position, powerup->color, 20);
    game->cameraShake = 0.2f;

    // Add score
    rider->score += 50;

    // Grow the line rider (add new segment) for energy powerups
    if (rider->segmentCount < MAX_SEGMENTS - 1 && powerup->type == POWERUP_ENERGY) {
        int newIndex = rider->segmentCount;
        LineSegment* lastSegment = &rider->segments[rider->segmentCount - 1];

        rider->segments[newIndex] = *lastSegment;
        rider->segments[newIndex].position = Vector3Add(
            lastSegment->position,
            (Vector3){0, 0, -SEGMENT_SPACING}
        );
        rider->segments[newIndex].isHead = false;
        rider->segmentCount++;
    }

    powerup->active = false;
}

void UpdatePowerups(GameState* game, float deltaTime) {
    LineRider* rider = &game->rider;

    for (int i = 0; i < 20; i++) {
        if (!game->powerups[i].active) continue;

        Powerup* powerup = &game->powerups[i];

        // Update lifetime
        powerup->lifetime -= deltaTime;
        if (powerup->lifetime <= 0) {
            powerup->active = false;
            continue;
        }

        // Animation
        powerup->rotation += deltaTime * 2.0f;
        float bob = sinf(game->gameTime * 2.0f + powerup->bobOffset) * 0.2f;
        powerup->position.y = 1.0f + bob;

        // Check collection
        float distance = Vector3Distance(rider->segments[0].position, powerup->position);
        if (distance < SEGMENT_SIZE + POWERUP_SIZE && rider->alive) {
            CollectPowerup(game, powerup);
        }
    }
}

void RenderLineRider(GameState* game) {
    LineRider* rider = &game->rider;

    // Draw segments from tail to head for proper layering
    for (int i = rider->segmentCount - 1; i >= 0; i--) {
        LineSegment* segment = &rider->segments[i];

        // Main segment body (hexagonal shape for sci-fi look)
        float size = SEGMENT_SIZE;
        if (segment->isHead) {
            size *= 1.3f;  // Larger head
        }

        // Draw main segment
        DrawCylinderEx(
            Vector3Add(segment->position, (Vector3){0, -SEGMENT_HEIGHT/2, 0}),
            Vector3Add(segment->position, (Vector3){0, SEGMENT_HEIGHT/2, 0}),
            size, size * 0.8f, 6, segment->color
        );

        // Glow effect (wireframe)
        Color glowColor = segment->color;
        glowColor.a = (int)(100 * segment->glowIntensity);
        DrawCylinderWiresEx(
            Vector3Add(segment->position, (Vector3){0, -SEGMENT_HEIGHT/2, 0}),
            Vector3Add(segment->position, (Vector3){0, SEGMENT_HEIGHT/2, 0}),
            size * TRAIL_GLOW_SIZE, size * TRAIL_GLOW_SIZE * 0.8f, 6, glowColor
        );

        // Shield effect
        if (rider->shieldTimer > 0) {
            float shieldAlpha = sinf(game->gameTime * 10.0f) * 0.5f + 0.5f;
            Color shieldColor = GREEN;
            shieldColor.a = (int)(50 * shieldAlpha);
            DrawSphereWires(segment->position, size * 1.5f, 4, 8, shieldColor);
        }
    }

    // Draw connection lines between segments
    for (int i = 0; i < rider->segmentCount - 1; i++) {
        Vector3 start = rider->segments[i].position;
        Vector3 end = rider->segments[i + 1].position;

        Color lineColor = rider->segments[i].color;
        lineColor.a = 150;
        DrawLine3D(start, end, lineColor);
    }

    // Draw boost effect
    if (rider->boosted && rider->boostTimer > 0) {
        LineSegment* head = &rider->segments[0];
        for (int i = 0; i < 3; i++) {
            float offset = i * 0.3f;
            Vector3 trailPos = Vector3Add(head->position,
                (Vector3){
                    -sinf(rider->direction) * offset,
                    0,
                    -cosf(rider->direction) * offset
                });

            Color trailColor = YELLOW;
            trailColor.a = (int)(100 * (1.0f - offset / 1.0f));
            DrawSphere(trailPos, SEGMENT_SIZE * 0.5f, trailColor);
        }
    }
}

void RenderPowerups(GameState* game) {
    for (int i = 0; i < 20; i++) {
        if (!game->powerups[i].active) continue;

        Powerup* powerup = &game->powerups[i];
        Vector3 pos = powerup->position;

        // Different shapes for different powerups
        switch (powerup->type) {
            case POWERUP_ENERGY:
                DrawCube(pos, POWERUP_SIZE, POWERUP_SIZE, POWERUP_SIZE, powerup->color);
                DrawCubeWires(pos, POWERUP_SIZE * 1.2f, POWERUP_SIZE * 1.2f, POWERUP_SIZE * 1.2f,
                             Fade(powerup->color, 0.5f));
                break;

            case POWERUP_SPEED_BOOST:
                DrawCylinder(pos, POWERUP_SIZE * 0.5f, 0.2f, POWERUP_SIZE * 1.5f, 4, powerup->color);
                break;

            case POWERUP_SLOW_TIME:
                DrawSphere(pos, POWERUP_SIZE, powerup->color);
                DrawSphereWires(pos, POWERUP_SIZE * 1.3f, 8, 8, Fade(powerup->color, 0.5f));
                break;

            case POWERUP_SHIELD:
                DrawCylinderEx(
                    Vector3Add(pos, (Vector3){0, -POWERUP_SIZE/2, 0}),
                    Vector3Add(pos, (Vector3){0, POWERUP_SIZE/2, 0}),
                    POWERUP_SIZE, POWERUP_SIZE * 0.7f, 8, powerup->color
                );
                break;

            case POWERUP_SHRINK:
                DrawCube(pos, POWERUP_SIZE * 0.6f, POWERUP_SIZE * 0.6f, POWERUP_SIZE * 0.6f, powerup->color);
                break;

            case POWERUP_BONUS_POINTS:
                // Star shape using multiple triangles
                for (int j = 0; j < 5; j++) {
                    float angle = j * 72 * DEG2RAD + powerup->rotation;
                    Vector3 p1 = Vector3Add(pos, (Vector3){cosf(angle) * POWERUP_SIZE, 0, sinf(angle) * POWERUP_SIZE});
                    Vector3 p2 = Vector3Add(pos, (Vector3){cosf(angle + 144 * DEG2RAD) * POWERUP_SIZE, 0,
                                           sinf(angle + 144 * DEG2RAD) * POWERUP_SIZE});
                    DrawLine3D(p1, p2, powerup->color);
                }
                break;

            default:
                break;
        }

        // Powerup label (floating text effect)
        if (powerup->lifetime < 5.0f) {
            Color fadeColor = powerup->color;
            fadeColor.a = (int)(255 * powerup->lifetime / 5.0f);
            DrawSphere(pos, POWERUP_SIZE * (1.0f + (5.0f - powerup->lifetime) * 0.2f),
                      Fade(fadeColor, 0.1f));
        }
    }
}

void RenderParticles(GameState* game) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        if (game->particles[i].lifetime > 0) {
            DrawSphere(game->particles[i].position, game->particles[i].size, game->particles[i].color);
        }
    }
}

void RenderStars(GameState* game) {
    for (int i = 0; i < STAR_COUNT; i++) {
        float twinkle = sinf(game->gameTime * 3.0f + game->stars[i].twinkle * 10.0f) * 0.3f + 0.7f;
        Color starColor = (Color){
            255, 255, 255,
            (int)(game->stars[i].brightness * twinkle * 255)
        };
        DrawSphere(game->stars[i].position, 0.1f, starColor);
    }
}

void RenderArena(GameState* game) {
    // Draw arena boundaries
    float halfSize = ARENA_SIZE / 2;
    Color boundaryColor = (Color){100, 100, 200, 50};

    // Draw boundary lines
    for (int i = 0; i < 4; i++) {
        float angle = i * 90 * DEG2RAD;
        Vector3 p1 = {cosf(angle) * halfSize, 0, sinf(angle) * halfSize};
        Vector3 p2 = {cosf(angle + 90 * DEG2RAD) * halfSize, 0, sinf(angle + 90 * DEG2RAD) * halfSize};
        DrawLine3D(p1, p2, boundaryColor);

        // Corner markers
        DrawCube(p1, 1, 3, 1, boundaryColor);
    }

    // Draw energy warning if low
    if (game->rider.energy < 20 && game->rider.alive) {
        float pulse = sinf(game->gameTime * 10.0f) * 0.5f + 0.5f;
        Color warningColor = (Color){255, 0, 0, (int)(pulse * 100)};
        DrawCylinder((Vector3){0, -1, 0}, 0, halfSize * 2, 0.1f, 32, warningColor);
    }
}

void RenderOffscreenIndicators(GameState* game, EngineState* engine) {
    // Draw indicators for off-screen energy pickups
    for (int i = 0; i < 20; i++) {
        if (!game->powerups[i].active) continue;
        if (game->powerups[i].type != POWERUP_ENERGY) continue;

        // Get screen position of powerup
        Vector2 screenPos = GetWorldToScreen(game->powerups[i].position, engine->camera);

        // Check if off-screen
        int screenWidth = engine->windowWidth;
        int screenHeight = engine->windowHeight;
        int margin = 50;

        if (screenPos.x < -margin || screenPos.x > screenWidth + margin ||
            screenPos.y < -margin || screenPos.y > screenHeight + margin) {

            // Calculate direction from screen center to powerup
            float centerX = screenWidth / 2.0f;
            float centerY = screenHeight / 2.0f;
            float dx = screenPos.x - centerX;
            float dy = screenPos.y - centerY;

            // Normalize direction
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < 0.001f) continue;
            dx /= dist;
            dy /= dist;

            // Calculate maximum distance from center to edge
            float maxDistX = (screenWidth / 2.0f) - margin;
            float maxDistY = (screenHeight / 2.0f) - margin;

            // Find the scale factor to reach the edge
            float scaleX = (dx != 0) ? maxDistX / fabsf(dx) : 99999.0f;
            float scaleY = (dy != 0) ? maxDistY / fabsf(dy) : 99999.0f;
            float scale = fminf(scaleX, scaleY);

            // Calculate edge position
            float edgeX = centerX + dx * scale;
            float edgeY = centerY + dy * scale;

            // Get angle for arrow
            float angle = atan2f(dy, dx);

            // Adjust arrow appearance based on energy urgency
            float energyPercent = game->rider.energy / MAX_ENERGY;
            float arrowSize = 20.0f;
            float pulseSpeed = 5.0f;
            Color arrowColor = SKYBLUE;
            Color outlineColor = SKYBLUE;

            // Critical energy (< 20%): Red, fast pulsing, larger
            if (energyPercent < 0.2f) {
                arrowSize = 30.0f;
                pulseSpeed = 10.0f;
                arrowColor = RED;
                outlineColor = (Color){255, 100, 100, 255};
            }
            // Low energy (< 40%): Orange, medium pulsing
            else if (energyPercent < 0.4f) {
                arrowSize = 25.0f;
                pulseSpeed = 7.0f;
                arrowColor = ORANGE;
                outlineColor = (Color){255, 200, 100, 255};
            }
            // Medium energy (< 60%): Yellow
            else if (energyPercent < 0.6f) {
                arrowColor = YELLOW;
                outlineColor = (Color){255, 255, 100, 255};
            }

            // Calculate arrow points
            Vector2 arrowTip = {edgeX, edgeY};
            Vector2 arrowBase1 = {
                edgeX - cosf(angle - 0.5f) * arrowSize,
                edgeY - sinf(angle - 0.5f) * arrowSize
            };
            Vector2 arrowBase2 = {
                edgeX - cosf(angle + 0.5f) * arrowSize,
                edgeY - sinf(angle + 0.5f) * arrowSize
            };

            // Pulsing effect - more intense when energy is low
            float pulse = sinf(game->gameTime * pulseSpeed) * 0.4f + 0.6f;

            // Apply pulse to arrow color alpha
            arrowColor.a = (int)(255 * pulse);

            // Draw glow effect when critical
            if (energyPercent < 0.2f) {
                float glowSize = arrowSize * 1.5f * pulse;
                Vector2 glowBase1 = {
                    edgeX - cosf(angle - 0.5f) * glowSize,
                    edgeY - sinf(angle - 0.5f) * glowSize
                };
                Vector2 glowBase2 = {
                    edgeX - cosf(angle + 0.5f) * glowSize,
                    edgeY - sinf(angle + 0.5f) * glowSize
                };
                DrawTriangle(arrowTip, glowBase1, glowBase2, Fade(RED, 0.3f));
            }

            // Draw the arrow triangle
            DrawTriangle(arrowTip, arrowBase1, arrowBase2, arrowColor);
            DrawTriangleLines(arrowTip, arrowBase1, arrowBase2, outlineColor);

            // Draw distance text with urgency coloring
            float distance = Vector3Distance(game->rider.segments[0].position, game->powerups[i].position);
            int fontSize = (energyPercent < 0.2f) ? 16 : 12;
            DrawText(TextFormat("%.0fm", distance), (int)(edgeX - 15), (int)(edgeY - 35), fontSize, outlineColor);

            // Add "ENERGY!" text when critical
            if (energyPercent < 0.2f) {
                DrawText("ENERGY!", (int)(edgeX - 25), (int)(edgeY + 10), 12, RED);
            }
        }
    }
}

void RenderUI(GameState* game, EngineState* engine) {
    int screenWidth = engine->windowWidth;
    int screenHeight = engine->windowHeight;

    // Show menu if in menu state
    if (game->inMenu) {
        DrawText(GAME_TITLE, screenWidth / 2 - MeasureText(GAME_TITLE, 50) / 2, 100, 50, WHITE);
        DrawText("You can only steer LEFT!", screenWidth / 2 - MeasureText("You can only steer LEFT!", 30) / 2, 160, 30, SKYBLUE);

        DrawText("SELECT DIFFICULTY", screenWidth / 2 - MeasureText("SELECT DIFFICULTY", 30) / 2, 250, 30, YELLOW);

        // Easy option
        Color easyColor = (game->difficulty == DIFFICULTY_EASY) ? GREEN : WHITE;
        DrawText("[1] EASY", screenWidth / 2 - 150, 320, 25, easyColor);
        DrawText("Normal speed", screenWidth / 2 - 150, 350, 18, LIGHTGRAY);
        DrawText("For beginners", screenWidth / 2 - 150, 370, 18, LIGHTGRAY);

        // Hardcore option
        Color hardcoreColor = (game->difficulty == DIFFICULTY_HARDCORE) ? RED : WHITE;
        DrawText("[2] HARDCORE", screenWidth / 2 + 20, 320, 25, hardcoreColor);
        DrawText("2x speed!", screenWidth / 2 + 20, 350, 18, ORANGE);
        DrawText("For experts", screenWidth / 2 + 20, 370, 18, ORANGE);

        // Start instruction
        DrawText("Press ENTER to start", screenWidth / 2 - MeasureText("Press ENTER to start", 25) / 2, 450, 25, LIME);

        // High scores
        DrawText(TextFormat("Easy High Score: %d", game->highScore), screenWidth / 2 - 200, 520, 20, WHITE);
        DrawText(TextFormat("Hardcore High Score: %d", game->highScoreHardcore), screenWidth / 2 + 20, 520, 20, WHITE);

        DrawText("Press ESC to exit", screenWidth / 2 - MeasureText("Press ESC to exit", 18) / 2, 570, 18, DARKGRAY);
        return;
    }

    // Game title (smaller when playing)
    DrawText(GAME_TITLE, screenWidth / 2 - MeasureText(GAME_TITLE, 30) / 2, 20, 30, WHITE);
    const char* diffText = game->difficulty == DIFFICULTY_HARDCORE ? "HARDCORE MODE" : "EASY MODE";
    Color diffColor = game->difficulty == DIFFICULTY_HARDCORE ? RED : GREEN;
    DrawText(diffText, screenWidth / 2 - MeasureText(diffText, 20) / 2, 55, 20, diffColor);

    // Score
    DrawText(TextFormat("Score: %d", (int)game->rider.score), 20, 100, 24, WHITE);
    int currentHighScore = (game->difficulty == DIFFICULTY_HARDCORE) ? game->highScoreHardcore : game->highScore;
    if (currentHighScore > 0) {
        DrawText(TextFormat("High: %d", currentHighScore), 20, 130, 18, GOLD);
    }

    // Energy bar
    float energyPercent = game->rider.energy / MAX_ENERGY;
    Color energyColor = energyPercent > 0.3f ? GREEN : (energyPercent > 0.1f ? YELLOW : RED);
    DrawRectangle(20, 160, 200, 20, DARKGRAY);
    DrawRectangle(20, 160, (int)(200 * energyPercent), 20, energyColor);
    DrawRectangleLines(20, 160, 200, 20, WHITE);
    DrawText("ENERGY", 25, 162, 16, WHITE);

    // Boost indicator
    if (game->rider.boosted) {
        DrawText("BOOST!", screenWidth / 2 - MeasureText("BOOST!", 40) / 2, 100, 40, YELLOW);
    }

    // Shield indicator
    if (game->rider.shieldTimer > 0) {
        DrawText(TextFormat("SHIELD: %.1fs", game->rider.shieldTimer), 20, 190, 18, GREEN);
    }

    // Segments count
    DrawText(TextFormat("Length: %d", game->rider.segmentCount), 20, 220, 18, SKYBLUE);

    // Turns completed
    if (game->rider.turnsCompleted > 0) {
        DrawText(TextFormat("Loops: %d", game->rider.turnsCompleted), 20, 250, 18, GOLD);
    }

    // Controls
    DrawText("SPACE or LEFT MOUSE: Turn Left", screenWidth - 300, screenHeight - 30, 16, LIGHTGRAY);

    // Pause indicator
    if (game->paused) {
        DrawText("PAUSED", screenWidth / 2 - MeasureText("PAUSED", 50) / 2, screenHeight / 2 - 25, 50, WHITE);
        DrawText("Press P to Resume", screenWidth / 2 - MeasureText("Press P to Resume", 20) / 2, screenHeight / 2 + 30, 20, LIGHTGRAY);
    }

    // Game over
    if (game->gameOver) {
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
        DrawText("GAME OVER", screenWidth / 2 - MeasureText("GAME OVER", 50) / 2, screenHeight / 2 - 100, 50, RED);
        DrawText(TextFormat("Final Score: %d", (int)game->rider.score),
                screenWidth / 2 - MeasureText(TextFormat("Final Score: %d", (int)game->rider.score), 30) / 2,
                screenHeight / 2 - 30, 30, WHITE);
        const char* modeText = game->difficulty == DIFFICULTY_HARDCORE ? "HARDCORE MODE" : "EASY MODE";
        Color modeColor = game->difficulty == DIFFICULTY_HARDCORE ? ORANGE : GREEN;
        DrawText(modeText, screenWidth / 2 - MeasureText(modeText, 20) / 2, screenHeight / 2 + 10, 20, modeColor);
        DrawText("Press ENTER to Restart", screenWidth / 2 - MeasureText("Press ENTER to Restart", 20) / 2,
                screenHeight / 2 + 40, 20, LIGHTGRAY);
        DrawText("Press M for Menu", screenWidth / 2 - MeasureText("Press M for Menu", 20) / 2,
                screenHeight / 2 + 65, 20, LIGHTGRAY);
        DrawText("Press ESC to Exit", screenWidth / 2 - MeasureText("Press ESC to Exit", 20) / 2,
                screenHeight / 2 + 90, 20, LIGHTGRAY);
    }
}

void InitGame(GameState* game) {
    // Preserve difficulty and high scores
    DifficultyLevel savedDifficulty = game->difficulty;
    int savedHighScore = game->highScore;
    int savedHighScoreHardcore = game->highScoreHardcore;

    memset(game, 0, sizeof(GameState));

    // Restore preserved values
    game->difficulty = savedDifficulty;
    game->highScore = savedHighScore;
    game->highScoreHardcore = savedHighScoreHardcore;

    // Set difficulty multiplier
    game->difficultyMultiplier = (game->difficulty == DIFFICULTY_HARDCORE) ? HARDCORE_SPEED_MULTI : 1.0f;

    game->slowTimeMultiplier = 1.0f;
    game->level = 1;
    game->paused = false;
    game->gameOver = false;
    game->inMenu = false;
    game->cameraShake = 0;
    game->powerupSpawnTimer = 2.0f / game->difficultyMultiplier;

    InitLineRider(game);
    InitStars(game);

    // Spawn initial powerups
    for (int i = 0; i < 8; i++) {
        SpawnPowerup(game);
    }
}

void UpdateGame(GameState* game, EngineState* engine) {
    float deltaTime = engine->deltaTime;

    // Handle menu input
    if (game->inMenu) {
        if (IsKeyPressed(KEY_ONE)) {
            game->difficulty = DIFFICULTY_EASY;
        }
        if (IsKeyPressed(KEY_TWO)) {
            game->difficulty = DIFFICULTY_HARDCORE;
        }
        if (IsKeyPressed(KEY_ENTER)) {
            game->inMenu = false;
            InitGame(game);
        }
        return;
    }

    // Update game time
    if (!game->paused && !game->gameOver) {
        game->gameTime += deltaTime;
    }

    // Handle pause
    if (IsKeyPressed(KEY_P) && !game->gameOver) {
        game->paused = !game->paused;
    }

    // Handle restart and menu
    if (game->gameOver) {
        // Save high score
        if (game->difficulty == DIFFICULTY_HARDCORE) {
            if (game->rider.score > game->highScoreHardcore) {
                game->highScoreHardcore = (int)game->rider.score;
            }
        } else {
            if (game->rider.score > game->highScore) {
                game->highScore = (int)game->rider.score;
            }
        }

        if (IsKeyPressed(KEY_ENTER)) {
            InitGame(game);
            return;
        }
        if (IsKeyPressed(KEY_M)) {
            game->inMenu = true;
            game->gameOver = false;
            return;
        }
    }

    if (game->paused || game->gameOver) {
        return;
    }

    // Update game systems
    UpdateLineRider(game, engine);
    UpdateParticles(game, deltaTime);
    UpdatePowerups(game, deltaTime);

    // Spawn new powerups periodically (faster in hardcore)
    game->powerupSpawnTimer -= deltaTime;
    if (game->powerupSpawnTimer <= 0) {
        SpawnPowerup(game);
        float baseTime = 3.0f + (float)(rand() % 30) / 10.0f;
        game->powerupSpawnTimer = baseTime / game->difficultyMultiplier;
    }

    // Slowly return time to normal
    if (game->slowTimeMultiplier < 1.0f) {
        game->slowTimeMultiplier += deltaTime * 0.1f;
        if (game->slowTimeMultiplier > 1.0f) {
            game->slowTimeMultiplier = 1.0f;
        }
    }

    // Update camera shake
    if (game->cameraShake > 0) {
        game->cameraShake -= deltaTime * 2.0f;
        if (game->cameraShake < 0) game->cameraShake = 0;
    }
}

// =====================================
// Main Program
// =====================================

int main(int argc, char* argv[]) {
    // Suppress unused parameter warnings
    (void)argc;
    (void)argv;

    // Initialize random seed
    srand(time(NULL));

    // Initialize engine
    EngineState* engine = Engine_Init(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, GAME_TITLE);
    if (!engine) {
        printf("Failed to initialize engine!\n");
        return 1;
    }

    // Initialize game state
    GameState* game = (GameState*)calloc(1, sizeof(GameState));
    if (!game) {
        printf("Failed to allocate game state!\n");
        Engine_Shutdown(engine);
        return 1;
    }

    // Start in menu
    game->inMenu = true;
    game->difficulty = DIFFICULTY_EASY;
    game->difficultyMultiplier = 1.0f;

    // Set up camera for the game
    engine->viewMode = VIEW_MODE_ORBIT;
    engine->orbitCamera.distance = 45.0f;
    engine->orbitCamera.rotationH = PI * 0.25f;
    engine->orbitCamera.rotationV = PI * 0.35f;
    engine->orbitCamera.target = (Vector3){0, 0, 0};

    // Set up isometric camera with same zoom level
    engine->isoCamera.height = 45.0f;
    engine->isoCamera.target = (Vector3){0, 0, 0};
    engine->isoCamera.targetTarget = (Vector3){0, 0, 0};

    // Disable some default debug displays for cleaner look
    engine->showGrid = false;
    engine->showAxes = false;
    engine->showDebugInfo = false;

    // Main game loop
    while (!Engine_ShouldClose(engine)) {
        // Update game
        UpdateGame(game, engine);

        // Follow the line rider head with camera (skip if in menu)
        if (game->rider.alive && !game->paused && !game->inMenu) {
            Vector3 headPos = game->rider.segments[0].position;

            if (engine->viewMode == VIEW_MODE_ORBIT) {
                // Smoothly follow the head with look-ahead
                Vector3 lookAhead = {
                    headPos.x + sinf(game->rider.direction) * 5.0f,
                    headPos.y,
                    headPos.z + cosf(game->rider.direction) * 5.0f
                };
                engine->orbitCamera.target = Vector3Lerp(engine->orbitCamera.target, lookAhead, 0.08f);

                // Apply camera shake
                if (game->cameraShake > 0) {
                    engine->orbitCamera.target.x += (float)(rand() % 100 - 50) * 0.01f * game->cameraShake;
                    engine->orbitCamera.target.z += (float)(rand() % 100 - 50) * 0.01f * game->cameraShake;
                }
            } else if (engine->viewMode == VIEW_MODE_ISOMETRIC) {
                // Follow with look-ahead like orbit camera
                Vector3 lookAhead = {
                    headPos.x + sinf(game->rider.direction) * 5.0f,
                    headPos.y,
                    headPos.z + cosf(game->rider.direction) * 5.0f
                };
                engine->isoCamera.targetTarget = Vector3Lerp(engine->isoCamera.targetTarget, lookAhead, 0.08f);

                // Apply camera shake
                if (game->cameraShake > 0) {
                    engine->isoCamera.targetTarget.x += (float)(rand() % 100 - 50) * 0.01f * game->cameraShake;
                    engine->isoCamera.targetTarget.z += (float)(rand() % 100 - 50) * 0.01f * game->cameraShake;
                }
            }
        }

        // Begin frame
        Engine_BeginFrame(engine);

        // Render game world (skip if in menu)
        if (!game->inMenu) {
            RenderStars(game);
            RenderArena(game);

            // Enable grid for space effect
            if (engine->showGrid) {
                Render_Grid(ARENA_SIZE, 20);
            }

            // Render game objects
            RenderPowerups(game);
            RenderLineRider(game);
            RenderParticles(game);
        }

        // End 3D rendering
        Engine_EndFrame(engine);

        // Render UI overlay (after EndFrame to draw on top)
        RenderUI(game, engine);

        // Render off-screen indicators for energy pickups (skip if in menu)
        if (game->rider.alive && !game->paused && !game->gameOver && !game->inMenu) {
            RenderOffscreenIndicators(game, engine);
        }
    }

    // Save high score if needed
    if (game->rider.score > game->highScore) {
        game->highScore = (int)game->rider.score;
        // Could save to file here
    }

    // Cleanup
    free(game);
    Engine_Shutdown(engine);

    return 0;
}
