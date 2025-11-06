#!/bin/bash
#
# GLFW Android Library Build Script
# Baut GLFW als vorkompilierte statische Library (.a) für ARM64-v8a
#
# Verwendung: ./build_glfw_library.sh
#

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║         GLFW Android Library Builder                       ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Konfiguration
NDK_VERSION="26.1.10909125"
ANDROID_SDK="${ANDROID_SDK_ROOT:-$HOME/Android/Sdk}"
NDK_ROOT="$ANDROID_SDK/ndk/$NDK_VERSION"
TOOLCHAIN="$NDK_ROOT/build/cmake/android.toolchain.cmake"
SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SOURCE_DIR/build_glfw_lib"
INSTALL_DIR="$SOURCE_DIR/glfw_prebuilt"

# Farben für Output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "📍 Konfiguration:"
echo "  NDK Version: $NDK_VERSION"
echo "  NDK Path:   $NDK_ROOT"
echo "  Source:     $SOURCE_DIR"
echo "  Build Dir:  $BUILD_DIR"
echo ""

# Prüfe ob NDK installiert ist
if [ ! -f "$TOOLCHAIN" ]; then
    echo -e "${RED}❌ Fehler: NDK nicht gefunden!${NC}"
    echo "   Expected: $TOOLCHAIN"
    echo ""
    echo "Lösungen:"
    echo "1. Stelle sicher, dass Android SDK/NDK installiert ist:"
    echo "   $ANDROID_SDK"
    echo ""
    echo "2. Setze ANDROID_SDK_ROOT in ~/.bashrc:"
    echo "   export ANDROID_SDK_ROOT=\$HOME/Android/Sdk"
    echo ""
    exit 1
fi

echo -e "${GREEN}✅ NDK gefunden${NC}"
echo ""

# Bau für verschiedene ABIs
declare -a ABIS=("arm64-v8a" "x86_64")
FAILED_BUILDS=""

for ABI in "${ABIS[@]}"; do
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Building for $ABI..."
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    
    BUILD_ABI_DIR="$BUILD_DIR/$ABI"
    mkdir -p "$BUILD_ABI_DIR"
    
    cd "$BUILD_ABI_DIR"
    
    echo "🔨 CMake konfigurieren..."
    if ! cmake \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DANDROID_ABI="$ABI" \
        -DANDROID_PLATFORM=android-24 \
        -DCMAKE_BUILD_TYPE=Release \
        -DGLFW_BUILD_EXAMPLES=OFF \
        -DGLFW_BUILD_TESTS=OFF \
        -DGLFW_BUILD_DOCS=OFF \
        "$SOURCE_DIR"; then
        echo -e "${RED}❌ CMake configuration failed for $ABI${NC}"
        FAILED_BUILDS="$FAILED_BUILDS $ABI"
        cd "$SOURCE_DIR"
        continue
    fi
    
    echo ""
    echo "🔨 Kompilieren..."
    if ! make -j$(nproc); then
        echo -e "${RED}❌ Build failed for $ABI${NC}"
        FAILED_BUILDS="$FAILED_BUILDS $ABI"
        cd "$SOURCE_DIR"
        continue
    fi
    
    echo ""
    echo -e "${GREEN}✅ Build erfolgreich für $ABI${NC}"
    
    # Zeige Ergebnis
    echo ""
    echo "📦 Output:"
    if [ -f "src/libglfw.a" ]; then
        SIZE=$(ls -lh src/libglfw.a | awk '{print $5}')
        echo "   ✓ src/libglfw.a ($SIZE)"
    fi
    
    cd "$SOURCE_DIR"
    echo ""
done

# Installiere Libraries
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Installiere Libraries..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

mkdir -p "$INSTALL_DIR/lib"
mkdir -p "$INSTALL_DIR/include/GLFW"

for ABI in "${ABIS[@]}"; do
    BUILD_ABI_DIR="$BUILD_DIR/$ABI"
    if [ -f "$BUILD_ABI_DIR/src/libglfw.a" ]; then
        mkdir -p "$INSTALL_DIR/lib/$ABI"
        cp "$BUILD_ABI_DIR/src/libglfw.a" "$INSTALL_DIR/lib/$ABI/"
        echo "✓ $ABI: libglfw.a installiert"
    fi
done

# Kopiere Header
cp include/GLFW/*.h "$INSTALL_DIR/include/GLFW/"
echo "✓ Header-Dateien installiert"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ -z "$FAILED_BUILDS" ]; then
    echo -e "${GREEN}✅ Alle Builds erfolgreich!${NC}"
    echo ""
    echo "📦 Installation:"
    echo "   $INSTALL_DIR/"
    echo "   ├── lib/"
    echo "   │   ├── arm64-v8a/libglfw.a"
    echo "   │   └── x86_64/libglfw.a"
    echo "   └── include/GLFW/"
    echo "       └── *.h"
    echo ""
    echo "🚀 Verwendung in anderem Projekt:"
    echo ""
    echo "1. Kopiere zu deinem Projekt:"
    echo "   cp -r $INSTALL_DIR my_project/glfw_lib"
    echo ""
    echo "2. In CMakeLists.txt:"
    echo "   add_library(glfw STATIC IMPORTED)"
    echo "   set_target_properties(glfw PROPERTIES"
    echo "     IMPORTED_LOCATION \${CMAKE_CURRENT_SOURCE_DIR}/glfw_lib/lib/arm64-v8a/libglfw.a"
    echo "     INTERFACE_INCLUDE_DIRECTORIES \${CMAKE_CURRENT_SOURCE_DIR}/glfw_lib/include"
    echo "   )"
    echo "   target_link_libraries(my_app glfw)"
    echo ""
else
    echo -e "${RED}❌ Einige Builds fehlgeschlagen:$FAILED_BUILDS${NC}"
    exit 1
fi

echo "📚 Dokumentation: BUILD_GLFW_LIBRARY.md"
echo ""
