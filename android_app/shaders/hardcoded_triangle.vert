#version 450

layout(location = 0) out vec3 fragColor;

void main() {
    // Hardcoded triangle positions - no arrays
    vec2 pos;
    vec3 col;
    
    if (gl_VertexIndex == 0) {
        pos = vec2(0.0, -0.5);
        col = vec3(1.0, 0.0, 0.0);  // red
    } else if (gl_VertexIndex == 1) {
        pos = vec2(0.5, 0.5);
        col = vec3(0.0, 1.0, 0.0);  // green
    } else {
        pos = vec2(-0.5, 0.5);
        col = vec3(0.0, 0.0, 1.0);  // blue
    }
    
    gl_Position = vec4(pos, 0.0, 1.0);
    fragColor = col;
}
