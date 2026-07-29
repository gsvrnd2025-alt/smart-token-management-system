# Native App Installation Guide

## ⚠️ Important Notice

The Flutter apps require **Windows Developer Mode** to be enabled for proper symlink support. 

### Enable Developer Mode

1. Press `Win + I` to open Settings
2. Go to **System > For developers**
3. Toggle on **Developer Mode**
4. Wait for installation to complete
5. Restart your computer

---

## Android App Installation

### Option A: Using Android Studio (Recommended)

1. **Install Android Studio**
   - Download from: https://developer.android.com/studio
   - Complete setup wizard
   - Install SDK components when prompted

2. **Build and Run**
   ```bash
   cd dashboard_flutter
   flutter run
   # Select your device/emulator from the list
   ```

3. **Create Release Build**
   ```bash
   flutter build apk --release
   # APK location: build/app/outputs/apk/release/app-release.apk
   ```

### Option B: Using Command Line

1. **Install Android SDK**
   ```bash
   # Download and set ANDROID_SDK_ROOT
   $env:ANDROID_SDK_ROOT = "C:\Android\sdk"
   ```

2. **Build Debug APK**
   ```bash
   cd dashboard_flutter
   flutter build apk
   # Output: build/app/outputs/apk/debug/app-debug.apk
   ```

3. **Build Release APK**
   ```bash
   flutter build apk --release --split-per-abi
   # Outputs:
   #   - app-armeabi-v7a-release.apk
   #   - app-arm64-v8a-release.apk
   #   - app-x86-release.apk
   ```

### Install on Device

```bash
# List devices
flutter devices

# Install APK
adb install build/app/outputs/apk/release/app-release.apk

# Or run directly
flutter run -d <device-id>
```

---

## iOS App Installation

### Prerequisites (Mac Only)
- Xcode 13+
- iOS deployment target: 11.0+
- CocoaPods

### Build Steps

```bash
cd dashboard_flutter

# Install dependencies
flutter pub get

# Build iOS app
flutter build ios --release

# Build IPA (for distribution)
flutter build ipa --release
```

### Install and Run

**Option 1: Xcode**
```bash
open ios/Runner.xcworkspace
# Build and Run from Xcode
```

**Option 2: Direct to Device**
```bash
flutter run -d <device-id>
```

**Option 3: TestFlight/App Store**
- Upload IPA from `build/ios/ipa/`

---

## Web App (No Installation Required)

The web version is the **fastest to deploy**:

```bash
# Option 1: Use existing server
npm run dev
# Open: http://localhost:3000

# Option 2: Build static files
cd dashboard_flutter
flutter build web --release
# Files in: build/web/

# Option 3: Deploy to hosting
# Copy build/web/ contents to:
#   - Firebase Hosting
#   - Netlify
#   - Vercel
#   - GitHub Pages
#   - AWS S3
```

---

## Windows Desktop App

```bash
# Enable Windows platform
flutter config --enable-windows

# Get dependencies
flutter pub get

# Build
flutter build windows --release

# Run
.\build\windows\x64\Release\dashboard_flutter.exe
```

**Distributable Location**: `build/windows/x64/Release/`

---

## Linux Desktop App

```bash
# Install dependencies
sudo apt-get install libgtk-3-dev libglib2.0-dev

# Enable Linux platform
flutter config --enable-linux

# Build
flutter build linux --release

# Run
./build/linux/x64/release/bundle/dashboard_flutter
```

---

## macOS Desktop App

```bash
# Enable macOS platform
flutter config --enable-macos

# Build
flutter build macos --release

# Run
open build/macos/Build/Release/dashboard_flutter.app
```

---

## Troubleshooting

### "Developer Mode Required" Error
**Solution**: Enable Developer Mode (see instructions above)

### "Could not find Android SDK" Error
```bash
flutter config --android-sdk C:\Android\sdk
flutter config --android-studio-path "C:\Program Files\Android\Android Studio"
```

### Build Timeout
```bash
cd dashboard_flutter
flutter clean
flutter pub get
flutter pub cache repair
flutter build apk --release
```

### CocoaPods Issues (iOS)
```bash
cd ios
rm -rf Pods
rm Podfile.lock
cd ..
flutter clean
flutter pub get
flutter build ios --release
```

### gradle.lock Issues
```bash
cd android
rm gradle/wrapper/gradle-wrapper.jar
cd ..
flutter clean
flutter pub get
flutter build apk
```

---

## Performance Tips

1. **Split APKs**: Use `--split-per-abi` to reduce app size
2. **Release Mode**: Always use `--release` for production
3. **Strip Symbols**: Reduces binary size but removes debug info
4. **Obfuscate**: `flutter build apk --release --obfuscate`

---

## Distribution

### Play Store (Android)
1. Create signed APK: `flutter build aab --release`
2. Upload to Google Play Console
3. Set up signing certificates

### App Store (iOS)
1. Build IPA: `flutter build ipa --release`
2. Upload via App Store Connect
3. Complete app review process

### Direct Distribution
- Share APK file
- Share EXE file
- Host on website
- Use Firebase App Distribution

---

## Development vs Release

| Aspect | Development | Release |
|--------|------------|---------|
| Build Time | ~2 min | ~5-10 min |
| App Size | ~50-100MB | ~15-30MB |
| Debug Info | ✅ Included | ❌ Removed |
| Performance | Good | Excellent |
| Distribution | Local only | App stores |

---

## Quick Start Commands

```bash
# Android
flutter build apk --release

# iOS
flutter build ipa --release

# Web
flutter build web --release

# Windows
flutter build windows --release

# Linux
flutter build linux --release

# macOS
flutter build macos --release

# All platforms
flutter build --release
```

---

## Next Steps

1. ✅ Enable Developer Mode
2. ✅ Install platform SDKs
3. ✅ Test build on target platform
4. ✅ Create signed releases
5. ✅ Distribute to users

For detailed logs during build:
```bash
flutter build apk --release -v
```

---

Last Updated: 2026-06-22
