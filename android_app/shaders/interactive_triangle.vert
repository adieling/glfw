#version 450

// Push constants für Touch-Position und Status
layout(push_constant) uniform PushConstants {
    vec2 touchPos;  // Touch position in normalized device coordinates (-1 to 1)
    int pressed;    // 1 wenn gedrückt, 0 sonst
} pc;

layout(location = 0) out vec3 fragColor;

void main() {
    vec2 pos;
    vec3 col;
    
    // Hardcoded triangle vertices (avoiding OpVariable Private arrays for Adreno 710)
    if (gl_VertexIndex == 0) {
        pos = vec2(0.0, -0.5);
        col = vec3(1.0, 0.0, 0.0);  // Rot
    } else if (gl_VertexIndex == 1) {
        pos = vec2(0.5, 0.5);
        col = vec3(0.0, 1.0, 0.0);  // Grün
    } else {
        pos = vec2(-0.5, 0.5);
        col = vec3(0.0, 0.0, 1.0);  // Blau
    }
    
    // Verschiebe Dreieck zur Touch-Position wenn gedrückt
    if (pc.pressed == 1) {
        pos += pc.touchPos * 0.5;  // 50% der Touch-Position als Offset
    }
    
    gl_Position = vec4(pos, 0.0, 1.0);
    
    // Ändere Farbe wenn gedrückt (mische mit Weiß für "glow" Effekt)
    if (pc.pressed == 1) {
        col = mix(col, vec3(1.0, 1.0, 1.0), 0.5);  // 50% heller
    }
    
    fragColor = col;
}
