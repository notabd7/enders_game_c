// web_adapter.c
#include <emscripten.h>
#include <emscripten/html5.h>

// Function declarations from game.c
extern void initGame(void);
extern void updateAndRenderFrame(void);
extern void cleanupGame(void);

// Main game loop function for emscripten
void emscripten_loop(void) {
    updateAndRenderFrame();
}

// JavaScript callable functions
EMSCRIPTEN_KEEPALIVE
void startGame() {
    initGame();
    emscripten_set_main_loop(emscripten_loop, 0, 1);
}

EMSCRIPTEN_KEEPALIVE
void stopGame() {
    emscripten_cancel_main_loop();
    cleanupGame();
}

// Main function for web
int main() {
    return 0;
}