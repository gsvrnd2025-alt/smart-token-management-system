import 'dart:async';
import 'dart:convert';
import 'dart:math';
import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;
import 'package:mailer/mailer.dart';
import 'package:mailer/smtp_server.dart';
import 'package:supabase_flutter/supabase_flutter.dart';

class ApiService {
  static const String supabaseUrl = "https://swqgfhtyfudkwvyuulzz.supabase.co";
  static const String supabaseAnonKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InN3cWdmaHR5ZnVka3d2eXV1bHp6Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODE2MDE4ODIsImV4cCI6MjA5NzE3Nzg4Mn0.qbjAR4I8NfCFusutfws4I4oZJsbCx4TGeaYtfSyA1fc";

  static final ApiService _instance = ApiService._internal();
  factory ApiService() => _instance;
  ApiService._internal();

  SupabaseClient get client => Supabase.instance.client;

  /// Initialize Supabase and check/seed credentials and configuration settings in database
  Future<void> initialize() async {
    try {
      await Supabase.initialize(
        url: supabaseUrl,
        anonKey: supabaseAnonKey,
      );
      await seedSettingsIfNeeded();
      debugPrint("✓ Supabase initialized successfully");
    } catch (e) {
      debugPrint("✗ Supabase initialization error: $e");
      rethrow;
    }
  }

  /// Ensure required settings keys exist in the Supabase database
  Future<void> seedSettingsIfNeeded() async {
    try {
      final response = await client.from('settings').select('key');
      final existingKeys = (response as List).map((e) => e['key'] as String).toSet();

      final requiredSettings = {
        'Admin Username': 'admin',
        'Admin Email': 'admin@example.com',
        'Admin Password': 'admin123',
        'Admin OTP': '',
        'Admin OTP Expiry': '',
        'Starting Token Number': '1',
        'Last Generated Token': '0',
        'Current Serving Token': '0',
        'Average Service Time': '10',
        'Organization Name': 'Smart Token Management System',
        'Enable Buzzer': 'true',
        'Thermal Printer Settings': jsonEncode({'connection': 'wire', 'device': ''}),
        'Scanned Bluetooth Printers': '[]',
        'Email Service Config': jsonEncode({
          'provider': 'demo',
          'resend_api_key': '',
          'resend_sender': 'onboarding@resend.dev',
          'smtp_host': '',
          'smtp_port': 587,
          'smtp_username': '',
          'smtp_password': '',
          'smtp_sender': 'noreply@yourdomain.com',
          'smtp_ssl': false,
        }),
        'Scan Request': 'false',
      };

      for (final entry in requiredSettings.entries) {
        if (!existingKeys.contains(entry.key)) {
          try {
            await client.from('settings').insert({'key': entry.key, 'value': entry.value});
            debugPrint("✓ Seeded setting: ${entry.key}");
          } catch (e) {
            debugPrint("✗ Error seeding ${entry.key}: $e");
          }
        }
      }
      
      debugPrint("✓ Settings initialization completed");
    } catch (e) {
      debugPrint("Settings table checking/seeding error: $e");
    }
  }

  // --- Auth / Password Recovery Functions ---

  /// Verify admin login credentials
  Future<bool> verifyLogin(String username, String password) async {
    try {
      final userResponse = await client.from('settings').select('value').eq('key', 'Admin Username').maybeSingle();
      final passResponse = await client.from('settings').select('value').eq('key', 'Admin Password').maybeSingle();

      final dbUsername = userResponse?['value'] ?? 'admin';
      final dbPassword = passResponse?['value'] ?? 'admin123';

      final isValid = username.toLowerCase() == dbUsername.toLowerCase() && password == dbPassword;
      debugPrint("${isValid ? '✓' : '✗'} Login verification: username=$username, password_match=${password == dbPassword}");
      return isValid;
    } catch (e) {
      debugPrint("✗ Login check error: $e");
      return false;
    }
  }

  /// Request Password Reset OTP
  /// Generates a 6-digit code, saves it to Supabase settings, and sends an email.
  Future<String> requestPasswordResetOTP(String username) async {
    final userResponse = await client.from('settings').select('value').eq('key', 'Admin Username').maybeSingle();
    final dbUsername = userResponse?['value'] ?? 'admin';

    if (username.toLowerCase() != dbUsername.toLowerCase()) {
      throw Exception("Username not found");
    }

    final emailResponse = await client.from('settings').select('value').eq('key', 'Admin Email').maybeSingle();
    final email = emailResponse?['value'] ?? 'admin@example.com';

    // Generate random 6-digit OTP
    final random = Random();
    final otp = (100000 + random.nextInt(900000)).toString();
    final expiry = DateTime.now().add(const Duration(minutes: 5)).toUtc().toIso8601String();

    // Update settings table with OTP and Expiry
    await client.from('settings').upsert([
      {'key': 'Admin OTP', 'value': otp},
      {'key': 'Admin OTP Expiry', 'value': expiry}
    ]);

    // Fetch Email Configuration
    final configResponse = await client.from('settings').select('value').eq('key', 'Email Service Config').maybeSingle();
    final configString = configResponse?['value'] ?? '{}';
    
    Map<String, dynamic> config = {};
    try {
      config = jsonDecode(configString);
    } catch (_) {}

    final provider = config['provider'] ?? 'demo';

    // Send the email based on provider
    if (provider == 'resend') {
      await _sendResendEmail(email, otp, config);
    } else if (provider == 'smtp') {
      await _sendSmtpEmail(email, otp, config);
    } else {
      // Demo Mode
      debugPrint("==================================================");
      debugPrint(" [DEMO MODE OTP] Sent OTP $otp to $email ");
      debugPrint("==================================================");
    }

    // Return masked email for security display (e.g. ad***n@example.com)
    final parts = email.split('@');
    if (parts.length == 2) {
      final name = parts[0];
      final domain = parts[1];
      if (name.length > 2) {
        return "${name[0]}***${name[name.length - 1]}@$domain";
      }
    }
    return email;
  }

  /// Verify entered OTP code
  Future<bool> verifyResetOTP(String otp) async {
    try {
      final otpRes = await client.from('settings').select('value').eq('key', 'Admin OTP').maybeSingle();
      final expRes = await client.from('settings').select('value').eq('key', 'Admin OTP Expiry').maybeSingle();

      final dbOtp = otpRes?['value'] ?? '';
      final dbExpiryStr = expRes?['value'] ?? '';

      if (dbOtp.isEmpty || otp != dbOtp) return false;

      final expiry = DateTime.tryParse(dbExpiryStr);
      if (expiry == null || DateTime.now().toUtc().isAfter(expiry)) {
        return false; // Expired
      }

      return true;
    } catch (e) {
      debugPrint("OTP verification failed: $e");
      return false;
    }
  }

  /// Change username and password in settings
  Future<void> updateCredentials(String newUsername, String newPassword) async {
    await client.from('settings').upsert([
      {'key': 'Admin Username', 'value': newUsername},
      {'key': 'Admin Password', 'value': newPassword},
      {'key': 'Admin OTP', 'value': ''}, // Clear OTP
      {'key': 'Admin OTP Expiry', 'value': ''}
    ]);
  }

  // --- Email Delivery Implementations ---

  Future<void> _sendResendEmail(String email, String otp, Map<String, dynamic> config) async {
    final apiKey = config['resend_api_key'] ?? '';
    final sender = config['resend_sender'] ?? 'onboarding@resend.dev';

    if (apiKey.isEmpty) {
      throw Exception("Resend API key is not configured in settings");
    }

    final response = await http.post(
      Uri.parse('https://api.resend.com/emails'),
      headers: {
        'Authorization': 'Bearer $apiKey',
        'Content-Type': 'application/json',
      },
      body: jsonEncode({
        'from': 'Smart Token Management <$sender>',
        'to': [email],
        'subject': 'Your Admin Reset OTP - Smart Token System',
        'html': '''
          <div style="font-family: Arial, sans-serif; padding: 20px; color: #333; max-width: 600px; margin: 0 auto; border: 1px solid #ddd; border-radius: 8px;">
            <h2 style="color: #6366F1; text-align: center;">Smart Token System</h2>
            <hr style="border: 0; border-top: 1px solid #eee;" />
            <p>Hello Admin,</p>
            <p>You requested a one-time password (OTP) reset for your credentials. Please use the verification code below:</p>
            <div style="text-align: center; margin: 30px 0;">
              <span style="font-size: 32px; font-weight: bold; letter-spacing: 4px; padding: 10px 20px; background-color: #F3F4F6; border-radius: 4px; color: #111827; border: 1px dashed #6366F1;">$otp</span>
            </div>
            <p style="color: #6B7280; font-size: 14px; text-align: center;">This code is valid for <strong>5 minutes</strong>. If you did not request this, please secure your account settings.</p>
          </div>
        ''',
      }),
    );

    if (response.statusCode != 200 && response.statusCode != 201) {
      throw Exception("Resend API failed: ${response.body}");
    }
  }

  Future<void> _sendSmtpEmail(String email, String otp, Map<String, dynamic> config) async {
    final host = config['smtp_host'] ?? '';
    final port = int.tryParse(config['smtp_port']?.toString() ?? '587') ?? 587;
    final username = config['smtp_username'] ?? '';
    final password = config['smtp_password'] ?? '';
    final sender = config['smtp_sender'] ?? 'noreply@yourdomain.com';
    final ssl = config['smtp_ssl'] ?? false;

    if (host.isEmpty || username.isEmpty || password.isEmpty) {
      throw Exception("SMTP Settings are incomplete");
    }

    final smtpServer = SmtpServer(
      host,
      port: port,
      ssl: ssl,
      username: username,
      password: password,
    );

    final message = Message()
      ..from = Address(sender, 'Smart Token System')
      ..recipients.add(email)
      ..subject = 'Your Admin Reset OTP - Smart Token System'
      ..html = '''
        <div style="font-family: Arial, sans-serif; padding: 20px; color: #333; max-width: 600px; margin: 0 auto; border: 1px solid #ddd; border-radius: 8px;">
          <h2 style="color: #6366F1; text-align: center;">Smart Token System</h2>
          <hr style="border: 0; border-top: 1px solid #eee;" />
          <p>Hello Admin,</p>
          <p>You requested a one-time password (OTP) reset for your credentials. Please use the verification code below:</p>
          <div style="text-align: center; margin: 30px 0;">
            <span style="font-size: 32px; font-weight: bold; letter-spacing: 4px; padding: 10px 20px; background-color: #F3F4F6; border-radius: 4px; color: #111827; border: 1px dashed #6366F1;">$otp</span>
          </div>
          <p style="color: #6B7280; font-size: 14px; text-align: center;">This code is valid for <strong>5 minutes</strong>. If you did not request this, please secure your account settings.</p>
        </div>
      ''';

    await send(message, smtpServer);
  }

  // --- General Token Operations ---

  /// Get real-time streams of all active tokens (Waiting, Serving)
  Stream<List<Map<String, dynamic>>> getQueueStream() {
    return client
        .from('tokens')
        .stream(primaryKey: ['id'])
        .eq('status', 'Waiting')
        .order('token_number', ascending: true);
  }

  /// Stream of both Waiting and Serving tokens
  Stream<List<Map<String, dynamic>>> getActiveTokensStream() {
    return client
        .from('tokens')
        .stream(primaryKey: ['id'])
        .inFilter('status', ['Waiting', 'Serving'])
        .order('token_number', ascending: true);
  }

  /// Generate a new token
  Future<Map<String, dynamic>> generateToken(Map<String, dynamic> details) async {
    // 1. Get settings keys
    final settingsList = await client.from('settings').select('*');
    final settingsMap = {for (var item in settingsList) item['key']: item['value']};

    final lastToken = int.tryParse(settingsMap['Last Generated Token']?.toString() ?? '0') ?? 0;
    final startingToken = int.tryParse(settingsMap['Starting Token Number']?.toString() ?? '100') ?? 100;
    final avgServiceTime = int.tryParse(settingsMap['Average Service Time']?.toString() ?? '10') ?? 10;

    int newTokenNum = lastToken + 1;
    if (newTokenNum < startingToken) {
      newTokenNum = startingToken;
    }

    // 2. Insert new token
    await client.from('tokens').insert({
      'token_number': newTokenNum,
      'customer_name': details['name'] ?? 'Walk-In',
      'phone_number': details['phone'] ?? '-',
      'email': details['email'] ?? '-',
      'service_type': details['serviceType'] ?? 'General Service',
      'source': details['source'] ?? 'Manual',
      'status': 'Waiting',
      'remarks': details['remarks'] ?? '',
    });

    // 3. Update Last Generated Token
    await client.from('settings').update({'value': newTokenNum.toString()}).eq('key', 'Last Generated Token');

    // 4. Calculate waiting tokens count before this token
    final waitCountRes = await client
        .from('tokens')
        .select('id')
        .inFilter('status', ['Waiting', 'Serving'])
        .lt('token_number', newTokenNum);

    final waitCount = (waitCountRes as List).length;

    return {
      'tokenNumber': newTokenNum,
      'customerName': details['name'] ?? 'Walk-In',
      'serviceType': details['serviceType'] ?? 'General Service',
      'source': details['source'] ?? 'Manual',
      'estimatedWaitingTimeMinutes': waitCount * avgServiceTime,
      'timeGenerated': DateTime.now().toLocal().toString().split(' ')[1].substring(0, 5),
    };
  }

  /// Call next waiting token
  Future<Map<String, dynamic>?> nextToken() async {
    // 1. Complete current serving tokens
    await client.from('tokens').update({'status': 'Completed'}).eq('status', 'Serving');

    // 2. Query next waiting token
    final nextWaiting = await client
        .from('tokens')
        .select('*')
        .eq('status', 'Waiting')
        .order('token_number', ascending: true)
        .limit(1)
        .maybeSingle();

    if (nextWaiting != null) {
      final nextId = nextWaiting['id'];
      final nextNum = nextWaiting['token_number'];

      // Mark serving
      await client.from('tokens').update({'status': 'Serving'}).eq('id', nextId);
      await client.from('settings').update({'value': nextNum.toString()}).eq('key', 'Current Serving Token');

      return {
        'tokenNumber': nextNum,
        'customerName': nextWaiting['customer_name'] ?? 'Walk-In',
        'serviceType': nextWaiting['service_type'] ?? 'General Service',
        'source': nextWaiting['source'] ?? 'Manual',
        'status': 'Serving'
      };
    } else {
      await client.from('settings').update({'value': '0'}).eq('key', 'Current Serving Token');
      return null;
    }
  }

  /// Complete a token
  Future<void> completeToken(int tokenNumber) async {
    await client.from('tokens').update({'status': 'Completed'}).eq('token_number', tokenNumber);

    // Reset settings Current Serving Token if matched
    final currentServing = await client.from('settings').select('value').eq('key', 'Current Serving Token').maybeSingle();
    if (currentServing?['value'] == tokenNumber.toString()) {
      await client.from('settings').update({'value': '0'}).eq('key', 'Current Serving Token');
    }
  }

  /// Skip a token
  Future<void> skipToken(int tokenNumber) async {
    await client.from('tokens').update({'status': 'Skipped'}).eq('token_number', tokenNumber);

    // Reset settings Current Serving Token if matched
    final currentServing = await client.from('settings').select('value').eq('key', 'Current Serving Token').maybeSingle();
    if (currentServing?['value'] == tokenNumber.toString()) {
      await client.from('settings').update({'value': '0'}).eq('key', 'Current Serving Token');
    }
  }

  /// Stream updates for all settings
  Stream<List<Map<String, dynamic>>> getSettingsStream() {
    return client.from('settings').stream(primaryKey: ['key']);
  }

  /// Fetch all settings as a key-value Map
  Future<Map<String, String>> getSettingsMap() async {
    try {
      final list = await client.from('settings').select('*');
      return {for (var item in list) item['key'].toString(): item['value'].toString()};
    } catch (e) {
      debugPrint("Fetch settings map error: $e");
      return {};
    }
  }

  /// Write multiple settings changes to Supabase
  Future<void> updateSettings(Map<String, String> updates) async {
    for (final entry in updates.entries) {
      await client.from('settings').upsert({
        'key': entry.key,
        'value': entry.value,
      });
    }
  }

  /// Fetch all tokens generated today
  Future<List<Map<String, dynamic>>> getTodayTokens() async {
    final startOfDay = DateTime.now().toLocal();
    final todayStr = DateTime(startOfDay.year, startOfDay.month, startOfDay.day).toUtc().toIso8601String();
    
    final response = await client
        .from('tokens')
        .select('*')
        .gte('created_at', todayStr)
        .order('created_at', ascending: false);

    return List<Map<String, dynamic>>.from(response);
  }

  /// Poll setting until Bluetooth printer scan is complete
  Future<List<Map<String, dynamic>>> scanBluetoothPrinters() async {
    // 1. Set Scan Request = "true"
    await updateSettings({'Scan Request': 'true'});

    // 2. Poll settings table for up to 15 attempts (30 seconds)
    int attempts = 0;
    while (attempts < 15) {
      await Future.delayed(const Duration(seconds: 2));
      attempts++;

      final settings = await getSettingsMap();
      if (settings['Scan Request'] != 'true') {
        // Scan has completed!
        final scannedPrintersJson = settings['Scanned Bluetooth Printers'] ?? '[]';
        try {
          final list = jsonDecode(scannedPrintersJson);
          if (list is List) {
            return List<Map<String, dynamic>>.from(list);
          }
        } catch (_) {}
        break;
      }
    }

    // If timeout, write Scan Request = "false"
    await updateSettings({'Scan Request': 'false'});
    throw TimeoutException("Bluetooth scan request timed out.");
  }

  /// Test database connection and basic operations
  Future<bool> testConnection() async {
    try {
      debugPrint("Testing Supabase connection...");
      
      // Test 1: Can we read from settings?
      final settingsTest = await client.from('settings').select('key').limit(1);
      debugPrint("✓ Settings table accessible");
      
      // Test 2: Can we read from tokens?
      final tokensTest = await client.from('tokens').select('id').limit(1);
      debugPrint("✓ Tokens table accessible");
      
      // Test 3: Check if admin credentials exist
      final adminTest = await client.from('settings').select('value').eq('key', 'Admin Username').maybeSingle();
      if (adminTest == null) {
        debugPrint("✗ Admin Username not found in settings");
        return false;
      }
      debugPrint("✓ Admin credentials found");
      
      debugPrint("✓ Database connection test PASSED");
      return true;
    } catch (e) {
      debugPrint("✗ Database connection test FAILED: $e");
      return false;
    }
  }
}
