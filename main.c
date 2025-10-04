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

// Sound effect types
typedef enum {
    SFX_PICKUP_ENERGY,
    SFX_PICKUP_BOOST,
    SFX_PICKUP_SLOW,
    SFX_PICKUP_SHIELD,
    SFX_PICKUP_SHRINK,
    SFX_PICKUP_BONUS,
    SFX_TURN,
    SFX_LOOP_COMPLETE,
    SFX_COLLISION,
    SFX_GAME_OVER,
    SFX_MENU_SELECT,
    SFX_MENU_MOVE,
    SFX_PAUSE,
    SFX_WARNING,
    SFX_COUNT
} SoundEffect;

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

    // Sound effects
    Sound soundPickup;
    Sound soundTurn;
    Sound soundGameOver;
    Sound soundBoost;
    Sound soundShield;
    Sound soundMenuSelect;
    Sound soundPause;
    Sound soundLoopComplete;
    bool soundEnabled;
    bool useFallbackAudio;
    float masterVolume;
    bool showFPS;
} GameState;

// =====================================
// Sound Generation Functions
// =====================================

// Fallback audio using system beep
void PlayFallbackBeep(int frequency, int duration) {
    #ifdef __linux__
    // Use beep command if available, or echo to PC speaker
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "( speaker-test -t sine -f %d -l 1 & pid=$!; sleep 0.%03d; kill -9 $pid ) >/dev/null 2>&1 &",
             frequency, duration);
    system(cmd);
    #else
    // For non-Linux, just print that sound would play
    printf("[Sound: %dHz for %dms]\n", frequency, duration);
    #endif
}

void PlayFallbackSound(GameState* game, Sound* sound) {
    if (!game->soundEnabled) return;

    // Map different sounds to different beep patterns
    if (sound == &game->soundPickup) {
        PlayFallbackBeep(800, 100);
    } else if (sound == &game->soundTurn) {
        PlayFallbackBeep(300, 30);
    } else if (sound == &game->soundGameOver) {
        PlayFallbackBeep(200, 500);
    } else if (sound == &game->soundBoost) {
        PlayFallbackBeep(1000, 150);
    } else if (sound == &game->soundShield) {
        PlayFallbackBeep(600, 200);
    } else if (sound == &game->soundMenuSelect) {
        PlayFallbackBeep(700, 80);
    } else if (sound == &game->soundPause) {
        PlayFallbackBeep(400, 100);
    } else if (sound == &game->soundLoopComplete) {
        PlayFallbackBeep(1200, 250);
    }
}

void PlayGameSound(GameState* game, Sound sound) {
    if (!game->soundEnabled) {
        return;
    }

    // If using fallback audio, we can't play the actual sound
    // For now, just skip it - the caller should use specific functions
    if (game->useFallbackAudio) {
        // Fallback audio needs specific handling per sound type
        // This is handled in the specific play functions below
        return;
    }

    // Play the actual raylib sound
    PlaySound(sound);
}

// Helper functions for playing specific sounds
void PlayPickupSound(GameState* game) {
    if (!game->soundEnabled) return;

    if (game->useFallbackAudio) {
        PlayFallbackBeep(800, 100);
    } else {
        PlaySound(game->soundPickup);
    }
}

void PlayBoostSound(GameState* game) {
    if (!game->soundEnabled) return;

    if (game->useFallbackAudio) {
        PlayFallbackBeep(1000, 150);
    } else {
        PlaySound(game->soundBoost);
    }
}

void PlayShieldSound(GameState* game) {
    if (!game->soundEnabled) return;

    if (game->useFallbackAudio) {
        PlayFallbackBeep(600, 200);
    } else {
        PlaySound(game->soundShield);
    }
}

void PlayMenuSound(GameState* game) {
    if (!game->soundEnabled) return;

    if (game->useFallbackAudio) {
        PlayFallbackBeep(700, 80);
    } else {
        PlaySound(game->soundMenuSelect);
    }
}

void PlayTurnSound(GameState* game) {
    if (!game->soundEnabled) return;

    if (game->useFallbackAudio) {
        PlayFallbackBeep(300, 30);
    } else {
        PlaySound(game->soundTurn);
    }
}

void PlayGameOverSound(GameState* game) {
    if (!game->soundEnabled) return;

    if (game->useFallbackAudio) {
        PlayFallbackBeep(200, 500);
    } else {
        PlaySound(game->soundGameOver);
    }
}

void PlayPauseSound(GameState* game) {
    if (!game->soundEnabled) return;

    if (game->useFallbackAudio) {
        PlayFallbackBeep(400, 100);
    } else {
        PlaySound(game->soundPause);
    }
}

void PlayLoopCompleteSound(GameState* game) {
    if (!game->soundEnabled) return;

    if (game->useFallbackAudio) {
        PlayFallbackBeep(1200, 250);
    } else {
        PlaySound(game->soundLoopComplete);
    }
}

Sound GenerateBeepSound(float frequency, float duration, int sampleRate) {
    int frames = (int)(duration * sampleRate);
    short* data = (short*)calloc(frames, sizeof(short));

    if (!data) {
        printf("ERROR: Failed to allocate memory for sound\n");
        return (Sound){0};
    }

    for (int i = 0; i < frames; i++) {
        float t = (float)i / sampleRate;
        float sample = sinf(2.0f * PI * frequency * t);

        // Simple envelope
        float envelope = 1.0f;
        if (i < frames / 10) {
            envelope = (float)i / (frames / 10);
        } else if (i > frames * 9 / 10) {
            envelope = (float)(frames - i) / (frames / 10);
        }

        // Increase amplitude
        data[i] = (short)(sample * envelope * 30000.0f);
    }

    Wave wave = {0};
    wave.frameCount = frames;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;

    printf("Generated beep: freq=%.1fHz, duration=%.2fs, frames=%d\n", frequency, duration, frames);

    Sound sound = LoadSoundFromWave(wave);
    free(data);  // Free data after loading sound

    if (sound.frameCount == 0) {
        printf("ERROR: Failed to load sound from wave\n");
    }

    return sound;
}

Sound GeneratePickupSound() {
    // Simple rising tone - use basic beep for now
    return GenerateBeepSound(800.0f, 0.15f, 22050);
}

Sound GenerateGameOverSound() {
    // Simple descending tone - use low beep for now
    return GenerateBeepSound(200.0f, 0.5f, 22050);
}

void InitSounds(GameState* game) {
    printf("\n=== AUDIO INITIALIZATION ===\n");
    printf("Initializing audio device...\n");

    // Try to initialize audio device
    InitAudioDevice();

    // Check if audio device is ready
    if (!IsAudioDeviceReady()) {
        printf("WARNING: Raylib audio not available!\n");
        printf("Using fallback system beeps instead.\n");
        game->soundEnabled = true;
        game->useFallbackAudio = true;

        // Initialize dummy sound structures so the game doesn't crash
        game->soundPickup = (Sound){0};
        game->soundPickup.frameCount = 1;
        game->soundTurn = (Sound){0};
        game->soundTurn.frameCount = 2;
        game->soundGameOver = (Sound){0};
        game->soundGameOver.frameCount = 3;
        game->soundBoost = (Sound){0};
        game->soundBoost.frameCount = 4;
        game->soundShield = (Sound){0};
        game->soundShield.frameCount = 5;
        game->soundMenuSelect = (Sound){0};
        game->soundMenuSelect.frameCount = 6;
        game->soundPause = (Sound){0};
        game->soundPause.frameCount = 7;
        game->soundLoopComplete = (Sound){0};
        game->soundLoopComplete.frameCount = 8;

        printf("=== FALLBACK AUDIO READY ===\n\n");
        return;
    }

    printf("Audio device ready!\n");
    game->useFallbackAudio = false;

    // Generate simple sounds with lower sample rates for better compatibility
    printf("Generating sounds...\n");

    game->soundPickup = GeneratePickupSound();
    printf("  - Pickup sound generated\n");

    game->soundTurn = GenerateBeepSound(300.0f, 0.05f, 22050);
    printf("  - Turn sound generated\n");

    game->soundGameOver = GenerateGameOverSound();
    printf("  - Game over sound generated\n");

    game->soundBoost = GenerateBeepSound(1000.0f, 0.2f, 22050);
    printf("  - Boost sound generated\n");

    game->soundShield = GenerateBeepSound(600.0f, 0.25f, 22050);
    printf("  - Shield sound generated\n");

    game->soundMenuSelect = GenerateBeepSound(700.0f, 0.1f, 22050);
    printf("  - Menu sound generated\n");

    game->soundPause = GenerateBeepSound(400.0f, 0.15f, 22050);
    printf("  - Pause sound generated\n");

    game->soundLoopComplete = GenerateBeepSound(1200.0f, 0.3f, 22050);
    printf("  - Loop complete sound generated\n");

    // Set volumes
    game->soundEnabled = true;
    game->masterVolume = 1.0f;  // Maximum volume

    SetMasterVolume(1.0f);
    SetSoundVolume(game->soundPickup, 1.0f);
    SetSoundVolume(game->soundTurn, 0.3f);  // Quieter for turn
    SetSoundVolume(game->soundGameOver, 1.0f);
    SetSoundVolume(game->soundBoost, 1.0f);
    SetSoundVolume(game->soundShield, 1.0f);
    SetSoundVolume(game->soundMenuSelect, 0.8f);
    SetSoundVolume(game->soundPause, 0.8f);
    SetSoundVolume(game->soundLoopComplete, 1.0f);

    printf("Sound volumes set\n");
    printf("=== AUDIO READY ===\n\n");

    // Play test sound
    printf("Playing test sound (menu select)...\n");
    PlaySound(game->soundMenuSelect);

    // Test all sounds
    printf("Testing sound playback:\n");
    printf("  Menu sound frameCount: %d\n", game->soundMenuSelect.frameCount);
    printf("  Pickup sound frameCount: %d\n", game->soundPickup.frameCount);
    printf("  Boost sound frameCount: %d\n", game->soundBoost.frameCount);
}

void UnloadSounds(GameState* game) {
    if (!game->soundEnabled || game->useFallbackAudio) return;

    printf("Unloading sounds...\n");
    UnloadSound(game->soundPickup);
    UnloadSound(game->soundTurn);
    UnloadSound(game->soundGameOver);
    UnloadSound(game->soundBoost);
    UnloadSound(game->soundShield);
    UnloadSound(game->soundMenuSelect);
    UnloadSound(game->soundPause);
    UnloadSound(game->soundLoopComplete);

    printf("Closing audio device...\n");
    CloseAudioDevice();
}

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
    float turnRate = 0.0f;

    // Keyboard and mouse controls (full speed)
    if (IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        turnRate = 1.0f;
    }

    // Add gamepad controls with analog support
    if (engine->activeGamepad >= 0) {
        // Digital controls (full speed) - A button only
        if (IsGamepadButtonDown(engine->activeGamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {  // A button
            turnRate = 1.0f;
        }

        // Analog controls (variable speed)
        float triggerValue = engine->gamepadRightTrigger[engine->activeGamepad];
        if (triggerValue > 0.1f) {
            turnRate = fmaxf(turnRate, triggerValue);  // Use trigger value for variable turn
        }

        // Left stick horizontal axis (only left direction)
        float stickX = engine->gamepadLeftStick[engine->activeGamepad].x;
        if (stickX < -0.1f) {
            turnRate = fmaxf(turnRate, fabsf(stickX));  // Variable turn based on stick position
        }
    }

    if (turnRate > 0) {
        float turnAmount = TURN_SPEED * turnRate * deltaTime * game->difficultyMultiplier;
        rider->direction += turnAmount;
        rider->totalRotation += turnAmount;

        // Play turn sound (with rate limiting)
        static float lastTurnSound = 0;
        if (game->gameTime - lastTurnSound > 0.1f) {
            PlayTurnSound(game);
            lastTurnSound = game->gameTime;
        }

        // Check for complete turns
        if (rider->totalRotation >= 2 * PI) {
            rider->turnsCompleted++;
            rider->totalRotation -= 2 * PI;
            rider->score += 100 * rider->turnsCompleted;  // Bonus for completing circles
            SpawnParticles(game, rider->segments[0].position, GOLD, 20);
            PlayLoopCompleteSound(game);
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
                PlayGameOverSound(game);
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
            PlayPickupSound(game);
            break;

        case POWERUP_SPEED_BOOST:
            rider->boosted = true;
            rider->boostTimer = 5.0f;
            PlayBoostSound(game);
            break;

        case POWERUP_SLOW_TIME:
            game->slowTimeMultiplier = 0.5f;
            PlayPickupSound(game);
            break;

        case POWERUP_SHIELD:
            rider->shieldTimer = 10.0f;
            PlayShieldSound(game);
            break;

        case POWERUP_SHRINK:
            // Remove last few segments if possible
            if (rider->segmentCount > INITIAL_SEGMENTS) {
                rider->segmentCount -= 3;
                if (rider->segmentCount < INITIAL_SEGMENTS) {
                    rider->segmentCount = INITIAL_SEGMENTS;
                }
            }
            PlayPickupSound(game);
            break;

        case POWERUP_BONUS_POINTS:
            rider->score += 500;
            PlayPickupSound(game);
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
        Color warningColor = (Color){255, 255, 255, (int)(pulse * 100)};
        DrawCylinder((Vector3){0, -1, 0}, 0, halfSize * 2, 0.1f, 32, warningColor);
    }
}

void RenderPickupIndicators(GameState* game, EngineState* engine) {
    // This function handles both ON-SCREEN and OFF-SCREEN indicators for energy pickups.
    for (int i = 0; i < 20; i++) {
        if (!game->powerups[i].active || game->powerups[i].type != POWERUP_ENERGY) {
            continue;
        }

        // Get actual render dimensions and center
        int renderWidth = engine->useInternalResolution ? engine->internalWidth : engine->windowWidth;
        int renderHeight = engine->useInternalResolution ? engine->internalHeight : engine->windowHeight;
        float centerX = renderWidth / 2.0f;
        float centerY = renderHeight / 2.0f;

        // Get the 3D -> 2D projection of the pickup's center
        Vector2 screenPos = GetWorldToScreen(game->powerups[i].position, engine->camera);

        // Check if the pickup is in front of the camera
        Vector3 toPowerup = Vector3Subtract(game->powerups[i].position, engine->camera.position);
        Vector3 cameraForward = Vector3Normalize(Vector3Subtract(engine->camera.target, engine->camera.position));
        float dotProduct = Vector3DotProduct(Vector3Normalize(toPowerup), cameraForward);

        // Determine if the pickup's center is within the visible screen area
        bool isOnScreen = (dotProduct > 0.0f &&
                           screenPos.x > 0 && screenPos.x < renderWidth &&
                           screenPos.y > 0 && screenPos.y < renderHeight);

        // ===================================================================
        // CASE 1: ON-SCREEN INDICATOR (This logic is correct)
        // ===================================================================
        if (isOnScreen) {

            Color arrowColor = SKYBLUE;

            float pulse = sinf(game->gameTime * 5.0f) * 5.0f + 10.0f;
            arrowColor.a = 200;

            float arrowSize = 15.0f;
            Vector2 arrowTip = { screenPos.x, screenPos.y - pulse };
            Vector2 arrowBase1 = { arrowTip.x - arrowSize / 2.0f, arrowTip.y - arrowSize };
            Vector2 arrowBase2 = { arrowTip.x + arrowSize / 2.0f, arrowTip.y - arrowSize };

            DrawTriangle(arrowTip, arrowBase1, arrowBase2, arrowColor);
            DrawTriangleLines(arrowTip, arrowBase1, arrowBase2, Fade(BLACK, 0.5f));

        }
        // ===================================================================
        // CASE 2: OFF-SCREEN INDICATOR (Revised for consistency)
        // ===================================================================
        else {
            // Use one consistent, reliable method for ALL off-screen objects.
            // Manually project the direction onto the camera's plane. This avoids
            // inconsistencies from using GetWorldToScreen for positioning.
            Matrix viewMatrix = GetCameraMatrix(engine->camera);
            Matrix invViewMatrix = MatrixInvert(viewMatrix);
            Vector3 camRight = { invViewMatrix.m0, invViewMatrix.m1, invViewMatrix.m2 };
            Vector3 camUp    = { invViewMatrix.m4, invViewMatrix.m5, invViewMatrix.m6 };

            float rightDot = Vector3DotProduct(toPowerup, camRight);
            float upDot = Vector3DotProduct(toPowerup, camUp);

            // For objects in front but off-screen, the direction might need to be flipped.
            // When dotProduct is positive, rightDot/upDot correctly map the direction.
            // We only need to account for the screen's inverted Y-axis.
            float dx = rightDot;
            float dy = -upDot;

            // --- Clamping and drawing logic (This part is correct) ---
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < 0.001f) continue;
            dx /= dist;
            dy /= dist;

            int edgeMargin = 30;
            float maxDistX = (renderWidth / 2.0f) - edgeMargin;
            float maxDistY = (renderHeight / 2.0f) - edgeMargin;

            float scaleX = (fabsf(dx) > 0.001f) ? maxDistX / fabsf(dx) : 99999.0f;
            float scaleY = (fabsf(dy) > 0.001f) ? maxDistY / fabsf(dy) : 99999.0f;
            float scale = fminf(scaleX, scaleY);

            float edgeX = centerX + dx * scale;
            float edgeY = centerY + dy * scale;

            // ... (Rest of drawing code is fine) ...
            float angle = atan2f(dy, dx);
            float energyPercent = game->rider.energy / MAX_ENERGY;
            float arrowSize = 20.0f;
            float pulseSpeed = 5.0f;
            Color arrowColor = SKYBLUE;
            Color outlineColor = SKYBLUE;

            if (energyPercent < 0.2f) {
                arrowSize = 30.0f; pulseSpeed = 10.0f; arrowColor = RED;
                outlineColor = (Color){255, 100, 100, 255};
            } else if (energyPercent < 0.4f) {
                arrowSize = 25.0f; pulseSpeed = 7.0f; arrowColor = ORANGE;
                outlineColor = (Color){255, 200, 100, 255};
            } else if (energyPercent < 0.6f) {
                arrowColor = YELLOW; outlineColor = (Color){255, 255, 100, 255};
            }

            Vector2 arrowTip = {edgeX, edgeY};
            Vector2 arrowBase1 = { edgeX - cosf(angle - 0.5f) * arrowSize, edgeY - sinf(angle - 0.5f) * arrowSize };
            Vector2 arrowBase2 = { edgeX - cosf(angle + 0.5f) * arrowSize, edgeY - sinf(angle + 0.5f) * arrowSize };

            float pulse = sinf(game->gameTime * pulseSpeed) * 0.4f + 0.6f;
            arrowColor.a = (int)(255 * pulse);

            if (energyPercent < 0.2f) {
                float glowSize = arrowSize * 1.5f * pulse;
                Vector2 glowBase1 = { edgeX - cosf(angle - 0.5f) * glowSize, edgeY - sinf(angle - 0.5f) * glowSize };
                Vector2 glowBase2 = { edgeX - cosf(angle + 0.5f) * glowSize, edgeY - sinf(angle + 0.5f) * glowSize };
                DrawTriangle(arrowTip, glowBase1, glowBase2, Fade(RED, 0.3f));
            }
            DrawTriangle(arrowTip, arrowBase1, arrowBase2, arrowColor);
            DrawTriangleLines(arrowTip, arrowBase1, arrowBase2, outlineColor);
            float distance = Vector3Distance(game->rider.segments[0].position, game->powerups[i].position);
            int fontSize = (energyPercent < 0.2f) ? 16 : 12;
            const char* distText = TextFormat("%.0fm", distance);
            int textWidth = MeasureText(distText, fontSize);
            Vector2 textPos = { edgeX, edgeY };
            textPos.x -= textWidth/2;
            textPos.y -= (dy > 0.5f ? arrowSize + 5 : -arrowSize - fontSize);
            textPos.x = fmaxf(5, fminf(renderWidth - textWidth - 5, textPos.x));
            textPos.y = fmaxf(5, fminf(renderHeight - fontSize - 5, textPos.y));
            DrawText(distText, (int)textPos.x, (int)textPos.y, fontSize, outlineColor);
        }
    }
}

void RenderUI(GameState* game, EngineState* engine) {
    // Use internal resolution dimensions when active, otherwise use actual window size
    int screenWidth = engine->useInternalResolution ? engine->internalWidth : engine->windowWidth;
    int screenHeight = engine->useInternalResolution ? engine->internalHeight : engine->windowHeight;

    // Show menu if in menu state
    if (game->inMenu) {
        DrawText(GAME_TITLE, screenWidth / 2 - MeasureText(GAME_TITLE, 30) / 2, 50, 30, WHITE);
        DrawText("You can only steer LEFT!", screenWidth / 2 - MeasureText("You can only steer LEFT!", 18) / 2, 80, 18, SKYBLUE);

        DrawText("SELECT DIFFICULTY", screenWidth / 2 - MeasureText("SELECT DIFFICULTY", 20) / 2, 120, 20, YELLOW);

        // Easy option
        Color easyColor = (game->difficulty == DIFFICULTY_EASY) ? GREEN : WHITE;
        const char* easyText = "[LEFT] EASY";
        DrawText(easyText, screenWidth / 2 - 120, 160, 16, easyColor);
        DrawText("Normal speed", screenWidth / 2 - 120, 180, 12, LIGHTGRAY);
        DrawText("For beginners", screenWidth / 2 - 120, 195, 12, LIGHTGRAY);

        // Hardcore option
        Color hardcoreColor = (game->difficulty == DIFFICULTY_HARDCORE) ? RED : WHITE;
        const char* hardcoreText = "[RIGHT] HARDCORE";
        DrawText(hardcoreText, screenWidth / 2 + 20, 160, 16, hardcoreColor);
        DrawText("2x speed!", screenWidth / 2 + 20, 180, 12, ORANGE);
        DrawText("For experts", screenWidth / 2 + 20, 195, 12, ORANGE);

        // Start instruction
        const char* startText = (engine->activeGamepad >= 0) ? "A to start" : "Press ENTER to start";
        DrawText(startText, screenWidth / 2 - MeasureText(startText, 16) / 2, 230, 16, LIME);

        // High scores
        DrawText(TextFormat("Easy High Score: %d", game->highScore), screenWidth / 2 - 150, 270, 14, WHITE);
        DrawText(TextFormat("Hardcore High Score: %d", game->highScoreHardcore), screenWidth / 2 + 10, 270, 14, WHITE);

        // Show gamepad status
        if (engine->activeGamepad >= 0) {
            DrawText(TextFormat("Gamepad %d Connected", engine->activeGamepad + 1),
                    screenWidth / 2 - MeasureText("Gamepad 1 Connected", 12) / 2, 295, 12, LIME);
        }

        DrawText("Press ESC to exit", screenWidth / 2 - MeasureText("Press ESC to exit", 12) / 2, 320, 12, DARKGRAY);
        return;
    }

    // Game title (smaller when playing)
    DrawText(GAME_TITLE, screenWidth / 2 - MeasureText(GAME_TITLE, 20) / 2, 10, 20, WHITE);
    const char* diffText = game->difficulty == DIFFICULTY_HARDCORE ? "HARDCORE MODE" : "EASY MODE";
    Color diffColor = game->difficulty == DIFFICULTY_HARDCORE ? RED : GREEN;
    DrawText(diffText, screenWidth / 2 - MeasureText(diffText, 14) / 2, 35, 14, diffColor);

    // Score
    DrawText(TextFormat("Score: %d", (int)game->rider.score), 10, 60, 16, WHITE);
    int currentHighScore = (game->difficulty == DIFFICULTY_HARDCORE) ? game->highScoreHardcore : game->highScore;
    if (currentHighScore > 0) {
        DrawText(TextFormat("High: %d", currentHighScore), 10, 80, 12, GOLD);
    }

    // Energy bar
    float energyPercent = game->rider.energy / MAX_ENERGY;
    Color energyColor = SKYBLUE;
    if (energyPercent < 0.2f) {
        // Flash white when energy is low
        if (fmod(game->gameTime, 0.4f) < 0.2f) {
            energyColor = WHITE;
        }
    }
    DrawRectangle(10, 100, 120, 12, DARKGRAY);
    DrawRectangle(10, 100, (int)(120 * energyPercent), 12, energyColor);
    DrawRectangleLines(10, 100, 120, 12, WHITE);
    DrawText("ENERGY", 12, 101, 10, WHITE);

    // Boost indicator
    if (game->rider.boosted) {
        DrawText("BOOST!", screenWidth / 2 - MeasureText("BOOST!", 24) / 2, 60, 24, YELLOW);
    }

    // FPS counter
    if (game->showFPS) {
        int fps = GetFPS();
        Color fpsColor = fps >= 55 ? GREEN : (fps >= 30 ? YELLOW : RED);
        DrawText(TextFormat("FPS: %d", fps), screenWidth - 60, 5, 14, fpsColor);
    }

    // Shield indicator
    if (game->rider.shieldTimer > 0) {
        DrawText(TextFormat("SHIELD: %.1fs", game->rider.shieldTimer), 10, 120, 12, GREEN);
    }

    // Segments count
    DrawText(TextFormat("Length: %d", game->rider.segmentCount), 10, 135, 12, SKYBLUE);

    // Turns completed
    if (game->rider.turnsCompleted > 0) {
        DrawText(TextFormat("Loops: %d", game->rider.turnsCompleted), 10, 150, 12, GOLD);
    }

    // Controls
    // Control hints - update based on gamepad connection
    if (engine->activeGamepad >= 0) {
        DrawText("SPACE/MOUSE/A/RT/L-STICK: Turn Left", screenWidth - 220, screenHeight - 20, 10, LIGHTGRAY);
        DrawText("LB/LT/L3/R3: Camera Zoom", screenWidth - 220, screenHeight - 32, 9, DARKGRAY);
    } else {
        DrawText("SPACE or LEFT MOUSE: Turn Left", screenWidth - 180, screenHeight - 20, 10, LIGHTGRAY);
    }

    // Sound indicator
    DrawText(game->soundEnabled ? "Sound: ON (S to toggle)" : "Sound: OFF (S to toggle)",
             10, screenHeight - 20, 10, game->soundEnabled ? GREEN : DARKGRAY);

    // FPS toggle indicator
    DrawText(game->showFPS ? "FPS: ON (F to toggle)" : "FPS: OFF (F to toggle)",
             10, screenHeight - 32, 10, game->showFPS ? GREEN : DARKGRAY);

    // Pause indicator
    if (game->paused && !game->gameOver) {
        DrawText("PAUSED", screenWidth / 2 - MeasureText("PAUSED", 24) / 2, screenHeight / 2 - 12, 24, YELLOW);
        const char* resumeText = (engine->activeGamepad >= 0) ? "Press P or START to resume" : "Press P to resume";
        DrawText(resumeText, screenWidth / 2 - MeasureText(resumeText, 14) / 2, screenHeight / 2 + 20, 14, WHITE);
    }

    // Game over
    if (game->gameOver) {
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
        DrawText("GAME OVER", screenWidth / 2 - MeasureText("GAME OVER", 30) / 2, screenHeight / 2 - 60, 30, RED);
        DrawText(TextFormat("Final Score: %d", (int)game->rider.score),
                screenWidth / 2 - MeasureText(TextFormat("Final Score: %d", (int)game->rider.score), 20) / 2,
                screenHeight / 2 - 20, 20, WHITE);
        const char* modeText = game->difficulty == DIFFICULTY_HARDCORE ? "HARDCORE MODE" : "EASY MODE";
        Color modeColor = game->difficulty == DIFFICULTY_HARDCORE ? ORANGE : GREEN;
        DrawText(modeText, screenWidth / 2 - MeasureText(modeText, 14) / 2, screenHeight / 2 + 5, 14, modeColor);
        DrawText("Press ENTER to Restart", screenWidth / 2 - MeasureText("Press ENTER to Restart", 14) / 2,
                screenHeight / 2 + 30, 14, LIGHTGRAY);
        DrawText("Press M for Menu", screenWidth / 2 - MeasureText("Press M for Menu", 14) / 2,
                screenHeight / 2 + 50, 14, LIGHTGRAY);
        DrawText("Press ESC to Exit", screenWidth / 2 - MeasureText("Press ESC to Exit", 14) / 2,
                screenHeight / 2 + 70, 14, LIGHTGRAY);
    }
}

void InitGame(GameState* game) {
    // Preserve difficulty and high scores
    DifficultyLevel savedDifficulty = game->difficulty;
    int savedHighScore = game->highScore;
    int savedHighScoreHardcore = game->highScoreHardcore;

    // Preserve sound system - IMPORTANT: Don't wipe out sounds!
    Sound savedSoundPickup = game->soundPickup;
    Sound savedSoundTurn = game->soundTurn;
    Sound savedSoundGameOver = game->soundGameOver;
    Sound savedSoundBoost = game->soundBoost;
    Sound savedSoundShield = game->soundShield;
    Sound savedSoundMenuSelect = game->soundMenuSelect;
    Sound savedSoundPause = game->soundPause;
    Sound savedSoundLoopComplete = game->soundLoopComplete;
    bool savedSoundEnabled = game->soundEnabled;
    bool savedUseFallbackAudio = game->useFallbackAudio;
    float savedMasterVolume = game->masterVolume;
    bool savedShowFPS = game->showFPS;

    memset(game, 0, sizeof(GameState));

    // Restore preserved values
    game->difficulty = savedDifficulty;
    game->highScore = savedHighScore;
    game->highScoreHardcore = savedHighScoreHardcore;

    // Restore sound system
    game->soundPickup = savedSoundPickup;
    game->soundTurn = savedSoundTurn;
    game->soundGameOver = savedSoundGameOver;
    game->soundBoost = savedSoundBoost;
    game->soundShield = savedSoundShield;
    game->soundMenuSelect = savedSoundMenuSelect;
    game->soundPause = savedSoundPause;
    game->soundLoopComplete = savedSoundLoopComplete;
    game->soundEnabled = savedSoundEnabled;
    game->useFallbackAudio = savedUseFallbackAudio;
    game->masterVolume = savedMasterVolume;
    game->showFPS = savedShowFPS;

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
        // Keyboard or gamepad D-pad for difficulty selection
        if (IsKeyPressed(KEY_LEFT) ||
            (engine->activeGamepad >= 0 && IsGamepadButtonPressed(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT))) {
            game->difficulty = DIFFICULTY_EASY;
            PlayMenuSound(game);
        }
        if (IsKeyPressed(KEY_RIGHT) ||
            (engine->activeGamepad >= 0 && IsGamepadButtonPressed(engine->activeGamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))) {
            game->difficulty = DIFFICULTY_HARDCORE;
            PlayMenuSound(game);
        }
        // Start game with Enter or gamepad A button
        if (IsKeyPressed(KEY_ENTER) ||
            (engine->activeGamepad >= 0 && IsGamepadButtonPressed(engine->activeGamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) {
            game->inMenu = false;
            InitGame(game);
            PlayMenuSound(game);
        }
        return;
    }

    // Update game time
    if (!game->paused && !game->gameOver) {
        game->gameTime += deltaTime;
    }

    // Handle pause
    if ((IsKeyPressed(KEY_P) ||
         (engine->activeGamepad >= 0 && IsGamepadButtonPressed(engine->activeGamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT)))
        && !game->gameOver) {
        game->paused = !game->paused;
        PlayPauseSound(game);
    }

    // Toggle sound with S key
    if (IsKeyPressed(KEY_S)) {
        game->soundEnabled = !game->soundEnabled;
        if (game->soundEnabled) PlayMenuSound(game);
    }

    // Toggle FPS display with F key
    if (IsKeyPressed(KEY_F)) {
        game->showFPS = !game->showFPS;
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

        // Restart with Enter or gamepad A button
        if (IsKeyPressed(KEY_ENTER) ||
            (engine->activeGamepad >= 0 && IsGamepadButtonPressed(engine->activeGamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN))) {
            InitGame(game);
            PlayMenuSound(game);
            return;
        }
        // Return to menu with M key or gamepad B button
        if (IsKeyPressed(KEY_M) ||
            (engine->activeGamepad >= 0 && IsGamepadButtonPressed(engine->activeGamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))) {
            game->inMenu = true;
            game->gameOver = false;
            PlayMenuSound(game);
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

    // Initialize engine (pass 0,0 to auto-detect monitor resolution)
    EngineState* engine = Engine_Init(0, 0, GAME_TITLE);
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

    // Initialize sound system
    InitSounds(game);

    // Enable FPS counter by default
    game->showFPS = true;

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



            // Render game objects
            RenderPowerups(game);
            RenderLineRider(game);
            RenderParticles(game);
        }

        // End 3D mode to begin 2D UI rendering
        Engine_End3D(engine);

        // Render UI overlay (2D elements)
        RenderUI(game, engine);

        // Render off-screen indicators for energy pickups (skip if in menu)
        if (game->rider.alive && !game->paused && !game->gameOver && !game->inMenu) {
            RenderPickupIndicators(game, engine);
        }

        // Finalize frame and draw to screen
        Engine_EndFrame(engine);
    }

    // Save high score if needed
    if (game->rider.score > game->highScore) {
        game->highScore = (int)game->rider.score;
        // Could save to file here
    }

    // Cleanup
    UnloadSounds(game);
    free(game);
    Engine_Shutdown(engine);

    return 0;
}
