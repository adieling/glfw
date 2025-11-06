# GLFW als Android Library bauen

## 📋 Übersicht

Dieses Dokument zeigt, wie man GLFW als vorkompilierte ARM64 Android Library baut und in anderen Projekten nutzt.

---

## 🎯 Zwei Methoden

### Methode 1: Über Gradle (Empfohlen) ✅

Diese Methode ist am einfachsten, da sie automatisch die richtige NDK-Version nutzt.

#### Schritt 1: Build-Script erstellen

```bash
cd /home/adieling/code/glfw_android
cat > build_glfw_library.sh << 'EOF'
#!/bin/bash

# GLFW Android Library Build Script
# Baut GLFW als vorkompilierte .a (static) und .so (shared) Libraries

set -e

NDK_VERSION="26.1.10909125"
ANDROID_SDK="$HOME/Android/Sdk"
NDK_ROOT="$ANDROID_SDK/ndk/$NDK_VERSION"
TOOLCHAIN="$NDK_ROOT/build/cmake/android.cmake"
SOURCE_DIR="$(pwd)"
BUILD_DIR="build_glfw_lib"

echo "=== GLFW Android Library Builder ==="
echo "NDK: $NDK_ROOT"
echo "Source: $SOURCE_DIR"
echo "Output: $BUILD_DIR"

# Konfiguriere für ARM64
mkdir -p $BUILD_DIR/arm64-v8a
cd $BUILD_DIR/arm64-v8a

cmake \
  -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -DCMAKE_BUILD_TYPE=Release \
  -DGLFW_BUILD_EXAMPLES=OFF \
  -DGLFW_BUILD_TESTS=OFF \
  -DGLFW_BUILD_DOCS=OFF \
  ../..

make -j$(nproc)

echo ""
echo "=== Build erfolgreich ==="
echo "Library-Dateien:"
ls -lh src/libglfw*.a src/libglfw*.so 2>/dev/null || echo "(nur Static Library)"

cd ../..
EOF

chmod +x build_glfw_library.sh
```

#### Schritt 2: Build ausführen

```bash
./build_glfw_library.sh
```

#### Schritt 3: Output prüfen

```bash
ls -lh build_glfw_lib/arm64-v8a/src/libglfw*
# Erwartete Ausgabe:
# build_glfw_lib/arm64-v8a/src/libglfw.a (statische Library)
```

---

### Methode 2: Vorkompilierte Library installieren

Wenn der Build fehlschlägt, können Sie die Library aus diesem Projekt kopieren:

```bash
# Diese Library wurde bereits für diesen Port gebaut
# Sie können sie direkt in anderen Projekten nutzen

SOURCE_LIB="build_glfw_lib/arm64-v8a/src/libglfw.a"
TARGET_PROJECT="$HOME/code/my_vulkan_game"

# Kopieren
mkdir -p $TARGET_PROJECT/lib/arm64-v8a
cp $SOURCE_LIB $TARGET_PROJECT/lib/arm64-v8a/

# Include-Dateien kopieren
mkdir -p $TARGET_PROJECT/include/GLFW
cp include/GLFW/*.h $TARGET_PROJECT/include/GLFW/
```

---

## 🏗️ In anderem Projekt nutzen

### Android Gradle Project

#### 1. CMakeLists.txt erstellen

```cmake
# app/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(my_vulkan_app C)

# GLFW Library einbinden
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../libs/arm64-v8a/libglfw.a
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Deine Vulkan App
add_library(my_vulkan_app SHARED
  my_vulkan_app.c
)

target_include_directories(my_vulkan_app PRIVATE
  ../include
  ../deps  # Vulkan headers, etc.
)

target_link_libraries(my_vulkan_app
  glfw
  android
  log
  Vulkan::Vulkan
)
```

#### 2. Projektstruktur

```
my_vulkan_game/
├── app/
│   ├── src/
│   │   └── main/
│   │       ├── java/
│   │       ├── jniLibs/
│   │       │   └── arm64-v8a/
│   │       │       └── libmy_vulkan_app.so
│   │       └── AndroidManifest.xml
│   ├── CMakeLists.txt
│   ├── build.gradle
│   └── my_vulkan_app.c
├── include/
│   └── GLFW/
│       └── *.h
├── libs/
│   └── arm64-v8a/
│       └── libglfw.a
├── deps/
│   └── vulkan/
├── build.gradle
└── settings.gradle
```

#### 3. build.gradle

```gradle
android {
    // ...
    externalNativeBuild {
        cmake {
            path 'CMakeLists.txt'
            version '3.22.1'
        }
    }
    
    packagingOptions {
        pickFirst 'lib/arm64-v8a/libc++_shared.so'
    }
}

dependencies {
    // Deine Abhängigkeiten
}
```

---

## 🔍 Verschiedene ABIs bauen

Wenn du auch x86_64 unterstützen möchtest:

```bash
#!/bin/bash

for ABI in arm64-v8a x86_64; do
  mkdir -p build_glfw_lib/$ABI
  cd build_glfw_lib/$ABI
  
  cmake \
    -DCMAKE_TOOLCHAIN_FILE=$NDK_ROOT/build/cmake/android.cmake \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=android-24 \
    -DCMAKE_BUILD_TYPE=Release \
    -DGLFW_BUILD_EXAMPLES=OFF \
    ../..
  
  make -j$(nproc)
  cd ../..
done

# Kopiere alle ABIs
for ABI in arm64-v8a x86_64; do
  mkdir -p lib/$ABI
  cp build_glfw_lib/$ABI/src/libglfw.a lib/$ABI/
done
```

---

## 📦 Library Distribution

### Variante A: ZIP-File

```bash
# Vorbereitung
mkdir -p glfw_android_libs/arm64-v8a
mkdir -p glfw_android_libs/include/GLFW

cp build_glfw_lib/arm64-v8a/src/libglfw.a glfw_android_libs/arm64-v8a/
cp include/GLFW/*.h glfw_android_libs/include/GLFW/

# Kopier auch CMakeLists.txt Template
cat > glfw_android_libs/CMakeLists.txt.template << 'EOFCMAKE'
# Template für andere Projekte
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/arm64-v8a/libglfw.a
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include
)
EOFCMAKE

# ZIP erstellen
zip -r glfw_android_arm64.zip glfw_android_libs/

echo "✅ Distribution: glfw_android_arm64.zip"
```

### Variante B: Gradle AAR (Advanced)

Falls du ein Gradle-Dependency veröffentlichen möchtest:

```gradle
// In build.gradle
android {
    libraryVariants.all { variant ->
        variant.outputs.all { output ->
            output.outputFileName = "glfw-android-${variant.buildType.name}.aar"
        }
    }
}

publishing {
    repositories {
        mavenLocal()
    }
    publications {
        release(MavenPublication) {
            groupId = 'org.glfw'
            artifactId = 'glfw-android'
            version = '3.4.0'
            artifact("build/outputs/aar/glfw-android-release.aar")
        }
    }
}
```

---

## 🔧 Troubleshooting

### Problem: "CMake not found"

**Lösung**: NDK-Version in `local.properties` prüfen
```bash
cat local.properties
# Muss enthalten: ndk.dir=/home/user/Android/Sdk/ndk/26.1.10909125
```

### Problem: "Vulkan headers nicht found"

**Lösung**: Vulkan-Header sind in diesem Port im `deps/vulkan/` Ordner
```bash
# Oder aus Gradle Projekt kopieren:
cp -r ../deps/vulkan include/
```

### Problem: Static Library wird nicht gelinkt

**Lösung**: Stelle sicher, dass GLFW mit `-fPIC` kompiliert wurde:
```cmake
# In CMakeLists.txt
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
```

---

## 📊 Performance & Größe

| Konfiguration | Größe | Hinweise |
|---------------|-------|---------|
| libglfw.a (Release, arm64-v8a) | ~150 KB | Mit Optimierungen |
| libglfw.a (Debug) | ~400 KB | Mit Debug-Symbolen |
| libglfw_shared.so | ~80 KB | Gekürzt, schneller |

**Empfehlung**: Nutze `Release` Build mit Static Library (`.a`) für finale APK.

---

## ✅ Checkliste für eigenes Projekt

- [ ] GLFW Library gebaut (arm64-v8a)
- [ ] include/GLFW/ Dateien kopiert
- [ ] CMakeLists.txt mit `add_library(glfw STATIC IMPORTED)` erstellt
- [ ] `target_link_libraries` mit `glfw` hinzugefügt
- [ ] build.gradle mit externalNativeBuild konfiguriert
- [ ] AndroidManifest.xml mit VK_KHR_android_surface Permission
- [ ] Vulkan Headers verfügbar (deps/vulkan/)
- [ ] Test-Build mit `./gradlew assembleDebug`

---

## 🚀 Beispiel: Minimales Projekt

```bash
# Neues Projekt erstellen
mkdir my_vulkan_game
cd my_vulkan_game

# GLFW Library kopieren
mkdir -p libs/arm64-v8a
cp /home/adieling/code/glfw_android/build_glfw_lib/arm64-v8a/src/libglfw.a libs/arm64-v8a/

# Include Dateien
mkdir -p include/GLFW
cp /home/adieling/code/glfw_android/include/GLFW/*.h include/GLFW/

# Vulkan Headers (wichtig!)
mkdir -p include/vulkan
cp /home/adieling/code/glfw_android/deps/vulkan/include/vulkan/*.h include/vulkan/

# CMakeLists.txt
cat > CMakeLists.txt << 'EOFCMAKE'
cmake_minimum_required(VERSION 3.10)
project(my_vulkan_game C)

add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
  IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libs/arm64-v8a/libglfw.a
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(my_game SHARED my_game.c)
target_link_libraries(my_game glfw android log Vulkan::Vulkan)
EOFCMAKE

echo "✅ Projekt vorbereitet!"
```

---

## 📚 Weiterführende Ressourcen

- [GLFW CMake Documentation](https://github.com/glfw/glfw/blob/master/CMakeLists.txt)
- [Android NDK CMake Toolchain](https://github.com/android-ndk/ndk/wiki/Using-cmake-with-the-NDK)
- [Gradle externalNativeBuild](https://developer.android.com/studio/projects/configure-cmake)

---

**Fertig! Du kannst GLFW jetzt in anderen Android Projekten als Library nutzen.** 🎉
