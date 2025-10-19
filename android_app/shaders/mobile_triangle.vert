#version 310 es
precision highp float;

layout(location = 0) out vec3 fragColor;

void main() {
    // Hardcoded triangle - optimized for mobile
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
