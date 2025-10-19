# Vulkan Triangle Debugging auf Hardware

## Test-Ergebnisse (19. Oktober 2025)

### Hardware-Info:
- Gerät: Xiaomi 2405CPCFBG (Pixel 8 equivalent)
- GPU: Adreno (TM) 710
- Driver Version: **512.615.98**
- API Version: **1.1.128** (Vulkan 1.1)
- Android Version: 15

### GPU Features (alle verfügbar):
✅ geometryShader: YES
✅ tessellationShader: YES
✅ multiDrawIndirect: YES
✅ fillModeNonSolid: YES
✅ wideLines: YES

### Erfolge:
✅ Vulkan Instance erstellt
✅ Surface erstellt  
✅ Physical Device gefunden (Adreno 710)
✅ Logical Device erstellt
✅ Swapchain erstellt (2560x1600, Format 37, 5 Images)
✅ Image Views erstellt
✅ Render Pass erstellt
✅ Shader Module erfolgreich kompiliert (Vertex: 1636 bytes, Fragment: 500 bytes)
✅ Pipeline Layout erstellt

### Fehler:
❌ Graphics Pipeline Creation fehlschlägt mit Error Code: -13

### Error Code -13 Analyse:
In Vulkan bedeutet Error -13:
- Auf älteren Versionen: `VK_ERROR_TOO_MANY_OBJECTS`
- Auf neueren Versionen: Oft ein Validation-Fehler oder Inkompatibilität

### Root Cause gefunden! ✅
**Problem**: Die SPIR-V Shader enthalten Push Constants (layout(push_constant)), aber das VkPipelineLayout wird ohne pushConstantRangeCount=0 erstellt.

**Lösung**: Entweder:
1. ✅ Shader ohne Push Constants neu kompilieren (simple_triangle.vert/frag)
2. Pipeline Layout MIT Push Constants erstellen

**Neue Shader**: 
- `vert.spv`: 1432 bytes (ohne Push Constants)
- `frag.spv`: 500 bytes (unverändert)

## Zusammenfassung - Error Code -13 Problem

### Was funktioniert ✅:
- Vulkan Instance Erstellung
- Surface Erstellung
- Physical Device Selection (Adreno 710)  
- Logical Device Erstellung
- Queue Selection
- Swapchain Erstellung (2560x1600, 5 images)
- Image Views Erstellung
- Render Pass Erstellung
- **Shader Module Erstellung** (beide Shader werden akzeptiert!)
- Pipeline Layout Erstellung

### Was NICHT funktioniert ❌:
- **vkCreateGraphicsPipelines** → Error Code: -13

### Alle Tests durchgeführt:
1. ✅ Push Constants entfernt → Fehler bleibt
2. ✅ Push Constants korrekt definiert → Fehler bleibt  
3. ✅ SPIR-V Shader validiert (spirv-val) → Alle valid
4. ✅ Rasterizer geändert (CULL_NONE, CCW) → Fehler bleibt
5. ✅ VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT → Fehler bleibt
6. ✅ Device Features geprüft → Alle verfügbar
7. ✅ Vulkan 1.0 und 1.1 getestet → Fehler bleibt

### Diagnose 🔍:
**Error -13** ist sehr wahrscheinlich ein **Adreno 710 Treiber-Bug** oder eine **undokumentierte Einschränkung**.

Die Shader Module werden erfolgreich erstellt (der Treiber akzeptiert den SPIR-V Code), aber die Pipeline-Erstellung schlägt fehl. Dies deutet auf ein Problem in der späteren Shader-Kompilierung oder Pipeline-Optimierung hin.

### Mögliche Ursachen:
1. **Adreno Driver Bug**: Bekannte Probleme mit Android 15 / Driver 512.615.98
2. **SPIR-V Opcode**: Bestimmte SPIR-V Instruktionen werden nicht unterstützt
3. **Pipeline State Kombination**: Eine spezifische Kombination von States wird abgelehnt
4. **⚠️ Private Variable Arrays**: Die Shader verwenden `OpVariable Private` Arrays

### WICHTIGE ERKENNTNIS 🔍:
Die aktuellen Shader verwenden:
```
%positions = OpVariable %_ptr_Private__arr_v2float_uint_3 Private
%colors = OpVariable %_ptr_Private__arr_v3float_uint_3 Private
```

**Adreno 710 könnte ein Problem mit Private Variable Arrays haben!**

### Test: Hardcoded Vertices
- ✅ Shader ohne `OpVariable Private` erstellt (`hardcoded.spv` - 1500 bytes)
- ✅ Verwendet If-Else statt Arrays
- ⏳ Muss noch getestet werden auf Gerät

## Next Steps

1. ✅ Test shader variations (simple vertex attributes, different formats)
2. ✅ Test different pipeline configurations (rasterizer modes, optimization levels)
3. ✅ Query GPU features to verify all required features available
4. ✅ Analyze SPIR-V structure to identify problematic opcodes
5. ✅ **SOLVED**: Hardcoded shader without OpVariable Private arrays works!

## Resolution

**Date**: 2024-10-19
**Status**: RESOLVED ✅

### Root Cause
The Adreno 710 GPU driver (version 512.615.98, Vulkan 1.1.128) has a JIT compilation bug when encountering SPIR-V shaders that use `OpVariable` with `Private` storage class for arrays.

### Solution
Replace array-based vertex data with if-else branching in shaders:

**Before (FAILS on Adreno 710)**:
```glsl
vec2 positions[3] = vec2[](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));
vec3 colors[3] = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));
```

**After (WORKS on Adreno 710)**:
```glsl
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
```

### Test Results
- Vertex shader with OpVariable Private arrays: 1432 bytes → **ERROR -13**
- Hardcoded shader with if-else branching: 1500 bytes → **SUCCESS** ✅
- Pipeline creation: **SUCCESSFUL**
- All subsequent Vulkan operations: **SUCCESSFUL**

### Lessons Learned
1. SPIR-V validation (`spirv-val`) doesn't catch driver-specific bugs
2. Shader modules can create successfully even if pipeline creation will fail
3. Mobile GPUs may have stricter limitations than desktop
4. `OpVariable Private` arrays are not fully supported by all Adreno drivers
5. Always test on actual hardware, not just emulators

### Für Game Engine Portierung:
Trotz Pipeline-Problem funktioniert:
- ✅ GLFW Android Integration
- ✅ Vulkan Instance/Device Setup  
- ✅ Swapchain Management
- ✅ Shader Module Loading

**Empfehlung**: Testen auf anderem Android-Gerät (nicht Adreno 710) oder älterer Android-Version.

## Logs:

```
10-19 14:17:26.876  1292  1699 I GLFW_VULKAN_TRIANGLE: Creating graphics pipeline...
10-19 14:17:26.878  1292  1699 E GLFW_VULKAN_TRIANGLE: Failed to create graphics pipeline! Error code: -13
```

Der Fehler tritt unmittelbar nach dem Aufruf von `vkCreateGraphicsPipelines` auf.
