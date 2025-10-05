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

# UPX compression targets
upx: $(TARGET)
	@command -v $(UPX) >/dev/null 2>&1 || { \
		echo "UPX is not installed. Install it to compress executables."; \
		echo "  Ubuntu/Debian: sudo apt-get install upx-ucl"; \
		echo "  Fedora: sudo dnf install upx"; \
		echo "  Arch: sudo pacman -S upx"; \
		exit 1; \
	}
	$(UPX) --best --lzma $(TARGET)

upx-windows: windows
	@command -v $(UPX) >/dev/null 2>&1 || { \
		echo "UPX is not installed. Install it to compress executables."; \
		echo "  Ubuntu/Debian: sudo apt-get install upx-ucl"; \
		echo "  Fedora: sudo dnf install upx"; \
		echo "  Arch: sudo pacman -S upx"; \
		exit 1; \
	}
	$(UPX) --best --lzma $(TARGET).exe

upx-windows-with-raylib: windows-with-raylib
	@command -v $(UPX) >/dev/null 2>&1 || { \
		echo "UPX is not installed. Install it to compress executables."; \
		echo "  Ubuntu/Debian: sudo apt-get install upx-ucl"; \
		echo "  Fedora: sudo dnf install upx"; \
		echo "  Arch: sudo pacman -S upx"; \
		exit 1; \
	}
	$(UPX) --best --lzma $(TARGET).exe

upx-linux-with-raylib: linux-with-raylib
	@command -v $(UPX) >/dev/null 2>&1 || { \
		echo "UPX is not installed. Install it to compress executables."; \
		echo "  Ubuntu/Debian: sudo apt-get install upx-ucl"; \
		echo "  Fedora: sudo dnf install upx"; \
		echo "  Arch: sudo pacman -S upx"; \
		exit 1; \
	}
	$(UPX) --best --lzma $(TARGET)

upx-linux-static: linux-static
	@command -v $(UPX) >/dev/null 2>&1 || { \
		echo "UPX is not installed. Install it to compress executables."; \
		echo "  Ubuntu/Debian: sudo apt-get install upx-ucl"; \
		echo "  Fedora: sudo dnf install upx"; \
		echo "  Arch: sudo pacman -S upx"; \
		exit 1; \
	}
	$(UPX) --best --lzma $(TARGET)-static

upx-linux-musl: linux-musl
	@command -v $(UPX) >/dev/null 2>&1 || { \
		echo "UPX is not installed. Install it to compress executables."; \
		echo "  Ubuntu/Debian: sudo apt-get install upx-ucl"; \
		echo "  Fedora: sudo dnf install upx"; \
		echo "  Arch: sudo pacman -S upx"; \
		exit 1; \
	}
	$(UPX) --best --lzma $(TARGET)-musl

upx-windows32: windows32
	@command -v $(UPX) >/dev/null 2>&1 || { \
		echo "UPX is not installed. Install it to compress executables."; \
		echo "  Ubuntu/Debian: sudo apt-get install upx-ucl"; \
		echo "  Fedora: sudo dnf install upx"; \
		echo "  Arch: sudo pacman -S upx"; \
		exit 1; \
	}
	$(UPX) --best --lzma $(TARGET)32.exe

# Compressed release builds
release-upx: CFLAGS += -O3 -DNDEBUG
release-upx: clean $(TARGET) upx

release-linux-upx: CFLAGS += -O3 -DNDEBUG
release-linux-upx: clean linux-with-raylib upx-linux-with-raylib

release-windows-upx: CFLAGS += -O3 -DNDEBUG
release-windows-upx: clean windows-with-raylib upx-windows-with-raylib

# Optimized static builds
release-linux-static: CFLAGS += -O3 -DNDEBUG
release-linux-static: clean linux-static upx-linux-static

release-linux-musl: CFLAGS += -O3 -DNDEBUG
release-linux-musl: clean linux-musl upx-linux-musl

# Build all platforms
all-platforms: all windows

# Run the program
run: $(TARGET)
	./$(TARGET)

# Build gamepad test utility
gamepad-test: tools/gamepad_test.c
	$(CC) tools/gamepad_test.c -o tools/gamepad_test $(CFLAGS) $(LIBS)

# Run gamepad test utility
run-gamepad-test: gamepad-test
	./tools/gamepad_test

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
	rm -f $(TARGET) $(TARGET).exe $(TARGET)32.exe $(TARGET)-static $(TARGET)-musl $(OBJECTS)
	rm -rf lib/
	rm -f tools/gamepad_test

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

# Install musl for static compilation
install-musl-ubuntu:
	sudo apt-get update
	sudo apt-get install -y musl-dev musl-tools

install-musl-fedora:
	sudo dnf install -y musl-gcc musl-libc-static

install-musl-arch:
	sudo pacman -S --needed musl

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
windows-with-raylib: download-raylib-windows
	$(MINGW_CC) $(SOURCES) -o $(TARGET).exe $(CFLAGS) \
		-I./lib/windows/raylib-5.5_win64_mingw-w64/include \
		-L./lib/windows/raylib-5.5_win64_mingw-w64/lib \
		-lraylib -lopengl32 -lgdi32 -lwinmm -static

# Linux compilation with downloaded RayLib (standalone)
linux-with-raylib: download-raylib-linux
	$(CC) $(SOURCES) -o $(TARGET) $(CFLAGS) \
		-I./lib/linux/raylib-5.5_linux_amd64/include \
		-L./lib/linux/raylib-5.5_linux_amd64/lib \
		-Wl,-Bstatic -lraylib -Wl,-Bdynamic \
		-lGL -lm -lpthread -ldl -lrt -lX11 \
		-static-libgcc -static-libstdc++

# Linux compilation with fully static linking (maximum portability)
linux-static: download-raylib-linux
	$(CC) $(SOURCES) -o $(TARGET)-static $(CFLAGS) \
		-I./lib/linux/raylib-5.5_linux_amd64/include \
		-L./lib/linux/raylib-5.5_linux_amd64/lib \
		-static -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
		-Wl,--whole-archive -lpthread -Wl,--no-whole-archive

# Linux compilation with musl for better static linking
linux-musl: download-raylib-linux
	@command -v musl-gcc >/dev/null 2>&1 || { \
		echo "musl-gcc not found. Install musl-dev package."; \
		echo "  Ubuntu/Debian: sudo apt-get install musl-dev musl-tools"; \
		echo "  Fedora: sudo dnf install musl-gcc musl-libc-static"; \
		echo "  Arch: sudo pacman -S musl"; \
		exit 1; \
	}
	musl-gcc $(SOURCES) -o $(TARGET)-musl $(CFLAGS) \
		-I./lib/linux/raylib-5.5_linux_amd64/include \
		-L./lib/linux/raylib-5.5_linux_amd64/lib \
		-static -lraylib -lm -lpthread

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

# Create standalone Linux distribution package
dist-linux: linux-with-raylib
	mkdir -p dist
	cp $(TARGET) dist/
	cp README.md dist/
	echo "Space is Left - Game Engine (Standalone Linux)" > dist/VERSION.txt
	echo "Version: 1.0.0" >> dist/VERSION.txt
	echo "Build Date: $(shell date)" >> dist/VERSION.txt
	tar -czf space-is-left-linux-$(shell date +%Y%m%d).tar.gz dist/
	rm -rf dist/
	echo "Standalone Linux distribution package created: space-is-left-linux-$(shell date +%Y%m%d).tar.gz"

# Create compressed distribution package
dist-upx: release-upx
	mkdir -p dist
	cp $(TARGET) dist/
	cp README.md dist/
	echo "Space is Left - Game Engine (UPX Compressed)" > dist/VERSION.txt
	echo "Version: 1.0.0" >> dist/VERSION.txt
	echo "Build Date: $(shell date)" >> dist/VERSION.txt
	tar -czf space-is-left-upx-$(shell date +%Y%m%d).tar.gz dist/
	rm -rf dist/
	echo "Compressed distribution package created: space-is-left-upx-$(shell date +%Y%m%d).tar.gz"

# Create compressed standalone Linux distribution package
dist-linux-upx: linux-with-raylib upx-linux-with-raylib
	mkdir -p dist
	cp $(TARGET) dist/
	cp README.md dist/
	echo "Space is Left - Game Engine (Standalone Linux - UPX Compressed)" > dist/VERSION.txt
	echo "Version: 1.0.0" >> dist/VERSION.txt
	echo "Build Date: $(shell date)" >> dist/VERSION.txt
	tar -czf space-is-left-linux-upx-$(shell date +%Y%m%d).tar.gz dist/
	rm -rf dist/
	echo "Compressed standalone Linux distribution package created: space-is-left-linux-upx-$(shell date +%Y%m%d).tar.gz"

# Create fully static Linux distribution package
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

# Create musl static Linux distribution package
dist-linux-musl: linux-musl
	mkdir -p dist
	cp $(TARGET)-musl dist/$(TARGET)
	cp README.md dist/
	echo "Space is Left - Game Engine (Musl Static Linux)" > dist/VERSION.txt
	echo "Version: 1.0.0" >> dist/VERSION.txt
	echo "Build Date: $(shell date)" >> dist/VERSION.txt
	tar -czf space-is-left-linux-musl-$(shell date +%Y%m%d).tar.gz dist/
	rm -rf dist/
	echo "Musl static Linux distribution package created: space-is-left-linux-musl-$(shell date +%Y%m%d).tar.gz"

# Create compressed fully static Linux distribution package
dist-linux-static-upx: linux-static upx-linux-static
	mkdir -p dist
	cp $(TARGET)-static dist/$(TARGET)
	cp README.md dist/
	echo "Space is Left - Game Engine (Fully Static Linux - UPX Compressed)" > dist/VERSION.txt
	echo "Version: 1.0.0" >> dist/VERSION.txt
	echo "Build Date: $(shell date)" >> dist/VERSION.txt
	tar -czf space-is-left-linux-static-upx-$(shell date +%Y%m%d).tar.gz dist/
	rm -rf dist/
	echo "Compressed fully static Linux distribution package created: space-is-left-linux-static-upx-$(shell date +%Y%m%d).tar.gz"

# Create Windows distribution package
dist-windows: windows-with-raylib
	mkdir -p dist-win
	cp $(TARGET).exe dist-win/
	echo "@echo off" > dist-win/run.bat
	echo "$(TARGET).exe" >> dist-win/run.bat
	echo "pause" >> dist-win/run.bat
	cp README.md dist-win/README.txt
	zip -r space-is-left-win-$(shell date +%Y%m%d).zip dist-win/
	rm -rf dist-win/
	echo "Windows distribution package created: space-is-left-win-$(shell date +%Y%m%d).zip"

# Create compressed Windows distribution package
dist-windows-upx: windows-with-raylib upx-windows-with-raylib
	mkdir -p dist-win
	cp $(TARGET).exe dist-win/
	echo "@echo off" > dist-win/run.bat
	echo "$(TARGET).exe" >> dist-win/run.bat
	echo "pause" >> dist-win/run.bat
	cp README.md dist-win/README.txt
	zip -r space-is-left-win-upx-$(shell date +%Y%m%d).zip dist-win/
	rm -rf dist-win/
	echo "Compressed Windows distribution package created: space-is-left-win-upx-$(shell date +%Y%m%d).zip"

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
	@echo "  make linux-with-raylib - Build standalone Linux with downloaded RayLib"
	@echo "  make linux-static - Build fully static Linux binary (maximum portability)"
	@echo "  make linux-musl   - Build static Linux binary using musl libc"
	@echo "  make test-windows - Test Windows exe with Wine"
	@echo "  make all-platforms - Build for both Linux and Windows"
	@echo ""
	@echo "UPX Compression:"
	@echo "  make upx          - Compress Linux binary with UPX"
	@echo "  make upx-windows  - Compress Windows exe with UPX"
	@echo "  make upx-linux-with-raylib - Compress standalone Linux binary with UPX"
	@echo "  make upx-linux-static - Compress fully static Linux binary with UPX"
	@echo "  make upx-linux-musl - Compress musl static Linux binary with UPX"
	@echo "  make upx-windows-with-raylib - Compress Windows exe (with RayLib) with UPX"
	@echo "  make upx-windows32 - Compress 32-bit Windows exe with UPX"
	@echo "  make release-upx  - Build optimized and compressed Linux binary"
	@echo "  make release-linux-upx - Build optimized and compressed standalone Linux"
	@echo "  make release-windows-upx - Build optimized and compressed Windows exe"
	@echo "  make release-linux-static - Build optimized and compressed fully static Linux"
	@echo "  make release-linux-musl - Build optimized and compressed musl static Linux"
	@echo ""
	@echo "Distribution:"
	@echo "  make dist         - Create distribution package"
	@echo "  make dist-upx     - Create compressed distribution package"
	@echo "  make dist-linux   - Create standalone Linux distribution package"
	@echo "  make dist-linux-upx - Create compressed standalone Linux distribution package"
	@echo "  make dist-linux-static - Create fully static Linux distribution package"
	@echo "  make dist-linux-musl - Create musl static Linux distribution package"
	@echo "  make dist-linux-static-upx - Create compressed fully static Linux distribution package"
	@echo "  make dist-windows - Create Windows distribution package"
	@echo "  make dist-windows-upx - Create compressed Windows distribution package"
	@echo ""
	@echo "Maintenance:"
	@echo "  make clean        - Remove all built files"
	@echo "  make rebuild      - Clean and rebuild"
	@echo "  make format       - Format code with clang-format"
	@echo "  make check        - Run static analysis with cppcheck"
	@echo "  make gamepad-test - Build gamepad test utility"
	@echo "  make run-gamepad-test - Run gamepad test utility"
	@echo ""
	@echo "Dependencies:"
	@echo "  make install-deps-ubuntu  - Install RayLib on Ubuntu/Debian"
	@echo "  make install-deps-fedora  - Install RayLib on Fedora"
	@echo "  make install-deps-arch    - Install RayLib on Arch"
	@echo "  make install-mingw-ubuntu - Install MinGW on Ubuntu/Debian"
	@echo "  make install-mingw-fedora - Install MinGW on Fedora"
	@echo "  make install-mingw-arch   - Install MinGW on Arch"
	@echo "  make install-musl-ubuntu  - Install musl development tools on Ubuntu/Debian"
	@echo "  make install-musl-fedora  - Install musl development tools on Fedora"
	@echo "  make install-musl-arch    - Install musl development tools on Arch"
	@echo "  make download-raylib-windows - Download RayLib for Windows"
	@echo "  make download-raylib-linux - Download RayLib for Linux"
	@echo ""
	@echo "Info:"
	@echo "  make help         - Show this help message"

.PHONY: all windows windows-dynamic windows32 windows-with-raylib linux-with-raylib linux-static linux-musl \
        debug release all-platforms run test-windows \
        upx upx-windows upx-linux-with-raylib upx-windows-with-raylib upx-windows32 upx-linux-static upx-linux-musl \
        release-upx release-linux-upx release-windows-upx release-linux-static release-linux-musl \
        clean rebuild dist dist-upx dist-linux dist-linux-upx dist-linux-static dist-linux-musl dist-linux-static-upx \
        dist-windows dist-windows-upx format check install-deps-ubuntu install-deps-fedora install-deps-arch \
        install-mingw-ubuntu install-mingw-fedora install-mingw-arch \
        install-musl-ubuntu install-musl-fedora install-musl-arch \
        download-raylib-windows download-raylib-linux gamepad-test run-gamepad-test help
