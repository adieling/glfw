# Android Vulkan Porting Guide mit GLFW
## Komplette Anleitung zum Portieren von Vulkan-Apps nach Android

**Zielgruppe**: Entwickler, die bestehende Desktop Vulkan-Anwendungen auf Android portieren möchten  
**Framework**: GLFW `android_port_continued` Branch  
**Getestet auf**: Adreno 710, Android 15, Vulkan 1.1.128

---

## Inhaltsverzeichnis

1. [Projekt-Setup](#1-projekt-setup)
2. [Build-System Konfiguration](#2-build-system-konfiguration)
3. [Vulkan-Code Adaptierung](#3-vulkan-code-adaptierung)
4. [Shader-Kompilierung](#4-shader-kompilierung)
5. [Kritische Adreno GPU Bugs](#5-kritische-adreno-gpu-bugs)
6. [Input-Handling](#6-input-handling)
7. [Performance-Optimierungen](#7-performance-optimierungen)
8. [Debugging & Testing](#8-debugging--testing)
9. [Best Practices](#9-best-practices)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. Projekt-Setup

### 1.1 Voraussetzungen

```bash
# Erforderliche Tools
- Android SDK (API Level 24+, empfohlen: 34)
- Android NDK (Version 26.1.10909125 empfohlen)
- CMake 3.22.1+
- Gradle 8.7+
- Java 17
- glslangValidator (für Shader-Kompilierung)
- spirv-tools (für Shader-Validierung)
```

### 1.2 GLFW Android Branch

```bash
git clone https://github.com/glfw/glfw.git
cd glfw
git checkout android_port_continued  # Wichtig: android_port_continued Branch!
```

### 1.3 Android Projekt-Struktur

```
your_app/
├── app/
│   ├── src/
│   │   └── main/
│   │       └── AndroidManifest.xml
│   ├── build.gradle
│   └── CMakeLists.txt          # Native Code Build
├── your_vulkan_app.c           # Ihre Vulkan App
├── shaders/                     # GLSL Shader
│   ├── shader.vert
│   ├── shader.frag
│   └── *.spv                    # Kompilierte SPIR-V
├── build.gradle                 # Root Gradle
├── settings.gradle
├── local.properties            # SDK/NDK Pfade
└── gradlew                     # Gradle Wrapper
```

### 1.4 local.properties Konfiguration

```properties
# KRITISCH: Korrekte Pfade setzen
sdk.dir=/path/to/Android/Sdk
ndk.dir=/path/to/Android/Sdk/ndk/26.1.10909125

# Beispiel Linux:
# sdk.dir=/home/user/Android/Sdk
# ndk.dir=/home/user/Android/Sdk/ndk/26.1.10909125
```

---

## 2. Build-System Konfiguration

### 2.1 Root build.gradle

```gradle
buildscript {
    repositories {
        google()
        mavenCentral()
    }
    dependencies {
        classpath 'com.android.tools.build:gradle:8.7.0'
    }
}

allprojects {
    repositories {
        google()
        mavenCentral()
    }
}
```

### 2.2 app/build.gradle

```gradle
plugins {
    id 'com.android.application'
}

android {
    namespace 'your.package.name'  // Ändern!
    compileSdk 34
    ndkVersion "26.1.10909125"

    defaultConfig {
        applicationId "your.package.name"  // Ändern!
        minSdk 24  // Vulkan verfügbar ab API 24
        targetSdk 34
        versionCode 1
        versionName "1.0"
        
        externalNativeBuild {
            cmake {
                arguments "-DANDROID_STL=c++_shared",
                          "-DANDROID_PLATFORM=android-24"
                cppFlags "-std=c++17"
                abiFilters 'arm64-v8a'  // WICHTIG: 64-bit für moderne GPUs
            }
        }
    }

    externalNativeBuild {
        cmake {
            path "CMakeLists.txt"
            version "3.22.1"
        }
    }

    buildTypes {
        release {
            minifyEnabled false
            debuggable false
        }
        debug {
            debuggable true
        }
    }
}
```

### 2.3 app/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.22)
project(your_vulkan_app C CXX)

# GLFW Pfad (relativ zu diesem CMakeLists.txt)
set(GLFW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# GLFW nicht examples/tests bauen
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

# GLFW für Android bauen
add_subdirectory("${GLFW_ROOT}" "${CMAKE_BINARY_DIR}/glfw_build")

# Ihre Vulkan App als Shared Library
add_library(your_vulkan_app SHARED
    "../your_vulkan_app.c"
    # Weitere Quelldateien hier
)

# WICHTIG: GLFW_INCLUDE_NONE setzen (Vulkan selbst includen)
target_compile_definitions(your_vulkan_app PRIVATE 
    GLFW_INCLUDE_NONE
    VK_USE_PLATFORM_ANDROID_KHR
)

# Includes
target_include_directories(your_vulkan_app PRIVATE
    "${GLFW_ROOT}/include"
    "${GLFW_ROOT}/deps"  # Für vulkan.h
)

# Linken
target_link_libraries(your_vulkan_app
    glfw              # GLFW Bibliothek
    android           # Android Native API
    log               # Für __android_log_print
)

# KRITISCH: Vulkan zur Linkzeit NICHT linken!
# Vulkan wird dynamisch via dlopen geladen
```

### 2.4 AndroidManifest.xml

```xml
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android">
    
    <!-- Vulkan Feature erforderlich -->
    <uses-feature android:name="android.hardware.vulkan.version" 
                  android:version="0x400003"  
                  android:required="true"/>  <!-- Vulkan 1.0.3 -->
    
    <!-- Optional: Spezifische Vulkan Level -->
    <uses-feature android:name="android.hardware.vulkan.level"
                  android:version="1"
                  android:required="false"/>

    <application
        android:label="Your Vulkan App"
        android:hasCode="false">  <!-- WICHTIG: hasCode="false" -->
        
        <activity android:name="android.app.NativeActivity"
                  android:exported="true"
                  android:configChanges="orientation|screenSize|keyboardHidden">
            
            <!-- KRITISCH: Library Name OHNE "lib" Prefix und ".so" -->
            <meta-data android:name="android.app.lib_name"
                       android:value="your_vulkan_app"/>  <!-- NOT libyour_vulkan_app.so! -->
            
            <intent-filter>
                <action android:name="android.intent.action.MAIN"/>
                <category android:name="android.intent.category.LAUNCHER"/>
            </intent-filter>
        </activity>
    </application>
</manifest>
```

---

## 3. Vulkan-Code Adaptierung

### 3.1 Includes

```c
// RICHTIG: GLFW_INCLUDE_NONE setzen
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Dann Vulkan includen
#include <vulkan/vulkan.h>

// Android Logging
#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "YOUR_APP", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "YOUR_APP", __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGE(...) fprintf(stderr, __VA_ARGS__)
#endif
```

### 3.2 Window Creation

```c
// Desktop vs Android unterschiede minimal
void createWindow() {
    glfwInit();
    
    // Vulkan unterstützung prüfen
    if (!glfwVulkanSupported()) {
        LOGE("Vulkan not supported!");
        exit(1);
    }
    
    // WICHTIG: GLFW_NO_API für Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
    // Android: Größe wird ignoriert, nutzt Bildschirm-Auflösung
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "My App", NULL, NULL);
    
    if (!window) {
        LOGE("Window creation failed!");
        glfwTerminate();
        exit(1);
    }
}
```

### 3.3 Vulkan Instance Creation

```c
void createVulkanInstance() {
    // GLFW gibt benötigte Extensions zurück
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    LOGI("Required Vulkan extensions: %u", glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        LOGI("  - %s", glfwExtensions[i]);
    }
    // Android: Erwartet VK_KHR_surface + VK_KHR_android_surface
    
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "My Vulkan App",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0  // WICHTIG: 1.0 für Kompatibilität
    };
    
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = glfwExtensionCount,
        .ppEnabledExtensionNames = glfwExtensions,
        .enabledLayerCount = 0  // Validation layers auf Android schwierig
    };
    
    VkResult result = vkCreateInstance(&createInfo, NULL, &instance);
    if (result != VK_SUCCESS) {
        LOGE("vkCreateInstance failed: %d", result);
        exit(1);
    }
}
```

### 3.4 Surface Creation

```c
// GLFW abstrahiert Platform-Details
VkSurfaceKHR surface;
VkResult result = glfwCreateWindowSurface(instance, window, NULL, &surface);
if (result != VK_SUCCESS) {
    LOGE("Surface creation failed: %d", result);
    exit(1);
}
// Android: GLFW nutzt vkCreateAndroidSurfaceKHR intern
```

### 3.5 Physical Device Selection

```c
void selectPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    
    if (deviceCount == 0) {
        LOGE("No Vulkan capable GPU found!");
        exit(1);
    }
    
    VkPhysicalDevice* devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    
    // Device Properties abfragen
    for (uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        
        LOGI("GPU %u: %s", i, props.deviceName);
        LOGI("  Driver Version: %u.%u.%u", 
             VK_VERSION_MAJOR(props.driverVersion),
             VK_VERSION_MINOR(props.driverVersion),
             VK_VERSION_PATCH(props.driverVersion));
        LOGI("  Vulkan API: %u.%u.%u",
             VK_VERSION_MAJOR(props.apiVersion),
             VK_VERSION_MINOR(props.apiVersion),
             VK_VERSION_PATCH(props.apiVersion));
        LOGI("  Vendor: 0x%X", props.vendorID);
        // 0x5143 = Qualcomm (Adreno)
        // 0x13B5 = ARM (Mali)
        // 0x1010 = ImgTec (PowerVR)
    }
    
    // Erste Device nutzen (für Production: bessere Auswahl!)
    physicalDevice = devices[0];
    free(devices);
}
```

### 3.6 Queue Family Selection

```c
uint32_t findQueueFamily(VkPhysicalDevice device, VkQueueFlags flags, 
                         VkBool32 presentSupport, VkSurfaceKHR surface) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
    
    VkQueueFamilyProperties* queueFamilies = 
        malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);
    
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        VkBool32 hasFlags = (queueFamilies[i].queueFlags & flags) == flags;
        VkBool32 supportsPresent = VK_FALSE;
        
        if (presentSupport) {
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent);
        }
        
        if (hasFlags && (!presentSupport || supportsPresent)) {
            free(queueFamilies);
            return i;
        }
    }
    
    free(queueFamilies);
    LOGE("No suitable queue family found!");
    exit(1);
}

// Verwendung:
uint32_t graphicsFamily = findQueueFamily(physicalDevice, 
                                          VK_QUEUE_GRAPHICS_BIT, 
                                          VK_FALSE, VK_NULL_HANDLE);
uint32_t presentFamily = findQueueFamily(physicalDevice, 
                                         0, 
                                         VK_TRUE, surface);
```

### 3.7 Swapchain Creation

```c
void createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, 
                     VkSurfaceKHR surface) {
    // Surface capabilities abfragen
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
    
    LOGI("Surface capabilities:");
    LOGI("  Min images: %u, Max images: %u", 
         capabilities.minImageCount, capabilities.maxImageCount);
    LOGI("  Current extent: %ux%u", 
         capabilities.currentExtent.width, capabilities.currentExtent.height);
    
    // Image Count
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    
    // Surface Format wählen
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
    VkSurfaceFormatKHR* formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);
    
    VkSurfaceFormatKHR chosenFormat = formats[0];
    // Bevorzuge SRGB wenn verfügbar
    for (uint32_t i = 0; i < formatCount; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFormat = formats[i];
            break;
        }
    }
    LOGI("Chosen format: %d, color space: %d", chosenFormat.format, chosenFormat.colorSpace);
    free(formats);
    
    // Swapchain erstellen
    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = chosenFormat.format,
        .imageColorSpace = chosenFormat.colorSpace,
        .imageExtent = capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,  // oder CONCURRENT
        .preTransform = capabilities.currentTransform,  // WICHTIG: currentTransform nutzen!
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,  // V-Sync garantiert
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };
    
    VkResult result = vkCreateSwapchainKHR(device, &createInfo, NULL, &swapchain);
    if (result != VK_SUCCESS) {
        LOGE("Swapchain creation failed: %d", result);
        exit(1);
    }
}
```

---

## 4. Shader-Kompilierung

### 4.1 GLSL Shader schreiben

```glsl
// vertex_shader.vert
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
}
```

```glsl
// fragment_shader.frag
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

### 4.2 SPIR-V Kompilierung

```bash
# Host-Rechner (Linux/Mac/Windows)
glslangValidator --target-env vulkan1.0 -V vertex_shader.vert -o vert.spv
glslangValidator --target-env vulkan1.0 -V fragment_shader.frag -o frag.spv

# WICHTIG: --target-env vulkan1.0 für maximale Kompatibilität!
# SPIR-V ist plattform-unabhängig (funktioniert auf Desktop UND Android)
```

### 4.3 SPIR-V Validierung

```bash
# Validieren
spirv-val --target-env vulkan1.0 vert.spv
spirv-val --target-env vulkan1.0 frag.spv

# Disassemble (zum Debuggen)
spirv-dis vert.spv > vert.spvasm

# Problematische Opcodes finden (Adreno Bug!)
spirv-dis vert.spv | grep "OpVariable.*Private"
# Wenn gefunden: Siehe Abschnitt 5!
```

### 4.4 SPIR-V in C einbetten

```bash
# Methode 1: xxd (einfach)
xxd -i vert.spv > vert_spv.h

# Methode 2: od + sed (mehr Kontrolle)
echo "const uint32_t vertShaderCode[] = {" > shader.h
od -An -t x4 -v vert.spv | \
    sed 's/^ *//' | \
    sed 's/ /,0x/g' | \
    sed 's/^/0x/' | \
    sed 's/$/,/' >> shader.h
echo "};" >> shader.h
```

### 4.5 Shader Module Creation

```c
VkShaderModule createShaderModule(VkDevice device, 
                                  const uint32_t* code, 
                                  size_t codeSize) {
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = codeSize,
        .pCode = code
    };
    
    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &createInfo, NULL, &shaderModule);
    
    if (result != VK_SUCCESS) {
        LOGE("Shader module creation failed: %d", result);
        return VK_NULL_HANDLE;
    }
    
    LOGI("Shader module created successfully (%zu bytes)", codeSize);
    return shaderModule;
}

// Verwendung:
VkShaderModule vertModule = createShaderModule(device, vertShaderCode, sizeof(vertShaderCode));
VkShaderModule fragModule = createShaderModule(device, fragShaderCode, sizeof(fragShaderCode));
```

---

## 5. Kritische Adreno GPU Bugs

### 5.1 OpVariable Private Array Bug

**Problem**: Adreno 710 (und möglicherweise andere Adreno GPUs) haben einen JIT-Compiler Bug bei `OpVariable` mit `Private` Storage Class für Arrays.

**Symptom**:
```
vkCreateGraphicsPipelines failed: -13 (VK_ERROR_INCOMPATIBLE_DRIVER)
```

**Diagnose**:
```bash
spirv-dis your_shader.spv | grep "OpVariable.*Private"

# Wenn Output wie:
# %positions = OpVariable %_ptr_Private__arr_v2float_uint_3 Private
# %colors = OpVariable %_ptr_Private__arr_v3float_uint_3 Private
# → BUG!
```

**Problemcode (GLSL)**:
```glsl
// VERMEIDEN auf Adreno!
void main() {
    vec2 positions[3] = vec2[](
        vec2(0.0, -0.5),
        vec2(0.5, 0.5),
        vec2(-0.5, 0.5)
    );
    
    vec3 colors[3] = vec3[](
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 0.0, 1.0)
    );
    
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

**Lösung 1: If-Else Branching**:
```glsl
// FUNKTIONIERT auf Adreno!
void main() {
    vec2 pos;
    vec3 col;
    
    if (gl_VertexIndex == 0) {
        pos = vec2(0.0, -0.5);
        col = vec3(1.0, 0.0, 0.0);
    } else if (gl_VertexIndex == 1) {
        pos = vec2(0.5, 0.5);
        col = vec3(0.0, 1.0, 0.0);
    } else {
        pos = vec2(-0.5, 0.5);
        col = vec3(0.0, 0.0, 1.0);
    }
    
    gl_Position = vec4(pos, 0.0, 1.0);
    fragColor = col;
}
```

**Lösung 2: Vertex Buffers** (Empfohlen für Production):
```glsl
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
}
```

**Lösung 3: Uniform/Storage Buffers**:
```glsl
layout(set = 0, binding = 0) uniform VertexData {
    vec2 positions[3];
    vec3 colors[3];
} vertexData;

void main() {
    gl_Position = vec4(vertexData.positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = vertexData.colors[gl_VertexIndex];
}
```

### 5.2 Betroffene GPUs

- **Qualcomm Adreno 710** (bestätigt, Driver 512.615.98)
- Möglicherweise andere Adreno 7xx Serie
- **NICHT betroffen**: Adreno 6xx, Desktop GPUs (NVIDIA, AMD, Intel)

### 5.3 Validierung

```bash
# Compiler-Output prüfen
glslangValidator -V shader.vert -o shader.spv
spirv-dis shader.spv | grep -E "OpVariable|OpTypeArray" | head -20

# Wenn "OpVariable %var %_ptr_Private_%arraytype Private" 
# → Umschreiben!
```

---

## 6. Input-Handling

### 6.1 Touch als Mouse

```c
// GLFW behandelt Touch wie Mouse auf Android
static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    // Bildschirm-Koordinaten → Normalized Device Coordinates
    float ndcX = (float)((xpos / (double)width) * 2.0 - 1.0);
    float ndcY = -(float)((ypos / (double)height) * 2.0 - 1.0);  // Y flippen!
    
    LOGI("Touch position: screen(%f,%f) → NDC(%f,%f)", xpos, ypos, ndcX, ndcY);
    
    // An Shader übergeben (z.B. via Push Constants)
    updateTouchPosition(ndcX, ndcY);
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            LOGI("Touch began");
            isTouching = true;
        } else if (action == GLFW_RELEASE) {
            LOGI("Touch ended");
            isTouching = false;
        }
    }
}

// Callbacks registrieren
glfwSetCursorPosCallback(window, cursorPosCallback);
glfwSetMouseButtonCallback(window, mouseButtonCallback);
```

### 6.2 Push Constants für Input

```c
typedef struct {
    float touchX;
    float touchY;
    int isTouching;
    int padding;  // 16-byte Alignment!
} InputConstants;

InputConstants inputData = {0};

// Pipeline Layout mit Push Constants
VkPushConstantRange pushConstantRange = {
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    .offset = 0,
    .size = sizeof(InputConstants)
};

VkPipelineLayoutCreateInfo layoutInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pushConstantRangeCount = 1,
    .pPushConstantRanges = &pushConstantRange
};

// Im Render Loop
vkCmdPushConstants(commandBuffer, pipelineLayout, 
                   VK_SHADER_STAGE_VERTEX_BIT, 
                   0, sizeof(InputConstants), &inputData);
```

### 6.3 Framebuffer Resize

```c
static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    LOGI("Framebuffer resized: %dx%d", width, height);
    needSwapchainRecreate = true;  // Flag setzen
}

glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

// Im Main Loop prüfen
void mainLoop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        if (needSwapchainRecreate) {
            recreateSwapchain(window);
            needSwapchainRecreate = false;
        }
        
        drawFrame();
    }
}
```

---

## 7. Performance-Optimierungen

### 7.1 Mobile-Specific Settings

```c
// Swapchain: FIFO Mode für konstante Framerate
.presentMode = VK_PRESENT_MODE_FIFO_KHR  // V-Sync, spart Batterie

// Multisampling: Niedrig halten
.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT  // 1x für Performance

// Image Count: Minimal
uint32_t imageCount = capabilities.minImageCount;  // Nicht +1!

// Texture Kompression nutzen
// - ASTC (Android Standard)
// - ETC2 (Fallback)
VkFormat textureFormat = VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
```

### 7.2 Shader Optimierungen

```glsl
// Precision Qualifiers (GLSL ES Style)
precision mediump float;  // 16-bit statt 32-bit

// Vermeiden:
- Dynamische Branches (if mit uniforms)
- Texture lookups in Loops
- Komplexe math (pow, exp, sin/cos wenn möglich durch Lookup-Tables)

// Bevorzugen:
- vec4 statt vec3 (bessere Alignment)
- Konstanten hardcoden
- MAD operations (multiply-add)
```

### 7.3 Memory Management

```c
// Device Local Memory bevorzugen für Vertex/Index Buffers
VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

// Staging Buffers für Uploads
VkMemoryPropertyFlags stagingFlags = 
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

// Memory Budget prüfen (optional)
VkPhysicalDeviceMemoryBudgetPropertiesEXT budgetProps = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT
};
// Nur wenn Extension verfügbar!
```

### 7.4 Command Buffer Management

```c
// Command Pool per Thread
VkCommandPoolCreateInfo poolInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,  // Individual reset
    .queueFamilyIndex = graphicsQueueFamily
};

// Command Buffers recyclen, nicht jedes Frame neu allocieren
vkResetCommandBuffer(commandBuffer, 0);
```

---

## 8. Debugging & Testing

### 8.1 Logcat Filtering

```bash
# Nur Ihre App
adb logcat -s YOUR_APP_TAG

# Mit Timestamps
adb logcat -v time -s YOUR_APP_TAG

# Live mit Grep
adb logcat | grep -E "YOUR_APP|AndroidRuntime"

# Logs löschen vor Test
adb logcat -c
```

### 8.2 GPU Debugging

```bash
# Vulkan Layers (wenn verfügbar)
adb shell setprop debug.vulkan.layers VK_LAYER_KHRONOS_validation

# RenderDoc (ARM64 Support erforderlich)
# - Schwierig auf Android, besser Desktop testen

# Adreno Profiler (Qualcomm)
# - Snapdragon Profiler herunterladen
# - USB Debugging aktivieren
```

### 8.3 Performance Profiling

```c
// Simple Frame Time Messung
double lastTime = glfwGetTime();
int frameCount = 0;

while (!glfwWindowShouldClose(window)) {
    double currentTime = glfwGetTime();
    frameCount++;
    
    if (currentTime - lastTime >= 1.0) {  // Jede Sekunde
        double fps = (double)frameCount / (currentTime - lastTime);
        double ms = 1000.0 / fps;
        LOGI("FPS: %.1f (%.2f ms/frame)", fps, ms);
        frameCount = 0;
        lastTime = currentTime;
    }
    
    drawFrame();
}
```

### 8.4 Error Handling Best Practices

```c
#define VK_CHECK(call) \
    do { \
        VkResult result = call; \
        if (result != VK_SUCCESS) { \
            LOGE("Vulkan call failed: %s returned %d at %s:%d", \
                 #call, result, __FILE__, __LINE__); \
            abort(); \
        } \
    } while(0)

// Verwendung:
VK_CHECK(vkCreateInstance(&createInfo, NULL, &instance));
VK_CHECK(vkCreateSwapchainKHR(device, &swapchainInfo, NULL, &swapchain));
```

---

## 9. Best Practices

### 9.1 Portierungs-Checkliste

- [ ] GLFW `android_port_continued` Branch nutzen
- [ ] `local.properties` mit korrekten SDK/NDK Pfaden
- [ ] AndroidManifest: `android.app.lib_name` OHNE lib-prefix
- [ ] AndroidManifest: `hasCode="false"` setzen
- [ ] CMakeLists: `GLFW_INCLUDE_NONE` definieren
- [ ] CMakeLists: `abiFilters 'arm64-v8a'` setzen
- [ ] Vulkan API Version 1.0 für Kompatibilität
- [ ] Shader mit `--target-env vulkan1.0` kompilieren
- [ ] `spirv-dis` checken für OpVariable Private
- [ ] Swapchain: `currentTransform` statt IDENTITY
- [ ] Input Callbacks registrieren
- [ ] Framebuffer Resize Callback implementieren
- [ ] Logging über `__android_log_print`
- [ ] Auf echtem Gerät testen (nicht nur Emulator!)

### 9.2 Code-Struktur

```c
// Trennen Sie Platform-Code
#ifdef __ANDROID__
    // Android-specific
#else
    // Desktop
#endif

// Oder verwenden Sie Function Pointers
struct Platform {
    void (*init)(void);
    void (*log)(const char* msg);
    // ...
};

#ifdef __ANDROID__
struct Platform platform = {
    .init = android_init,
    .log = android_log
};
#endif
```

### 9.3 Asset Loading

```c
#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android_native_app_glue.h>

// Assets aus APK laden
AAsset* asset = AAssetManager_open(assetManager, "shader.spv", AASSET_MODE_BUFFER);
off_t length = AAsset_getLength(asset);
uint32_t* buffer = malloc(length);
AAsset_read(asset, buffer, length);
AAsset_close(asset);
#else
// Desktop: aus Dateisystem
FILE* file = fopen("shader.spv", "rb");
// ...
#endif
```

### 9.4 Memory Leaks vermeiden

```c
// Immer Cleanup!
void cleanup() {
    vkDeviceWaitIdle(device);
    
    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    vkDestroyRenderPass(device, renderPass, NULL);
    
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        vkDestroyFramebuffer(device, framebuffers[i], NULL);
        vkDestroyImageView(device, imageViews[i], NULL);
    }
    
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    
    free(framebuffers);
    free(imageViews);
    free(swapchainImages);
    
    glfwDestroyWindow(window);
    glfwTerminate();
}
```

---

## 10. Troubleshooting

### 10.1 Häufige Build-Fehler

**Problem**: `library "android_vulkan_triangle" not found`
```
Lösung: Prüfen Sie AndroidManifest.xml meta-data Name
- OHNE "lib" prefix
- OHNE ".so" suffix
- Muss mit CMakeLists.txt add_library Name übereinstimmen
```

**Problem**: `undefined reference to 'glfwCreateWindow'`
```
Lösung: target_link_libraries in CMakeLists.txt prüfen
- Muss 'glfw' enthalten
- Nach add_library Deklaration
```

**Problem**: NDK Version mismatch
```
Lösung: 
1. local.properties: ndk.dir auf korrekte Version
2. build.gradle: ndkVersion explizit setzen
3. ./gradlew clean
```

### 10.2 Runtime-Fehler

**Problem**: App startet nicht, sofort Crash
```
Diagnose: adb logcat | grep -E "AndroidRuntime|FATAL"
Häufige Ursachen:
- Falscher Library Name in AndroidManifest
- Fehlende Vulkan Support Deklaration
- main() Funktion nicht gefunden
```

**Problem**: `VK_ERROR_INCOMPATIBLE_DRIVER (-13)`
```
Ursache: Adreno OpVariable Private Bug
Lösung: Shader umschreiben (siehe Abschnitt 5.1)
Validierung: spirv-dis shader.spv | grep "OpVariable.*Private"
```

**Problem**: Swapchain creation failed
```
Diagnose:
1. Surface capabilities loggen
2. currentTransform prüfen (nicht IDENTITY!)
3. Format/Present Mode Kompatibilität
```

**Problem**: Touch funktioniert nicht
```
Prüfen:
- Callbacks mit glfwSet...Callback registriert?
- window Parameter korrekt?
- Loggen Sie Callback Aufrufe (LOGI)
```

### 10.3 Performance-Probleme

**Problem**: Niedrige FPS
```
Ursachen:
- Multisampling zu hoch → auf 1x setzen
- Zu viele Swapchain Images → minImageCount nutzen
- V-Sync deaktiviert → FIFO_KHR nutzen
- Shader zu komplex → Precision reduzieren
- Overdraw → Frühe Depth Testing
```

**Problem**: Hoher Batterieverbrauch
```
Optimierungen:
- VK_PRESENT_MODE_FIFO_KHR (V-Sync)
- Frame Rate limitieren (30 fps für UI)
- Idle Detection (kein Rendering wenn nichts ändert)
- GPU Frequency Scaling nutzen
```

### 10.4 Debugging-Tools

```bash
# Vulkan Info
adb shell dumpsys gpu

# Memory Info
adb shell dumpsys meminfo your.package.name

# CPU/GPU Profiling
adb shell perfetto

# Screenshot
adb shell screencap /sdcard/screenshot.png
adb pull /sdcard/screenshot.png

# GPU Frequency
adb shell cat /sys/class/kgsl/kgsl-3d0/gpuclk
```

---

## 11. Beispiel: Komplette Minimal-App

```c
// minimal_vulkan_android.c
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <android/log.h>
#include <stdlib.h>

#define LOG_TAG "MinimalVulkan"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Globals (für Simplicity)
VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice;
VkDevice device;
VkQueue graphicsQueue;
VkSwapchainKHR swapchain;

// Ihre Shader-Daten hier (aus Abschnitt 4.4)
#include "vert_spv.h"
#include "frag_spv.h"

int main(void) {
    // 1. GLFW Init
    if (!glfwInit()) {
        LOGE("glfwInit failed");
        return 1;
    }
    
    if (!glfwVulkanSupported()) {
        LOGE("Vulkan not supported");
        glfwTerminate();
        return 1;
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Minimal Vulkan", NULL, NULL);
    
    if (!window) {
        LOGE("Window creation failed");
        glfwTerminate();
        return 1;
    }
    
    // 2. Vulkan Instance
    uint32_t extCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extCount);
    
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_0
    };
    
    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = extCount,
        .ppEnabledExtensionNames = extensions
    };
    
    if (vkCreateInstance(&instanceInfo, NULL, &instance) != VK_SUCCESS) {
        LOGE("Instance creation failed");
        return 1;
    }
    
    // 3. Surface
    if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
        LOGE("Surface creation failed");
        return 1;
    }
    
    // 4. Physical Device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    VkPhysicalDevice* devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    physicalDevice = devices[0];
    free(devices);
    
    // 5. Logical Device (simplified)
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = 0,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };
    
    const char* deviceExt[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkPhysicalDeviceFeatures features = {0};
    
    VkDeviceCreateInfo deviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = deviceExt,
        .pEnabledFeatures = &features
    };
    
    if (vkCreateDevice(physicalDevice, &deviceInfo, NULL, &device) != VK_SUCCESS) {
        LOGE("Device creation failed");
        return 1;
    }
    
    vkGetDeviceQueue(device, 0, 0, &graphicsQueue);
    
    // 6. Swapchain (siehe Abschnitt 3.7)
    // ... (Code hier einfügen)
    
    LOGI("Vulkan initialization complete!");
    
    // Main Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // Rendering hier
    }
    
    // Cleanup
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
```

---

## 12. Weiterführende Ressourcen

### 12.1 Dokumentation

- **GLFW Android**: https://github.com/glfw/glfw (Branch: android_port_continued)
- **Vulkan Spec**: https://www.khronos.org/vulkan/
- **Android NDK**: https://developer.android.com/ndk/guides
- **Adreno GPU**: https://developer.qualcomm.com/software/adreno-gpu-sdk

### 12.2 Tools

- **glslangValidator**: https://github.com/KhronosGroup/glslang
- **spirv-tools**: https://github.com/KhronosGroup/SPIRV-Tools
- **RenderDoc**: https://renderdoc.org/ (Desktop Debugging)
- **Snapdragon Profiler**: https://developer.qualcomm.com/software/snapdragon-profiler

### 12.3 Beispiel-Projekte

- Diese Demo: `android_app/android_vulkan_triangle_interactive.c`
- GLFW Beispiele: `glfw/examples/` (Desktop-Referenz)
- Vulkan Samples: https://github.com/KhronosGroup/Vulkan-Samples

---

## 13. Zusammenfassung

### Kritische Punkte für erfolgreiche Portierung:

1. ✅ **GLFW Branch**: `android_port_continued` verwenden
2. ✅ **Build-System**: Korrekte CMakeLists.txt + AndroidManifest.xml
3. ✅ **Shader**: SPIR-V für Vulkan 1.0, keine Private arrays
4. ✅ **Swapchain**: `currentTransform` nutzen, nicht IDENTITY
5. ✅ **Input**: GLFW Callbacks für Touch
6. ✅ **Testen**: Auf echter Hardware, nicht nur Emulator
7. ✅ **Performance**: Mobile-spezifische Optimierungen
8. ✅ **Debugging**: Logcat + SPIR-V Analyse

### Typischer Workflow:

```
1. Desktop Version funktioniert
2. Android Projekt Setup (Build.gradle, CMakeLists, Manifest)
3. GLFW integrieren (android_port_continued)
4. Shader für Vulkan 1.0 kompilieren
5. Adreno Bugs fixen (OpVariable Private)
6. Auf Gerät deployen und testen
7. Input/Touch implementieren
8. Performance optimieren
9. Fertig! 🎉
```

---

**Viel Erfolg bei der Portierung Ihrer Vulkan-App! 🚀**

Bei Fragen: Siehe Abschnitt 10 (Troubleshooting) oder erstellen Sie ein Issue im GLFW Repository.
