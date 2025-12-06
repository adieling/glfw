# Android Emulator mit KVM - Setup & Verwendung

## Übersicht
Dieser Guide zeigt, wie man einen Android Emulator mit Hardware-Beschleunigung (KVM) auf Linux startet, um Vulkan-Apps zu testen.

## Voraussetzungen prüfen

### 1. CPU-Virtualisierung überprüfen
```bash
grep -E '(vmx|svm)' /proc/cpuinfo | wc -l
```
Wenn das Ergebnis > 0 ist, unterstützt deine CPU Virtualisierung ✅

### 2. KVM-Device prüfen
```bash
ls -l /dev/kvm
```
Output sollte sein:
```
crw-rw-rw- 1 root kvm 10, 232 ...  /dev/kvm
```
Wenn sichtbar, ist KVM installiert ✅

## Emulator starten

### Mit KVM-Beschleunigung (empfohlen für Vulkan)
```bash
emulator -avd Pixel_8 -gpu host -accel on &
```

**Flags erklären:**
- `-avd Pixel_8`: Name des Android Virtual Device
- `-gpu host`: Nutzt Host-GPU für Rendering
- `-accel on`: Hardware-Beschleunigung via KVM

### Alternative: Ohne KVM (Software-Rendering, langsam)
```bash
emulator -avd Pixel_8 -gpu swiftshader_indirect &
```
⚠️ **Warnung:** Funktioniert NICHT zuverlässig für Vulkan-Apps!

## Emulator Boot-Status checken

Warte bis zum vollständigen Boot (ca. 30-60 Sekunden):

```bash
# Geräte auflisten
adb devices

# Output sollte zeigen:
# emulator-5554   device
```

## APK installieren und starten

```bash
# APK auf Emulator installieren
adb -s emulator-5554 install -r /pfad/zur/app-debug.apk

# App starten
adb -s emulator-5554 shell am start -n org.glfw.example/android.app.NativeActivity

# Screenshot machen
adb -s emulator-5554 shell screencap -p /sdcard/screen.png
adb -s emulator-5554 pull /sdcard/screen.png /tmp/screen.png
```

## Emulator stoppen

```bash
adb -s emulator-5554 emu kill
```

## Troubleshooting

### Emulator startet nicht
- Stelle sicher, dass `/dev/kvm` existiert und lesbar ist
- Prüfe, dass keine anderen Emulatoren laufen: `adb devices`

### Vulkan funktioniert nicht
- Nutze `-gpu host -accel on` statt `-gpu swiftshader_indirect`
- Prüfe mit: `adb shell getprop ro.hardware.vulkan`

### Langsamer Emulator
- KVM muss aktiv sein (nicht nur aktivierbar)
- Prüfe mit: `adb shell cat /proc/cpuinfo | grep "processor"`

## Build & Test Workflow

```bash
# 1. Bauen
cd /home/adieling/code/glfw_android/android_app
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk
./gradlew clean assembleDebug

# 2. Emulator starten (im Hintergrund)
emulator -avd Pixel_8 -gpu host -accel on &

# 3. Warten auf Boot
sleep 60

# 4. Installieren & Testen
adb -s emulator-5554 install -r app/build/outputs/apk/debug/app-debug.apk
adb -s emulator-5554 shell am start -n org.glfw.example/android.app.NativeActivity
```

## Performance Vergleich

| Methode | Boot-Zeit | Stabilität | Vulkan |
|---------|-----------|-----------|--------|
| KVM (-gpu host -accel on) | 30-60s | ✅ Stabil | ✅ Funktioniert |
| Software (-gpu swiftshader_indirect) | 60-120s | ❌ Crashes | ❌ Fehler |

## Wichtige Paths

- **Emulator Binary:** `/home/adieling/Android/Sdk/emulator/emulator`
- **AVD Config:** `~/.android/avd/Pixel_8.avd/`
- **APK Output:** `app/build/outputs/apk/debug/app-debug.apk`
- **Java Home:** `/usr/lib/jvm/java-17-openjdk`

## Weitere Infos

- [Android Emulator Dokumentation](https://developer.android.com/studio/run/emulator)
- [KVM für Android Emulator](https://developer.android.com/studio/run/emulator-acceleration)
