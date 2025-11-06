// Interactive Android GLFW Vulkan Triangle - Touch Demo
// Demonstrates touch interaction: triangle follows finger and glows when pressed
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
    // Convert from screen coordinates to NDC (-1.0 to 1.0)
    // X-Achse: left(-1) to right(+1)
    // Y-Achse: top(-1) to bottom(+1) - note the negation to flip Y
    g_pushConsts.y = (float)((xpos / (double)fbw) * 2.0 - 1.0);      // Swap: was x, now y
    g_pushConsts.x = (float)(-((ypos / (double)fbh) * 2.0 - 1.0));   // Swap: was y, now x
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
// INTERACTIVE Vertex shader - with Push Constants for Touch
// Triangle moves and glows when touched (NO Private Variable Arrays for Adreno 710)
// #version 450
// layout(push_constant) uniform PushConstants {
//     vec2 touchPos; vec2 touchPos;  // Touch position in NDC (-1 to 1)
//     int pressed;    // 1 wenn gedrückt, 0 sonst
// } pc;
// layout(location = 0) out vec3 fragColor;
// void main() {
//     vec2 pos; vec3 col;
//     if (gl_VertexIndex == 0) { pos = vec2(0.0, -0.5); col = vec3(1.0, 0.0, 0.0); }
//     else if (gl_VertexIndex == 1) { pos = vec2(0.5, 0.5); col = vec3(0.0, 1.0, 0.0); }
//     else { pos = vec2(-0.5, 0.5); col = vec3(0.0, 0.0, 1.0); }
//     if (pc.pressed == 1) { pos += pc.touchPos * 0.5; }
//     gl_Position = vec4(pos, 0.0, 1.0);
//     if (pc.pressed == 1) { col = mix(col, vec3(1.0), 0.5); }  // Glow effect
//     fragColor = col;
// }
const uint32_t vertShaderCode[] = {
0x07230203,0x00010000,0x0008000b,0x0000004f,0x00000000,0x00020011,0x00000001,0x0006000b,
0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
0x0008000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x00000008,0x0000003c,0x0000004d,
0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00060005,
0x00000008,0x565f6c67,0x65747265,0x646e4978,0x00007865,0x00030005,0x00000012,0x00736f70,
0x00030005,0x00000018,0x006c6f63,0x00060005,0x00000027,0x68737550,0x736e6f43,0x746e6174,
0x00000073,0x00060006,0x00000027,0x00000000,0x63756f74,0x736f5068,0x00000000,0x00050006,
0x00000027,0x00000001,0x73657270,0x00646573,0x00030005,0x00000029,0x00006370,0x00060005,
0x0000003a,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x0000003a,0x00000000,
0x505f6c67,0x7469736f,0x006e6f69,0x00070006,0x0000003a,0x00000001,0x505f6c67,0x746e696f,
0x657a6953,0x00000000,0x00070006,0x0000003a,0x00000002,0x435f6c67,0x4470696c,0x61747369,
0x0065636e,0x00070006,0x0000003a,0x00000003,0x435f6c67,0x446c6c75,0x61747369,0x0065636e,
0x00030005,0x0000003c,0x00000000,0x00050005,0x0000004d,0x67617266,0x6f6c6f43,0x00000072,
0x00040047,0x00000008,0x0000000b,0x0000002a,0x00030047,0x00000027,0x00000002,0x00050048,
0x00000027,0x00000000,0x00000023,0x00000000,0x00050048,0x00000027,0x00000001,0x00000023,
0x00000008,0x00030047,0x0000003a,0x00000002,0x00050048,0x0000003a,0x00000000,0x0000000b,
0x00000000,0x00050048,0x0000003a,0x00000001,0x0000000b,0x00000001,0x00050048,0x0000003a,
0x00000002,0x0000000b,0x00000003,0x00050048,0x0000003a,0x00000003,0x0000000b,0x00000004,
0x00040047,0x0000004d,0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,
0x00000002,0x00040015,0x00000006,0x00000020,0x00000001,0x00040020,0x00000007,0x00000001,
0x00000006,0x0004003b,0x00000007,0x00000008,0x00000001,0x0004002b,0x00000006,0x0000000a,
0x00000000,0x00020014,0x0000000b,0x00030016,0x0000000f,0x00000020,0x00040017,0x00000010,
0x0000000f,0x00000002,0x00040020,0x00000011,0x00000007,0x00000010,0x0004002b,0x0000000f,
0x00000013,0x00000000,0x0004002b,0x0000000f,0x00000014,0xbf000000,0x0005002c,0x00000010,
0x00000015,0x00000013,0x00000014,0x00040017,0x00000016,0x0000000f,0x00000003,0x00040020,
0x00000017,0x00000007,0x00000016,0x0004002b,0x0000000f,0x00000019,0x3f800000,0x0006002c,
0x00000016,0x0000001a,0x00000019,0x00000013,0x00000013,0x0004002b,0x00000006,0x0000001d,
0x00000001,0x0004002b,0x0000000f,0x00000021,0x3f000000,0x0005002c,0x00000010,0x00000022,
0x00000021,0x00000021,0x0006002c,0x00000016,0x00000023,0x00000013,0x00000019,0x00000013,
0x0005002c,0x00000010,0x00000025,0x00000014,0x00000021,0x0006002c,0x00000016,0x00000026,
0x00000013,0x00000013,0x00000019,0x0004001e,0x00000027,0x00000010,0x00000006,0x00040020,
0x00000028,0x00000009,0x00000027,0x0004003b,0x00000028,0x00000029,0x00000009,0x00040020,
0x0000002a,0x00000009,0x00000006,0x00040020,0x00000030,0x00000009,0x00000010,0x00040017,
0x00000036,0x0000000f,0x00000004,0x00040015,0x00000037,0x00000020,0x00000000,0x0004002b,
0x00000037,0x00000038,0x00000001,0x0004001c,0x00000039,0x0000000f,0x00000038,0x0006001e,
0x0000003a,0x00000036,0x0000000f,0x00000039,0x00000039,0x00040020,0x0000003b,0x00000003,
0x0000003a,0x0004003b,0x0000003b,0x0000003c,0x00000003,0x00040020,0x00000041,0x00000003,
0x00000036,0x0006002c,0x00000016,0x00000049,0x00000019,0x00000019,0x00000019,0x00040020,
0x0000004c,0x00000003,0x00000016,0x0004003b,0x0000004c,0x0000004d,0x00000003,0x00050036,
0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003b,0x00000011,
0x00000012,0x00000007,0x0004003b,0x00000017,0x00000018,0x00000007,0x0004003d,0x00000006,
0x00000009,0x00000008,0x000500aa,0x0000000b,0x0000000c,0x00000009,0x0000000a,0x000300f7,
0x0000000e,0x00000000,0x000400fa,0x0000000c,0x0000000d,0x0000001b,0x000200f8,0x0000000d,
0x0003003e,0x00000012,0x00000015,0x0003003e,0x00000018,0x0000001a,0x000200f9,0x0000000e,
0x000200f8,0x0000001b,0x0004003d,0x00000006,0x0000001c,0x00000008,0x000500aa,0x0000000b,
0x0000001e,0x0000001c,0x0000001d,0x000300f7,0x00000020,0x00000000,0x000400fa,0x0000001e,
0x0000001f,0x00000024,0x000200f8,0x0000001f,0x0003003e,0x00000012,0x00000022,0x0003003e,
0x00000018,0x00000023,0x000200f9,0x00000020,0x000200f8,0x00000024,0x0003003e,0x00000012,
0x00000025,0x0003003e,0x00000018,0x00000026,0x000200f9,0x00000020,0x000200f8,0x00000020,
0x000200f9,0x0000000e,0x000200f8,0x0000000e,0x00050041,0x0000002a,0x0000002b,0x00000029,
0x0000001d,0x0004003d,0x00000006,0x0000002c,0x0000002b,0x000500aa,0x0000000b,0x0000002d,
0x0000002c,0x0000001d,0x000300f7,0x0000002f,0x00000000,0x000400fa,0x0000002d,0x0000002e,
0x0000002f,0x000200f8,0x0000002e,0x00050041,0x00000030,0x00000031,0x00000029,0x0000000a,
0x0004003d,0x00000010,0x00000032,0x00000031,0x0005008e,0x00000010,0x00000033,0x00000032,
0x00000021,0x0004003d,0x00000010,0x00000034,0x00000012,0x00050081,0x00000010,0x00000035,
0x00000034,0x00000033,0x0003003e,0x00000012,0x00000035,0x000200f9,0x0000002f,0x000200f8,
0x0000002f,0x0004003d,0x00000010,0x0000003d,0x00000012,0x00050051,0x0000000f,0x0000003e,
0x0000003d,0x00000000,0x00050051,0x0000000f,0x0000003f,0x0000003d,0x00000001,0x00070050,
0x00000036,0x00000040,0x0000003e,0x0000003f,0x00000013,0x00000019,0x00050041,0x00000041,
0x00000042,0x0000003c,0x0000000a,0x0003003e,0x00000042,0x00000040,0x00050041,0x0000002a,
0x00000043,0x00000029,0x0000001d,0x0004003d,0x00000006,0x00000044,0x00000043,0x000500aa,
0x0000000b,0x00000045,0x00000044,0x0000001d,0x000300f7,0x00000047,0x00000000,0x000400fa,
0x00000045,0x00000046,0x00000047,0x000200f8,0x00000046,0x0004003d,0x00000016,0x00000048,
0x00000018,0x00060050,0x00000016,0x0000004a,0x00000021,0x00000021,0x00000021,0x0008000c,
0x00000016,0x0000004b,0x00000001,0x0000002e,0x00000048,0x00000049,0x0000004a,0x0003003e,
0x00000018,0x0000004b,0x000200f9,0x00000047,0x000200f8,0x00000047,0x0004003d,0x00000016,
0x0000004e,0x00000018,0x0003003e,0x0000004d,0x0000004e,0x000100fd,0x00010038,
};

// Fragment shader: passes through the color
// #version 450
// layout(location = 0) in vec3 fragColor;
// layout(location = 0) out vec4 outColor;
// void main() { outColor = vec4(fragColor, 1.0); }
const uint32_t fragShaderCode[] = {
    0x07230203,0x00010000,0x0008000a,0x00000013,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
    0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00050005,0x00000009,0x4374756f,0x726f6c6f,0x00000000,0x00050005,0x0000000d,
    0x67617266,0x6f6c6f43,0x00000072,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,
    0x0000000d,0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
    0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,
    0x00000008,0x00000003,0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,
    0x0000000b,0x00000006,0x00000003,0x00040020,0x0000000c,0x00000001,0x0000000b,0x0004003b,
    0x0000000c,0x0000000d,0x00000001,0x0004002b,0x00000006,0x00000010,0x3f800000,0x00050036,
    0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003d,0x0000000b,
    0x0000000e,0x0000000d,0x00050051,0x00000006,0x0000000f,0x0000000e,0x00000000,0x00050051,
    0x00000006,0x00000010,0x0000000e,0x00000001,0x00050051,0x00000006,0x00000011,0x0000000e,
    0x00000002,0x00070050,0x00000007,0x00000012,0x0000000f,0x00000010,0x00000011,0x00000010,
    0x0003003e,0x00000009,0x00000012,0x000100fd,0x00010038
};

// --- Main ---
int main(void) {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        LOGE("glfwInit failed");
        return 1;
    }

    if (!glfwVulkanSupported()) {
        LOGE("Vulkan not supported on this system.");
        glfwTerminate();
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    LOGI("Creating window for Vulkan");
    GLFWwindow* window = glfwCreateWindow(1080, 1920, "GLFW Vulkan Triangle", NULL, NULL);
    if (!window) {
        LOGE("glfwCreateWindow failed");
        glfwTerminate();
        return 1;
    }

    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    initVulkan(window);
    mainLoop(window);
    cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// --- Vulkan Implementation ---
void createInstance() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    
    LOGI("Required extensions: %u", glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        LOGI("  Extension %u: %s", i, glfwExtensions[i]);
    }

    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;

    VkResult result = vkCreateInstance(&createInfo, NULL, &g_instance);
    if (result != VK_SUCCESS) {
        LOGE("Failed to create Vulkan instance, error code: %d", result);
        exit(1);
    }
    LOGI("Vulkan instance created successfully");
}

uint32_t findQueueFamily(VkQueueFlags requiredFlags, VkBool32 presentSupport) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(g_physicalDevice, &queueFamilyCount, NULL);
    
    VkQueueFamilyProperties* queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(g_physicalDevice, &queueFamilyCount, queueFamilies);
    
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        VkBool32 hasRequiredFlags = (queueFamilies[i].queueFlags & requiredFlags) == requiredFlags;
        VkBool32 supportsPresent = VK_FALSE;
        
        if (presentSupport) {
            vkGetPhysicalDeviceSurfaceSupportKHR(g_physicalDevice, i, g_surface, &supportsPresent);
        }
        
        if (hasRequiredFlags && (!presentSupport || supportsPresent)) {
            free(queueFamilies);
            return i;
        }
    }
    
    free(queueFamilies);
    LOGE("Failed to find suitable queue family");
    exit(1);
}

void pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(g_instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        LOGE("Failed to find GPUs with Vulkan support");
        exit(1);
    }
    VkPhysicalDevice* devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(g_instance, &deviceCount, devices);
    g_physicalDevice = devices[0]; // Simplification: pick first device
    free(devices);
    
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(g_physicalDevice, &deviceProperties);
    LOGI("Using GPU: %s", deviceProperties.deviceName);
}

void createLogicalDevice() {
    // Find queue families
    g_graphicsQueueFamily = findQueueFamily(VK_QUEUE_GRAPHICS_BIT, VK_FALSE);
    g_presentQueueFamily = findQueueFamily(0, VK_TRUE);
    
    LOGI("Graphics queue family: %u, Present queue family: %u", 
         g_graphicsQueueFamily, g_presentQueueFamily);
    
    // Create queue create infos
    float queuePriority = 1.0f;
    uint32_t uniqueQueueFamilies[2] = {g_graphicsQueueFamily, g_presentQueueFamily};
    uint32_t queueCreateInfoCount = (g_graphicsQueueFamily == g_presentQueueFamily) ? 1 : 2;
    
    VkDeviceQueueCreateInfo queueCreateInfos[2] = {0};
    for (uint32_t i = 0; i < queueCreateInfoCount; i++) {
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfos[i].queueCount = 1;
        queueCreateInfos[i].pQueuePriorities = &queuePriority;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {0};
    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.queueCreateInfoCount = queueCreateInfoCount;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(g_physicalDevice, &createInfo, NULL, &g_device) != VK_SUCCESS) {
        LOGE("Failed to create logical device");
        exit(1);
    }
    vkGetDeviceQueue(g_device, g_graphicsQueueFamily, 0, &g_graphicsQueue);
    vkGetDeviceQueue(g_device, g_presentQueueFamily, 0, &g_presentQueue);
    LOGI("Logical device created");
}

void createSwapChain(GLFWwindow* window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    if (width <= 0 || height <= 0) {
        LOGW("Invalid framebuffer size: %dx%d", width, height);
        width = 1;
        height = 1;
    }
    
    g_swapChainExtent = (VkExtent2D){ (uint32_t)width, (uint32_t)height };
    
    // Query surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(g_physicalDevice, g_surface, &formatCount, NULL);
    VkSurfaceFormatKHR* formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(g_physicalDevice, g_surface, &formatCount, formats);
    
    // Prefer SRGB format
    g_swapChainImageFormat = formats[0].format;
    VkColorSpaceKHR colorSpace = formats[0].colorSpace;
    for (uint32_t i = 0; i < formatCount; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && 
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            g_swapChainImageFormat = formats[i].format;
            colorSpace = formats[i].colorSpace;
            break;
        }
    }
    free(formats);
    
    // Query surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_physicalDevice, g_surface, &capabilities);
    
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    
    LOGI("Creating swapchain: %ux%u, format=%d, images=%u", 
         width, height, g_swapChainImageFormat, imageCount);

    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = g_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = g_swapChainImageFormat;
    createInfo.imageColorSpace = colorSpace;
    createInfo.imageExtent = g_swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    if (g_graphicsQueueFamily != g_presentQueueFamily) {
        uint32_t queueFamilyIndices[] = {g_graphicsQueueFamily, g_presentQueueFamily};
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(g_device, &createInfo, NULL, &g_swapChain) != VK_SUCCESS) {
        LOGE("Failed to create swap chain");
        exit(1);
    }

    vkGetSwapchainImagesKHR(g_device, g_swapChain, &g_swapChainImageCount, NULL);
    g_swapChainImages = (VkImage*)malloc(g_swapChainImageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(g_device, g_swapChain, &g_swapChainImageCount, g_swapChainImages);
    LOGI("Swapchain created with %u images", g_swapChainImageCount);
}

void createImageViews() {
    g_swapChainImageViews = (VkImageView*)malloc(g_swapChainImageCount * sizeof(VkImageView));
    for (uint32_t i = 0; i < g_swapChainImageCount; i++) {
        VkImageViewCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = g_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = g_swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(g_device, &createInfo, NULL, &g_swapChainImageViews[i]) != VK_SUCCESS) {
            LOGE("Failed to create image view %u", i);
            exit(1);
        }
    }
    LOGI("Image views created");
}

void createRenderPass() {
    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = g_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(g_device, &renderPassInfo, NULL, &g_renderPass) != VK_SUCCESS) {
        LOGE("Failed to create render pass!");
        exit(1);
    }
    LOGI("Render pass created");
}

void createGraphicsPipeline() {
    LOGI("Creating shader modules...");
    VkShaderModule vertModule = createShaderModule(vertShaderCode, sizeof(vertShaderCode));
    if (vertModule == VK_NULL_HANDLE) {
        LOGE("Failed to create vertex shader module!");
        exit(1);
    }
    LOGI("Vertex shader module created");
    
    VkShaderModule fragModule = createShaderModule(fragShaderCode, sizeof(fragShaderCode));
    if (fragModule == VK_NULL_HANDLE) {
        LOGE("Failed to create fragment shader module!");
        vkDestroyShaderModule(g_device, vertModule, NULL);
        exit(1);
    }
    LOGI("Fragment shader module created");

    VkPipelineShaderStageCreateInfo vertStageInfo = {0};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertModule;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo = {0};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragModule;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)g_swapChainExtent.width;
    viewport.height = (float)g_swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent = g_swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;  // Changed from BACK_BIT for testing
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;  // Changed from CLOCKWISE
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {0};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPushConstantRange pushConstantRange = {0};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    VkResult result = vkCreatePipelineLayout(g_device, &pipelineLayoutInfo, NULL, &g_pipelineLayout);
    if (result != VK_SUCCESS) {
        LOGE("Failed to create pipeline layout! Error code: %d", result);
        exit(1);
    }
    LOGI("Pipeline layout created");

    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = NULL;
    pipelineInfo.pDynamicState = NULL;
    pipelineInfo.layout = g_pipelineLayout;
    pipelineInfo.renderPass = g_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    LOGI("Creating graphics pipeline...");
    pipelineInfo.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;  // Try without optimization
    result = vkCreateGraphicsPipelines(g_device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &g_graphicsPipeline);
    if (result != VK_SUCCESS) {
        LOGE("Failed to create graphics pipeline! Error code: %d", result);
        
        // Try to get more info about the failure
        if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
            LOGE("  -> Out of host memory");
        } else if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            LOGE("  -> Out of device memory");
        } else if (result == VK_ERROR_INVALID_SHADER_NV) {
            LOGE("  -> Invalid shader (possibly SPIR-V issue)");
        }
        
        // Don't exit, clean up shaders first
        vkDestroyShaderModule(g_device, fragModule, NULL);
        vkDestroyShaderModule(g_device, vertModule, NULL);
        exit(1);
    }

    vkDestroyShaderModule(g_device, fragModule, NULL);
    vkDestroyShaderModule(g_device, vertModule, NULL);
    LOGI("Graphics pipeline created successfully");
    LOGI("Graphics pipeline created");
}

void createFramebuffers() {
    g_framebuffers = malloc(g_swapChainImageCount * sizeof(VkFramebuffer));
    for (size_t i = 0; i < g_swapChainImageCount; i++) {
        VkImageView attachments[] = { g_swapChainImageViews[i] };
        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = g_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = g_swapChainExtent.width;
        framebufferInfo.height = g_swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(g_device, &framebufferInfo, NULL, &g_framebuffers[i]) != VK_SUCCESS) {
            LOGE("Failed to create framebuffer %zu!", i);
            exit(1);
        }
    }
    LOGI("Framebuffers created");
}

void createCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = g_graphicsQueueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(g_device, &poolInfo, NULL, &g_commandPool) != VK_SUCCESS) {
        LOGE("Failed to create command pool!");
        exit(1);
    }
    LOGI("Command pool created");
}

void createCommandBuffers() {
    g_commandBuffers = malloc(g_swapChainImageCount * sizeof(VkCommandBuffer));
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = g_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = g_swapChainImageCount;

    if (vkAllocateCommandBuffers(g_device, &allocInfo, g_commandBuffers) != VK_SUCCESS) {
        LOGE("Failed to allocate command buffers!");
        exit(1);
    }
    LOGI("Command buffers allocated");
}

void recordCommandBuffer(uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(g_commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        LOGE("Failed to begin recording command buffer!");
        return;
    }

    VkRenderPassBeginInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = g_renderPass;
    renderPassInfo.framebuffer = g_framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderPassInfo.renderArea.extent = g_swapChainExtent;
    VkClearValue clearColor = {{0.05f, 0.05f, 0.07f, 1.0f}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(g_commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(g_commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, g_graphicsPipeline);
    vkCmdPushConstants(g_commandBuffers[imageIndex], g_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 
                       0, sizeof(PushConstants), &g_pushConsts);
    vkCmdDraw(g_commandBuffers[imageIndex], 3, 1, 0, 0);
    vkCmdEndRenderPass(g_commandBuffers[imageIndex]);

    if (vkEndCommandBuffer(g_commandBuffers[imageIndex]) != VK_SUCCESS) {
        LOGE("Failed to record command buffer!");
    }
}

void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(g_device, &semaphoreInfo, NULL, &g_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(g_device, &semaphoreInfo, NULL, &g_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(g_device, &fenceInfo, NULL, &g_inFlightFences[i]) != VK_SUCCESS) {
            LOGE("Failed to create sync objects for frame %d!", i);
            exit(1);
        }
    }
    LOGI("Sync objects created");
}

void initVulkan(GLFWwindow* window) {
    createInstance();
    if (glfwCreateWindowSurface(g_instance, window, NULL, &g_surface) != VK_SUCCESS) {
        LOGE("Failed to create window surface");
        exit(1);
    }
    LOGI("Window surface created");
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain(window);
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
    LOGI("Vulkan initialization complete.");
}

void drawFrame() {
    vkWaitForFences(g_device, 1, &g_inFlightFences[g_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(g_device, g_swapChain, UINT64_MAX, 
                                            g_imageAvailableSemaphores[g_currentFrame], 
                                            VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        g_framebufferResized = 1;
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOGE("Failed to acquire swap chain image! Result: %d", result);
        return;
    }
    
    vkResetFences(g_device, 1, &g_inFlightFences[g_currentFrame]);
    
    vkResetCommandBuffer(g_commandBuffers[imageIndex], 0);
    recordCommandBuffer(imageIndex);

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {g_imageAvailableSemaphores[g_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &g_commandBuffers[imageIndex];
    VkSemaphore signalSemaphores[] = {g_renderFinishedSemaphores[g_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(g_graphicsQueue, 1, &submitInfo, g_inFlightFences[g_currentFrame]) != VK_SUCCESS) {
        LOGE("Failed to submit draw command buffer!");
        return;
    }

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {g_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(g_presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        g_framebufferResized = 1;
    } else if (result != VK_SUCCESS) {
        LOGE("Failed to present swap chain image! Result: %d", result);
    }

    g_currentFrame = (g_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void mainLoop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (g_framebufferResized) {
            g_framebufferResized = 0;
            recreateSwapChain(window);
        }
        drawFrame();
    }
    vkDeviceWaitIdle(g_device);
}

void cleanupSwapChain() {
    for (uint32_t i = 0; i < g_swapChainImageCount; i++) {
        vkDestroyFramebuffer(g_device, g_framebuffers[i], NULL);
    }
    free(g_framebuffers);
    g_framebuffers = NULL;
    
    // Free command buffers
    vkFreeCommandBuffers(g_device, g_commandPool, g_swapChainImageCount, g_commandBuffers);
    free(g_commandBuffers);
    g_commandBuffers = NULL;
    
    for (uint32_t i = 0; i < g_swapChainImageCount; i++) {
        vkDestroyImageView(g_device, g_swapChainImageViews[i], NULL);
    }
    free(g_swapChainImageViews);
    g_swapChainImageViews = NULL;
    
    free(g_swapChainImages);
    g_swapChainImages = NULL;
    
    vkDestroySwapchainKHR(g_device, g_swapChain, NULL);
}

void cleanup() {
    LOGI("Cleaning up...");
    cleanupSwapChain();
    
    vkDestroyPipeline(g_device, g_graphicsPipeline, NULL);
    vkDestroyPipelineLayout(g_device, g_pipelineLayout, NULL);
    vkDestroyRenderPass(g_device, g_renderPass, NULL);
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(g_device, g_renderFinishedSemaphores[i], NULL);
        vkDestroySemaphore(g_device, g_imageAvailableSemaphores[i], NULL);
        vkDestroyFence(g_device, g_inFlightFences[i], NULL);
    }
    
    vkDestroyCommandPool(g_device, g_commandPool, NULL);
    vkDestroyDevice(g_device, NULL);
    vkDestroySurfaceKHR(g_instance, g_surface, NULL);
    vkDestroyInstance(g_instance, NULL);
    LOGI("Cleanup complete");
}

void recreateSwapChain(GLFWwindow* window) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    
    LOGI("Recreating swapchain: %dx%d", width, height);
    vkDeviceWaitIdle(g_device);
    cleanupSwapChain();

    createSwapChain(window);
    createImageViews();
    createFramebuffers();
    createCommandBuffers();
}

VkShaderModule createShaderModule(const uint32_t* code, size_t size) {
    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = code;

    LOGI("Creating shader module, size: %zu bytes", size);
    
    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(g_device, &createInfo, NULL, &shaderModule);
    if (result != VK_SUCCESS) {
        LOGE("Failed to create shader module! Error code: %d", result);
        LOGE("  Code size: %zu bytes", size);
        LOGE("  Code pointer: %p", (void*)code);
        return VK_NULL_HANDLE;
    }
    LOGI("Shader module created successfully");
    return shaderModule;
}
