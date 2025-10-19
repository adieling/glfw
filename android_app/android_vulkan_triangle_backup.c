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
} PushConstants;

static PushConstants g_pushConsts = {0.0f, 0.0f, 0};

// --- Helper Declarations ---
void initVulkan(GLFWwindow* window);
void mainLoop(GLFWwindow* window);
void cleanup();
void cleanupSwapChain();
void recreateSwapChain(GLFWwindow* window);
VkShaderModule createShaderModule(const uint32_t* code, size_t size);
uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

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
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        g_pushConsts.pressed = (action == GLFW_PRESS);
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    g_framebufferResized = 1;
}

// --- SPIR-V Shaders (embedded) ---
// Vertex shader with push constants and gl_VertexIndex
const uint32_t vertShaderCode[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x0000001f, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000d, 0x00040010,
    0x00000000, 0x00000000, 0x00000009, 0x00050011, 0x00000000, 0x00000001, 0x0000000b, 0x00000000,
    0x00030010, 0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004,
    0x6e69616d, 0x00000000, 0x00050005, 0x00000009, 0x736f5067, 0x6974696f, 0x0000006e, 0x00060005,
    0x0000000d, 0x6e69616d, 0x5f74756f, 0x00000030, 0x00040006, 0x0000000d, 0x00000000, 0x72615676,
    0x00000073, 0x00040006, 0x0000000d, 0x00000001, 0x6f6c6f43, 0x00000072, 0x00050005, 0x00000019,
    0x43687375, 0x74736e6f, 0x73746e61, 0x00000000, 0x00040047, 0x00000009, 0x0000001e, 0x00000000,
    0x00040047, 0x0000000d, 0x0000001e, 0x00000000, 0x00040047, 0x00000019, 0x00000000, 0x00000030,
    0x00040047, 0x00000019, 0x00000001, 0x00000004, 0x00040047, 0x00000019, 0x00000002, 0x00000008,
    0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00040020, 0x00000004, 0x00000003,
    0x00000000, 0x00040020, 0x00000005, 0x00000001, 0x00000004, 0x0004002b, 0x00000004, 0x00000006,
    0x00000001, 0x00040020, 0x00000007, 0x00000003, 0x00000005, 0x0004002b, 0x00000005, 0x00000008,
    0x00000000, 0x00040020, 0x00000009, 0x00000000, 0x0000000d, 0x0005002c, 0x00000002, 0x0000000a,
    0x00000008, 0x00000006, 0x00040020, 0x0000000b, 0x00000002, 0x0000000a, 0x0004003b, 0x0000000b,
    0x0000000d, 0x00000002, 0x0003001e, 0x0000000e, 0x0000000d, 0x00040020, 0x0000000f, 0x00000002,
    0x0000000e, 0x0004002b, 0x0000000e, 0x00000010, 0x00000000, 0x00040020, 0x00000011, 0x00000002,
    0x0000000e, 0x0004002b, 0x0000000e, 0x00000012, 0x00000001, 0x00040020, 0x00000013, 0x00000004,
    0x0000000f, 0x00040020, 0x00000019, 0x00000009, 0x00000013, 0x00050036, 0x00000002, 0x00000004,
    0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x0000000a, 0x0000000c, 0x0000000d,
    0x00050041, 0x0000000f, 0x00000011, 0x0000000c, 0x00000010, 0x0004003d, 0x0000000a, 0x00000015,
    0x0000000d, 0x00050051, 0x00000004, 0x00000015, 0x00000016, 0x00000003, 0x00050041, 0x00000011,
    0x00000017, 0x0000000c, 0x00000012, 0x00050085, 0x0000000a, 0x00000018, 0x00000015, 0x00000017,
    0x00050041, 0x00000011, 0x0000001a, 0x0000000c, 0x00000008, 0x00050085, 0x0000000a, 0x0000001b,
    0x00000015, 0x0000001a, 0x0005006c, 0x0000000a, 0x0000001c, 0x0000001b, 0x00000018, 0x00050041,
    0x0000000f, 0x0000001d, 0x0000000c, 0x00000016, 0x0003003e, 0x00000009, 0x0000001d, 0x000100fd,
    0x00010038
};

// Fragment shader
const uint32_t fragShaderCode[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x0000000e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000007, 0x0000000a, 0x00030010,
    0x00000000, 0x00000001, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00050005, 0x00000007,
    0x6f6c6f43, 0x61566e69, 0x00000072, 0x00050005, 0x0000000a, 0x67617246, 0x6f6c6f43, 0x000072,
    0x00040047, 0x00000007, 0x0000001e, 0x00000000, 0x00040047, 0x0000000a, 0x0000001e, 0x00000000,
    0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00040020, 0x00000004, 0x00000003,
    0x00000000, 0x00040020, 0x00000005, 0x00000001, 0x00000004, 0x0004003b, 0x00000005, 0x00000007,
    0x00000001, 0x00040020, 0x00000008, 0x00000004, 0x00000004, 0x0004002b, 0x00000004, 0x00000009,
    0x3f800000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005,
    0x0004003d, 0x00000001, 0x0000000c, 0x00000007, 0x00050051, 0x00000004, 0x0000000c, 0x0000000d,
    0x00000003, 0x0003003e, 0x0000000a, 0x0000000d, 0x000100fd, 0x00010038
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

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, NULL, &g_instance) != VK_SUCCESS) {
        LOGE("Failed to create Vulkan instance");
        exit(1);
    }
}

void pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(g_instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        LOGE("Failed to find GPUs with Vulkan support");
        exit(1);
    }
    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(g_instance, &deviceCount, devices);
    g_physicalDevice = devices[0]; // Simplification
}

void createLogicalDevice() {
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = 0; // Simplification
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};
    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(g_physicalDevice, &createInfo, NULL, &g_device) != VK_SUCCESS) {
        LOGE("Failed to create logical device");
        exit(1);
    }
    vkGetDeviceQueue(g_device, 0, 0, &g_graphicsQueue);
    g_presentQueue = g_graphicsQueue; // Simplification
}

void createSwapChain(GLFWwindow* window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    g_swapChainExtent = (VkExtent2D){ (uint32_t)width, (uint32_t)height };
    g_swapChainImageFormat = VK_FORMAT_B8G8R8A8_UNORM; // Simplification

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = g_surface;
    createInfo.minImageCount = MAX_FRAMES_IN_FLIGHT;
    createInfo.imageFormat = g_swapChainImageFormat;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = g_swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // vsync
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(g_device, &createInfo, NULL, &g_swapChain) != VK_SUCCESS) {
        LOGE("Failed to create swap chain");
        exit(1);
    }

    vkGetSwapchainImagesKHR(g_device, g_swapChain, &g_swapChainImageCount, NULL);
    g_swapChainImages = (VkImage*)malloc(g_swapChainImageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(g_device, g_swapChain, &g_swapChainImageCount, g_swapChainImages);
}

void createImageViews() {
    g_swapChainImageViews = (VkImageView*)malloc(g_swapChainImageCount * sizeof(VkImageView));
    for (uint32_t i = 0; i < g_swapChainImageCount; i++) {
        VkImageViewCreateInfo createInfo = {};
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
            LOGE("Failed to create image view %d", i);
            exit(1);
        }
    }
}

void createRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = g_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(g_device, &renderPassInfo, NULL, &g_renderPass) != VK_SUCCESS) {
        LOGE("failed to create render pass!");
        exit(1);
    }
}

void createGraphicsPipeline() {
    VkShaderModule vertModule = createShaderModule(vertShaderCode, sizeof(vertShaderCode));
    VkShaderModule fragModule = createShaderModule(fragShaderCode, sizeof(fragShaderCode));

    VkPipelineShaderStageCreateInfo vertStageInfo = {};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertModule;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo = {};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragModule;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)g_swapChainExtent.width;
    viewport.height = (float)g_swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent = g_swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(g_device, &pipelineLayoutInfo, NULL, &g_pipelineLayout) != VK_SUCCESS) {
        LOGE("failed to create pipeline layout!");
        exit(1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = g_pipelineLayout;
    pipelineInfo.renderPass = g_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(g_device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &g_graphicsPipeline) != VK_SUCCESS) {
        LOGE("failed to create graphics pipeline!");
        exit(1);
    }

    vkDestroyShaderModule(g_device, fragModule, NULL);
    vkDestroyShaderModule(g_device, vertModule, NULL);
}

void createFramebuffers() {
    g_framebuffers = malloc(g_swapChainImageCount * sizeof(VkFramebuffer));
    for (size_t i = 0; i < g_swapChainImageCount; i++) {
        VkImageView attachments[] = { g_swapChainImageViews[i] };
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = g_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = g_swapChainExtent.width;
        framebufferInfo.height = g_swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(g_device, &framebufferInfo, NULL, &g_framebuffers[i]) != VK_SUCCESS) {
            LOGE("failed to create framebuffer %zu!", i);
            exit(1);
        }
    }
}

void createCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = 0; // Simplification
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(g_device, &poolInfo, NULL, &g_commandPool) != VK_SUCCESS) {
        LOGE("failed to create command pool!");
        exit(1);
    }
}

void createCommandBuffers() {
    g_commandBuffers = malloc(g_swapChainImageCount * sizeof(VkCommandBuffer));
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = g_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = g_swapChainImageCount;

    if (vkAllocateCommandBuffers(g_device, &allocInfo, g_commandBuffers) != VK_SUCCESS) {
        LOGE("failed to allocate command buffers!");
        exit(1);
    }
}

void recordCommandBuffer(uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(g_commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        LOGE("failed to begin recording command buffer!");
        exit(1);
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = g_renderPass;
    renderPassInfo.framebuffer = g_framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderPassInfo.renderArea.extent = g_swapChainExtent;
    VkClearValue clearColor = {0.05f, 0.05f, 0.07f, 1.0f};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(g_commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(g_commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, g_graphicsPipeline);
    vkCmdPushConstants(g_commandBuffers[imageIndex], g_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &g_pushConsts);
    vkCmdDraw(g_commandBuffers[imageIndex], 3, 1, 0, 0);
    vkCmdEndRenderPass(g_commandBuffers[imageIndex]);

    if (vkEndCommandBuffer(g_commandBuffers[imageIndex]) != VK_SUCCESS) {
        LOGE("failed to record command buffer!");
        exit(1);
    }
}

void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(g_device, &semaphoreInfo, NULL, &g_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(g_device, &semaphoreInfo, NULL, &g_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(g_device, &fenceInfo, NULL, &g_inFlightFences[i]) != VK_SUCCESS) {
            LOGE("failed to create sync objects for a frame!");
            exit(1);
        }
    }
}


void initVulkan(GLFWwindow* window) {
    createInstance();
    if (glfwCreateWindowSurface(g_instance, window, NULL, &g_surface) != VK_SUCCESS) {
        LOGE("Failed to create window surface");
        exit(1);
    }
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
    VkResult result = vkAcquireNextImageKHR(g_device, g_swapChain, UINT64_MAX, g_imageAvailableSemaphores[g_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        g_framebufferResized = 1;
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOGE("failed to acquire swap chain image!");
        return;
    }
    
    vkResetFences(g_device, 1, &g_inFlightFences[g_currentFrame]);
    
    recordCommandBuffer(imageIndex);

    VkSubmitInfo submitInfo = {};
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
        LOGE("failed to submit draw command buffer!");
        exit(1);
    }

    VkPresentInfoKHR presentInfo = {};
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
        LOGE("failed to present swap chain image!");
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
    for (uint32_t i = 0; i < g_swapChainImageCount; i++) {
        vkDestroyImageView(g_device, g_swapChainImageViews[i], NULL);
    }
    free(g_swapChainImageViews);
    free(g_swapChainImages);
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
}

void recreateSwapChain(GLFWwindow* window) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    
    vkDeviceWaitIdle(g_device);
    cleanupSwapChain();

    createSwapChain(window);
    createImageViews();
    createFramebuffers();
}

VkShaderModule createShaderModule(const uint32_t* code, size_t size) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = code;

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(g_device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        LOGE("Failed to create shader module!");
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}
