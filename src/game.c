#include <raylib.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <raymath.h>
#include <time.h>
#include <string.h>
#include "game.h"

#include <emscripten.h>

#ifdef __EMSCRIPTEN__
#define GAME_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define GAME_EXPORT
#endif
#define THICKNESS 1.5f
#define SCALE 30.0f
#define PI 3.14159265358979323846f


// Global state variables
static GameState state;
static GameSound sound;
static Vector2 window_size = {800, 800};


// Functions to get alien properties based on size
float getAlienCollisionSize(AlienSize size) {
    switch (size) {
        case ALIEN_BIG: return SCALE * 0.8f;
        case ALIEN_SMALL: return SCALE * 0.5f;
    }
    return 0.0f;
}

float getAlienDirectionChangeTime(AlienSize size) {
    switch (size) {
        case ALIEN_BIG: return 0.85f;
        case ALIEN_SMALL: return 0.35f;
    }
    return 0.0f;
}

float getAlienShotTime(AlienSize size) {
    switch (size) {
        case ALIEN_BIG: return 1.25f;
        case ALIEN_SMALL: return 0.75f;
    }
    return 0.0f;
}

float getAlienSpeed(AlienSize size) {
    switch (size) {
        case ALIEN_BIG: return 3.0f;
        case ALIEN_SMALL: return 6.0f;
    }
    return 0.0f;
}

// Functions to get asteroid properties based on size
int getAsteroidScore(AsteroidSize size) {
    switch (size) {
        case ASTEROID_BIG: return 20;
        case ASTEROID_MEDIUM: return 50;
        case ASTEROID_SMALL: return 100;
    }
    return 0;
}

float getAsteroidSize(AsteroidSize size) {
    switch (size) {
        case ASTEROID_BIG: return SCALE * 3.0f;
        case ASTEROID_MEDIUM: return SCALE * 1.4f;
        case ASTEROID_SMALL: return SCALE * 0.8f;
    }
    return 0.0f;
}

float getAsteroidCollisionScale(AsteroidSize size) {
    switch (size) {
        case ASTEROID_BIG: return 0.4f;
        case ASTEROID_MEDIUM: return 0.65f;
        case ASTEROID_SMALL: return 1.0f;
    }
    return 0.0f;
}

float getAsteroidVelocityScale(AsteroidSize size) {
    switch (size) {
        case ASTEROID_BIG: return 0.75f;
        case ASTEROID_MEDIUM: return 0.9f;
        case ASTEROID_SMALL: return 1.5f;
    }
    return 0.0f;
}

// Dynamic array management functions
#define INITIAL_CAPACITY 16

// Generic array resize function
static void* resizeArray(void* array, size_t* capacity, size_t element_size) {
    size_t new_capacity = (*capacity == 0) ? INITIAL_CAPACITY : *capacity * 2;
    void* new_array = realloc(array, new_capacity * element_size);
    if (new_array) {
        *capacity = new_capacity;
    }
    return new_array;
}

// Asteroid array functions
static bool addAsteroid(GameState* state, Asteroid asteroid) {
    if (state->asteroids_count >= state->asteroids_capacity) {
        Asteroid* new_array = resizeArray(state->asteroids, 
                                        &state->asteroids_capacity, 
                                        sizeof(Asteroid));
        if (!new_array) return false;
        state->asteroids = new_array;
    }
    state->asteroids[state->asteroids_count++] = asteroid;
    return true;
}

static bool addAsteroidToQueue(GameState* state, Asteroid asteroid) {
    if (state->asteroids_queue_count >= state->asteroids_queue_capacity) {
        Asteroid* new_array = resizeArray(state->asteroids_queue, 
                                        &state->asteroids_queue_capacity, 
                                        sizeof(Asteroid));
        if (!new_array) return false;
        state->asteroids_queue = new_array;
    }
    state->asteroids_queue[state->asteroids_queue_count++] = asteroid;
    return true;
}

// Particle array functions
static bool addParticle(GameState* state, Particle particle) {
    if (state->particles_count >= state->particles_capacity) {
        Particle* new_array = resizeArray(state->particles, 
                                        &state->particles_capacity, 
                                        sizeof(Particle));
        if (!new_array) return false;
        state->particles = new_array;
    }
    state->particles[state->particles_count++] = particle;
    return true;
}

// Projectile array functions
static bool addProjectile(GameState* state, Projectile projectile) {
    if (state->projectiles_count >= state->projectiles_capacity) {
        Projectile* new_array = resizeArray(state->projectiles, 
                                          &state->projectiles_capacity, 
                                          sizeof(Projectile));
        if (!new_array) return false;
        state->projectiles = new_array;
    }
    state->projectiles[state->projectiles_count++] = projectile;
    return true;
}

// Alien array functions
static bool addAlien(GameState* state, Alien alien) {
    if (state->aliens_count >= state->aliens_capacity) {
        Alien* new_array = resizeArray(state->aliens, 
                                     &state->aliens_capacity, 
                                     sizeof(Alien));
        if (!new_array) return false;
        state->aliens = new_array;
    }
    state->aliens[state->aliens_count++] = alien;
    return true;
}

// Initialize game state arrays
static bool initGameArrays(GameState* state) {
    // Initialize all arrays to NULL and counts/capacities to 0
    state->asteroids = NULL;
    state->asteroids_queue = NULL;
    state->particles = NULL;
    state->projectiles = NULL;
    state->aliens = NULL;
    
    state->asteroids_count = 0;
    state->asteroids_queue_count = 0;
    state->particles_count = 0;
    state->projectiles_count = 0;
    state->aliens_count = 0;
    
    state->asteroids_capacity = 0;
    state->asteroids_queue_capacity = 0;
    state->particles_capacity = 0;
    state->projectiles_capacity = 0;
    state->aliens_capacity = 0;
    
    return true;
}



// Free game state arrays
static void freeGameArrays(GameState* state) {
    free(state->asteroids);
    free(state->asteroids_queue);
    free(state->particles);
    free(state->projectiles);
    free(state->aliens);
}

// Remove element at index from array
static void removeFromArray(void* array, size_t* count, size_t index, size_t element_size) {
    if (index >= *count) return;
    
    char* char_array = (char*)array;
    if (index < *count - 1) {
        memmove(&char_array[index * element_size],
                &char_array[(index + 1) * element_size],
                (*count - index - 1) * element_size);
    }
    (*count)--;
}

// Helper functions for removing elements from specific arrays
static void removeAsteroid(GameState* state, size_t index) {
    removeFromArray(state->asteroids, &state->asteroids_count, index, sizeof(Asteroid));
}

static void removeParticle(GameState* state, size_t index) {
    removeFromArray(state->particles, &state->particles_count, index, sizeof(Particle));
}

static void removeProjectile(GameState* state, size_t index) {
    removeFromArray(state->projectiles, &state->projectiles_count, index, sizeof(Projectile));
}

static void removeAlien(GameState* state, size_t index) {
    removeFromArray(state->aliens, &state->aliens_count, index, sizeof(Alien));
}

// Clear array contents without freeing memory
static void clearArray(void* array, size_t* count) {
    *count = 0;
}

// Helper functions for clearing specific arrays
static void clearAsteroids(GameState* state) {
    clearArray(state->asteroids, &state->asteroids_count);
}

static void clearAsteroidsQueue(GameState* state) {
    clearArray(state->asteroids_queue, &state->asteroids_queue_count);
}

static void clearParticles(GameState* state) {
    clearArray(state->particles, &state->particles_count);
}

static void clearProjectiles(GameState* state) {
    clearArray(state->projectiles, &state->projectiles_count);
}

static void clearAliens(GameState* state) {
    clearArray(state->aliens, &state->aliens_count);
}

void UpdateWindowSize(void) {
    #ifdef __EMSCRIPTEN__
    // Get the browser window size
    int browserWidth = EM_ASM_INT({
        return window.innerWidth;
    });
    int browserHeight = EM_ASM_INT({
        return window.innerHeight;
    });
    
    // Calculate new game size (maintain square aspect ratio)
    int size = browserHeight < browserWidth ? browserHeight : browserWidth;
    size = size > 1000 ? 1000 : size; // Cap maximum size
    size = (size / 100) * 100; // Round to nearest hundred
    
    window_size.x = size;
    window_size.y = size;
    
    SetWindowSize(size, size);
    #endif
}

// Utility function to draw lines from points
static void drawLines(Vector2 origin, float scale, float rotation, const Vector2* points, int pointCount, bool connect) {
    // Temporary array for transformed points
    Vector2* transformed = (Vector2*)malloc(pointCount * sizeof(Vector2));
    if (!transformed) return;

    // Transform all points
    for (int i = 0; i < pointCount; i++) {
        // Scale
        Vector2 scaled = Vector2Scale(points[i], scale);
        // Rotate
        Vector2 rotated = Vector2Rotate(scaled, rotation);
        // Translate
        transformed[i] = Vector2Add(rotated, origin);
    }

    // Draw lines between points
    int bound = connect ? pointCount : (pointCount - 1);
    for (int i = 0; i < bound; i++) {
        DrawLineEx(
            transformed[i],
            transformed[(i + 1) % pointCount],
            THICKNESS,
            WHITE
        );
    }

    free(transformed);
}

// Draw a number at specified position
static void drawNumber(size_t n, Vector2 position) {
    // Number drawing patterns (coordinates for line segments)
    static const float NUMBER_LINES[10][10][2] = {
        {{0,0}, {1,0}, {1,1}, {0,1}, {0,0}},  // 0
        {{0.5,0}, {0.5,1}},                    // 1
        {{0,1}, {1,1}, {1,0.5}, {0,0.5}, {0,0}, {1,0}},  // 2
        {{0,1}, {1,1}, {1,0.5}, {0,0.5}, {1,0.5}, {1,0}, {0,0}},  // 3
        {{0,1}, {0,0.5}, {1,0.5}, {1,1}, {1,0}},  // 4
        {{1,1}, {0,1}, {0,0.5}, {1,0.5}, {1,0}, {0,0}},  // 5
        {{0,1}, {0,0}, {1,0}, {1,0.5}, {0,0.5}},  // 6
        {{0,1}, {1,1}, {1,0}},  // 7
        {{0,0}, {1,0}, {1,1}, {0,1}, {0,0.5}, {1,0.5}},  // 8
        {{1,0}, {1,1}, {0,1}, {0,0.5}, {1,0.5}}  // 9
    };
    
    static const int NUMBER_POINTS[] = {5,2,6,7,5,6,5,3,6,5};  // Number of points for each digit

    Vector2 pos = position;
    
    // Count digits
    size_t temp = n;
    int digits = 0;
    do {
        digits++;
        temp /= 10;
    } while (temp > 0);

    // Draw each digit
    temp = n;
    for (int i = 0; i < digits; i++) {
        int digit = temp % 10;
        
        // Create points array for this digit
        Vector2 points[10];  // Max 10 points per digit
        for (int j = 0; j < NUMBER_POINTS[digit]; j++) {
            points[j] = (Vector2){
                NUMBER_LINES[digit][j][0] - 0.5f,
                (1.0f - NUMBER_LINES[digit][j][1]) - 0.5f
            };
        }
        
        drawLines(pos, SCALE * 0.8f, 0, points, NUMBER_POINTS[digit], false);
        pos.x -= SCALE;
        temp /= 10;
    }
}

// Draw the ship
static void drawShip(const Ship* ship) {
    if (ship->deathTime == 0.0f) {  // Only draw if ship is alive
        static const Vector2 SHIP_POINTS[] = {
            {-0.4f, -0.5f},
            {0.0f, 0.5f},
            {0.4f, -0.5f},
            {0.3f, -0.4f},
            {-0.3f, -0.4f}
        };
        
        drawLines(ship->position, SCALE, ship->rotation, SHIP_POINTS, 5, true);
    }
}

// Draw ship thrust
static void drawShipThrust(const Ship* ship) {
    static const Vector2 THRUST_POINTS[] = {
        {-0.3f, -0.4f},
        {0.0f, -0.8f},
        {0.3f, -0.4f}
    };
    
    drawLines(ship->position, SCALE, ship->rotation, THRUST_POINTS, 3, true);
}

// Draw an asteroid
static void drawAsteroid(const Asteroid* asteroid) {
    // Generate points based on seed
    srand(asteroid->seed);
    
    int numPoints = 8 + (rand() % 7);  // 8 to 14 points
    Vector2* points = (Vector2*)malloc(numPoints * sizeof(Vector2));
    if (!points) return;

    for (int i = 0; i < numPoints; i++) {
        float radius = 0.3f + (0.2f * ((float)rand() / RAND_MAX));
        if ((float)rand() / RAND_MAX < 0.2f) {
            radius -= 0.2f;
        }

        float angle = ((float)i * (2.0f * PI / numPoints)) + 
                     (PI * 0.125f * ((float)rand() / RAND_MAX));
        
        points[i] = (Vector2){
            radius * cosf(angle),
            radius * sinf(angle)
        };
    }

    drawLines(asteroid->position, getAsteroidSize(asteroid->size), 0.0f, 
             points, numPoints, true);
    
    free(points);
}

// Draw an alien
static void drawAlien(const Alien* alien) {
    float scale = (alien->size == ALIEN_BIG) ? 1.0f : 0.5f;
    
    // Body points
    static const Vector2 ALIEN_BODY[] = {
        {-0.5f, 0.0f},
        {-0.3f, 0.3f},
        {0.3f, 0.3f},
        {0.5f, 0.0f},
        {0.3f, -0.3f},
        {-0.3f, -0.3f},
        {-0.5f, 0.0f},
        {0.5f, 0.0f}
    };
    
    // Bottom points
    static const Vector2 ALIEN_BOTTOM[] = {
        {-0.2f, -0.3f},
        {-0.1f, -0.5f},
        {0.1f, -0.5f},
        {0.2f, -0.3f}
    };
    
    drawLines(alien->position, SCALE * scale, 0, ALIEN_BODY, 8, false);
    drawLines(alien->position, SCALE * scale, 0, ALIEN_BOTTOM, 4, false);
}

// Draw a particle
static void drawParticle(const Particle* particle) {
    switch (particle->type) {
        case PARTICLE_LINE:
            {
                Vector2 line_points[] = {
                    {-0.5f, 0.0f},
                    {0.5f, 0.0f}
                };
                drawLines(particle->position, particle->values.line.length,
                         particle->values.line.rotation, line_points, 2, true);
            }
            break;
            
        case PARTICLE_DOT:
            DrawCircleV(particle->position, particle->values.dot.radius, WHITE);
            break;
    }
}

// Draw a projectile
static void drawProjectile(const Projectile* projectile) {
    DrawCircleV(projectile->position, 
                fmaxf(SCALE * 0.05f, 1.0f), WHITE);
}

// Main game initialization and loop
#include <time.h>







// Modified sound loading for web
static bool loadGameSounds(void) {
    #ifdef __EMSCRIPTEN__
    // Web audio needs to load from preloaded files
    sound.bloopLo = LoadSound("sounds/bloop_lo.wav");
    sound.bloopHi = LoadSound("sounds/bloop_hi.wav");
    sound.shoot = LoadSound("sounds/shoot.wav");
    sound.thrust = LoadSound("sounds/thrust.wav");
    sound.asteroid = LoadSound("sounds/asteroid.wav");
    sound.boom = LoadSound("sounds/explode.wav");
    #else
    // Original desktop sound loading
    sound.bloopLo = LoadSound("./sounds/bloop_lo.wav");
    sound.bloopHi = LoadSound("./sounds/bloop_hi.wav");
    sound.shoot = LoadSound("./sounds/shoot.wav");
    sound.thrust = LoadSound("./sounds/thrust.wav");
    sound.asteroid = LoadSound("./sounds/asteroid.wav");
    sound.boom = LoadSound("./sounds/explode.wav");
    #endif
    return true;
}
// Unload game sounds
static void unloadGameSounds(void) {
    UnloadSound(sound.bloopLo);
    UnloadSound(sound.bloopHi);
    UnloadSound(sound.shoot);
    UnloadSound(sound.thrust);
    UnloadSound(sound.asteroid);
    UnloadSound(sound.boom);
}

// // Add game over text rendering
// static void drawGameOver(void) {
//     const Vector2 position = {WINDOW_SIZE.x / 2 - SCALE * 4, WINDOW_SIZE.y / 2};
//     const char* text = "GAME OVER";
//     float x_offset = 0;
    
//     for (int i = 0; text[i] != '\0'; i++) {
//         Vector2 char_pos = {position.x + x_offset, position.y};

//         DrawText(&text[i], char_pos.x, char_pos.y, SCALE, WHITE);
//         x_offset += SCALE;
//     }
// }


// Reset the game stage
static bool resetStage(void) {
    if (state.ship.deathTime != 0) {
        if (state.lives == 0) {
            // If no lives left, set flag for full game reset
            state.reset = true;
            return true;
        } else {
            state.lives--;
            // Clear all projectiles and particles for clean restart
            clearProjectiles(&state);
            clearParticles(&state);
        }
    }

    // Reset ship position and state
    state.ship.deathTime = 0;
    state.ship.position = (Vector2){window_size.x / 2, window_size.y / 2};
    state.ship.velocity = (Vector2){0, 0};
    state.ship.rotation = 0;

    return true;
}

// Reset asteroids for a new stage
static bool resetAsteroids(void) {
    clearAsteroids(&state);
    
    // Calculate number of asteroids based on score
    size_t numAsteroids = 30 + (state.score / 1500);
    
    for (size_t i = 0; i < numAsteroids; i++) {
        float angle = (2.0f * PI) * ((float)rand() / RAND_MAX);
        AsteroidSize size = rand() % 3; // Random size
        
        Asteroid asteroid = {
            .position = {
                ((float)rand() / RAND_MAX) * window_size.x,
                ((float)rand() / RAND_MAX) * window_size.y
            },
            .velocity = {
                cosf(angle) * getAsteroidVelocityScale(size) * 1.5f * ((float)rand() / RAND_MAX),
                sinf(angle) * getAsteroidVelocityScale(size) * 1.5f * ((float)rand() / RAND_MAX)
            },
            .size = size,
            .seed = (unsigned long long)rand(),
            .remove = false
        };
        
        if (!addAsteroidToQueue(&state, asteroid)) {
            return false;
        }
    }
    
    state.stageStart = state.current;
    return true;
}

// Reset entire game
static bool resetGame(void) {
    state.lives = 3;
    state.lastScore = state.score;  // Store last score before resetting
    state.score = 0;
    state.current = 0;
    
    // Clear all game objects
    clearProjectiles(&state);
    clearParticles(&state);
    clearAliens(&state);
    
    if (!resetStage()) return false;
    if (!resetAsteroids()) return false;
    
    return true;
}

// Main render function
static void renderGame(void) {
    BeginDrawing();
    ClearBackground(BLACK);
    
    // Draw lives
    for (size_t i = 0; i < state.lives; i++) {
         const Vector2 SHIP_POINTS[] = {
            {-0.4f, -0.5f},
            {0.0f, 0.5f},
            {0.4f, -0.5f},
            {0.3f, -0.4f},
            {-0.3f, -0.4f}
        };
        
        Vector2 position = {SCALE + (i * SCALE), SCALE};
        drawLines(position, SCALE, -PI, SHIP_POINTS, 5, true);
    }
    
    // Draw score
    drawNumber(state.score, (Vector2){window_size.x - SCALE, SCALE});
    
    // Draw ship
    drawShip(&state.ship);
    if (!state.ship.deathTime && IsKeyDown(KEY_W) && 
        (state.frame % 2) == 0) {
        drawShipThrust(&state.ship);
    }
    
    // Draw game objects
    for (size_t i = 0; i < state.asteroids_count; i++) {
        drawAsteroid(&state.asteroids[i]);
    }
    
    for (size_t i = 0; i < state.aliens_count; i++) {
        drawAlien(&state.aliens[i]);
    }
    
    for (size_t i = 0; i < state.particles_count; i++) {
        drawParticle(&state.particles[i]);
    }
    
    for (size_t i = 0; i < state.projectiles_count; i++) {
        drawProjectile(&state.projectiles[i]);
    }
    // if (state.lives == 0) {
    //     drawGameOver();
    // }
    
    EndDrawing();
}
static void updateShipPosition(Ship* ship) {
    // Update position based on velocity
    ship->position = Vector2Add(ship->position, ship->velocity);
    
    // Wrap around screen edges
    ship->position.x = fmodf(ship->position.x + window_size.x, window_size.x);
    ship->position.y = fmodf(ship->position.y + window_size.y, window_size.y);
}

// Handle ship controls and movement
static bool updateShipControls(GameState* state) {
    if (state->ship.deathTime != 0) return true;  // Don't process controls if ship is dead
    
    const float ROTATION_SPEED = 2.0f;  // Rotations per second
    const float SHIP_SPEED = 24.0f;
    const float DRAG = 0.019f;  // Velocity dampening
    
    // Rotate ship
    if (IsKeyDown(KEY_A)) {
        state->ship.rotation -= state->change * (2.0f * PI) * ROTATION_SPEED;
    }
    if (IsKeyDown(KEY_D)) {
        state->ship.rotation += state->change * (2.0f * PI) * ROTATION_SPEED;
    }
    
    // Calculate ship's forward direction
    const float directionAngle = state->ship.rotation + (PI * 0.5f);
    const Vector2 shipDirection = {
        cosf(directionAngle),
        sinf(directionAngle)
    };
    
    // Apply thrust
    if (IsKeyDown(KEY_W)) {
        Vector2 thrust = Vector2Scale(shipDirection, state->change * SHIP_SPEED);
        state->ship.velocity = Vector2Add(state->ship.velocity, thrust);
        
        // Play thrust sound every other frame when thrusting
        if (state->frame % 2 == 0) {
            PlaySound(sound.thrust);
        }
    }
    
    // Apply drag to velocity
    state->ship.velocity = Vector2Scale(state->ship.velocity, 1.0f - DRAG);
    
    // Handle shooting
    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // Create new projectile
        Projectile proj = {
            .position = Vector2Add(
                state->ship.position,
                Vector2Scale(shipDirection, SCALE * 0.55f)
            ),
            .velocity = Vector2Scale(shipDirection, 10.0f),
            .ttl = 2.0f,
            .spawn = state->current,
            .remove = false
        };
        
        if (!addProjectile(state, proj)) return false;
        PlaySound(sound.shoot);
        
        // Add recoil to ship
        state->ship.velocity = Vector2Add(
            state->ship.velocity,
            Vector2Scale(shipDirection, -0.5f)
        );
    }
    
    // Update ship position
    updateShipPosition(&state->ship);
    
    return true;
}

static bool createExplosionParticles(GameState* state, Vector2 position, float size) {
    const int DOT_COUNT = 10;
    const int LINE_COUNT = 5;
    
    // Create dot particles
    for (int i = 0; i < DOT_COUNT; i++) {
        float angle = (2.0f * PI) * ((float)rand() / RAND_MAX);
        float speed = 2.0f + 4.0f * ((float)rand() / RAND_MAX);
        
        Particle particle = {
            .position = position,
            .velocity = {
                cosf(angle) * speed,
                sinf(angle) * speed
            },
            .ttl = 0.5f + (0.4f * ((float)rand() / RAND_MAX)),
            .type = PARTICLE_DOT,
            .values.dot.radius = SCALE * 0.025f
        };
        
        if (!addParticle(state, particle)) return false;
    }
    
    // Create line particles
    for (int i = 0; i < LINE_COUNT; i++) {
        float angle = (2.0f * PI) * ((float)rand() / RAND_MAX);
        float speed = 2.0f * ((float)rand() / RAND_MAX);
        
        Particle particle = {
            .position = position,
            .velocity = {
                cosf(angle) * speed,
                sinf(angle) * speed
            },
            .ttl = 3.0f + ((float)rand() / RAND_MAX),
            .type = PARTICLE_LINE,
            .values.line = {
                .rotation = 2.0f * PI * ((float)rand() / RAND_MAX),
                .length = size * (0.6f + (0.4f * ((float)rand() / RAND_MAX)))
            }
        };
        
        if (!addParticle(state, particle)) return false;
    }
    
    return true;
}

// Generic function to wrap position around screen
static Vector2 wrapPosition(Vector2 pos) {
    return (Vector2){
        fmodf(pos.x + window_size.x, window_size.x),
        fmodf(pos.y + window_size.y, window_size.y)
    };
}

// Process asteroids in queue
static bool processAsteroidQueue(GameState* state) {
    // Add all queued asteroids to the main asteroid array
    for (size_t i = 0; i < state->asteroids_queue_count; i++) {
        if (!addAsteroid(state, state->asteroids_queue[i])) {
            return false;
        }
    }
    clearAsteroidsQueue(state);
    return true;
}

// Split asteroid into smaller pieces
static bool splitAsteroid(GameState* state, Asteroid* asteroid, Vector2* impact_dir) {
    PlaySound(sound.asteroid);
    
    // Add to score
    state->score += getAsteroidScore(asteroid->size);
    asteroid->remove = true;

    // Don't split if it's already the smallest size
    if (asteroid->size == ASTEROID_SMALL) {
        return true;
    }

    // Create two new smaller asteroids
    for (int i = 0; i < 2; i++) {
        Vector2 direction = Vector2Normalize(asteroid->velocity);
        AsteroidSize new_size = (asteroid->size == ASTEROID_BIG) ? 
                               ASTEROID_MEDIUM : ASTEROID_SMALL;

        float speed = getAsteroidVelocityScale(new_size) * 2.2f * 
                     ((float)rand() / RAND_MAX);

        Vector2 new_velocity = Vector2Scale(direction, speed);
        
        // If there was an impact, add its direction to the velocity
        if (impact_dir != NULL) {
            new_velocity = Vector2Add(new_velocity, 
                                    Vector2Scale(*impact_dir, 0.7f));
        }

        Asteroid new_asteroid = {
            .position = asteroid->position,
            .velocity = new_velocity,
            .size = new_size,
            .seed = (unsigned long long)rand(),
            .remove = false
        };

        if (!addAsteroidToQueue(state, new_asteroid)) {
            return false;
        }
    }

    return true;
}

// Update asteroid positions and handle collisions



// Helper function to safely normalize a vector
static Vector2 normalizeVector(Vector2 v) {
    float length = sqrtf(v.x * v.x + v.y * v.y);
    if (length > 0) {
        return (Vector2){v.x / length, v.y / length};
    }
    return v;
}

// Split asteroid into smaller pieces


// Update asteroid positions and handle collisions
static bool updateAsteroids(GameState* state) {
    // First process any queued asteroids
    if (!processAsteroidQueue(state)) {
        return false;
    }

    size_t i = 0;
    while (i < state->asteroids_count) {
        Asteroid* asteroid = &state->asteroids[i];
        
        // Update position
        asteroid->position = Vector2Add(asteroid->position, asteroid->velocity);
        asteroid->position = wrapPosition(asteroid->position);

        // Check for collisions with ship
        if (!state->ship.deathTime && 
            Vector2Distance(asteroid->position, state->ship.position) < 
            getAsteroidSize(asteroid->size) * getAsteroidCollisionScale(asteroid->size)) {
            
            state->ship.deathTime = state->current;
            Vector2 impact_vec = normalizeVector(state->ship.velocity);
            if (!splitAsteroid(state, asteroid, &impact_vec)) {  // Pass address of impact_vec
                return false;
            }
        }

        // Check for collisions with projectiles
        for (size_t j = 0; j < state->projectiles_count; j++) {
            Projectile* proj = &state->projectiles[j];
            if (!proj->remove && 
                Vector2Distance(asteroid->position, proj->position) < 
                getAsteroidSize(asteroid->size) * getAsteroidCollisionScale(asteroid->size)) {
                
                proj->remove = true;
                Vector2 impact_vec = normalizeVector(proj->velocity);
                if (!splitAsteroid(state, asteroid, &impact_vec)) {  // Pass address of impact_vec
                    return false;
                }
            }
        }

        if (asteroid->remove) {
            removeAsteroid(state, i);
        } else {
            i++;
        }
    }

    // If no asteroids left, generate new wave
    if (state->asteroids_count == 0) {
        return resetAsteroids();
    }

    return true;
}
static bool updateProjectiles(GameState* state) {
    size_t i = 0;
    while (i < state->projectiles_count) {
        Projectile* proj = &state->projectiles[i];
        
        // Update projectile lifetime
        if (proj->ttl > state->change) {
            proj->ttl -= state->change;
            
            // Update position
            proj->position = Vector2Add(proj->position, proj->velocity);
            proj->position = wrapPosition(proj->position);
            
            // Check for collision with ship (only if projectile isn't too new)
            if ((state->current - proj->spawn) > 0.15f && 
                !state->ship.deathTime &&
                Vector2Distance(proj->position, state->ship.position) < (SCALE * 0.7f)) {
                state->ship.deathTime = state->current;
                PlaySound(sound.boom);
                proj->remove = true;
            }
            
            if (!proj->remove) {
                i++;
                continue;
            }
        }
        
        // Remove projectile if it's marked for removal or lifetime expired
        removeProjectile(state, i);
    }
    
    return true;
}

static void updateScore(GameState* state) {
    const float POINTS_PER_SECOND = 3.0f;
    
    // Only update score if player is alive and game is not over
    if (!state->ship.deathTime && state->lives > 0) {
        // Multiply points per second by the time elapsed since last frame
        state->score += (size_t)(state->change * POINTS_PER_SECOND);
    }
}

GAME_EXPORT bool initGame(void) {
    // Initialize window with web-specific settings
    #ifdef __EMSCRIPTEN__
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(window_size.x, window_size.y, "Ender's Game");
    UpdateWindowSize(); // Initial size update
    #else
    InitWindow(window_size.x, window_size.y, "Asteroids");
    SetWindowPosition(100, 100);
    #endif
    
    SetTargetFPS(60);
    
    // Initialize audio
    InitAudioDevice();
    if (!loadGameSounds()) {
        return false;
    }
    
    // Initialize game state
    if (!initGameArrays(&state)) {
        return false;
    }
    
    // Initialize basic state values
    state.change = 0;
    state.current = 0;
    state.stageStart = 0;
    state.lives = 3;
    state.lastScore = 0;
    state.score = 0;
    state.reset = false;
    state.lastBloop = 0;
    state.bloop = 0;
    state.frame = 0;

    // Initialize ship
    state.ship.position = (Vector2){window_size.x / 2, window_size.y / 2};
    state.ship.velocity = (Vector2){0, 0};
    state.ship.rotation = 0;
    state.ship.deathTime = 0;

    // Initialize random seed
    srand(time(NULL));

    // Initial game reset
    if (!resetGame()) {
        return false;
    }

    return true;
}

GAME_EXPORT void cleanupGame(void) {
    freeGameArrays(&state);
    unloadGameSounds();
    CloseAudioDevice();
    CloseWindow();
}



// Main game loop update function (placeholder for now)
static bool updateGame(void) {
    // Only process game updates if we have lives left
    if (state.lives > 0) {
        if (!updateShipControls(&state)) {
            return false;
        }
        if (!updateAsteroids(&state)) {
            return false;
        }
        if (!updateProjectiles(&state)) {
            return false;
        }
        
        // Only update score if ship is alive
        updateScore(&state);
    }
    
    // Handle ship death and stage reset
    if (state.ship.deathTime != 0 && 
        (state.current - state.ship.deathTime) > 2.0f) {  // 2 second delay before reset
        if (!resetStage()) {
            return false;
        }
    }

    // Update particles regardless of game state
    size_t i = 0;
    while (i < state.particles_count) {
        Particle* particle = &state.particles[i];
        if (particle->ttl > state.change) {
            particle->ttl -= state.change;
            particle->position = Vector2Add(particle->position, particle->velocity);
            particle->position = wrapPosition(particle->position);
            i++;
        } else {
            removeParticle(&state, i);
        }
    }

    return true;
}

//function for single frame update and render
GAME_EXPORT void updateAndRenderFrame(void) {
    // Update timing
    state.change = GetFrameTime();
    state.current += state.change;
    
    // Update window size (for responsive web)
    UpdateWindowSize();
    
    // Update game state
    if (!updateGame()) {
        return;
    }
    
    // Handle reset if needed
    if (state.reset) {
        state.reset = false;
        if (!resetGame()) {
            return;
        }
    }
    
    // Render frame
    renderGame();
    
    state.frame++;
}

#ifndef __EMSCRIPTEN__
int main(void) {
    if (!initGame()) {
        return 1;
    }
    
    while (!WindowShouldClose()) {
        updateAndRenderFrame();
    }
    
    cleanupGame();
    return 0;
}
#endif
