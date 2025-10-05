# Makefile for Space is Left
# A universal game engine with a space snake game

# Program name
TARGET = space-is-left

# Source files
SOURCES = main.c engine.c camera.c render.c input.c utils.c
HEADERS = engine.h

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Default compiler
CC = gcc

# Windows cross-compiler
MINGW_CC = x86_64-w64-mingw32-gcc

# Common compiler flags
CFLAGS = -Wall -Wextra -O2 -std=c99

# Platform-specific libraries
LIBS_LINUX = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
LIBS_WINDOWS = -lraylib -lopengl32 -lgdi32 -lwinmm

# Detect OS for native compilation
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LIBS = $(LIBS_LINUX)
endif
ifeq ($(UNAME_S),Darwin)
    LIBS = $(LIBS_LINUX)
endif
ifeq ($(OS),Windows_NT)
    LIBS = $(LIBS_WINDOWS)
    TARGET := $(TARGET).exe
endif

# UPX compression tool
UPX = upx

# Default target
all: $(TARGET)

# Native compilation
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(CFLAGS) $(LIBS)

# Compile source files to object files
%.o: %.c $(HEADERS)
	$(CC) -c $< -o $@ $(CFLAGS)

# Download pre-compiled RayLib for Windows (MinGW)
download-raylib-windows:
	@echo "Downloading RayLib for Windows MinGW..."
	@mkdir -p lib/windows
	@cd lib/windows && \
		wget -q --show-progress https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_mingw-w64.zip && \
		unzip -q raylib-5.5_win64_mingw-w64.zip && \
		echo "RayLib for Windows downloaded successfully!"

# Download pre-compiled RayLib for Linux
download-raylib-linux:
	@echo "Downloading RayLib for Linux..."
	@mkdir -p lib/linux
	@cd lib/linux && \
		wget -q --show-progress https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_linux_amd64.tar.gz && \
		tar -xzf raylib-5.5_linux_amd64.tar.gz && \
		echo "RayLib for Linux downloaded successfully!"

# Windows compilation with downloaded RayLib
windows: download-raylib-windows
	@command -v $(UPX) >/dev/null 2>&1 || { \
		echo "UPX is not installed. Install it to compress executables."; \
		echo "  Ubuntu/Debian: sudo apt-get install upx-ucl"; \
		echo "  Fedora: sudo dnf install upx"; \
		echo "  Arch: sudo pacman -S upx"; \
		exit 1; \
	}
	$(MINGW_CC) $(SOURCES) -o $(TARGET).exe -O3 -DNDEBUG $(CFLAGS) \
		-I./lib/windows/raylib-5.5_win64_mingw-w64/include \
		-L./lib/windows/raylib-5.5_win64_mingw-w64/lib \
		-lraylib -lopengl32 -lgdi32 -lwinmm -static
	$(UPX) --best --lzma $(TARGET).exe

# Linux compilation with downloaded RayLib (standalone)
linux: download-raylib-linux
	@command -v $(UPX) >/dev/null 2>&1 || { \
		echo "UPX is not installed. Install it to compress executables."; \
		echo "  Ubuntu/Debian: sudo apt-get install upx-ucl"; \
		echo "  Fedora: sudo dnf install upx"; \
		echo "  Arch: sudo pacman -S upx"; \
		exit 1; \
	}
	$(CC) $(SOURCES) -o $(TARGET) -O3 -DNDEBUG $(CFLAGS) \
		-I./lib/linux/raylib-5.5_linux_amd64/include \
		-L./lib/linux/raylib-5.5_linux_amd64/lib \
		-Wl,-Bstatic -lraylib -Wl,-Bdynamic \
		-lGL -lm -lpthread -ldl -lrt -lX11 \
		-static-libgcc -static-libstdc++
	$(UPX) --best --lzma $(TARGET)

# Linux compilation with fully static linking (maximum portability)
linux-static: download-raylib-linux
	@command -v $(UPX) >/dev/null 2>&1 || { \
		echo "UPX is not installed. Install it to compress executables."; \
		echo "  Ubuntu/Debian: sudo apt-get install upx-ucl"; \
		echo "  Fedora: sudo dnf install upx"; \
		echo "  Arch: sudo pacman -S upx"; \
		exit 1; \
	}
	$(CC) $(SOURCES) -o $(TARGET)-static -O3 -DNDEBUG $(CFLAGS) \
		-I./lib/linux/raylib-5.5_linux_amd64/include \
		-L./lib/linux/raylib-5.5_linux_amd64/lib \
		-static -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
		-Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	$(UPX) --best --lzma $(TARGET)-static

# Release builds for all platforms
release: clean linux windows

# Create Linux distribution package
dist-linux: linux
	mkdir -p dist
	cp $(TARGET) dist/
	cp README.md dist/
	echo "Space is Left - Game Engine (Standalone Linux)" > dist/VERSION.txt
	echo "Version: 1.0.0" >> dist/VERSION.txt
	echo "Build Date: $(shell date)" >> dist/VERSION.txt
	tar -czf space-is-left-linux-$(shell date +%Y%m%d).tar.gz dist/
	rm -rf dist/
	echo "Standalone Linux distribution package created: space-is-left-linux-$(shell date +%Y%m%d).tar.gz"

# Create static Linux distribution package
dist-linux-static: linux-static
	mkdir -p dist
	cp $(TARGET)-static dist/$(TARGET)
	cp README.md dist/
	echo "Space is Left - Game Engine (Fully Static Linux)" > dist/VERSION.txt
	echo "Version: 1.0.0" >> dist/VERSION.txt
	echo "Build Date: $(shell date)" >> dist/VERSION.txt
	tar -czf space-is-left-linux-static-$(shell date +%Y%m%d).tar.gz dist/
	rm -rf dist/
	echo "Fully static Linux distribution package created: space-is-left-linux-static-$(shell date +%Y%m%d).tar.gz"

# Create Windows distribution package
dist-windows: windows
	mkdir -p dist-win
	cp $(TARGET).exe dist-win/
	echo "@echo off" > dist-win/run.bat
	echo "$(TARGET).exe" >> dist-win/run.bat
	echo "pause" >> dist-win/run.bat
	cp README.md dist-win/README.txt
	zip -r space-is-left-win-$(shell date +%Y%m%d).zip dist-win/
	rm -rf dist-win/
	echo "Windows distribution package created: space-is-left-win-$(shell date +%Y%m%d).zip"

# Run the program
run: $(TARGET)
	./$(TARGET)

# Clean build files
clean:
	rm -f $(TARGET) $(TARGET).exe $(TARGET)-static $(OBJECTS)
	rm -rf lib/ dist/ dist-win/

# Clean and rebuild
rebuild: clean all

# Help
help:
	@echo "Space is Left - Makefile"
	@echo "========================"
	@echo ""
	@echo "Building:"
	@echo "  make              - Build for current system"
	@echo "  make run          - Build and run the game"
	@echo "  make clean        - Remove all built files"
	@echo "  make rebuild      - Clean and rebuild"
	@echo ""
	@echo "Cross-Platform Builds (Optimized + UPX Compressed):"
	@echo "  make linux        - Build portable Linux binary with bundled RayLib"
	@echo "  make linux-static - Build fully static Linux binary (maximum portability)"
	@echo "  make windows      - Build Windows exe with bundled RayLib"
	@echo "  make release      - Build both Linux and Windows binaries"
	@echo ""
	@echo "Distribution Packages:"
	@echo "  make dist-linux        - Create Linux distribution package"
	@echo "  make dist-linux-static - Create fully static Linux distribution package"
	@echo "  make dist-windows      - Create Windows distribution package"
	@echo ""
	@echo "Info:"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - gcc (for Linux builds)"
	@echo "  - mingw-w64 (for Windows cross-compilation)"
	@echo "  - upx (for compression)"
	@echo "  - wget, unzip, tar (for downloading RayLib)"

.PHONY: all windows linux linux-static release dist-linux dist-linux-static dist-windows \
        run clean rebuild help download-raylib-windows download-raylib-linux
