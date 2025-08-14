Android example app for GLFW triangle

How to build (from repo root):

- Ensure ANDROID_SDK_ROOT and ANDROID_NDK are set (or install via Android Studio).
- Build native libs and APK via Gradle:

  cd android_app
  ./gradlew assembleDebug

This app uses a NativeActivity and loads libandroid_triangle.so, which is built from examples/android_triangle.c via the top-level CMakeLists through externalNativeBuild.

Supported ABIs: arm64-v8a, x86_64.
