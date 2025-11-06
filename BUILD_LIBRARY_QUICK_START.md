# GLFW Library für andere Projekte bauen - Quick Start

## 🚀 In 2 Minuten

### 1. Baue die Library

```bash
cd /home/adieling/code/glfw_android
./build_glfw_library.sh
```

**Output:**
```
glfw_prebuilt/
├── lib/
│   ├── arm64-v8a/libglfw.a
│   └── x86_64/libglfw.a
└── include/GLFW/
    ├── glfw3.h
    ├── glfw3native.h
    └── ...
```

### 2. Kopiere zu deinem Projekt

```bash
# Neues Projekt
mkdir ~/code/my_vulkan_game
cd ~/code/my_vulkan_game

# Kopiere GLFW Library
cp -r /home/adieling/code/glfw_android/glfw_prebuilt .

# Kopiere auch Vulkan Headers und andere Dependencies
cp -r /home/adieling/code/glfw_android/deps .
```

### 3. CMakeLists.txt erstellen

```cmake
# app/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(my_vulkan_game C)

# GLFW als Static Library einbinden
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../glfw_prebuilt/lib/arm64-v8a/libglfw.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/../glfw_prebuilt/include
)

# Deine App
add_library(my_vulkan_game SHARED my_game.c)

target_include_directories(my_vulkan_game PRIVATE
    ../glfw_prebuilt/include
    ../deps
)

target_link_libraries(my_vulkan_game
    glfw
    android
    log
    Vulkan::Vulkan
)
```

### 4. build.gradle anpassen

```gradle
android {
    externalNativeBuild {
        cmake {
            path 'CMakeLists.txt'
            version '3.22.1'
        }
    }
}
```

### 5. Neues Projekt ist ready!

```bash
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

---

## 📋 Struktur des eigenen Projekts

```
my_vulkan_game/
├── app/
│   ├── src/main/
│   │   ├── AndroidManifest.xml
│   │   ├── java/org/glfw/example/
│   │   │   └── MainActivity.java (oder NativeActivity)
│   │   └── jniLibs/
│   │       └── arm64-v8a/
│   │           └── libmy_vulkan_game.so
│   ├── CMakeLists.txt         ← Dein CMake File
│   └── build.gradle
│
├── glfw_prebuilt/             ← Kopiert von diesem Projekt
│   ├── lib/arm64-v8a/libglfw.a
│   └── include/GLFW/*.h
│
├── deps/                       ← Vulkan, linmath, etc.
│   ├── vulkan/
│   ├── linmath.h
│   └── ...
│
├── my_game.c                   ← Dein Vulkan Code
├── build.gradle
├── settings.gradle
└── gradlew
```

---

## ✅ Was bekommst du

✓ **libglfw.a** (150 KB) - Statische Library mit allen GLFW Funktionen  
✓ **Header-Dateien** - glfw3.h, glfw3native.h, etc.  
✓ **Android-ready** - Bereits für ARM64 und x86_64 kompiliert  
✓ **Production-ready** - Release Build mit Optimierungen  

---

## 🔥 Bonus: Mehrere ABIs

Wenn du auch x86_64 unterstützen möchtest (für Emulator):

```cmake
# CMakeLists.txt - Beide ABIs
add_library(glfw STATIC IMPORTED)

if(ANDROID_ABI STREQUAL "arm64-v8a")
    set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../glfw_prebuilt/lib/arm64-v8a/libglfw.a
    )
elseif(ANDROID_ABI STREQUAL "x86_64")
    set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../glfw_prebuilt/lib/x86_64/libglfw.a
    )
endif()
```

---

## 🆘 Fehlerbehandlung

**"libglfw.a not found"**
→ Stelle sicher, dass build_glfw_library.sh erfolgreich war

**"GLFW/glfw3.h not found"**
→ Prüfe: `ls glfw_prebuilt/include/GLFW/`

**"undefined reference to 'glfwCreateWindow'"**
→ Prüfe in CMakeLists.txt: `target_link_libraries` enthält `glfw`

---

## 📚 Weitere Infos

- Vollständige Dokumentation: `BUILD_GLFW_LIBRARY.md`
- GLFW API: `https://www.glfw.org/docs/3.4/`
- GLFW GitHub: `https://github.com/glfw/glfw`

---

**Du bist ready! 🚀**
