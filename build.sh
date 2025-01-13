#!/bin/bash

# Create directories
mkdir -p build
mkdir -p public

# Define paths
RAYLIB_SRC="./raylib/src"

# Compile each raylib source file separately
echo "Compiling raylib..."
emcc -c $RAYLIB_SRC/rcore.c -o build/rcore.o -I$RAYLIB_SRC -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
emcc -c $RAYLIB_SRC/rshapes.c -o build/rshapes.o -I$RAYLIB_SRC -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
emcc -c $RAYLIB_SRC/rtextures.c -o build/rtextures.o -I$RAYLIB_SRC -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
emcc -c $RAYLIB_SRC/rtext.c -o build/rtext.o -I$RAYLIB_SRC -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
emcc -c $RAYLIB_SRC/raudio.c -o build/raudio.o -I$RAYLIB_SRC -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
emcc -c $RAYLIB_SRC/rmodels.c -o build/rmodels.o -I$RAYLIB_SRC -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
emcc -c $RAYLIB_SRC/utils.c -o build/utils.o -I$RAYLIB_SRC -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2

# Compile the game and link everything together
echo "Compiling game..."
emcc src/game.c src/web_adapter.c \
    build/rcore.o \
    build/rshapes.o \
    build/rtextures.o \
    build/rtext.o \
    build/raudio.o \
    build/rmodels.o \
    build/utils.o \
    -o public/game.js \
    -I$RAYLIB_SRC \
    -s USE_GLFW=3 \
    -s WASM=1 \
    -s USE_WEBGL2=1 \
    -s ASYNCIFY=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s "EXPORTED_RUNTIME_METHODS=['ccall']" \
    -s EXPORTED_FUNCTIONS='["_startGame", "_stopGame"]' \
    --preload-file sounds \
    -DPLATFORM_WEB \
    -O2

echo "Build complete! Run a local server to test."