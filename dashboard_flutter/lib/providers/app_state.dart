import 'dart:async';
import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../services/api_service.dart';

class AppState extends ChangeNotifier {
  final ApiService _api = ApiService();

  bool _isInitialized = false;
  bool get isInitialized => _isInitialized;

  bool _isLoggedIn = false;
  bool get isLoggedIn => _isLoggedIn;

  Map<String, String> _settings = {};
  Map<String, String> get settings => _settings;

  List<Map<String, dynamic>> _activeTokens = [];
  List<Map<String, dynamic>> get activeTokens => _activeTokens;

  List<Map<String, dynamic>> _todayTokens = [];
  List<Map<String, dynamic>> get todayTokens => _todayTokens;

  // Stats cache
  int get currentlyServingNum => int.tryParse(_settings['Current Serving Token'] ?? '0') ?? 0;
  int get waitingQueueCount => _activeTokens.where((t) => t['status'] == 'Waiting').length;
  int get completedTodayCount => _todayTokens.where((t) => t['status'] == 'Completed').length;
  int get onlineBookingsCount => _activeTokens.where((t) => t['source'] == 'Online').length;
  int get walkInCount => _activeTokens.where((t) => t['source'] == 'Manual').length;

  // Forgot password flow states
  String _maskedEmail = '';
  String get maskedEmail => _maskedEmail;

  bool _isOtpSent = false;
  bool get isOtpSent => _isOtpSent;

  bool _isOtpVerified = false;
  bool get isOtpVerified => _isOtpVerified;

  String _tempUsername = '';

  // Stream Subscriptions
  StreamSubscription? _settingsSub;
  StreamSubscription? _tokensSub;

  AppState() {
    _initApp();
  }

  Future<void> _initApp() async {
    try {
      await _api.initialize();
      
      final prefs = await SharedPreferences.getInstance();
      _isLoggedIn = prefs.getBool('is_logged_in') ?? false;

      // Wait a moment for initialization to complete
      await Future.delayed(const Duration(milliseconds: 500));

      // Start listening to settings and tokens real-time streams
      _listenToDatabase();

      _isInitialized = true;
      notifyListeners();
      debugPrint("✓ App initialization completed successfully");
    } catch (e) {
      debugPrint("✗ AppState Initialization Error: $e");
      _isInitialized = true; // Still mark as initialized to prevent stuck state
      notifyListeners();
    }
  }

  void _listenToDatabase() {
    // Listen to Settings changes
    _settingsSub?.cancel();
    _settingsSub = _api.getSettingsStream().listen(
      (list) {
        _settings = {for (var item in list) item['key'].toString(): item['value'].toString()};
        notifyListeners();
        debugPrint("✓ Settings updated from stream: ${_settings.keys.length} keys");
      },
      onError: (error) {
        debugPrint("✗ Settings stream error: $error");
        // Try to reconnect after delay
        Future.delayed(const Duration(seconds: 5), _listenToDatabase);
      },
    );

    // Listen to active tokens changes
    _tokensSub?.cancel();
    _tokensSub = _api.getActiveTokensStream().listen(
      (list) {
        _activeTokens = list;
        _refreshTodayTokens(); // Update logs history
        notifyListeners();
        debugPrint("✓ Active tokens updated: ${_activeTokens.length} tokens");
      },
      onError: (error) {
        debugPrint("✗ Active tokens stream error: $error");
        // Try to reconnect after delay
        Future.delayed(const Duration(seconds: 5), _listenToDatabase);
      },
    );
  }

  Future<void> _refreshTodayTokens() async {
    try {
      _todayTokens = await _api.getTodayTokens();
      notifyListeners();
    } catch (e) {
      debugPrint("Refresh today tokens error: $e");
    }
  }

  // --- Auth Actions ---

  Future<bool> login(String username, String password) async {
    final success = await _api.verifyLogin(username, password);
    if (success) {
      _isLoggedIn = true;
      final prefs = await SharedPreferences.getInstance();
      await prefs.setBool('is_logged_in', true);
      notifyListeners();
    }
    return success;
  }

  Future<void> logout() async {
    _isLoggedIn = false;
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool('is_logged_in', false);
    notifyListeners();
  }

  // --- Forgot Password OTP Flow State Actions ---

  Future<void> sendPasswordResetOTP(String username) async {
    _maskedEmail = await _api.requestPasswordResetOTP(username);
    _tempUsername = username;
    _isOtpSent = true;
    _isOtpVerified = false;
    notifyListeners();
  }

  Future<bool> verifyResetOTP(String otp) async {
    final isValid = await _api.verifyResetOTP(otp);
    if (isValid) {
      _isOtpVerified = true;
      notifyListeners();
    }
    return isValid;
  }

  Future<void> resetCredentials(String newPassword, {String? newUsername}) async {
    final usernameToUse = newUsername ?? _tempUsername;
    await _api.updateCredentials(usernameToUse, newPassword);
    
    // Log user in automatically on successful change
    _isLoggedIn = true;
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool('is_logged_in', true);

    // Reset OTP screen state flags
    _isOtpSent = false;
    _isOtpVerified = false;
    _maskedEmail = '';
    _tempUsername = '';
    notifyListeners();
  }

  void cancelResetFlow() {
    _isOtpSent = false;
    _isOtpVerified = false;
    _maskedEmail = '';
    _tempUsername = '';
    notifyListeners();
  }

  // --- Queue Actions ---

  Future<Map<String, dynamic>> generateWalkinToken(String name, String serviceType) async {
    final result = await _api.generateToken({
      'name': name.isEmpty ? 'Walk-In Customer' : name,
      'serviceType': serviceType,
      'source': 'Manual',
      'phone': '-',
      'email': '-',
      'remarks': 'Generated from Flutter Dashboard',
    });
    _refreshTodayTokens();
    return result;
  }

  Future<void> callNextToken() async {
    await _api.nextToken();
    _refreshTodayTokens();
  }

  Future<void> skipToken(int tokenNum) async {
    await _api.skipToken(tokenNum);
    _refreshTodayTokens();
  }

  Future<void> completeToken(int tokenNum) async {
    await _api.completeToken(tokenNum);
    _refreshTodayTokens();
  }

  // --- Settings Actions ---

  Future<void> saveSettings(Map<String, String> updates) async {
    await _api.updateSettings(updates);
  }

  Future<List<Map<String, dynamic>>> triggerBluetoothPrinterScan() async {
    return await _api.scanBluetoothPrinters();
  }

  /// Test database connection (for debugging)
  Future<bool> testDatabaseConnection() async {
    return await _api.testConnection();
  }

  @override
  void dispose() {
    _settingsSub?.cancel();
    _tokensSub?.cancel();
    super.dispose();
  }
}
