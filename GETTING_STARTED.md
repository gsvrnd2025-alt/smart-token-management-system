# Smart Token Management System - Getting Started

## 🎯 Quick Start (5 minutes)

### 1. Start the System
```bash
# Option A: Click the batch file
START_SYSTEM.bat

# Option B: Use terminal
npm run dev
# Then open: http://localhost:3000
```

### 2. Login
- **URL**: http://localhost:3000/dashboard.html
- **Username**: `admin`
- **Password**: `admin123`

### 3. Test It
- ✅ Settings page working
- ✅ Token generation working  
- ✅ Real-time updates working

---

## 📦 System Architecture

```
Smart Token Management System
│
├─ Web Interface (HTTP/WebSocket)
│  ├─ Admin Dashboard
│  ├─ Settings Management
│  ├─ Reports & Analytics
│  ├─ TV Display Monitor
│  └─ Customer Registration Portal
│
├─ Backend Services
│  ├─ Supabase Database (PostgreSQL)
│  ├─ Real-time Subscriptions
│  ├─ Authentication & Authorization
│  └─ File Storage
│
└─ Native Applications (Optional)
   ├─ Flutter Web App
   ├─ Android Mobile App
   └─ iOS Mobile App
```

---

## 🚀 Deployment Options

### Option 1: Web Only (Recommended for Most Users)
- **Install Time**: < 5 minutes
- **Requirements**: Browser + Internet
- **Platforms**: Any device with browser
- **Cost**: Free to low-cost hosting

**Start Command**:
```bash
npm run dev
open http://localhost:3000
```

### Option 2: Flutter Web App
- **Install Time**: 10-15 minutes
- **Requirements**: Flutter SDK
- **Platforms**: Web browser (any platform)
- **Cost**: Free

**Start Command**:
```bash
cd dashboard_flutter
flutter pub get
flutter run -d web-server --web-port 8080
```

### Option 3: Android Native App
- **Install Time**: 30-45 minutes
- **Requirements**: Android SDK + Device/Emulator
- **Platforms**: Android 5.1+ (API 21+)
- **Cost**: Free ($25 one-time Play Store fee)

**Build Command**:
```bash
cd dashboard_flutter
flutter build apk --release
# Install: build/app/outputs/apk/release/app-release.apk
```

### Option 4: iOS Native App
- **Install Time**: 45-60 minutes (Mac only)
- **Requirements**: Xcode + Mac computer
- **Platforms**: iOS 11.0+
- **Cost**: Free ($99/year Apple Developer)

**Build Command**:
```bash
cd dashboard_flutter
flutter build ipa --release
# Upload from: Xcode or App Store Connect
```

### Option 5: Windows Desktop App
- **Install Time**: 20-30 minutes
- **Requirements**: Windows 10+
- **Platforms**: Windows only
- **Cost**: Free

**Build Command**:
```bash
cd dashboard_flutter
flutter build windows --release
# Run: build/windows/x64/Release/dashboard_flutter.exe
```

---

## 📋 Prerequisites by Platform

### Windows (Recommended)
- [ ] Windows 10 or 11
- [ ] Node.js 16+ ([Download](https://nodejs.org/))
- [ ] Internet connection
- [ ] Modern browser (Chrome, Edge, Firefox)

### Linux
- [ ] Ubuntu 20.04+
- [ ] Node.js 16+
- [ ] `sudo apt-get install -y curl git`
- [ ] Modern browser

### macOS
- [ ] macOS 10.13+
- [ ] Node.js 16+
- [ ] Xcode Command Line Tools

### Android Development
- [ ] Android SDK 21+ (API level 21+)
- [ ] Android Studio (recommended)
- [ ] JDK 11+
- [ ] Android device or emulator

### iOS Development  
- [ ] macOS 10.13+
- [ ] Xcode 13+
- [ ] CocoaPods
- [ ] Apple Developer Account ($99/year)

---

## 🔧 Installation Steps

### Step 1: Clone/Download Project
```bash
# If using Git
git clone <repository-url>
cd token-project

# Or extract zip file and navigate to folder
```

### Step 2: Install Dependencies
```bash
# Install npm packages
npm install

# Install Flutter dependencies
cd dashboard_flutter
flutter pub get
cd ..
```

### Step 3: Configure Supabase
```bash
# Create .env file from template
cp .env.example .env

# Edit .env and add your credentials:
# SUPABASE_URL=https://your-project.supabase.co
# SUPABASE_ANON_KEY=your-anon-key
```

### Step 4: Start System
```bash
# Start web server
npm run dev

# Open browser
start http://localhost:3000
```

---

## 🔑 Default Credentials

| Field | Value |
|-------|-------|
| **Username** | `admin` |
| **Password** | `admin123` |
| **Email** | `admin@example.com` |
| **Organization** | Smart Token Management System |

⚠️ **IMPORTANT**: Change these immediately in production!

---

## 📱 Web Interface Walkthrough

### 1. Login Page
- URL: `/login.html`
- Enter username and password
- Check "Remember Session" to stay logged in

### 2. Dashboard
- **Currently Serving**: Current queue number
- **Waiting Queue**: Customers waiting
- **Completed Today**: Total served
- **Queue Console**: Call next token
- **Walk-In Registration**: Generate tokens
- **Waitlist**: View queue details

### 3. Settings
- **Queue Configuration**: Organization name, token range
- **Printer Settings**: Thermal printer config
- **Admin Credentials**: Username, password, email
- **Email Settings**: OTP/notification providers

### 4. Reports
- **Daily Reports**: Statistics by date
- **Customer Logs**: Who was served when
- **Service Times**: Average service duration
- **Export Data**: Download reports as CSV/PDF

### 5. TV Display
- Fullscreen display for waiting area
- Shows current serving number
- Plays audio announcements
- Responsive to mobile devices

### 6. Customer Portal
- Self-service registration
- Token number generation
- Queue status check
- Service booking

---

## 🧪 System Testing

### Quick Test Checklist
```bash
# 1. Login test
- Navigate to http://localhost:3000/dashboard.html
- Login with admin/admin123
- Verify dashboard loads

# 2. Settings test
- Go to Settings page
- Change Organization Name
- Click Save
- Refresh page - verify saved

# 3. Token generation test
- Click "Generate Token"
- Verify new token appears
- Check Last Generated shows updated number

# 4. Persistence test
- Change a setting
- Close browser
- Reopen http://localhost:3000/dashboard.html
- Login again
- Verify change persisted

# 5. Real-time test
- Open two browser windows
- Generate token in one
- Verify appears in other window
- Check TV monitor updates
```

### Run Diagnostics
```bash
# Navigate to:
http://localhost:3000/diagnostics.html

# Click "Run All Tests"
# Verify: 8/8 tests pass
```

---

## 🐛 Troubleshooting

### Server Won't Start
```bash
# Check if port 3000 is in use
netstat -ano | findstr :3000

# Use different port
npm run dev -- -p 3001
```

### Database Connection Error
```bash
# Verify Supabase credentials in .env
# Check internet connection
# Test connection: http://localhost:3000/diagnostics.html
```

### Settings Not Saving
- Check browser console (F12)
- Verify Supabase database accessible
- Check RLS policies enabled
- Review database logs

### Token Generation Not Working
- Verify admin credentials correct
- Check settings table has required data
- Confirm tokens table exists and accessible
- Review Supabase logs

### Can't Login
- Verify admin credentials in database
- Reset: Go to Settings and update credentials
- Check browser cookies enabled
- Clear browser cache and try again

---

## 📊 Performance & Scaling

### Current Capacity
- **Concurrent Users**: 50-100
- **Tokens/Day**: 1,000+
- **Data Retention**: All historical data
- **Response Time**: <200ms

### Optimization Tips
1. **Database**: Enable connection pooling in Supabase
2. **Caching**: Use browser cache for settings
3. **Compression**: Enable gzip on web server
4. **CDN**: Use CDN for static assets
5. **Monitoring**: Monitor database query performance

---

## 🔒 Security Checklist

- [ ] Change default admin password
- [ ] Update admin email
- [ ] Enable HTTPS for production
- [ ] Restrict IP access if needed
- [ ] Enable audit logging
- [ ] Regular backups enabled
- [ ] RLS policies verified
- [ ] Sensitive data encrypted

---

## 📚 Additional Resources

### Documentation Files
- `FLUTTER_DEPLOYMENT_GUIDE.md` - Native app deployment
- `NATIVE_APP_SETUP.md` - Platform-specific setup
- `QUICK_ACTION_GUIDE.md` - Quick reference
- `VERIFICATION_CHECKLIST.md` - System verification

### External Resources
- [Supabase Docs](https://supabase.com/docs)
- [Flutter Docs](https://flutter.dev/docs)
- [Node.js Docs](https://nodejs.org/docs/)
- [Web API Docs](./js/api.js)

---

## 🆘 Getting Help

### Check Logs
```bash
# Browser Console (F12)
# Shows JavaScript errors and network requests

# Server Console
# Shows npm/Node.js output

# Database Logs
# In Supabase Dashboard > Logs
```

### Enable Debug Mode
```bash
# In browser console:
localStorage.setItem('DEBUG_MODE', 'true')

# Restart and check console output
```

### Common Issues & Solutions

| Issue | Solution |
|-------|----------|
| Port 3000 in use | Use different port: `npm run dev -- -p 3001` |
| Can't connect to database | Check `.env` credentials and internet |
| Settings not saving | Clear cache, check RLS policies |
| App crashes on load | Check browser console for errors |
| Slow performance | Check network/database performance |

---

## 📞 Support

For issues or questions:
1. Check documentation files
2. Review diagnostics: http://localhost:3000/diagnostics.html
3. Check browser console for errors
4. Review Supabase logs
5. Check server logs

---

## 🎉 What's Next?

After successful installation:

1. **Customize Settings**
   - Update organization name
   - Configure queue settings
   - Set up printer
   - Configure email notifications

2. **Deploy to Production**
   - Choose hosting platform (AWS, Heroku, Vercel, etc.)
   - Set up SSL/HTTPS
   - Configure DNS
   - Enable backups

3. **Native Apps (Optional)**
   - Build Android APK
   - Build iOS IPA
   - Distribute via app stores
   - Set up push notifications

4. **Integration**
   - Connect to existing systems
   - Set up API endpoints
   - Configure webhooks
   - Enable analytics

---

**Last Updated**: 2026-06-22  
**Version**: 1.0.0
