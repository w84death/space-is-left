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
LIBS_MACOS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

# Detect OS for native compilation
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LIBS = $(LIBS_LINUX)
endif
ifeq ($(UNAME_S),Darwin)
    LIBS = $(LIBS_MACOS)
endif
ifeq ($(OS),Windows_NT)
    LIBS = $(LIBS_WINDOWS)
    TARGET := $(TARGET).exe
endif

# Default target
all: $(TARGET)

# Native compilation
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(CFLAGS) $(LIBS)

# Compile source files to object files
%.o: %.c $(HEADERS)
	$(CC) -c $< -o $@ $(CFLAGS)

# Windows cross-compilation from Linux
windows: $(SOURCES) $(HEADERS)
	$(MINGW_CC) $(SOURCES) -o $(TARGET).exe $(CFLAGS) $(LIBS_WINDOWS) -static

# Windows cross-compilation with dynamic linking
windows-dynamic: $(SOURCES) $(HEADERS)
	$(MINGW_CC) $(SOURCES) -o $(TARGET).exe $(CFLAGS) $(LIBS_WINDOWS)

# Windows 32-bit cross-compilation
windows32: $(SOURCES) $(HEADERS)
	i686-w64-mingw32-gcc $(SOURCES) -o $(TARGET)32.exe $(CFLAGS) $(LIBS_WINDOWS) -static

# Development build with debug symbols
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# Release build with optimizations
release: CFLAGS += -O3 -DNDEBUG
release: clean $(TARGET)

# Build all platforms
all-platforms: all windows

# Run the program
run: $(TARGET)
	./$(TARGET)

# Test Windows executable with Wine
test-windows: windows
	@command -v wine >/dev/null 2>&1 || { \
		echo "Wine is not installed. Install it to test Windows executables on Linux."; \
		echo "  Ubuntu/Debian: sudo apt-get install wine"; \
		echo "  Fedora: sudo dnf install wine"; \
		echo "  Arch: sudo pacman -S wine"; \
		exit 1; \
	}
	wine $(TARGET).exe

# Clean build files
clean:
	rm -f $(TARGET) $(TARGET).exe $(TARGET)32.exe $(OBJECTS)
	rm -rf lib/

# Clean and rebuild
rebuild: clean all

# Install dependencies for native Linux compilation
install-deps-ubuntu:
	sudo apt-get update
	sudo apt-get install -y build-essential libraylib-dev libgl1-mesa-dev

install-deps-fedora:
	sudo dnf install -y gcc raylib-devel mesa-libGL-devel

install-deps-arch:
	sudo pacman -S --needed gcc raylib mesa

# Install MinGW for cross-compilation
install-mingw-ubuntu:
	sudo apt-get update
	sudo apt-get install -y mingw-w64 mingw-w64-tools

install-mingw-fedora:
	sudo dnf install -y mingw64-gcc mingw64-winpthreads-static

install-mingw-arch:
	sudo pacman -S --needed mingw-w64-gcc

# Download pre-compiled RayLib for Windows (MinGW)
download-raylib-windows:
	@echo "Downloading RayLib for Windows MinGW..."
	@mkdir -p lib/windows
	@cd lib/windows && \
		wget -q --show-progress https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_mingw-w64.zip && \
		unzip -q raylib-5.5_win64_mingw-w64.zip && \
		echo "RayLib for Windows downloaded successfully!"

# Windows compilation with downloaded RayLib
windows-with-raylib: download-raylib-windows
	$(MINGW_CC) $(SOURCES) -o $(TARGET).exe $(CFLAGS) \
		-I./lib/windows/raylib-5.5_win64_mingw-w64/include \
		-L./lib/windows/raylib-5.5_win64_mingw-w64/lib \
		-lraylib -lopengl32 -lgdi32 -lwinmm -static

# Create distribution package
dist: release
	mkdir -p dist
	cp $(TARGET) dist/
	cp README.md dist/
	echo "Space is Left - Game Engine" > dist/VERSION.txt
	echo "Version: 1.0.0" >> dist/VERSION.txt
	echo "Build Date: $(shell date)" >> dist/VERSION.txt
	tar -czf space-is-left-$(shell date +%Y%m%d).tar.gz dist/
	rm -rf dist/
	echo "Distribution package created: space-is-left-$(shell date +%Y%m%d).tar.gz"

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

# Development helpers
format:
	@command -v clang-format >/dev/null 2>&1 || { \
		echo "clang-format not found. Install it for code formatting."; \
		exit 1; \
	}
	clang-format -i $(SOURCES) $(HEADERS)

check:
	@command -v cppcheck >/dev/null 2>&1 || { \
		echo "cppcheck not found. Install it for static analysis."; \
		exit 1; \
	}
	cppcheck --enable=all --suppress=missingIncludeSystem $(SOURCES)

# Help
help:
	@echo "Space is Left - Makefile"
	@echo "========================"
	@echo ""
	@echo "Building:"
	@echo "  make              - Build for current system"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make release      - Build optimized release version"
	@echo "  make run          - Build and run the game"
	@echo ""
	@echo "Cross-Compilation:"
	@echo "  make windows      - Build Windows exe (static linking)"
	@echo "  make windows-dynamic - Build Windows exe (dynamic linking)"
	@echo "  make windows32    - Build 32-bit Windows exe"
	@echo "  make windows-with-raylib - Build with downloaded RayLib"
	@echo "  make test-windows - Test Windows exe with Wine"
	@echo "  make all-platforms - Build for both Linux and Windows"
	@echo ""
	@echo "Distribution:"
	@echo "  make dist         - Create distribution package"
	@echo "  make dist-windows - Create Windows distribution package"
	@echo ""
	@echo "Maintenance:"
	@echo "  make clean        - Remove all built files"
	@echo "  make rebuild      - Clean and rebuild"
	@echo "  make format       - Format code with clang-format"
	@echo "  make check        - Run static analysis with cppcheck"
	@echo ""
	@echo "Dependencies:"
	@echo "  make install-deps-ubuntu  - Install RayLib on Ubuntu/Debian"
	@echo "  make install-deps-fedora  - Install RayLib on Fedora"
	@echo "  make install-deps-arch    - Install RayLib on Arch"
	@echo "  make install-mingw-ubuntu - Install MinGW on Ubuntu/Debian"
	@echo "  make install-mingw-fedora - Install MinGW on Fedora"
	@echo "  make install-mingw-arch   - Install MinGW on Arch"
	@echo "  make download-raylib-windows - Download RayLib for Windows"
	@echo ""
	@echo "Info:"
	@echo "  make help         - Show this help message"

.PHONY: all windows windows-dynamic windows32 windows-with-raylib \
        debug release all-platforms run test-windows \
        clean rebuild dist dist-windows format check \
        install-deps-ubuntu install-deps-fedora install-deps-arch \
        install-mingw-ubuntu install-mingw-fedora install-mingw-arch \
        download-raylib-windows help
