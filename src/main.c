#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define THICKNESS 1.5f
#define SCALE 30.0f
const Vector2 SIZE = {800, 800};

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float rotation;
    float deathTime;
} Ship;

typedef enum {
    ALIEN_BIG,
    ALIEN_SMALL
} AlienSize;

typedef enum {
    ASTEROID_BIG,
    ASTEROID_MEDIUM,
    ASTEROID_SMALL
} AsteroidSize;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    AsteroidSize size;
    unsigned long seed;
    bool remove;
} Asteroid;

typedef enum {
    PARTICLE_LINE,
    PARTICLE_DOT
} ParticleType;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float ttl;
    union {
        struct {
            float rotation;
            float length;
        } line;
        struct {
            float radius;
        } dot;
    } values;
    ParticleType type;
} Particle;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float ttl;
    float spawn;
    bool remove;
} Projectile;

typedef struct {
    Vector2 position;
    Vector2 direction;
    AlienSize size;
    bool remove;
    float lastShot;
    float lastDirection;
} Alien;

typedef struct {
    Ship ship;
    float change;
    float current;
    float stageStart;
    Asteroid* asteroids;
    int asteroidsCount;
    int asteroidsCapacity;
    Asteroid* asteroidsQueue;
    int asteroidsQueueCount;
    int asteroidsQueueCapacity;
    Particle* particles;
    int particlesCount;
    int particlesCapacity;
    Projectile* projectiles;
    int projectilesCount;
    int projectilesCapacity;
    Alien* aliens;
    int aliensCount;
    int aliensCapacity;
    size_t lives;
    size_t lastScore;
    size_t score;
    bool reset;
    size_t lastBloop;
    size_t bloop;
    size_t frame;
} GameState;

typedef struct {
    Sound bloopLo;
    Sound bloopHi;
    Sound shoot;
    Sound thrust;
    Sound asteroid;
    Sound boom;
} GameSound;

GameState state;
GameSound sound;

// Function prototypes
void drawLines(Vector2 origin, float scale, float rotation, Vector2* points, int pointCount, bool connect);
void drawNumber(size_t n, Vector2 position);
void drawAsteroid(Vector2 position, AsteroidSize size, unsigned long seed);
void splatLines(Vector2 position, int count);
void splatDots(Vector2 position, int count);
void hitAsteroid(Asteroid* a, Vector2* impact);
void update(void);
void drawAlien(Vector2 position, AlienSize size);
void render(void);
void resetAsteroids(void);
void resetGame(void);
void resetStage(void);

// Utility functions for dynamic arrays
void initArray(void** array, int* capacity, size_t elementSize) {
    *capacity = 16;
    *array = malloc(*capacity * elementSize);
}

void expandArray(void** array, int* capacity, size_t elementSize) {
    *capacity *= 2;
    void* newArray = realloc(*array, *capacity * elementSize);
    if (newArray) {
        *array = newArray;
    }
}

// Helper functions for game mechanics
float getAlienCollisionSize(AlienSize size) {
    return size == ALIEN_BIG ? SCALE * 0.8f : SCALE * 0.5f;
}

float getAlienDirectionChangeTime(AlienSize size) {
    return size == ALIEN_BIG ? 0.85f : 0.35f;
}

float getAlienShotTime(AlienSize size) {
    return size == ALIEN_BIG ? 1.25f : 0.75f;
}

float getAlienSpeed(AlienSize size) {
    return size == ALIEN_BIG ? 3.0f : 6.0f;
}

// Main game functions
void drawLines(Vector2 origin, float scale, float rotation, Vector2* points, int pointCount, bool connect) {
    for (int i = 0; i < (connect ? pointCount : pointCount - 1); i++) {
        Vector2 p1 = points[i];
        Vector2 p2 = points[(i + 1) % pointCount];
        
        // Scale and rotate points
        p1 = Vector2Rotate(p1, rotation);
        p2 = Vector2Rotate(p2, rotation);
        p1 = Vector2Scale(p1, scale);
        p2 = Vector2Scale(p2, scale);
        
        // Translate to origin
        p1 = Vector2Add(p1, origin);
        p2 = Vector2Add(p2, origin);
        
        DrawLineEx(p1, p2, THICKNESS, WHITE);
    }
}

#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

// [Previous code remains the same up to the main function]

// Asteroid size utilities
float getAsteroidScore(AsteroidSize size) {
    switch (size) {
        case ASTEROID_BIG: return 20;
        case ASTEROID_MEDIUM: return 50;
        case ASTEROID_SMALL: return 100;
        default: return 0;
    }
}

float getAsteroidSize(AsteroidSize size) {
    switch (size) {
        case ASTEROID_BIG: return SCALE * 3.0f;
        case ASTEROID_MEDIUM: return SCALE * 1.4f;
        case ASTEROID_SMALL: return SCALE * 0.8f;
        default: return 0;
    }
}

float getAsteroidCollisionScale(AsteroidSize size) {
    switch (size) {
        case ASTEROID_BIG: return 0.4f;
        case ASTEROID_MEDIUM: return 0.65f;
        case ASTEROID_SMALL: return 1.0f;
        default: return 0;
    }
}

float getAsteroidVelocityScale(AsteroidSize size) {
    switch (size) {
        case ASTEROID_BIG: return 0.75f;
        case ASTEROID_MEDIUM: return 0.9f;
        case ASTEROID_SMALL: return 1.5f;
        default: return 0;
    }
}

void drawShip(void) {
    const Vector2 shipPoints[] = {
        {-0.4f, -0.5f},
        {0.0f, 0.5f},
        {0.4f, -0.5f},
        {0.3f, -0.4f},
        {-0.3f, -0.4f},
    };

    if (!state.ship.deathTime) {
        drawLines(
            state.ship.position,
            SCALE,
            state.ship.rotation,
            (Vector2*)shipPoints,
            5,
            true
        );

        // Draw thrust
        if (IsKeyDown(KEY_W) && ((int)(state.current * 30) % 2 == 0)) {
            const Vector2 thrustPoints[] = {
                {-0.3f, -0.4f},
                {0.0f, -0.8f},
                {0.3f, -0.4f}
            };
            drawLines(
                state.ship.position,
                SCALE,
                state.ship.rotation,
                (Vector2*)thrustPoints,
                3,
                true
            );
        }
    }
}

void drawAsteroid(Vector2 position, AsteroidSize size, unsigned long seed) {
    srand(seed);
    int n = 8 + rand() % 7;  // 8 to 14 points
    Vector2 points[15];  // Max points + 1 for safety

    for (int i = 0; i < n; i++) {
        float radius = 0.3f + (0.2f * ((float)rand() / RAND_MAX));
        if ((float)rand() / RAND_MAX < 0.2f) {
            radius -= 0.2f;
        }

        float angle = ((float)i * (2 * PI / n)) + (PI * 0.125f * ((float)rand() / RAND_MAX));
        points[i] = (Vector2){
            cosf(angle) * radius,
            sinf(angle) * radius
        };
    }

    drawLines(position, getAsteroidSize(size), 0.0f, points, n, true);
}

void update(void) {
    if (state.reset) {
        state.reset = false;
        resetGame();
        return;
    }

    // Ship controls
    if (!state.ship.deathTime) {
        const float ROTATION_SPEED = 2.0f;
        const float SHIP_SPEED = 24.0f;

        // Rotation
        if (IsKeyDown(KEY_A)) {
            state.ship.rotation -= state.change * (2 * PI) * ROTATION_SPEED;
        }
        if (IsKeyDown(KEY_D)) {
            state.ship.rotation += state.change * (2 * PI) * ROTATION_SPEED;
        }

        // Thrust
        float directionAngle = state.ship.rotation + (PI * 0.5f);
        Vector2 shipDirection = {
            cosf(directionAngle),
            sinf(directionAngle)
        };

        if (IsKeyDown(KEY_W)) {
            Vector2 thrust = Vector2Scale(shipDirection, state.change * SHIP_SPEED);
            state.ship.velocity = Vector2Add(state.ship.velocity, thrust);

            if (state.frame % 2 == 0) {
                PlaySound(sound.thrust);
            }
        }

        // Drag
        const float DRAG = 0.019f;
        state.ship.velocity = Vector2Scale(state.ship.velocity, 1.0f - DRAG);
        state.ship.position = Vector2Add(state.ship.position, state.ship.velocity);

        // Wrap around screen
        state.ship.position.x = fmodf(state.ship.position.x + SIZE.x, SIZE.x);
        state.ship.position.y = fmodf(state.ship.position.y + SIZE.y, SIZE.y);

        // Shooting
        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (state.projectilesCount < state.projectilesCapacity) {
                Vector2 shootPos = Vector2Add(
                    state.ship.position,
                    Vector2Scale(shipDirection, SCALE * 0.55f)
                );
                
                Projectile newProjectile = {
                    .position = shootPos,
                    .velocity = Vector2Scale(shipDirection, 10.0f),
                    .ttl = 2.0f,
                    .spawn = state.current,
                    .remove = false
                };

                state.projectiles[state.projectilesCount++] = newProjectile;
                PlaySound(sound.shoot);

                // Recoil
                state.ship.velocity = Vector2Add(
                    state.ship.velocity, 
                    Vector2Scale(shipDirection, -0.5f)
                );
            }
        }
    }

    // Update asteroids
    for (int i = 0; i < state.asteroidsCount; i++) {
        Asteroid* a = &state.asteroids[i];
        if (a->remove) continue;

        a->position = Vector2Add(a->position, a->velocity);
        a->position.x = fmodf(a->position.x + SIZE.x, SIZE.x);
        a->position.y = fmodf(a->position.y + SIZE.y, SIZE.y);

        // Check ship collision
        if (!state.ship.deathTime) {
            float dist = Vector2Distance(a->position, state.ship.position);
            if (dist < getAsteroidSize(a->size) * getAsteroidCollisionScale(a->size)) {
                state.ship.deathTime = state.current;
                hitAsteroid(a, &state.ship.velocity);
            }
        }
    }

    // Clean up destroyed asteroids
    for (int i = state.asteroidsCount - 1; i >= 0; i--) {
        if (state.asteroids[i].remove) {
            state.asteroids[i] = state.asteroids[--state.asteroidsCount];
        }
    }

    // Add queued asteroids
    for (int i = 0; i < state.asteroidsQueueCount; i++) {
        if (state.asteroidsCount < state.asteroidsCapacity) {
            state.asteroids[state.asteroidsCount++] = state.asteroidsQueue[i];
        }
    }
    state.asteroidsQueueCount = 0;
}

void render(void) {
    // Draw lives
    for (size_t i = 0; i < state.lives; i++) {
        const Vector2 shipPoints[] = {
            {-0.4f, -0.5f},
            {0.0f, 0.5f},
            {0.4f, -0.5f},
            {0.3f, -0.4f},
            {-0.3f, -0.4f}
        };
        drawLines(
            (Vector2){SCALE + (i * SCALE), SCALE},
            SCALE,
            -PI,
            (Vector2*)shipPoints,
            5,
            true
        );
    }

    // Draw score
    drawNumber(state.score, (Vector2){SIZE.x - SCALE, SCALE});

    // Draw ship
    drawShip();

    // Draw asteroids
    for (int i = 0; i < state.asteroidsCount; i++) {
        if (!state.asteroids[i].remove) {
            drawAsteroid(
                state.asteroids[i].position,
                state.asteroids[i].size,
                state.asteroids[i].seed
            );
        }
    }

    // Draw projectiles
    for (int i = 0; i < state.projectilesCount; i++) {
        if (!state.projectiles[i].remove) {
            DrawCircleV(
                state.projectiles[i].position,
                fmaxf(SCALE * 0.05f, 1.0f),
                WHITE
            );
        }
    }
}

// Particle effects and alien ships implementation

void splatDots(Vector2 position, int count) {
    for (int i = 0; i < count; i++) {
        if (state.particlesCount >= state.particlesCapacity) {
            expandArray((void**)&state.particles, &state.particlesCapacity, sizeof(Particle));
        }

        float angle = (2 * PI) * ((float)rand() / RAND_MAX);
        Vector2 velocity = {
            cosf(angle) * (2.0f + 4.0f * ((float)rand() / RAND_MAX)),
            sinf(angle) * (2.0f + 4.0f * ((float)rand() / RAND_MAX))
        };

        Particle particle = {
            .position = Vector2Add(
                position,
                (Vector2){
                    ((float)rand() / RAND_MAX) * 3,
                    ((float)rand() / RAND_MAX) * 3
                }
            ),
            .velocity = velocity,
            .ttl = 0.5f + (0.4f * ((float)rand() / RAND_MAX)),
            .type = PARTICLE_DOT,
            .values.dot.radius = SCALE * 0.025f
        };

        state.particles[state.particlesCount++] = particle;
    }
}

void splatLines(Vector2 position, int count) {
    for (int i = 0; i < count; i++) {
        if (state.particlesCount >= state.particlesCapacity) {
            expandArray((void**)&state.particles, &state.particlesCapacity, sizeof(Particle));
        }

        float angle = (2 * PI) * ((float)rand() / RAND_MAX);
        Vector2 velocity = {
            cosf(angle) * (2.0f * ((float)rand() / RAND_MAX)),
            sinf(angle) * (2.0f * ((float)rand() / RAND_MAX))
        };

        Particle particle = {
            .position = Vector2Add(
                position,
                (Vector2){
                    ((float)rand() / RAND_MAX) * 3,
                    ((float)rand() / RAND_MAX) * 3
                }
            ),
            .velocity = velocity,
            .ttl = 3.0f + ((float)rand() / RAND_MAX),
            .type = PARTICLE_LINE,
            .values.line = {
                .rotation = (2 * PI) * ((float)rand() / RAND_MAX),
                .length = SCALE * (0.6f + (0.4f * ((float)rand() / RAND_MAX)))
            }
        };

        state.particles[state.particlesCount++] = particle;
    }
}

void drawAlien(Vector2 position, AlienSize size) {
    float scale = (size == ALIEN_BIG) ? 1.0f : 0.5f;
    
    const Vector2 bodyPoints[] = {
        {-0.5f, 0.0f},
        {-0.3f, 0.3f},
        {0.3f, 0.3f},
        {0.5f, 0.0f},
        {0.3f, -0.3f},
        {-0.3f, -0.3f},
        {-0.5f, 0.0f},
        {0.5f, 0.0f}
    };

    const Vector2 domePoints[] = {
        {-0.2f, -0.3f},
        {-0.1f, -0.5f},
        {0.1f, -0.5f},
        {0.2f, -0.3f}
    };

    drawLines(position, SCALE * scale, 0, (Vector2*)bodyPoints, 8, false);
    drawLines(position, SCALE * scale, 0, (Vector2*)domePoints, 4, false);
}

void updateParticles(void) {
    int i = 0;
    while (i < state.particlesCount) {
        Particle* p = &state.particles[i];
        p->position = Vector2Add(p->position, p->velocity);
        p->position.x = fmodf(p->position.x + SIZE.x, SIZE.x);
        p->position.y = fmodf(p->position.y + SIZE.y, SIZE.y);

        p->ttl -= state.change;
        if (p->ttl <= 0) {
            state.particles[i] = state.particles[--state.particlesCount];
        } else {
            i++;
        }
    }
}

void updateAliens(void) {
    int i = 0;
    while (i < state.aliensCount) {
        Alien* a = &state.aliens[i];
        
        if (!a->remove) {
            // Update direction periodically
            if ((state.current - a->lastDirection) > getAlienDirectionChangeTime(a->size)) {
                a->lastDirection = state.current;
                float angle = (2 * PI) * ((float)rand() / RAND_MAX);
                a->direction = (Vector2){cosf(angle), sinf(angle)};
            }

            // Move alien
            a->position = Vector2Add(
                a->position,
                Vector2Scale(a->direction, getAlienSpeed(a->size))
            );
            a->position.x = fmodf(a->position.x + SIZE.x, SIZE.x);
            a->position.y = fmodf(a->position.y + SIZE.y, SIZE.y);

            // Shooting logic
            if ((state.current - a->lastShot) > getAlienShotTime(a->size)) {
                a->lastShot = state.current;
                if (state.projectilesCount < state.projectilesCapacity) {
                    Vector2 direction = Vector2Normalize(
                        Vector2Subtract(state.ship.position, a->position)
                    );
                    
                    Projectile newProjectile = {
                        .position = Vector2Add(
                            a->position,
                            Vector2Scale(direction, SCALE * 0.55f)
                        ),
                        .velocity = Vector2Scale(direction, 6.0f),
                        .ttl = 2.0f,
                        .spawn = state.current,
                        .remove = false
                    };

                    state.projectiles[state.projectilesCount++] = newProjectile;
                    PlaySound(sound.shoot);
                }
            }

            // Check for collision with ship
            if (!state.ship.deathTime && 
                Vector2Distance(a->position, state.ship.position) < getAlienCollisionSize(a->size)) {
                a->remove = true;
                state.ship.deathTime = state.current;
            }
        }

        if (a->remove) {
            PlaySound(sound.asteroid);
            splatDots(a->position, 15);
            splatLines(a->position, 4);
            state.aliens[i] = state.aliens[--state.aliensCount];
        } else {
            i++;
        }
    }
}

void hitAsteroid(Asteroid* a, Vector2* impact) {
    PlaySound(sound.asteroid);
    state.score += getAsteroidScore(a->size);
    a->remove = true;

    splatDots(a->position, 10);

    if (a->size == ASTEROID_SMALL) {
        return;
    }

    // Split asteroid into smaller pieces
    for (int i = 0; i < 2; i++) {
        if (state.asteroidsQueueCount >= state.asteroidsQueueCapacity) {
            expandArray((void**)&state.asteroidsQueue, &state.asteroidsQueueCapacity, sizeof(Asteroid));
        }

        Vector2 direction = Vector2Normalize(a->velocity);
        AsteroidSize newSize = (a->size == ASTEROID_BIG) ? ASTEROID_MEDIUM : ASTEROID_SMALL;

        Vector2 newVelocity = Vector2Scale(
            direction,
            getAsteroidVelocityScale(a->size) * 2.2f * ((float)rand() / RAND_MAX)
        );

        if (impact) {
            newVelocity = Vector2Add(newVelocity, Vector2Scale(*impact, 0.7f));
        }

        Asteroid newAsteroid = {
            .position = a->position,
            .velocity = newVelocity,
            .size = newSize,
            .seed = rand(),
            .remove = false
        };

        state.asteroidsQueue[state.asteroidsQueueCount++] = newAsteroid;
    }
}

void resetAsteroids(void) {
    state.asteroidsCount = 0;
    state.asteroidsQueueCount = 0;

    int baseCount = 30 + (state.score / 1500);
    for (int i = 0; i < baseCount; i++) {
        if (state.asteroidsQueueCount >= state.asteroidsQueueCapacity) {
            expandArray((void**)&state.asteroidsQueue, &state.asteroidsQueueCapacity, sizeof(Asteroid));
        }

        float angle = (2 * PI) * ((float)rand() / RAND_MAX);
        AsteroidSize size = rand() % 3;  // Random size between BIG, MEDIUM, SMALL

        Asteroid newAsteroid = {
            .position = (Vector2){
                ((float)rand() / RAND_MAX) * SIZE.x,
                ((float)rand() / RAND_MAX) * SIZE.y
            },
            .velocity = Vector2Scale(
                (Vector2){cosf(angle), sinf(angle)},
                getAsteroidVelocityScale(size) * 3.0f * ((float)rand() / RAND_MAX)
            ),
            .size = size,
            .seed = rand(),
            .remove = false
        };

        state.asteroidsQueue[state.asteroidsQueueCount++] = newAsteroid;
    }

    state.stageStart = state.current;
}

// Add these function calls to the main update() function:
void updateGameLogic(void) {
    updateParticles();
    updateAliens();

    // Spawn new aliens based on score
    if ((state.lastScore / 5000) != (state.score / 5000)) {
        if (state.aliensCount < state.aliensCapacity) {
            Alien newAlien = {
                .position = (Vector2){
                    (rand() % 2) ? 0 : SIZE.x - SCALE,
                    ((float)rand() / RAND_MAX) * SIZE.y
                },
                .direction = (Vector2){0, 0},
                .size = ALIEN_BIG,
                .remove = false,
                .lastShot = 0,
                .lastDirection = 0
            };
            state.aliens[state.aliensCount++] = newAlien;
        }
    }

    if ((state.lastScore / 8000) != (state.score / 8000)) {
        if (state.aliensCount < state.aliensCapacity) {
            Alien newAlien = {
                .position = (Vector2){
                    (rand() % 2) ? 0 : SIZE.x - SCALE,
                    ((float)rand() / RAND_MAX) * SIZE.y
                },
                .direction = (Vector2){0, 0},
                .size = ALIEN_SMALL,
                .remove = false,
                .lastShot = 0,
                .lastDirection = 0
            };
            state.aliens[state.aliensCount++] = newAlien;
        }
    }

    state.lastScore = state.score;
}



int main(void) {
    InitWindow(SIZE.x, SIZE.y, "Asteroids");
    SetWindowPosition(100, 100);
    SetTargetFPS(60);

    InitAudioDevice();

    // Initialize game state
    state = (GameState){0};
    state.ship.position = (Vector2){SIZE.x/2, SIZE.y/2};
    
    // Initialize dynamic arrays
    initArray((void**)&state.asteroids, &state.asteroidsCapacity, sizeof(Asteroid));
    initArray((void**)&state.asteroidsQueue, &state.asteroidsQueueCapacity, sizeof(Asteroid));
    initArray((void**)&state.particles, &state.particlesCapacity, sizeof(Particle));
    initArray((void**)&state.projectiles, &state.projectilesCapacity, sizeof(Projectile));
    initArray((void**)&state.aliens, &state.aliensCapacity, sizeof(Alien));

    // Load sounds
    sound.bloopLo = LoadSound("sounds/bloop_lo.wav");
    sound.bloopHi = LoadSound("sounds/bloop_hi.wav");
    sound.shoot = LoadSound("sounds/shoot.wav");
    sound.thrust = LoadSound("sounds/thrust.wav");
    sound.asteroid = LoadSound("sounds/asteroid.wav");
    sound.boom = LoadSound("sounds/explode.wav");

    resetGame();

    while (!WindowShouldClose()) {
        state.change = GetFrameTime();
        state.current += state.change;

        update();

        BeginDrawing();
        ClearBackground(BLACK);
        render();
        EndDrawing();

        state.frame++;
    }

    // Cleanup
    free(state.asteroids);
    free(state.asteroidsQueue);
    free(state.particles);
    free(state.projectiles);
    free(state.aliens);

    UnloadSound(sound.bloopLo);
    UnloadSound(sound.bloopHi);
    UnloadSound(sound.shoot);
    UnloadSound(sound.thrust);
    UnloadSound(sound.asteroid);
    UnloadSound(sound.boom);

    CloseAudioDevice();
    CloseWindow();

    return 0;
}