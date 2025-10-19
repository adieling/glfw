# GLFW Android Vulkan Demo & Porting Guide

## 📋 Übersicht

Dieses Projekt demonstriert **Vulkan auf Android mit GLFW** und bietet eine vollständige Anleitung zum Portieren von Desktop Vulkan-Anwendungen nach Android.

### ✨ Features

- ✅ **GLFW Integration** - Plattform-unabhängiges Window & Input Management
- ✅ **Vulkan 1.0** - Maximale Kompatibilität mit mobilen GPUs
- ✅ **Touch Support** - Interaktive Demo mit Touch-Reaktion
- ✅ **Adreno GPU Support** - Getestet auf Qualcomm Adreno 710
- ✅ **Production-Ready** - Vollständige Dokumentation für echte Projekte

### 🎮 Demos

1. **Hardcoded Triangle** (`android_vulkan_triangle_hardcoded.c`)
   - Einfaches buntes Dreieck
   - Minimaler Code für schnellen Start
   - Adreno 710 kompatibel

2. **Interactive Touch Demo** (`android_vulkan_triangle_interactive.c`)
   - Dreieck folgt Finger
   - Visuelles Feedback (Glow-Effekt)
   - Push Constants für Echtzeit-Input
   - **→ Aktuell aktiv!**

---

## 🚀 Quick Start (5 Minuten)

### Voraussetzungen

```bash
# Erforderlich:
- Android SDK (API 24+, empfohlen 34)
- Android NDK 26.1.10909125
- CMake 3.22.1+
- Java 17
- USB-Debugging am Gerät aktiviert
```

### Build & Install

```bash
cd android_app

# 1. SDK/NDK Pfade setzen (einmalig)
echo "sdk.dir=$HOME/Android/Sdk" > local.properties
echo "ndk.dir=$HOME/Android/Sdk/ndk/26.1.10909125" >> local.properties

# 2. Build
./gradlew assembleDebug

# 3. Install & Run
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am start -S -n org.glfw.example/android.app.NativeActivity

# 4. Logs ansehen
adb logcat -s GLFW_VULKAN_TRIANGLE
```

**Berühren Sie den Bildschirm** - das Dreieck folgt Ihrem Finger! 🎨

---

## 📚 Dokumentation

### Für Einsteiger

- **[QUICK_REFERENCE.md](QUICK_REFERENCE.md)** ⭐
  - Templates für alle wichtigen Dateien
  - Copy-Paste fertige Code-Snippets
  - Häufige Fehler & Lösungen
  - One-Liner Commands

### Für Fortgeschrittene

- **[ANDROID_VULKAN_PORTING_GUIDE.md](ANDROID_VULKAN_PORTING_GUIDE.md)** 📖
  - Vollständige Schritt-für-Schritt Anleitung
  - Build-System Details (Gradle, CMake)
  - Vulkan-Code Adaptierung
  - Performance-Optimierungen
  - Debugging & Profiling
  - **→ Hauptdokumentation für Portierungen!**

### Spezifische Themen

- **[SOLUTION_SUMMARY.md](SOLUTION_SUMMARY.md)** 🐛
  - Adreno 710 GPU Bug Details
  - OpVariable Private Array Problem
  - SPIR-V Analyse
  - Workarounds & Fixes

- **[INTERACTIVE_DEMO.md](INTERACTIVE_DEMO.md)** 🎮
  - Touch-Input Implementation
  - Push Constants
  - Shader-Effekte
  - Erweiterungsmöglichkeiten

- **[DEBUGGING_NOTES.md](DEBUGGING_NOTES.md)** 🔍
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
