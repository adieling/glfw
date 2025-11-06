````markdown
# GLFW Android App - Build Environment

⚠️ **This is the Android Gradle build wrapper. For the actual Vulkan demo code, see [`../vulkan_demo/`](../vulkan_demo/)!**

## Overview

This directory contains the **Android Gradle project** that compiles a native Android app using GLFW.

The actual Vulkan implementation, shaders, and documentation have been moved to the separate `vulkan_demo/` directory because they form a self-contained demonstration project independent from GLFW core.

---

## Directory Structure

```
android_app/
├── README.md                    ← You are here
├── app/
│   ├── build.gradle             ← App module configuration
│   ├── CMakeLists.txt           ← Points to ../../vulkan_demo/ for source
│   └── src/main/
│       ├── AndroidManifest.xml
│       ├── java/                ← Java NativeActivity
│       └── res/                 ← Resources
├── build.gradle                 ← Root Gradle configuration
├── settings.gradle
├── local.properties             ← SDK/NDK paths (user-specific)
├── gradlew / gradlew.bat        ← Gradle wrapper
└── gradle/                      ← Gradle wrapper jars
```

---

## 🚀 Quick Start

### Prerequisites

```bash
# Required:
- Android SDK (API 24+, recommended 34)
- Android NDK 26.1.10909125
- CMake 3.22.1+
- Java 17
- USB debugging enabled
```

### Build & Install

```bash
cd android_app

# 1. Configure SDK/NDK paths (one-time)
echo "sdk.dir=$HOME/Android/Sdk" > local.properties
echo "ndk.dir=$HOME/Android/Sdk/ndk/26.1.10909125" >> local.properties

# 2. Build debug APK
./gradlew clean assembleDebug

# 3. Install to device
adb install -r app/build/outputs/apk/debug/app-debug.apk

# 4. Launch app
adb shell am start -S -n org.glfw.example/android.app.NativeActivity

# 5. View logs
adb logcat -d | grep "GLFW_VULKAN"
```

**Touch the screen** - the triangle follows your finger! 🎨

---

## ⭐ For the Actual Demo

See **[`../vulkan_demo/`](../vulkan_demo/)** for:
- Vulkan C source code
- Shader implementations
- Complete documentation
- Porting guides
- Build scripts

---

## 📋 Configuration Files

| File | Purpose |
|------|---------|
| `build.gradle` | Project-level Gradle configuration |
| `app/build.gradle` | App module, dependencies, signing |
| `app/CMakeLists.txt` | CMake build (points to vulkan_demo source) |
| `local.properties` | SDK/NDK paths (not in git) |
| `settings.gradle` | Gradle project structure |

---

## 🔧 Build Configuration

### Key CMake Changes

After moving code to `vulkan_demo/`, the CMakeLists.txt now references:

```cmake
# app/CMakeLists.txt
add_library(android_vulkan_triangle SHARED
    "../../vulkan_demo/android_vulkan_triangle.c"  # Updated path
)
```

---

## 🧪 Gradle Tasks

```bash
./gradlew tasks                 # List all available tasks
./gradlew clean                 # Clean build artifacts
./gradlew build                 # Build all variants
./gradlew installDebug          # Build and install to device
./gradlew lint                  # Run Android Lint checks
```

---

## 🔍 Troubleshooting

### Build Error: NDK not found
```bash
# Set NDK location properly
echo "ndk.dir=$HOME/Android/Sdk/ndk/26.1.10909125" > local.properties
```

### CMake Error: Source files not found
```bash
# Verify vulkan_demo directory exists
ls ../../vulkan_demo/*.c
```

### APK Installation Fails
```bash
# Check device connection
adb devices

# Check device CPU architecture
adb shell getprop ro.product.cpu.abilist
```

---

## 📚 Documentation

All documentation and implementation details have moved to **`../vulkan_demo/`**:

- **`ANDROID_VULKAN_PORTING_GUIDE.md`** - Complete step-by-step guide (8000+ lines)
- **`QUICK_REFERENCE.md`** - Quick lookup for templates and snippets
- **`SOLUTION_SUMMARY.md`** - Adreno 710 bug analysis
- **`DEBUGGING_NOTES.md`** - Full debugging history
- **`INTERACTIVE_DEMO.md`** - Touch input implementation
- **`FIXES_APPLIED.md`** - Comprehensive fix documentation

---

## 🏗️ Java Integration

The Android Java code provides:
- **NativeActivity**: Entry point for native code
- **JNI Bridge**: Communication with GLFW C code
- **Lifecycle Management**: onCreate, onResume, onPause, onDestroy
- **Input Routing**: Touch and keyboard events to GLFW

---

## 📄 License

Same as GLFW (zlib/libpng)

---

**For the actual implementation, visit [`../vulkan_demo/`](../vulkan_demo/)!** �
  - Komplette Debugging-Historie
  - Error-Codes & Ursachen
  - Test-Resultate

- **[FIXES_APPLIED.md](FIXES_APPLIED.md)** ✅
  - Alle angewandten Fixes
  - Vor/Nach Vergleiche

---

## 🏗️ Projekt-Struktur

```
android_app/
├── README.md                                    ← Sie sind hier
├── QUICK_REFERENCE.md                           ← Schnelle Hilfe
├── ANDROID_VULKAN_PORTING_GUIDE.md             ← Hauptanleitung
│
├── android_vulkan_triangle_interactive.c        ← Aktuelle Demo (Touch)
├── android_vulkan_triangle_hardcoded.c         ← Basis-Version
├── android_vulkan_triangle_fixed.c             ← Mit allen Fixes
│
├── shaders/
│   ├── interactive_triangle.vert               ← Touch-Shader
│   ├── interactive.spv                         ← Compiled (2140 bytes)
│   ├── hardcoded_triangle.vert                 ← Basis-Shader
│   └── hardcoded.spv                           ← Compiled (1500 bytes)
│
├── app/
│   ├── src/main/AndroidManifest.xml
│   ├── build.gradle
│   └── CMakeLists.txt
│
├── build.gradle                                 ← Root Gradle
├── settings.gradle
├── local.properties                            ← SDK/NDK Pfade (gitignored)
└── gradlew                                     ← Gradle Wrapper
```

---

## 🎯 Anwendungsfälle

### 1. Ich will einfach nur die Demo testen

→ Siehe [Quick Start](#-quick-start-5-minuten) oben

### 2. Ich will meine Vulkan Desktop-App nach Android portieren

→ Lesen Sie **[ANDROID_VULKAN_PORTING_GUIDE.md](ANDROID_VULKAN_PORTING_GUIDE.md)**

Workflow:
1. Desktop-Version funktioniert ✅
2. Android-Projekt Setup (Templates aus QUICK_REFERENCE.md)
3. GLFW `android_port_continued` Branch integrieren
4. Shader für Vulkan 1.0 + Adreno kompilieren
5. Testen & Optimieren

### 3. Ich habe einen Fehler

→ Siehe **[QUICK_REFERENCE.md - Häufige Fehler](QUICK_REFERENCE.md#-häufige-fehler)**

Häufigste Probleme:
- Library not found → AndroidManifest prüfen
- Error -13 → Adreno Bug, Shader umschreiben
- Build fails → local.properties Pfade
- Touch funktioniert nicht → Callbacks registriert?

### 4. Ich will Touch-Input hinzufügen

→ Siehe **[INTERACTIVE_DEMO.md](INTERACTIVE_DEMO.md)**

---

## ⚠️ Wichtige Hinweise

### Adreno GPU Bug (KRITISCH!)

**Problem**: Adreno 710 (und möglicherweise andere) haben einen Bug mit SPIR-V `OpVariable Private` Arrays.

**Symptom**: `vkCreateGraphicsPipelines failed: -13 (VK_ERROR_INCOMPATIBLE_DRIVER)`

**Lösung**: Verwenden Sie if-else statt Arrays in Shadern - Details in [SOLUTION_SUMMARY.md](SOLUTION_SUMMARY.md)

---

## 🧪 Getestet auf

| Gerät | GPU | Driver | Vulkan | Status |
|-------|-----|--------|--------|--------|
| Xiaomi 2405CPCFBG | Adreno 710 | 512.615.98 | 1.1.128 | ✅ Funktioniert |
| Android Emulator | SwiftShader | - | 1.1.0 | ✅ Funktioniert |

---

## 📄 Lizenz

Gleiche Lizenz wie GLFW (zlib/libpng)

---

**Happy Coding! 🚀🎮**
