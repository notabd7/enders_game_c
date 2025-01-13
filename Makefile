CC = emcc
CFLAGS = -s USE_GLFW=3 -s WASM=1 -s USE_WEBGL2=1 -s ASYNCIFY=1 \
         -s ALLOW_MEMORY_GROWTH=1 -s "EXPORTED_RUNTIME_METHODS=['ccall']" \
         -I$(EMSDK)/upstream/emscripten/cache/sysroot/include

RAYLIB_FLAGS = -s USE_GLFW=3 -s WASM=1 -s USE_WEBGL2=1 \
               -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2

TARGET = public/game.js
SOURCES = src/game.c src/web_adapter.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(RAYLIB_FLAGS) $(SOURCES) -o $(TARGET) \
		--preload-file sounds
	@echo "Build complete! Open public/index.html in a web server"