// Minimal Android GLFW example: draws a triangle with GLES2 and reacts to touch
// This is built as a shared library on Android (NativeActivity loads it).
// GLFW android backend calls main() from android_main.

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#ifdef __ANDROID__
#include <GLES2/gl2.h>
#include <android/log.h>
#define LOG_TAG "GLFW_TRIANGLE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#error "android_triangle.c is intended for Android builds only"
#endif

static float g_touchX = 0.0f;   // NDC space [-1, 1]
static float g_touchY = 0.0f;   // NDC space [-1, 1]
static int   g_pressed = 0;

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    if (fbw <= 0 || fbh <= 0)
        return;
    // Convert to NDC [-1,1]
    g_touchX = (float)((xpos / (double)fbw) * 2.0 - 1.0);
    // Note: invert Y because window coords origin is top-left on Android surface
    g_touchY = (float)(-((ypos / (double)fbh) * 2.0 - 1.0));
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)window; (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        g_pressed = (action == GLFW_PRESS);
}

static GLuint compile_shader(GLenum type, const char* src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[512];
        GLsizei len = 0;
        glGetShaderInfoLog(sh, sizeof(log), &len, log);
        fprintf(stderr, "Shader compile error: %.*s\n", (int)len, log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static GLuint link_program(GLuint vs, GLuint fs)
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "aPos");
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[512];
        GLsizei len = 0;
        glGetProgramInfoLog(prog, sizeof(log), &len, log);
        fprintf(stderr, "Program link error: %.*s\n", (int)len, log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

static void error_callback(int code, const char* desc)
{
    LOGE("GLFW error %d: %s", code, desc);
}

int main(void)
{
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        LOGE("glfwInit failed");
        return 1;
    }

    // Request GLES2 context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    LOGI("Creating window");
    GLFWwindow* window = glfwCreateWindow(0, 0, "GLFW Android Triangle", NULL, NULL);
    if (!window)
    {
        LOGE("glfwCreateWindow failed");
        glfwTerminate();
        return 1;
    }

    LOGI("Making context current");
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    const char* vsSrc =
        "attribute vec2 aPos;\n"
        "uniform vec2 uOffset;\n"
        "void main(){\n"
        "  gl_Position = vec4(aPos + uOffset, 0.0, 1.0);\n"
        "}\n";

    const char* fsSrc =
        "precision mediump float;\n"
        "uniform float uPhase;\n"
        "uniform int uPressed;\n"
        "void main(){\n"
        "  vec3 base = vec3(0.2, 0.6, 1.0);\n"
        "  vec3 pulse = vec3(0.8, 0.2, 0.3) * (0.5 + 0.5 * sin(uPhase));\n"
        "  vec3 color = mix(base, pulse, 0.5);\n"
        "  if (uPressed == 1) color = vec3(1.0, 0.8, 0.2);\n"
        "  gl_FragColor = vec4(color, 1.0);\n"
        "}\n";

    LOGI("Compiling shaders");
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs) {
        LOGE("Shader compilation failed");
        return 1;
    }
    LOGI("Linking program");
    GLuint prog = link_program(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!prog)
        return 1;

    GLint locOffset = glGetUniformLocation(prog, "uOffset");
    GLint locPhase  = glGetUniformLocation(prog, "uPhase");
    GLint locPressed= glGetUniformLocation(prog, "uPressed");

    // A simple triangle in NDC centered at origin
    const GLfloat tri[6] = {
        0.0f,  0.5f,
       -0.5f, -0.5f,
        0.5f, -0.5f
    };

    LOGI("Entering render loop");
    while (!glfwWindowShouldClose(window))
    {
        int fbw = 0, fbh = 0;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        if (fbw < 1 || fbh < 1) { glfwPollEvents(); continue; }
        glViewport(0, 0, fbw, fbh);
        glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        // Phase animates over time
        float phase = (float)glfwGetTime();
        glUniform1f(locPhase, phase);
        glUniform1i(locPressed, g_pressed ? 1 : 0);

        // Map touch to small offset of triangle
        float k = 0.5f; // limit offset
        float ox = fmaxf(fminf(g_touchX, 1.0f), -1.0f) * k;
        float oy = fmaxf(fminf(g_touchY, 1.0f), -1.0f) * k;
        glUniform2f(locOffset, ox, oy);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, tri);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisableVertexAttribArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(prog);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
