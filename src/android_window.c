//========================================================================
// GLFW 3.3 - www.glfw.org
//------------------------------------------------------------------------
// Copyright (c) 2017 Curi0 <curi0minecraft@gmail.com>
// Copyright (c) 2006-2016 Camilla Löwy <elmindreda@glfw.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================

#include "internal.h"
#include <string.h>
#include <time.h>

static int mapAndroidKeyToGlfw(int akey)
{
    switch (akey)
    {
        case AKEYCODE_BACK: return GLFW_KEY_ESCAPE; // or special-case Back
        case AKEYCODE_ENTER: return GLFW_KEY_ENTER;
        case AKEYCODE_DEL: return GLFW_KEY_BACKSPACE;
        case AKEYCODE_SPACE: return GLFW_KEY_SPACE;
        case AKEYCODE_TAB: return GLFW_KEY_TAB;
        case AKEYCODE_DPAD_LEFT: return GLFW_KEY_LEFT;
        case AKEYCODE_DPAD_RIGHT: return GLFW_KEY_RIGHT;
        case AKEYCODE_DPAD_UP: return GLFW_KEY_UP;
        case AKEYCODE_DPAD_DOWN: return GLFW_KEY_DOWN;
        default: return akey; // fallback: use android keycode as scancode
    }
}

static int32_t handle_input(struct android_app* app, AInputEvent* event)
{
    _GLFWwindow* win = _glfw.windowListHead;

    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
    {
        const int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        const size_t index = (AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        const float px = AMotionEvent_getX(event, index);
        const float py = AMotionEvent_getY(event, index);

        if (win)
        {
            win->android.cursorX = px;
            win->android.cursorY = py;
            _glfwInputCursorPos(win, px, py);

            // Check for tap timeout - execute pending action if no more taps coming
            if (action != AMOTION_EVENT_ACTION_DOWN) {
                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);
                long currentTime = now.tv_sec * 1000 + now.tv_nsec / 1000000;
                const long TAP_TIMEOUT = 400;
                
                if (win->android.tapCount > 0 && currentTime - win->android.lastTapTime > TAP_TIMEOUT) {
                    // Timeout reached - execute pending tap action
                    if (win->android.tapCount == 3) {
                        _glfwInputKey(win, GLFW_KEY_T, 0, GLFW_PRESS, 0);
                        _glfwInputKey(win, GLFW_KEY_T, 0, GLFW_RELEASE, 0);
                    } else if (win->android.tapCount == 4) {
                        _glfwInputKey(win, GLFW_KEY_O, 0, GLFW_PRESS, 0);
                        _glfwInputKey(win, GLFW_KEY_O, 0, GLFW_RELEASE, 0);
                    } else if (win->android.tapCount == 1 || win->android.tapCount == 2) {
                        _glfwInputKey(win, GLFW_KEY_ENTER, 0, GLFW_PRESS, 0);
                        _glfwInputKey(win, GLFW_KEY_ENTER, 0, GLFW_RELEASE, 0);
                    }
                    win->android.tapCount = 0;
                }
            }

            // Touch event handling
            if (index == 0)
            {
                // Update max pointer count for multi-touch gesture detection
                int currentPointerCount = AMotionEvent_getPointerCount(event);
                if (currentPointerCount > win->android.maxPointerCount) {
                    win->android.maxPointerCount = currentPointerCount;
                }

                if (action == AMOTION_EVENT_ACTION_DOWN)
                {
                    // Mouse click for menu interaction
                    _glfwInputMouseClick(win, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
                    
                    // Start gesture tracking
                    win->android.gestureStartX = px;
                    win->android.gestureStartY = py;
                    win->android.gestureTracking = 1;
                    win->android.maxPointerCount = currentPointerCount;
                }
                else if (action == AMOTION_EVENT_ACTION_MOVE)
                {
                    // Continuous mouse position update for smooth menu interaction
                    _glfwInputCursorPos(win, px, py);
                }
                else if (action == AMOTION_EVENT_ACTION_UP)
                {
                    // Mouse release for menu interaction
                    _glfwInputMouseClick(win, GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0);
                    
                    // Detect swipe/tap and emit keyboard events
                    if (win->android.gestureTracking)
                    {
                        float dx = px - win->android.gestureStartX;
                        float dy = py - win->android.gestureStartY;
                        float absDx = dx > 0 ? dx : -dx;
                        float absDy = dy > 0 ? dy : -dy;
                        const float SWIPE_THRESHOLD = 80.0f;
                        const float TAP_THRESHOLD = 30.0f;
                        
                        int pointerCount = win->android.maxPointerCount;

                        // Single finger or no multi-touch specified
                        if (pointerCount == 1 || action == AMOTION_EVENT_ACTION_UP)
                        {
                            // Detect if it's a swipe or tap
                            if (absDx > SWIPE_THRESHOLD || absDy > SWIPE_THRESHOLD)
                            {
                                // Swipe gesture
                                if (absDx > absDy)
                                {
                                    // Horizontal swipe
                                    if (dx > 0)
                                    {
                                        // Swipe Right -> Right Arrow (or D with ALT for gameplay)
                                        _glfwInputKey(win, GLFW_KEY_RIGHT, 0, GLFW_PRESS, 0);
                                        _glfwInputKey(win, GLFW_KEY_RIGHT, 0, GLFW_RELEASE, 0);
                                    }
                                    else
                                    {
                                        // Swipe Left -> Left Arrow (or A with ALT for gameplay)
                                        _glfwInputKey(win, GLFW_KEY_LEFT, 0, GLFW_PRESS, 0);
                                        _glfwInputKey(win, GLFW_KEY_LEFT, 0, GLFW_RELEASE, 0);
                                    }
                                }
                                else
                                {
                                    // Vertical swipe
                                    if (dy > 0)
                                    {
                                        // Swipe Down -> Down Arrow (or S with ALT for gameplay)
                                        _glfwInputKey(win, GLFW_KEY_DOWN, 0, GLFW_PRESS, 0);
                                        _glfwInputKey(win, GLFW_KEY_DOWN, 0, GLFW_RELEASE, 0);
                                    }
                                    else
                                    {
                                        // Swipe Up -> Up Arrow (or W with ALT for gameplay)
                                        _glfwInputKey(win, GLFW_KEY_UP, 0, GLFW_PRESS, 0);
                                        _glfwInputKey(win, GLFW_KEY_UP, 0, GLFW_RELEASE, 0);
                                    }
                                }
                            }
                            else if (absDx < TAP_THRESHOLD && absDy < TAP_THRESHOLD)
                            {
                                // Tap gesture - detect multi-taps for T (3x) and O (4x)
                                struct timespec now;
                                clock_gettime(CLOCK_MONOTONIC, &now);
                                long currentTime = now.tv_sec * 1000 + now.tv_nsec / 1000000;
                                const long TAP_TIMEOUT = 400; // 400ms window for multi-tap
                                
                                // Check if this is a new tap or continuation of previous taps
                                if (currentTime - win->android.lastTapTime > TAP_TIMEOUT) {
                                    // New tap sequence (or timeout, execute previous action)
                                    if (win->android.tapCount == 3) {
                                        // Execute T toggle (from previous 3-tap)
                                        _glfwInputKey(win, GLFW_KEY_T, 0, GLFW_PRESS, 0);
                                        _glfwInputKey(win, GLFW_KEY_T, 0, GLFW_RELEASE, 0);
                                    } else if (win->android.tapCount == 4) {
                                        // Execute O toggle (from previous 4-tap)
                                        _glfwInputKey(win, GLFW_KEY_O, 0, GLFW_PRESS, 0);
                                        _glfwInputKey(win, GLFW_KEY_O, 0, GLFW_RELEASE, 0);
                                    } else if (win->android.tapCount == 1) {
                                        // Execute Enter (from previous single tap)
                                        _glfwInputKey(win, GLFW_KEY_ENTER, 0, GLFW_PRESS, 0);
                                        _glfwInputKey(win, GLFW_KEY_ENTER, 0, GLFW_RELEASE, 0);
                                    }
                                    // Start new sequence
                                    win->android.tapCount = 1;
                                } else {
                                    // Continuation of tap sequence
                                    win->android.tapCount++;
                                }
                                
                                win->android.lastTapTime = currentTime;
                            }
                        }
                        else if (pointerCount == 2)
                        {
                            // Two finger swipe for Q/E (up/down camera)
                            if (absDx > SWIPE_THRESHOLD && absDx > absDy)
                            {
                                if (dx > 0)
                                {
                                    // Swipe Right -> E (ALT+E)
                                    _glfwInputKey(win, GLFW_KEY_E, 0, GLFW_PRESS, GLFW_MOD_ALT);
                                    _glfwInputKey(win, GLFW_KEY_E, 0, GLFW_RELEASE, GLFW_MOD_ALT);
                                }
                                else
                                {
                                    // Swipe Left -> Q (ALT+Q)
                                    _glfwInputKey(win, GLFW_KEY_Q, 0, GLFW_PRESS, GLFW_MOD_ALT);
                                    _glfwInputKey(win, GLFW_KEY_Q, 0, GLFW_RELEASE, GLFW_MOD_ALT);
                                }
                            }
                        }
                        
                        win->android.gestureTracking = 0;
                    }
                }
            }
        }
        return 1;
    }
    else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
    {
        const int aaction = AKeyEvent_getAction(event);
        int action = (aaction == AKEY_EVENT_ACTION_DOWN) ? GLFW_PRESS :
                     (aaction == AKEY_EVENT_ACTION_UP)   ? GLFW_RELEASE : GLFW_REPEAT;
        const int akey = AKeyEvent_getKeyCode(event);
        const int key = mapAndroidKeyToGlfw(akey);
        if (win)
            _glfwInputKey(win, key, akey, action, 0);
        return 1;
    }

    return 0;
}

static void handleEvents(int timeout)
{
    int ident;
    do
    {
        ident = ALooper_pollOnce(timeout, NULL, NULL, (void**) &_glfw.gstate.source);
        if (_glfw.gstate.source)
            _glfw.gstate.source->process(_glfw.gstate.app, _glfw.gstate.source);
        // After the first poll, do not block again in this call
        timeout = 0;
    }
    while (ident >= 0);
}

//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

int _glfwPlatformCreateWindow(_GLFWwindow* window,
                              const _GLFWwndconfig* wndconfig,
                              const _GLFWctxconfig* ctxconfig,
                              const _GLFWfbconfig* fbconfig)
{
    // wait for window to become ready
    while (_glfw.gstate.app->window == NULL) {
        handleEvents(-1);
    }
    // Attach per-window Android state
    window->android.app = _glfw.gstate.app;
    window->android.nativeWindow = _glfw.gstate.app->window;
    window->android.cursorX = 0.0;
    window->android.cursorY = 0.0;
    window->android.tapCount = 0;
    window->android.lastTapTime = 0;
    _glfw.gstate.app->onInputEvent = handle_input;

    //ANativeWindow_setBuffersGeometry(window->android->window, wndconfig->width, wndconfig->height, 0);

    if (ctxconfig->client != GLFW_NO_API)
    {
        if ((ctxconfig->source == GLFW_NATIVE_CONTEXT_API) |
            (ctxconfig->source == GLFW_EGL_CONTEXT_API))
        {
            if (!_glfwInitEGL())
                return GLFW_FALSE;
            if (!_glfwCreateContextEGL(window, ctxconfig, fbconfig))
                return GLFW_FALSE;
        }
        else if (ctxconfig->source == GLFW_OSMESA_CONTEXT_API)
        {
            if (!_glfwInitOSMesa())
                return GLFW_FALSE;
            if (!_glfwCreateContextOSMesa(window, ctxconfig, fbconfig))
                return GLFW_FALSE;
        }

    }
    return GLFW_TRUE;
}


void _glfwPlatformDestroyWindow(_GLFWwindow* window)
{
    if (window->context.destroy)
        window->context.destroy(window);
    // Do not force-finish the Activity here; let the app control its lifecycle
    window->android.nativeWindow = NULL;
    window->android.app = NULL;
}

void _glfwPlatformSetWindowTitle(_GLFWwindow* window, const char* title)
{
}

void _glfwPlatformSetWindowIcon(_GLFWwindow* window, int count,
                                const GLFWimage* images)
{
}

void _glfwPlatformSetWindowMonitor(_GLFWwindow* window,
                                   _GLFWmonitor* monitor,
                                   int xpos, int ypos,
                                   int width, int height,
                                   int refreshRate)
{
}

void _glfwPlatformGetWindowPos(_GLFWwindow* window, int* xpos, int* ypos)
{

}

void _glfwPlatformSetWindowPos(_GLFWwindow* window, int xpos, int ypos)
{
}

void _glfwPlatformGetWindowSize(_GLFWwindow* window, int* width, int* height)
{
    if (height)
        *height = ANativeWindow_getHeight(window->android.nativeWindow);
    if (width)
        *width = ANativeWindow_getWidth(window->android.nativeWindow);
}

void _glfwPlatformSetWindowSize(_GLFWwindow* window, int width, int height)
{
}

void _glfwPlatformSetWindowSizeLimits(_GLFWwindow* window,
                                      int minwidth, int minheight,
                                      int maxwidth, int maxheight)
{

}

void _glfwPlatformSetWindowAspectRatio(_GLFWwindow* window, int n, int d)
{
}

void _glfwPlatformGetFramebufferSize(_GLFWwindow* window, int* width, int* height)
{
    // the underlying buffergeometry is currently being initialized from the
    // window width and height...so high resolution displays are currently
    // not supported...so it is safe to just call GetWindowSize() for now
    _glfwPlatformGetWindowSize(window, width, height);
}

void _glfwPlatformGetWindowFrameSize(_GLFWwindow* window,
                                     int* left, int* top,
                                     int* right, int* bottom)
{
}

void _glfwPlatformGetWindowContentScale(_GLFWwindow* window, float* xscale, float* yscale)
{
    if (xscale) *xscale = 1.f;
    if (yscale) *yscale = 1.f;
}

void _glfwPlatformIconifyWindow(_GLFWwindow* window)
{
}

void _glfwPlatformRestoreWindow(_GLFWwindow* window)
{
}

void _glfwPlatformMaximizeWindow(_GLFWwindow* window)
{
}

int _glfwPlatformWindowMaximized(_GLFWwindow* window)
{
    return GLFW_FALSE;
}

void _glfwPlatformSetWindowResizable(_GLFWwindow* window, GLFWbool enabled)
{
}

void _glfwPlatformSetWindowDecorated(_GLFWwindow* window, GLFWbool enabled)
{
}

void _glfwPlatformSetWindowFloating(_GLFWwindow* window, GLFWbool enabled)
{
}

void _glfwPlatformShowWindow(_GLFWwindow* window)
{
}


void _glfwPlatformRequestWindowAttention(_GLFWwindow* window)
{
}

void _glfwPlatformUnhideWindow(_GLFWwindow* window)
{
}

void _glfwPlatformHideWindow(_GLFWwindow* window)
{
}

void _glfwPlatformFocusWindow(_GLFWwindow* window)
{
}

int _glfwPlatformWindowFocused(_GLFWwindow* window)
{
    return GLFW_FALSE;
}

int _glfwPlatformWindowIconified(_GLFWwindow* window)
{
    return GLFW_FALSE;
}

int _glfwPlatformWindowVisible(_GLFWwindow* window)
{
    return GLFW_FALSE;
}

int _glfwPlatformWindowHovered(_GLFWwindow* window)
{
    return GLFW_FALSE;
}

float _glfwPlatformGetWindowOpacity(_GLFWwindow* window)
{
    return 1.f;
}

void _glfwPlatformSetWindowOpacity(_GLFWwindow* window, float opacity)
{
    (void)window; (void)opacity;
}

void _glfwPlatformPollEvents(void)
{
    handleEvents(0);
}

void _glfwPlatformWaitEvents(void)
{
    handleEvents(-1);
}

void _glfwPlatformWaitEventsTimeout(double timeout)
{
    handleEvents(timeout * 1e3);
}

void _glfwPlatformPostEmptyEvent(void)
{
}

void _glfwPlatformGetCursorPos(_GLFWwindow* window, double* xpos, double* ypos)
{
    if (xpos)
        *xpos = window->android.cursorX;
    if (ypos)
        *ypos = window->android.cursorY;
}

void _glfwPlatformSetCursorPos(_GLFWwindow* window, double x, double y)
{
}

void _glfwPlatformSetCursorMode(_GLFWwindow* window, int mode)
{
}

int _glfwPlatformCreateCursor(_GLFWcursor* cursor,
                              const GLFWimage* image,
                              int xhot, int yhot)
{
    return GLFW_TRUE;
}

int _glfwPlatformCreateStandardCursor(_GLFWcursor* cursor, int shape)
{
    return GLFW_TRUE;
}

void _glfwPlatformDestroyCursor(_GLFWcursor* cursor)
{
}

void _glfwPlatformSetCursor(_GLFWwindow* window, _GLFWcursor* cursor)
{
}

void _glfwPlatformSetClipboardString(const char* string)
{
    (void) string; // Clipboard not supported on Android backend yet
}

const char* _glfwPlatformGetClipboardString(void)
{
    return NULL; // Clipboard not supported on Android backend yet
}

const char* _glfwPlatformGetScancodeName(int scancode)
{
    return "";
}

int _glfwPlatformGetKeyScancode(int key)
{
    return -1;
}

void _glfwPlatformGetRequiredInstanceExtensions(char** extensions)
{
    if (!_glfw.vk.KHR_surface || !_glfw.vk.KHR_android_surface)
        return;

    extensions[0] = "VK_KHR_surface";
    extensions[1] = "VK_KHR_android_surface";
}

int _glfwPlatformGetPhysicalDevicePresentationSupport(VkInstance instance,
                                                      VkPhysicalDevice device,
                                                      uint32_t queuefamily)
{
    return GLFW_TRUE;
}

int _glfwPlatformFramebufferTransparent(_GLFWwindow* window)
{
    return GLFW_FALSE;
}

VkResult _glfwPlatformCreateWindowSurface(VkInstance instance,
                                          _GLFWwindow* window,
                                          const VkAllocationCallbacks* allocator,
                                          VkSurfaceKHR* surface)
{
    VkResult err;
    VkAndroidSurfaceCreateInfoKHR sci;
    PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;

    vkCreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)
            vkGetInstanceProcAddr(instance, "vkCreateAndroidSurfaceKHR");
    if (!vkCreateAndroidSurfaceKHR)
    {
        _glfwInputError(GLFW_API_UNAVAILABLE,
                        "Android: Vulkan instance missing VK_KHR_android_surface extension");
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    memset(&sci, 0, sizeof(sci));
    sci.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    sci.window = window->android.nativeWindow;

    err = vkCreateAndroidSurfaceKHR(instance, &sci, allocator, surface);
    if (err)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "Android: Failed to create Vulkan surface: %s",
                        _glfwGetVulkanResultString(err));
    }

    return err;
}

//////////////////////////////////////////////////////////////////////////
//////                        GLFW native API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI struct android_app * glfwGetAndroidApp(GLFWwindow* handle)
{
    _GLFWwindow *window = (_GLFWwindow*)handle;
    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    return window->android.app;
}

GLFWAPI struct ANativeWindow* glfwGetAndroidNativeWindow(GLFWwindow* handle)
{
    _GLFWwindow *window = (_GLFWwindow*)handle;
    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);
    return window->android.nativeWindow;
}
