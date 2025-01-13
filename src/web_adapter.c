#include "game.h"
#include <emscripten.h>
#include <emscripten/html5.h>

void emscripten_loop(void) {
    updateAndRenderFrame();
}

EMSCRIPTEN_KEEPALIVE
void startGame() {
    // Add debug message
    EM_ASM(
        console.log("Starting game initialization...");
    );
    
    if (initGame()) {
        EM_ASM(
            console.log("Game initialized successfully, starting main loop...");
        );
        emscripten_set_main_loop(emscripten_loop, 0, 1);
    } else {
        EM_ASM(
            console.error("Game initialization failed!");
        );
    }
}
EM_BOOL key_callback(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
    if (eventType == EMSCRIPTEN_EVENT_KEYDOWN || eventType == EMSCRIPTEN_EVENT_KEYUP) {
        // Map the key codes to our game's input system
        KeyboardKey key = KEY_NULL;
        
        if (strcmp(keyEvent->code, "Space") == 0) key = KEY_SPACE;
        else if (strcmp(keyEvent->code, "KeyW") == 0) key = KEY_W;
        else if (strcmp(keyEvent->code, "KeyA") == 0) key = KEY_A;
        else if (strcmp(keyEvent->code, "KeyD") == 0) key = KEY_D;
        else if (strcmp(keyEvent->code, "Escape") == 0) key = KEY_ESCAPE;
        
        // Set the key state in raylib's input system
        if (eventType == EMSCRIPTEN_EVENT_KEYDOWN) {
            SetKeyPressed(key);
        } else {
            SetKeyReleased(key);
        }
    }
    
    return EM_TRUE; // Event was handled
}
EMSCRIPTEN_KEEPALIVE
void startGame() {
    // Set up keyboard handling
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, key_callback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, EM_TRUE, key_callback);
    
    if (initGame()) {
        emscripten_set_main_loop(emscripten_loop, 0, 1);
    }
}
#ifdef __EMSCRIPTEN__
int main(void) {
    EM_ASM(
        console.log("Web adapter initialized");
    );
    return 0;
}
#endif