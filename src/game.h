#ifndef GAME_H
#define GAME_H

#include <raylib.h>
#include <stdbool.h>
#include <string.h>
// Structure definitions first
typedef struct {
    float x;
    float y;
} Vec2;

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

// Forward declare Asteroid for use in GameState
typedef struct Asteroid Asteroid;

typedef enum {
    PARTICLE_LINE,
    PARTICLE_DOT
} ParticleType;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float ttl;
    ParticleType type;
    union {
        struct {
            float rotation;
            float length;
        } line;
        struct {
            float radius;
        } dot;
    } values;
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
    Sound bloopLo;
    Sound bloopHi;
    Sound shoot;
    Sound thrust;
    Sound asteroid;
    Sound boom;
} GameSound;

typedef struct {
    Ship ship;
    float change;
    float current;
    float stageStart;
    Asteroid* asteroids;
    size_t asteroids_count;
    size_t asteroids_capacity;
    Asteroid* asteroids_queue;
    size_t asteroids_queue_count;
    size_t asteroids_queue_capacity;
    Particle* particles;
    size_t particles_count;
    size_t particles_capacity;
    Projectile* projectiles;
    size_t projectiles_count;
    size_t projectiles_capacity;
    Alien* aliens;
    size_t aliens_count;
    size_t aliens_capacity;
    size_t lives;
    size_t lastScore;
    size_t score;
    bool reset;
    size_t lastBloop;
    size_t bloop;
    size_t frame;
} GameState;

// Now define Asteroid structure after GameState is defined
struct Asteroid {
    Vector2 position;
    Vector2 velocity;
    AsteroidSize size;
    unsigned long long seed;
    bool remove;
};

#ifdef __cplusplus
extern "C" {
#endif

extern bool initGame(void);
extern void updateAndRenderFrame(void);
extern void cleanupGame(void);
extern void UpdateWindowSize(void);

#ifdef __cplusplus
}
#endif

#endif