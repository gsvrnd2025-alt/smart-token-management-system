# Quick Reference Guide

## Starting All Services

```bash
# Web Server (port 3000)
npm run dev

# Flutter Dashboard
.\run_dashboard.bat

# Windows App
.\run_windows_app.bat
```

## Quick Diagnostics

### Test Everything
1. Open http://localhost:3000/diagnostics.html
2. Click "Run All Tests"
3. Review results

### Check Database
```sql
-- Login to Supabase
SELECT COUNT(*) as total_tokens FROM tokens;
SELECT COUNT(*) as total_settings FROM settings;
```

### Verify Credentials
```sql
SELECT key, value FROM settings 
WHERE key IN ('Admin Username', 'Admin Password', 'Admin Email');
```

## Common Tasks

### Change Admin Password
**Via Flutter App:**
1. System Settings → Admin Credentials
2. Update password
3. Click Save

**Via Database:**
```sql
UPDATE settings SET value = 'newpassword' WHERE key = 'Admin Password';
```

### Enable Email Recovery
**Via Flutter App:**
1. System Settings → Email Configuration
2. Choose provider (Demo/Resend/SMTP)
3. Fill in credentials
4. Click Save

### Generate Test Token
1. Login to web dashboard or Flutter app
2. Go to Queue Console or Registration
3. Fill in customer details
4. Click "Generate Token"
5. Token number should increment

### View Today's Tokens
1. Go to Reports & Logs (web) or Reports (Flutter)
2. See all tokens generated today
3. Status: Waiting, Serving, Completed, Skipped

### Call Next Token
**Queue Console:**
1. Click "Next Token" button
2. Waiting token moves to Serving
3. Display shows current token
4. Customer name and service type shown

## Troubleshooting Commands

### Reset to Defaults
```sql
-- Reset all settings to defaults
UPDATE settings SET value = 'admin' WHERE key = 'Admin Username';
UPDATE settings SET value = 'admin123' WHERE key = 'Admin Password';
UPDATE settings SET value = 'admin@example.com' WHERE key = 'Admin Email';
UPDATE settings SET value = '0' WHERE key = 'Last Generated Token';
UPDATE settings SET value = '0' WHERE key = 'Current Serving Token';
UPDATE settings SET value = '1' WHERE key = 'Starting Token Number';
```

### Clear Today's Tokens
```sql
-- Delete all tokens from today
DELETE FROM tokens WHERE DATE(created_at) = CURRENT_DATE;
```

### Fix Stream Issues (Flutter)
1. Close app completely
2. Reopen app
3. Wait for "✓ Settings updated from stream" message

### Check System Status
1. Open browser console (F12)
2. Look for connection status messages
3. Verify no errors in Network tab

## File Locations

- **Web Files**: `./` (index.html, dashboard.html, etc.)
- **Flutter App**: `./dashboard_flutter/`
- **Database Config**: `./supabase/config.toml`
- **JavaScript API**: `./js/api.js`
- **CSS Styles**: `./css/style.css`
- **Configuration**: `./INTEGRATION_SETUP.md`

## System Status Indicators

### Green ✓
- System Live
- Connection active
- All tests passed

### Yellow ⚠️
- Demo mode active
- Offline setup
- Settings being configured

### Red ✗
- Connection failed
- Credentials invalid
- Database unavailable

## Default Login
- **Username**: admin
- **Password**: admin123

⚠️ **CHANGE THESE IMMEDIATELY IN PRODUCTION!**

## API Endpoints (Web)

```javascript
// Login
SmartTokenAPI.verifyLogin(username, password)

// Generate token
SmartTokenAPI.generateToken({ 
  name, phone, email, serviceType, remarks 
})

// Get queue
SmartTokenAPI.getQueue()

// Next token
SmartTokenAPI.nextToken()

// Complete token
SmartTokenAPI.completeToken(tokenNumber)

// Skip token
SmartTokenAPI.skipToken(tokenNumber)
```

## Emergency Contacts
1. Check INTEGRATION_SETUP.md for detailed troubleshooting
2. Review browser console for error messages
3. Check Supabase status: https://status.supabase.com
4. Review application logs

## Performance Tips
- Clear browser cache periodically
- Restart apps if experiencing lag
- Monitor database query performance
- Use diagnostics tool weekly

## Security Reminders
✓ Change default credentials
✓ Use HTTPS in production
✓ Enable RLS policies
✓ Regular backups
✓ Monitor access logs

---
Last Updated: 2026-06-22
