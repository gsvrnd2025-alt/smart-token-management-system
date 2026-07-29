# Flutter Deployment Guide - Smart Token Management System

## Overview
This guide provides instructions for deploying the Smart Token Management System as a native Flutter app on Android, iOS, and web platforms.

## Current Setup Status

✅ **Completed**:
- Flutter SDK installed at: `C:\Users\jagad\flutter`
- Dart SDK: 3.8.1 (compatible)
- Web server running: http://localhost:3000
- Supabase integration configured
- All system tests passing (8/8)

⚠️ **In Progress**:
- Android build configuration
- Platform setup and dependencies

---

## Quick Start

### 1. Web Deployment (Fastest - Ready Now)
```bash
cd dashboard_flutter
npm run dev  # or use the existing web server
# Open: http://localhost:3000
```

**Status**: ✅ Fully operational
- All features working
- Real-time database sync
- Settings management
- Token generation

---

## 2. Android Deployment

### Prerequisites
- Android SDK (API 21 or higher)
- Android Studio (optional, but recommended)
- Gradle
- JDK 11 or higher

### Installation Steps

#### Step 1: Enable Android Platform
```bash
cd dashboard_flutter
# Enable Android support
flutter config --enable-android
flutter pub get
```

#### Step 2: Build APK
```bash
# Development build (faster)
flutter build apk --debug

# Production build (optimized)
flutter build apk --release

# Build as App Bundle (for Play Store)
flutter build appbundle --release
```

#### Step 3: Install on Device/Emulator
```bash
# List connected devices
flutter devices

# Run on device
flutter run -d <device-id>

# Or install APK directly
adb install build/app/outputs/apk/release/app-release.apk
```

### Build Output Locations
- **Debug APK**: `build/app/outputs/apk/debug/app-debug.apk`
- **Release APK**: `build/app/outputs/apk/release/app-release.apk`
- **App Bundle**: `build/app/outputs/bundle/release/app-release.aab`

---

## 3. iOS Deployment

### Prerequisites (Mac only)
- Xcode 13 or higher
- CocoaPods
- iOS deployment target: 11.0+

### Installation Steps

#### Step 1: Enable iOS Platform
```bash
cd dashboard_flutter
flutter config --enable-ios
flutter pub get
```

#### Step 2: Build IPA
```bash
# Development build
flutter build ios --debug

# Production build
flutter build ios --release
```

#### Step 3: Deploy
- Via Xcode: `open ios/Runner.xcworkspace`
- Via TestFlight: Upload to App Store Connect
- Via direct installation: Use generated IPA file

---

## 4. Desktop Deployment

### Windows
```bash
flutter config --enable-windows
flutter build windows --release
# Output: build/windows/x64/Release/dashboard_flutter.exe
```

### macOS
```bash
flutter config --enable-macos
flutter build macos --release
# Output: build/macos/Build/Release/dashboard_flutter.app
```

### Linux
```bash
flutter config --enable-linux
flutter build linux --release
# Output: build/linux/x64/release/bundle/
```

---

## Configuration Files

### `android/app/build.gradle`
Update for:
- Min SDK: 21
- Target SDK: 34
- Package name
- Signing certificates (for release)

### `ios/Runner.xcodeproj/project.pbxproj`
Update for:
- Bundle ID
- Development team
- Code signing identity

### `pubspec.yaml`
Key dependencies:
- `supabase_flutter`: ^2.0.0
- `provider`: ^6.0.0
- `intl`: ^0.19.0
- `mobile_scanner`: ^5.0.0 (for QR codes)

---

## Troubleshooting

### Android Build Issues

**Error: "Could not find Android SDK"**
```bash
flutter config --android-sdk /path/to/android/sdk
```

**Error: "Gradle build failed"**
```bash
cd android
gradlew clean
cd ..
flutter clean
flutter pub get
flutter build apk
```

### iOS Build Issues

**Error: "Pod install failed"**
```bash
cd ios
rm -rf Pods
rm Podfile.lock
cd ..
flutter pub get
```

**Error: "Module not found"**
```bash
flutter clean
flutter pub get
cd ios
pod install --repo-update
```

### General Issues

**Clean build**:
```bash
flutter clean
flutter pub get
flutter doctor -v
```

**Update Flutter**:
```bash
flutter upgrade
flutter pub get
```

---

## Performance Optimization

### Reduce Build Time
```bash
# Use split APKs (faster than universal APK)
flutter build apk --split-per-abi

# Skip tests
flutter build apk --release --dart-define=FLUTTER_WEB_USE_SKE=true
```

### Code Optimization
- Enable tree-shaking: `flutter build apk --split-per-abi`
- Minimize assets
- Remove unused packages from pubspec.yaml

### Runtime Optimization
- Use `const` constructors
- Implement efficient rebuild strategies with Provider
- Lazy load assets

---

## Deployment Checklist

- [ ] Update version number in `pubspec.yaml`
- [ ] Update `VERSION` in Flutter app
- [ ] Test on multiple devices/screen sizes
- [ ] Verify Supabase credentials are production-ready
- [ ] Update admin credentials
- [ ] Test all critical features
- [ ] Create signed APK/IPA
- [ ] Prepare release notes
- [ ] Upload to app stores

---

## Security Considerations

1. **Credentials**: Never commit `.env` files with real keys
2. **API Keys**: Use Supabase service role in backend only
3. **Code Obfuscation**: Enable for release builds
4. **HTTPS Only**: Ensure Supabase uses HTTPS
5. **Update Regularly**: Keep dependencies updated

---

## Support & Resources

- [Flutter Documentation](https://flutter.dev/docs)
- [Supabase Flutter Guide](https://supabase.com/docs/guides/getting-started/tutorials/with-flutter)
- [Android Build Documentation](https://flutter.dev/docs/deployment/android)
- [iOS Build Documentation](https://flutter.dev/docs/deployment/ios)

---

## Quick Commands Reference

```bash
# Setup
flutter pub get
flutter doctor -v

# Build
flutter build apk --release                    # Android
flutter build ios --release                    # iOS
flutter build web --release                    # Web
flutter build windows --release                # Windows

# Deploy
flutter run                                    # Run on device
flutter run -d all                            # Run on all connected devices
adb install build/app/outputs/apk/release/app-release.apk

# Clean
flutter clean
flutter pub get
```

---

Last Updated: 2026-06-22
