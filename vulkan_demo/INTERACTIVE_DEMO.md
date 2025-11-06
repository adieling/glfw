# Interactive Touch Demo - GLFW Android Vulkan Triangle

## Features ✨

Diese Demo zeigt Touch-Interaktivität mit GLFW und Vulkan auf Android:

### 1. **Touch-Verfolgung**
- Das Dreieck folgt Ihrem Finger wenn Sie den Bildschirm berühren
- Die Touch-Position wird als Push Constants an den Shader übergeben
- Sanfte Bewegung: Dreieck verschiebt sich um 50% der Touch-Position

### 2. **Visuelles Feedback**
- **Normaler Zustand**: Buntes RGB-Dreieck (Rot-Grün-Blau)
- **Gedrückt**: Dreieck leuchtet auf (50% heller durch Mix mit Weiß)
- Echtzeit-Response ohne Latenz

### 3. **GLFW Input System**
- Nutzt GLFW's plattform-unabhängige Input-Callbacks
- `glfwSetCursorPosCallback()` - Finger-Position tracking
- `glfwSetMouseButtonCallback()` - Touch pressed/released
- Automatische Konvertierung in normalized device coordinates (-1 to 1)

## Technische Details

### Shader Implementation
```glsl
layout(push_constant) uniform PushConstants {
    vec2 touchPos;  // Normalized device coordinates
    int pressed;    // 1 = touching, 0 = not touching
} pc;

void main() {
    // Hardcoded vertex positions (Adreno 710 compatible - no OpVariable Private)
    vec2 pos;
    vec3 col;
    if (gl_VertexIndex == 0) { pos = vec2(0.0, -0.5); col = vec3(1.0, 0.0, 0.0); }
    else if (gl_VertexIndex == 1) { pos = vec2(0.5, 0.5); col = vec3(0.0, 1.0, 0.0); }
    else { pos = vec2(-0.5, 0.5); col = vec3(0.0, 0.0, 1.0); }
    
    // Move triangle when touching
    if (pc.pressed == 1) {
        pos += pc.touchPos * 0.5;  // 50% offset
    }
    
    gl_Position = vec4(pos, 0.0, 1.0);
    
    // Glow effect when touching
    if (pc.pressed == 1) {
        col = mix(col, vec3(1.0), 0.5);  // 50% brighter
    }
    
    fragColor = col;
}
```

### Push Constants
```c
typedef struct {
    float x;          // Touch X in NDC
    float y;          // Touch Y in NDC
    int pressed;      // Touch state
    int padding;      // Alignment
} PushConstants;
```

**Size**: 16 bytes (aligned)
**Transfer**: Updated every frame via `vkCmdPushConstants()`
**Stage**: VK_SHADER_STAGE_VERTEX_BIT

### Input Callbacks
```c
static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    int fbw, fbh;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    // Convert screen coordinates to NDC (-1 to 1)
    g_pushConsts.x = (float)((xpos / (double)fbw) * 2.0 - 1.0);
    g_pushConsts.y = (float)(-((ypos / (double)fbh) * 2.0 - 1.0));  // Flip Y
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        g_pushConsts.pressed = (action == GLFW_PRESS) ? 1 : 0;
}
```

## Adreno 710 Compatibility ✅

Der Shader verwendet **keine `OpVariable Private` Arrays**, wodurch er auf Adreno 710 GPU funktioniert:

**Problematisch** (Error -13):
```glsl
vec2 positions[3] = vec2[](...);  // ❌ Generates OpVariable Private
```

**Funktioniert** (dieser Shader):
```glsl
vec2 pos;  // ✅ Local variable
if (gl_VertexIndex == 0) { pos = vec2(0.0, -0.5); }
```

## Performance

- **FPS**: Native refresh rate (typisch 60-120 Hz)
- **Latency**: < 16ms (1 frame) Touch-zu-Render
- **GPU Load**: Minimal (nur 3 vertices, einfache Shader)
- **Memory**: ~2KB SPIR-V shader code
- **Push Constants**: 16 bytes per frame (sehr effizient)

## Controls

- **Touch & Hold**: Dreieck folgt Finger und leuchtet auf
- **Release**: Dreieck kehrt zur Original-Position zurück
- **Multi-Touch**: Aktuell nur erster Touch tracked

## Erweiterungsmöglichkeiten

### 1. Animation
```glsl
uniform float time;
pos.y += sin(time + gl_VertexIndex) * 0.1;  // Wellen-Effekt
```

### 2. Rotation
```glsl
float angle = atan(touchPos.y, touchPos.x);
mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
pos = rotation * pos;
```

### 3. Scale
```glsl
float distance = length(touchPos);
pos *= (1.0 + distance * 0.5);  // Größer wenn weiter außen
```

### 4. Trail-Effekt
- Speichere letzte N Touch-Positionen
- Rendere mehrere Dreiecke mit Transparenz
- Fade-out über Zeit

### 5. Particles
- Generiere Partikel bei Touch
- Nutze Geometry Shader oder Instancing
- Physik-Simulation in Compute Shader

## Files

- `android_vulkan_triangle_interactive.c` - C implementation mit Touch-Support
- `shaders/interactive_triangle.vert` - GLSL vertex shader
- `shaders/interactive.spv` - Compiled SPIR-V (2140 bytes)
- `SOLUTION_SUMMARY.md` - Adreno 710 bug fix dokumentation

## Building

```bash
cd android_app
./gradlew assembleDebug installDebug
```

Oder manuell:
```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
adb shell am start -S -n org.glfw.example/android.app.NativeActivity
```

## Tested On

✅ **Xiaomi 2405CPCFBG**
- GPU: Adreno 710
- Driver: 512.615.98
- Vulkan: 1.1.128
- Android: 15

## Next Steps für Game Engine

1. **Input System**: Erweitern Sie GLFW callbacks für Gamepad, Keyboard
2. **Asset Loading**: Laden Sie Texturen, Modelle, Sounds
3. **Camera System**: Implementieren Sie FPS/Orbit/Follow Kameras
4. **Vertex Buffers**: Ersetzen Sie hardcoded vertices durch VBO
5. **Descriptor Sets**: Für Texturen, Uniforms, Storage Buffers
6. **Compute Shaders**: Für Physik, Particles, Post-Processing
7. **Multi-Pass Rendering**: Shadow maps, deferred shading, bloom

## License

Same as GLFW (zlib/libpng)

---

**Viel Erfolg mit Ihrer Vulkan Game Engine Portierung!** 🚀🎮
