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

#include <android/log.h>
#include <android/input.h>
#include <android_native_app_glue.h>
#include "internal.h"

struct android_app* _globalApp;

extern int main();
static AInputQueue* s_attachedQueue = NULL;

void handle_cmd(struct android_app* _app, int32_t cmd) {
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        // Window surface is available; EGL/Vulkan surface could be (re)created by client
        if (_app->window)
            ANativeWindow_acquire(_app->window);
        break;
    case APP_CMD_GAINED_FOCUS:
        if (_glfw.windowListHead)
            _glfwInputWindowFocus(_glfw.windowListHead, GLFW_TRUE);
        break;
    case APP_CMD_LOST_FOCUS:
        if (_glfw.windowListHead)
            _glfwInputWindowFocus(_glfw.windowListHead, GLFW_FALSE);
        break;
    case APP_CMD_TERM_WINDOW:
        // Surface is about to be destroyed; release it and notify client
        if (_app->window)
            ANativeWindow_release(_app->window);
        if (_glfw.windowListHead)
            _glfwInputWindowCloseRequest(_glfw.windowListHead);
        break;
    case APP_CMD_INPUT_CHANGED:
        if (s_attachedQueue && s_attachedQueue != _app->inputQueue)
            AInputQueue_detachLooper(s_attachedQueue);
        if (_app->inputQueue)
        {
            AInputQueue_attachLooper(_app->inputQueue, _app->looper, LOOPER_ID_INPUT, NULL, NULL);
            s_attachedQueue = _app->inputQueue;
        }
        else
            s_attachedQueue = NULL;
        break;
    default:
        break;
    }
}

// Android Entry Point
void android_main(struct android_app *app) {
    // Prevent native_app_glue from being stripped by the linker
    void app_dummy();
    app_dummy();

    app->onAppCmd = handle_cmd;
    // hmmm...global....eek
    _globalApp = app;
    main();
}
//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

int _glfwPlatformInit(void)
{
    _glfw.gstate.app = _globalApp;
    _glfwInitTimerPOSIX();
    return GLFW_TRUE;
}

void _glfwPlatformTerminate(void)
{
    _glfwTerminateEGL();
    _glfwTerminateOSMesa();
}

const char* _glfwPlatformGetVersionString(void)
{
    return _GLFW_VERSION_NUMBER " Android EGL";
}

