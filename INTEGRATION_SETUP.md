# Smart Token Management System - Integration Setup Guide

## Quick Start

### Prerequisites
- Supabase account and project
- Flutter SDK (for mobile/desktop app)
- Node.js and npm (for web dev server)
- Modern web browser

### Step 1: Configure Supabase

1. Create a new Supabase project or use existing one
2. Copy your Supabase URL and Anon Key from the project settings
3. Create a `.env` file in the project root (copy from `.env.example`)
4. Update the `SUPABASE_URL` and `SUPABASE_ANON_KEY` values

### Step 2: Initialize Database

The database schema will be automatically created on first connection. Verify:

```sql
-- Check if tables exist
SELECT * FROM settings LIMIT 1;
SELECT * FROM tokens LIMIT 1;
```

All required settings should be pre-populated:
- Admin Username: `admin`
- Admin Password: `admin123`
- Admin Email: `admin@example.com`
- Starting Token Number: `1`
- Last Generated Token: `0`
- Current Serving Token: `0`
- Average Service Time: `10`

### Step 3: Update Credentials

**IMPORTANT: Change default credentials immediately in production!**

1. Open the Flutter app and go to System Settings
2. Update:
   - Admin Username
   - Admin Password
   - Admin Email
3. Settings are automatically saved to Supabase

Or update directly in Supabase:
```sql
UPDATE settings SET value = 'newusername' WHERE key = 'Admin Username';
UPDATE settings SET value = 'newpassword' WHERE key = 'Admin Password';
UPDATE settings SET value = 'newemail@domain.com' WHERE key = 'Admin Email';
```

### Step 4: Run Applications

**Web Dashboard (port 3000):**
```bash
npm run dev
```

**Flutter Dashboard (Desktop):**
```bash
./run_dashboard.bat
```

**Windows App:**
```bash
./run_windows_app.bat
```

## Troubleshooting

### Authentication Issues

**Problem: "Invalid username or password" error**

1. Verify credentials in Supabase:
   ```sql
   SELECT key, value FROM settings WHERE key IN ('Admin Username', 'Admin Password');
   ```

2. Check credentials are exactly matching (case-sensitive for password)

3. Clear app cache:
   - Flutter: Delete `pubspec.lock` and run `flutter pub get`
   - Web: Clear browser localStorage and cache

### Database Connection Issues

**Problem: Settings not loading or saving**

1. Check Supabase Status:
   - Visit https://status.supabase.com
   - Verify your project is running

2. Verify API Keys:
   - Go to Supabase Dashboard → Settings → API
   - Confirm Anon Key is correct

3. Check RLS Policies:
   ```sql
   SELECT * FROM pg_policies WHERE tablename IN ('settings', 'tokens');
   ```

4. Test connection manually:
   ```javascript
   // In browser console:
   const client = supabase.createClient(URL, KEY);
   const result = await client.from('settings').select('*').limit(1);
   console.log(result);
   ```

### Token Generation Issues

**Problem: Tokens not being generated or incremented**

1. Check database state:
   ```sql
   SELECT * FROM tokens LIMIT 5 ORDER BY created_at DESC;
   SELECT key, value FROM settings WHERE key LIKE '%Token%';
   ```

2. Verify settings are properly initialized:
   ```sql
   SELECT COUNT(*) FROM settings WHERE value IS NOT NULL;
   ```

3. Check for unique constraint on token_number:
   ```sql
   \d tokens  -- List table structure
   ```

### Real-time Updates Not Working

**Problem: Flutter app not showing live updates**

1. Restart the app to reconnect streams

2. Check stream subscriptions in debug output:
   - Look for "✓ Active tokens updated" messages
   - Look for "✗ Stream error" messages

3. Verify Realtime is enabled in Supabase:
   - Dashboard → Settings → Realtime
   - Ensure `tokens` and `settings` tables are enabled

4. Check internet connection and firewall settings

## Environment Variables

Create a `.env` file for production deployment:

```env
SUPABASE_URL=https://your-project.supabase.co
SUPABASE_ANON_KEY=eyJ...
ADMIN_USERNAME=yourusername
ADMIN_PASSWORD=securepassword
ADMIN_EMAIL=admin@yourdomain.com
EMAIL_PROVIDER=smtp
SMTP_HOST=smtp.gmail.com
SMTP_PORT=587
SMTP_USERNAME=your-email@gmail.com
SMTP_PASSWORD=your-app-password
```

## Security Best Practices

1. **Change Default Credentials**: Update username and password immediately
2. **Use Strong Passwords**: Minimum 12 characters with mixed case and numbers
3. **Configure Email Properly**: Set up SMTP or Resend for password recovery
4. **Enable HTTPS**: Always use HTTPS in production
5. **Restrict Database Access**: Use RLS policies to limit data access
6. **Regular Backups**: Back up Supabase data regularly

## System Settings Reference

| Key | Default | Description |
|-----|---------|-------------|
| Admin Username | admin | Administrator login username |
| Admin Email | admin@example.com | Email for password recovery |
| Admin Password | admin123 | Administrator login password |
| Starting Token Number | 1 | First token number to generate |
| Last Generated Token | 0 | Last token number generated (auto-updated) |
| Current Serving Token | 0 | Token currently being served |
| Average Service Time | 10 | Minutes per service (for wait time calculation) |
| Organization Name | Smart Token... | Business name displayed in app |
| Enable Buzzer | true | Audio notification when calling tokens |
| Thermal Printer Settings | {...} | Printer connection configuration |
| Email Service Config | {...} | Email provider configuration |

## Support

For issues or questions:
1. Check the troubleshooting section above
2. Review browser console and Flutter debug output
3. Verify all Supabase settings are correct
4. Check database directly for data integrity
5. Restart all applications and clear caches

---
Last Updated: 2026-06-22
