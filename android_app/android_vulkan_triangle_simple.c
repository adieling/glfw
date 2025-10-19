// Minimal Android GLFW example with Vulkan: draws a triangle and reacts to touch
// This is built as a shared library on Android (NativeActivity loads it).
// GLFW android backend calls main() from android_main.

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "GLFW_VULKAN_TRIANGLE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#error "android_vulkan_triangle.c is intended for Android builds only"
#endif

// --- Vulkan globals ---
VkInstance g_instance;
VkSurfaceKHR g_surface;
VkPhysicalDevice g_physicalDevice = VK_NULL_HANDLE;
VkDevice g_device;
VkQueue g_graphicsQueue;
VkQueue g_presentQueue;
uint32_t g_graphicsQueueFamily = 0;
uint32_t g_presentQueueFamily = 0;

VkSwapchainKHR g_swapChain;
VkImage* g_swapChainImages;
VkFormat g_swapChainImageFormat;
VkExtent2D g_swapChainExtent;
VkImageView* g_swapChainImageViews;
uint32_t g_swapChainImageCount;
VkRenderPass g_renderPass;
VkPipelineLayout g_pipelineLayout;
VkPipeline g_graphicsPipeline;
VkFramebuffer* g_framebuffers;
VkCommandPool g_commandPool;
VkCommandBuffer* g_commandBuffers;

#define MAX_FRAMES_IN_FLIGHT 2
VkSemaphore g_imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
VkSemaphore g_renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
VkFence g_inFlightFences[MAX_FRAMES_IN_FLIGHT];
uint32_t g_currentFrame = 0;
int g_framebufferResized = 0;

// --- App state ---
typedef struct {
    float x;
    float y;
    int pressed;
    int padding; // Ensure 16-byte alignment
} PushConstants;

static PushConstants g_pushConsts = {0.0f, 0.0f, 0, 0};

// --- Helper Declarations ---
void initVulkan(GLFWwindow* window);
void mainLoop(GLFWwindow* window);
void cleanup();
void cleanupSwapChain();
void recreateSwapChain(GLFWwindow* window);
VkShaderModule createShaderModule(const uint32_t* code, size_t size);

// --- Callbacks ---
static void error_callback(int code, const char* desc) {
    LOGE("GLFW error %d: %s", code, desc);
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    int fbw, fbh;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    if (fbw <= 0 || fbh <= 0) return;
    g_pushConsts.x = (float)((xpos / (double)fbw) * 2.0 - 1.0);
    g_pushConsts.y = (float)(-((ypos / (double)fbh) * 2.0 - 1.0));
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    (void)window; (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        g_pushConsts.pressed = (action == GLFW_PRESS) ? 1 : 0;
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window; (void)width; (void)height;
    g_framebufferResized = 1;
}

// --- SPIR-V Shaders (embedded) ---
// Vertex shader: draws triangle with push constants for offset
// #version 450
// layout(push_constant) uniform PushConstants { vec2 offset; int pressed; } pc;
// layout(location = 0) out vec3 fragColor;
