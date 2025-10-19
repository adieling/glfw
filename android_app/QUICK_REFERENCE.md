# Android Vulkan Quick Reference

Schnelle Referenz für häufige Aufgaben beim Portieren von Vulkan-Apps nach Android mit GLFW.

---

## 🚀 Projekt-Setup (5 Minuten)

### 1. Basis-Dateien erstellen

```bash
your_app/
├── app/
│   ├── src/main/AndroidManifest.xml
│   ├── build.gradle
│   └── CMakeLists.txt
├── your_app.c
├── build.gradle
├── settings.gradle
├── local.properties      # SDK/NDK Pfade
└── gradlew              # Gradle Wrapper
```

### 2. local.properties

```properties
sdk.dir=/path/to/Android/Sdk
ndk.dir=/path/to/Android/Sdk/ndk/26.1.10909125
```

### 3. AndroidManifest.xml - Template

```xml
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android">
    <uses-feature android:name="android.hardware.vulkan.version" 
                  android:version="0x400003" android:required="true"/>
    
    <application android:label="Your App" android:hasCode="false">
        <activity android:name="android.app.NativeActivity" android:exported="true">
            <meta-data android:name="android.app.lib_name" 
                       android:value="your_lib_name"/>  <!-- OHNE lib prefix! -->
            <intent-filter>
                <action android:name="android.intent.action.MAIN"/>
                <category android:name="android.intent.category.LAUNCHER"/>
            </intent-filter>
        </activity>
    </application>
</manifest>
```

### 4. CMakeLists.txt - Template

```cmake
cmake_minimum_required(VERSION 3.22)
project(your_app C)

set(GLFW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory("${GLFW_ROOT}" "${CMAKE_BINARY_DIR}/glfw_build")

add_library(your_lib_name SHARED "../your_app.c")  # Muss mit AndroidManifest übereinstimmen!
target_compile_definitions(your_lib_name PRIVATE GLFW_INCLUDE_NONE)
target_include_directories(your_lib_name PRIVATE "${GLFW_ROOT}/include" "${GLFW_ROOT}/deps")
target_link_libraries(your_lib_name glfw android log)
```

---

## 📝 Code-Anpassungen

### Includes

```c
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "APP", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "APP", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGE(...) fprintf(stderr, __VA_ARGS__)
#endif
```

### Vulkan Instance

```c
uint32_t extCount;
const char** exts = glfwGetRequiredInstanceExtensions(&extCount);

VkInstanceCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &(VkApplicationInfo){
        .apiVersion = VK_API_VERSION_1_0  // Wichtig: 1.0!
    },
    .enabledExtensionCount = extCount,
    .ppEnabledExtensionNames = exts
};
vkCreateInstance(&info, NULL, &instance);
```

### Swapchain

```c
// Surface capabilities abfragen
VkSurfaceCapabilitiesKHR caps;
vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);

VkSwapchainCreateInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = surface,
    .minImageCount = caps.minImageCount,
    .imageExtent = caps.currentExtent,
    .preTransform = caps.currentTransform,  // WICHTIG: currentTransform!
    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    // ... rest
};
```

---

## 🎨 Shader-Kompilierung

### GLSL → SPIR-V

```bash
# Vulkan 1.0 Target (wichtig!)
glslangValidator --target-env vulkan1.0 -V shader.vert -o vert.spv
glslangValidator --target-env vulkan1.0 -V shader.frag -o frag.spv

# Validieren
spirv-val --target-env vulkan1.0 vert.spv

# Adreno Bug Check
spirv-dis vert.spv | grep "OpVariable.*Private"
# Wenn gefunden → Shader umschreiben (siehe unten)
```

### SPIR-V → C Array

```bash
printf 'const uint32_t vertShaderCode[] = {\n' > shader.h
od -An -t x4 -v vert.spv | sed 's/^ *//' | sed 's/ /,0x/g' | sed 's/^/0x/' | sed 's/$/,/' >> shader.h
printf '};\n' >> shader.h
```

---

## ⚠️ Adreno GPU Bug Fix

### Problem-Code (VERMEIDEN!)

```glsl
void main() {
    vec2 positions[3] = vec2[](...);  // ❌ Erzeugt OpVariable Private
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
```

### Lösung: If-Else

```glsl
void main() {
    vec2 pos;
    if (gl_VertexIndex == 0) {
        pos = vec2(0.0, -0.5);
    } else if (gl_VertexIndex == 1) {
        pos = vec2(0.5, 0.5);
    } else {
        pos = vec2(-0.5, 0.5);
    }
    gl_Position = vec4(pos, 0.0, 1.0);
}
```

---

## 🎮 Touch Input

### Callbacks registrieren

```c
void cursorPosCallback(GLFWwindow* w, double x, double y) {
    int width, height;
    glfwGetFramebufferSize(w, &width, &height);
    float ndcX = (float)((x / width) * 2.0 - 1.0);
    float ndcY = -(float)((y / height) * 2.0 - 1.0);
    // Touch position nutzen
}

void mouseButtonCallback(GLFWwindow* w, int btn, int act, int mods) {
    if (btn == GLFW_MOUSE_BUTTON_LEFT) {
        bool pressed = (act == GLFW_PRESS);
        // Touch state nutzen
    }
}

glfwSetCursorPosCallback(window, cursorPosCallback);
glfwSetMouseButtonCallback(window, mouseButtonCallback);
```

---

## 🔧 Build & Deploy

### Build

```bash
cd your_app
./gradlew assembleDebug
```

### Install & Run

```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am start -S -n your.package.name/android.app.NativeActivity
```

### Logs ansehen

```bash
adb logcat -s YOUR_TAG
adb logcat | grep -E "YOUR_TAG|AndroidRuntime"
```

---

## 🐛 Häufige Fehler

### Library not found

❌ **Fehler**: `library "libyour_app.so" not found`  
✅ **Fix**: AndroidManifest: `android:value="your_app"` (OHNE lib/so)

### VK_ERROR_INCOMPATIBLE_DRIVER (-13)

❌ **Fehler**: Pipeline creation failed -13  
✅ **Fix**: Adreno Bug - Shader umschreiben (siehe oben)  
🔍 **Check**: `spirv-dis shader.spv | grep "OpVariable.*Private"`

### Undefined reference to glfwXXX

❌ **Fehler**: Linker error  
✅ **Fix**: CMakeLists: `target_link_libraries(... glfw)`

### Window/Surface creation failed

❌ **Fehler**: glfwCreateWindow returns NULL  
✅ **Fix**: Check `glfwVulkanSupported()` vor Window creation

---

## 📊 Performance Tips

```c
// Swapchain: Minimal images, FIFO mode
.minImageCount = caps.minImageCount  // Nicht +1
.presentMode = VK_PRESENT_MODE_FIFO_KHR

// MSAA: Niedrig für Mobile
.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT

// Shader: Precision qualifiers
precision mediump float;
```

---

## ✅ Pre-Flight Checklist

Vor dem ersten Build:

- [ ] `local.properties` mit korrekten Pfaden
- [ ] AndroidManifest: Library name OHNE lib/so
- [ ] AndroidManifest: `hasCode="false"`
- [ ] CMakeLists: Library name identisch mit Manifest
- [ ] CMakeLists: `GLFW_INCLUDE_NONE` definiert
- [ ] Shader mit `--target-env vulkan1.0`
- [ ] Shader gecheckt für OpVariable Private
- [ ] USB Debugging am Gerät aktiviert
- [ ] `adb devices` zeigt Gerät

---

## 🔗 Quick Links

- **GLFW Android Branch**: `git checkout android_port_continued`
- **Vollständige Anleitung**: `ANDROID_VULKAN_PORTING_GUIDE.md`
- **Touch Demo**: `android_vulkan_triangle_interactive.c`
- **Adreno Bug Details**: `SOLUTION_SUMMARY.md`

---

## 💡 One-Liner Helpers

```bash
# Gradle clean build
./gradlew clean assembleDebug

# Install & run & log
adb install -r app/build/outputs/apk/debug/app-debug.apk && \
adb shell am start -S -n your.package/android.app.NativeActivity && \
adb logcat -s YOUR_TAG

# Clear logs
adb logcat -c

# GPU info
adb shell dumpsys gpu

# Take screenshot
adb shell screencap /sdcard/screen.png && adb pull /sdcard/screen.png

# List devices
adb devices -l
```

---

**Schneller Start:**
1. Files aus Template kopieren
2. Library Namen anpassen (3 Stellen: Manifest, CMakeLists, C-Code)
3. SDK/NDK Pfade in local.properties
4. Shader kompilieren
5. `./gradlew assembleDebug installDebug`
6. Fertig! 🎉
