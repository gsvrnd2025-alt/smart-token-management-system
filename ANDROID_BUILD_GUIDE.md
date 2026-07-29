# Android App Build & Deployment Guide

## Quick Build (Copy & Paste Commands)

### Prerequisites Check
```bash
where java
where gradle
where android
flutter doctor
```

### Build APK (Debug - Fast)
```bash
cd dashboard_flutter
flutter build apk
# Output: build/app/outputs/apk/debug/app-debug.apk
```

### Build APK (Release - Production)
```bash
cd dashboard_flutter
flutter build apk --release
# Output: build/app/outputs/apk/release/app-release.apk
```

### Install on Device/Emulator
```bash
# List devices
flutter devices

# Run on device
flutter run

# Or install APK directly
adb install build/app/outputs/apk/release/app-release.apk
```

---

## 📋 Step-by-Step Setup

### Step 1: Install Android SDK

#### Option A: Android Studio (Recommended)
1. Download Android Studio: https://developer.android.com/studio
2. Run installer
3. Complete setup wizard
4. Install SDK components:
   - Android SDK
   - Android SDK Platform
   - Android Emulator
   - SDK Build-Tools
5. Accept all license agreements

#### Option B: Command Line
```bash
# Create SDK directory
mkdir C:\Android\sdk

# Download Android SDK Command-Line Tools
# From: https://developer.android.com/studio/command-line/sdkmanager

# Add to PATH or use full path in commands
```

### Step 2: Install Java Development Kit (JDK)

```bash
# Check if installed
java -version

# If not installed, download JDK 11 or higher
# From: https://www.oracle.com/java/technologies/downloads/

# Or use OpenJDK
# Download from: https://jdk.java.net/
```

### Step 3: Configure Flutter for Android

```bash
# Set Android SDK path
flutter config --android-sdk C:\Android\sdk
flutter config --android-studio-path "C:\Program Files\Android\Android Studio"

# Verify setup
flutter doctor
```

Expected output should show:
```
✓ Flutter
✓ Android toolchain
✓ Android Studio
✓ Java
```

### Step 4: Get Flutter Dependencies

```bash
cd dashboard_flutter
flutter pub get --no-precompile
```

---

## 🔨 Build Options

### 1. Debug APK (Development)
Fastest build, includes debug symbols.

```bash
flutter build apk
```

**Output**: `build/app/outputs/apk/debug/app-debug.apk`  
**File Size**: ~50-100MB  
**Build Time**: ~2-3 minutes  
**Use For**: Testing, development

### 2. Release APK (Production Single)
Optimized universal APK for all devices.

```bash
flutter build apk --release
```

**Output**: `build/app/outputs/apk/release/app-release.apk`  
**File Size**: ~30-50MB  
**Build Time**: ~5-10 minutes  
**Use For**: Distribution

### 3. Split APK (Production Multiple)
Smaller APKs for each CPU architecture.

```bash
flutter build apk --release --split-per-abi
```

**Outputs**:
- `app-armeabi-v7a-release.apk` (~15MB) - 32-bit ARM
- `app-arm64-v8a-release.apk` (~18MB) - 64-bit ARM
- `app-x86-release.apk` (~20MB) - Intel x86
- `app-x86_64-release.apk` (~22MB) - Intel x64

**File Size**: 15-22MB each  
**Build Time**: ~10-15 minutes  
**Use For**: Play Store (smaller downloads)

### 4. App Bundle (Play Store)
Required format for Google Play Store.

```bash
flutter build appbundle --release
```

**Output**: `build/app/outputs/bundle/release/app-release.aab`  
**File Size**: ~25MB  
**Build Time**: ~8-12 minutes  
**Use For**: Google Play Store distribution

---

## 🚀 Installation Methods

### Method 1: Direct APK Installation (Fastest)

```bash
# Connect device via USB (enable developer mode)
# Or launch Android Emulator

# Check devices
adb devices

# Install debug APK
adb install build/app/outputs/apk/debug/app-debug.apk

# Or release APK
adb install -r build/app/outputs/apk/release/app-release.apk
```

### Method 2: Flutter Run (Easiest)

```bash
# List available devices
flutter devices

# Run on specific device
flutter run -d <device-id>

# Run on all connected devices
flutter run -d all
```

### Method 3: Android Studio

```bash
# Open project
start android\

# Or use VS Code
flutter run
```

---

## 🛠️ Configuration

### Update App Metadata

Edit `android/app/build.gradle`:

```gradle
android {
    namespace "com.example.dashboard_flutter"
    
    compileSdkVersion 34
    
    defaultConfig {
        applicationId "com.example.dashboard_flutter"
        minSdkVersion 21          // Android 5.1
        targetSdkVersion 34       // Latest
        versionCode 1
        versionName "1.0.0"
    }
}
```

### Update App Icons

Replace files in:
- `android/app/src/main/res/mipmap-*`

### Update App Name

Edit `android/app/src/main/AndroidManifest.xml`:

```xml
<application
    android:label="Smart Token System"
    android:icon="@mipmap/ic_launcher"
    android:roundIcon="@mipmap/ic_launcher_round">
```

---

## 📦 Release Build Signing

### Generate Signing Key

```bash
keytool -genkey -v -keystore "C:\Users\jagad\key.jks" `
  -keyalg RSA `
  -keysize 2048 `
  -validity 10000 `
  -alias upload_key
```

**Prompts**:
- Keystore password: [Create strong password]
- Key password: [Same password]
- First and Last Name: [Your name]
- Organizational Unit: [Your company]
- Organization: [Your organization]
- City/Locality: [Your city]
- State/Province: [Your state]
- Country Code: [US, IN, etc.]
- Confirm: [yes]

### Configure Signing in Gradle

Edit `android/app/build.gradle`:

```gradle
android {
    signingConfigs {
        release {
            keyAlias 'upload_key'
            keyPassword 'your_key_password'
            storeFile file('C:\\Users\\jagad\\key.jks')
            storePassword 'your_keystore_password'
        }
    }
    
    buildTypes {
        release {
            signingConfig signingConfigs.release
        }
    }
}
```

### Build Signed Release

```bash
flutter build apk --release
```

Or with specific key:
```bash
flutter build apk --release -v
```

---

## 📊 Build Troubleshooting

### Error: "Could not find Android SDK"

```bash
# Set path manually
flutter config --android-sdk "C:\Android\sdk"

# Verify
flutter doctor
```

### Error: "Gradle build failed"

```bash
# Clean build
cd android
gradlew clean
cd ..
flutter clean
flutter pub get
flutter build apk --release
```

### Error: "SDK version too low"

Edit `android/app/build.gradle`:
```gradle
minSdkVersion 21        // Minimum supported
targetSdkVersion 34     // Latest available
```

### Error: "Out of Memory"

Increase Gradle heap:
```bash
# Create gradle.properties in android/ directory
org.gradle.jvmargs=-Xmx2048m
```

### Error: "Connection refused"

```bash
# Restart emulator or check device connection
adb kill-server
adb start-server
flutter devices
```

### Error: "Package name conflicts"

Change package name in `android/app/build.gradle`:
```gradle
applicationId "com.mycompany.tokenapp"
```

---

## 🧪 Testing Before Release

### Test on Multiple Devices
```bash
# Real device
flutter run -d <device-1>

# Emulator
flutter emulators --launch <emulator-name>
flutter run -d <emulator-id>

# All devices
flutter run -d all
```

### Test Functionality
- [ ] Login works
- [ ] Settings load and save
- [ ] Token generation works
- [ ] Real-time updates work
- [ ] TV display updates
- [ ] Customer portal works
- [ ] Database connectivity OK
- [ ] Network requests succeed

### Check App Size
```bash
# APK size
dir build\app\outputs\apk\release\app-release.apk

# App Bundle size
dir build\app\outputs\bundle\release\app-release.aab
```

---

## 📤 Distribution

### Google Play Store

1. **Prepare**
   ```bash
   # Create signed bundle
   flutter build appbundle --release
   ```

2. **Create Account**
   - Go to Google Play Console
   - Create developer account ($25 one-time)
   - Create new app

3. **Upload**
   - Sign in to Play Console
   - Upload `app-release.aab`
   - Fill app details:
     - Title
     - Description
     - Screenshots (5)
     - Icon & Feature graphic
     - Content rating
     - Privacy policy

4. **Review & Release**
   - Submit for review (1-3 days)
   - Once approved, release to production

### Direct Distribution

1. **Host APK**
   ```bash
   # Copy to server/website
   scp build/app/outputs/apk/release/app-release.apk user@server:./
   ```

2. **Create Download Link**
   ```html
   <a href="/app-release.apk">Download Smart Token App</a>
   ```

3. **Share with Users**
   - Email link
   - QR code
   - Website
   - WhatsApp/Telegram

### Enterprise Distribution

1. **MDM Solutions**
   - Intune
   - AirWatch
   - MobileIron

2. **In-house Apps**
   ```bash
   # Use internal Play Store
   flutter build appbundle --release
   ```

---

## 📈 Release Checklist

Before releasing:

- [ ] Change version number in `pubspec.yaml`
- [ ] Update app metadata (name, description)
- [ ] Update app icons
- [ ] Test on multiple Android versions
- [ ] Test on multiple devices (phones, tablets)
- [ ] Verify all features work
- [ ] Test offline functionality
- [ ] Update Supabase credentials to production
- [ ] Change default admin password
- [ ] Review privacy policy
- [ ] Configure analytics
- [ ] Test payment methods (if applicable)
- [ ] Create signed release bundle
- [ ] Upload to Play Store or distribution platform

---

## 🔄 Continuous Updates

### Minor Update (patch)
```yaml
# pubspec.yaml
version: 1.0.1
```

### Major Update
```yaml
# pubspec.yaml
version: 2.0.0
```

### Build and Upload
```bash
flutter build appbundle --release
# Upload to Play Store
```

### User Experience
- Google Play handles update distribution
- Users get automatic update prompt
- Or can be forced to update

---

## Performance Optimization

### Reduce Build Time
```bash
# Skip unused platforms
flutter config --no-analytics

# Use local mirrors (if in China)
export PUB_HOSTED_URL=https://pub.flutter-io.cn
export FLUTTER_STORAGE_BASE_URL=https://storage.flutter-io.cn
```

### Reduce App Size
```bash
# Enable shrinking
flutter build apk --release --split-per-abi

# With obfuscation
flutter build apk --release --obfuscate --split-symbols
```

### Optimize Runtime
- Use efficient Flutter widgets
- Implement lazy loading
- Cache API responses
- Minimize database queries

---

## Commands Reference

| Command | Purpose |
|---------|---------|
| `flutter build apk` | Debug APK |
| `flutter build apk --release` | Release APK |
| `flutter build apk --release --split-per-abi` | Split APKs |
| `flutter build appbundle --release` | App Bundle |
| `flutter run` | Run on device |
| `flutter devices` | List devices |
| `adb install <file>` | Install APK |
| `flutter doctor` | Check setup |

---

## Next Steps

1. ✅ Build first APK
2. ✅ Test on emulator/device
3. ✅ Generate signed release
4. ✅ Create Play Store account
5. ✅ Upload to Play Store
6. ✅ Monitor user reviews
7. ✅ Plan updates

---

**Last Updated**: 2026-06-22
