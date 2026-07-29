import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:google_fonts/google_fonts.dart';
import '../providers/app_state.dart';

class SettingsView extends StatefulWidget {
  const SettingsView({super.key});

  @override
  State<SettingsView> createState() => _SettingsViewState();
}

class _SettingsViewState extends State<SettingsView> {
  // Core controllers
  final _startingTokenController = TextEditingController();
  final _avgServiceTimeController = TextEditingController();
  final _orgNameController = TextEditingController();
  bool _enableBuzzer = true;

  // Printer controllers
  String _printerConnection = 'wire';
  String _selectedPrinterMac = '';
  List<Map<String, dynamic>> _discoveredPrinters = [];
  bool _isBluetoothScanning = false;

  // Credential controllers
  final _adminUsernameController = TextEditingController();
  final _adminEmailController = TextEditingController();
  final _adminPasswordController = TextEditingController();
  
  // Email Provider controllers
  String _emailProvider = 'demo';
  final _resendApiKeyController = TextEditingController();
  final _resendSenderController = TextEditingController();
  final _smtpHostController = TextEditingController();
  final _smtpPortController = TextEditingController();
  final _smtpUsernameController = TextEditingController();
  final _smtpPasswordController = TextEditingController();
  final _smtpSenderController = TextEditingController();
  bool _smtpSsl = false;

  bool _isSavingCore = false;
  bool _isSavingCreds = false;
  bool _isSavingEmail = false;

  @override
  void initState() {
    super.initState();
    // Load initial values from AppState
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _loadSettingsFromState();
    });
  }

  void _loadSettingsFromState() {
    final state = Provider.of<AppState>(context, listen: false);
    final s = state.settings;

    debugPrint('Loading settings from state, total keys: ${s.length}');

    // Core settings
    _startingTokenController.text = s['Starting Token Number'] ?? '1';
    _avgServiceTimeController.text = s['Average Service Time'] ?? '10';
    _orgNameController.text = s['Organization Name'] ?? 'Smart Token Management System';
    _enableBuzzer = (s['Enable Buzzer'] ?? 'true') == 'true';

    // Printer settings
    try {
      final printerString = s['Thermal Printer Settings'] ?? '{"connection":"wire","device":""}';
      final Map<String, dynamic> printerConfig = jsonDecode(printerString);
      _printerConnection = printerConfig['connection'] ?? 'wire';
      _selectedPrinterMac = printerConfig['device'] ?? '';
      debugPrint('✓ Printer settings loaded: connection=$_printerConnection');
    } catch (e) {
      debugPrint('✗ Error parsing printer settings: $e');
    }

    // Scanned printers
    try {
      final listJson = s['Scanned Bluetooth Printers'] ?? '[]';
      final list = jsonDecode(listJson);
      if (list is List) {
        _discoveredPrinters = List<Map<String, dynamic>>.from(list);
        debugPrint('✓ Discovered ${_discoveredPrinters.length} printers');
      }
    } catch (e) {
      debugPrint('✗ Error parsing printer list: $e');
    }

    // Admin credentials
    _adminUsernameController.text = s['Admin Username'] ?? 'admin';
    _adminEmailController.text = s['Admin Email'] ?? 'admin@example.com';
    _adminPasswordController.text = s['Admin Password'] ?? 'admin123';
    debugPrint('✓ Admin credentials loaded');

    // Email configuration
    try {
      final configString = s['Email Service Config'] ?? '{"provider":"demo"}';
      final Map<String, dynamic> config = jsonDecode(configString);
      _emailProvider = config['provider'] ?? 'demo';
      _resendApiKeyController.text = config['resend_api_key'] ?? '';
      _resendSenderController.text = config['resend_sender'] ?? 'onboarding@resend.dev';
      _smtpHostController.text = config['smtp_host'] ?? '';
      _smtpPortController.text = config['smtp_port']?.toString() ?? '587';
      _smtpUsernameController.text = config['smtp_username'] ?? '';
      _smtpPasswordController.text = config['smtp_password'] ?? '';
      _smtpSenderController.text = config['smtp_sender'] ?? 'noreply@yourdomain.com';
      _smtpSsl = config['smtp_ssl'] ?? false;
      debugPrint('✓ Email config loaded: provider=$_emailProvider');
    } catch (e) {
      debugPrint('✗ Error parsing email config: $e');
    }

    setState(() {});
  }

  @override
  void dispose() {
    _startingTokenController.dispose();
    _avgServiceTimeController.dispose();
    _orgNameController.dispose();
    _adminUsernameController.dispose();
    _adminEmailController.dispose();
    _adminPasswordController.dispose();
    _resendApiKeyController.dispose();
    _resendSenderController.dispose();
    _smtpHostController.dispose();
    _smtpPortController.dispose();
    _smtpUsernameController.dispose();
    _smtpPasswordController.dispose();
    _smtpSenderController.dispose();
    super.dispose();
  }

  Future<void> _saveCoreSettings(AppState state) async {
    setState(() => _isSavingCore = true);
    try {
      // Validate inputs
      if (_startingTokenController.text.isEmpty) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('Starting Token Number cannot be empty'), backgroundColor: Colors.redAccent),
          );
        }
        setState(() => _isSavingCore = false);
        return;
      }

      final printerSettings = jsonEncode({
        'connection': _printerConnection,
        'device': _selectedPrinterMac,
      });

      await state.saveSettings({
        'Starting Token Number': _startingTokenController.text.trim(),
        'Average Service Time': _avgServiceTimeController.text.trim().isEmpty ? '10' : _avgServiceTimeController.text.trim(),
        'Organization Name': _orgNameController.text.trim(),
        'Enable Buzzer': _enableBuzzer ? 'true' : 'false',
        'Thermal Printer Settings': printerSettings,
      });

      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('✓ Core system parameters updated successfully!'),
            backgroundColor: Colors.green,
            duration: Duration(seconds: 2),
          ),
        );
      }
      debugPrint('✓ Core settings saved successfully');
    } catch (e) {
      debugPrint('✗ Error saving core settings: $e');
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('✗ Error: $e'), backgroundColor: Colors.redAccent),
        );
      }
    } finally {
      if (mounted) setState(() => _isSavingCore = false);
    }
  }

  Future<void> _triggerBluetoothScan(AppState state) async {
    setState(() {
      _isBluetoothScanning = true;
      _discoveredPrinters = [];
    });

    try {
      final list = await state.triggerBluetoothPrinterScan();
      setState(() {
        _discoveredPrinters = list;
      });
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Discovered ${list.length} Bluetooth devices.'), backgroundColor: Colors.indigo),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Bluetooth scan failed: $e'), backgroundColor: Colors.redAccent),
        );
      }
    } finally {
      if (mounted) setState(() => _isBluetoothScanning = false);
    }
  }

  Future<void> _saveCredentials(AppState state) async {
    setState(() => _isSavingCreds = true);
    try {
      // Validate inputs
      if (_adminUsernameController.text.isEmpty) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('Admin username cannot be empty'), backgroundColor: Colors.redAccent),
          );
        }
        setState(() => _isSavingCreds = false);
        return;
      }

      if (_adminPasswordController.text.isEmpty) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('Admin password cannot be empty'), backgroundColor: Colors.redAccent),
          );
        }
        setState(() => _isSavingCreds = false);
        return;
      }

      if (!_adminEmailController.text.contains('@')) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('Please enter a valid email address'), backgroundColor: Colors.redAccent),
          );
        }
        setState(() => _isSavingCreds = false);
        return;
      }

      await state.saveSettings({
        'Admin Username': _adminUsernameController.text.trim(),
        'Admin Email': _adminEmailController.text.trim(),
        'Admin Password': _adminPasswordController.text,
      });

      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('✓ Admin credentials updated successfully!'),
            backgroundColor: Colors.green,
            duration: Duration(seconds: 2),
          ),
        );
      }
      debugPrint('✓ Credentials saved successfully');
    } catch (e) {
      debugPrint('✗ Error saving credentials: $e');
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('✗ Error: $e'), backgroundColor: Colors.redAccent),
        );
      }
    } finally {
      if (mounted) setState(() => _isSavingCreds = false);
    }
  }

  Future<void> _saveEmailSettings(AppState state) async {
    setState(() => _isSavingEmail = true);
    try {
      // Validate SMTP settings if selected
      if (_emailProvider == 'smtp') {
        if (_smtpHostController.text.isEmpty || _smtpUsernameController.text.isEmpty || _smtpPasswordController.text.isEmpty) {
          if (mounted) {
            ScaffoldMessenger.of(context).showSnackBar(
              const SnackBar(content: Text('Please fill in all SMTP fields'), backgroundColor: Colors.redAccent),
            );
          }
          setState(() => _isSavingEmail = false);
          return;
        }
      }

      // Validate Resend settings if selected
      if (_emailProvider == 'resend') {
        if (_resendApiKeyController.text.isEmpty || _resendSenderController.text.isEmpty) {
          if (mounted) {
            ScaffoldMessenger.of(context).showSnackBar(
              const SnackBar(content: Text('Please fill in all Resend fields'), backgroundColor: Colors.redAccent),
            );
          }
          setState(() => _isSavingEmail = false);
          return;
        }
      }

      final configJson = jsonEncode({
        'provider': _emailProvider,
        'resend_api_key': _resendApiKeyController.text.trim(),
        'resend_sender': _resendSenderController.text.trim(),
        'smtp_host': _smtpHostController.text.trim(),
        'smtp_port': int.tryParse(_smtpPortController.text) ?? 587,
        'smtp_username': _smtpUsernameController.text.trim(),
        'smtp_password': _smtpPasswordController.text,
        'smtp_sender': _smtpSenderController.text.trim(),
        'smtp_ssl': _smtpSsl,
      });

      await state.saveSettings({'Email Service Config': configJson});

      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('✓ Email notification service configured successfully!'),
            backgroundColor: Colors.green,
            duration: Duration(seconds: 2),
          ),
        );
      }
      debugPrint('✓ Email settings saved successfully');
    } catch (e) {
      debugPrint('✗ Error saving email settings: $e');
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('✗ Error: $e'), backgroundColor: Colors.redAccent),
        );
      }
    } finally {
      if (mounted) setState(() => _isSavingEmail = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final state = Provider.of<AppState>(context, listen: true);
    
    // Reload settings when state changes (from database updates)
    if (state.settings.isNotEmpty && _adminUsernameController.text.isEmpty) {
      _loadSettingsFromState();
    }

    final size = MediaQuery.of(context).size;
    final isDesktop = size.width > 900;

    Widget buildPanelWrapper(String title, List<Widget> children) {
      return Container(
        margin: const EdgeInsets.only(bottom: 24),
        padding: const EdgeInsets.all(24),
        decoration: BoxDecoration(
          color: const Color(0xFF0F1426),
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: const Color(0xFF1E293B)),
          boxShadow: [
            BoxShadow(color: Colors.black.withOpacity(0.1), blurRadius: 10, offset: const Offset(0, 4)),
          ],
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Row(
              children: [
                Container(
                  width: 4,
                  height: 18,
                  decoration: BoxDecoration(
                    gradient: const LinearGradient(
                      colors: [Color(0xFF6366F1), Color(0xFF8B5CF6)],
                      begin: Alignment.topCenter,
                      end: Alignment.bottomCenter,
                    ),
                    borderRadius: BorderRadius.circular(2),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Text(
                    title,
                    style: GoogleFonts.outfit(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold),
                  ),
                ),
              ],
            ),
            const Divider(color: Color(0xFF1E293B), height: 32),
            ...children,
          ],
        ),
      );
    }

    Widget buildSaveButton(String label, VoidCallback onPressed, bool isLoading) {
      return Container(
        width: double.infinity,
        height: 48,
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(8),
        ),
        child: ElevatedButton(
          onPressed: isLoading ? null : onPressed,
          style: ElevatedButton.styleFrom(
            backgroundColor: Colors.transparent,
            foregroundColor: Colors.white,
            padding: EdgeInsets.zero,
            elevation: 0,
            shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
          ),
          child: Ink(
            decoration: BoxDecoration(
              gradient: const LinearGradient(
                colors: [Color(0xFF6366F1), Color(0xFF8B5CF6)],
                begin: Alignment.topLeft,
                end: Alignment.bottomRight,
              ),
              borderRadius: BorderRadius.circular(8),
              boxShadow: [
                BoxShadow(
                  color: const Color(0xFF6366F1).withOpacity(0.3),
                  blurRadius: 10,
                  offset: const Offset(0, 3),
                ),
              ],
            ),
            child: Container(
              alignment: Alignment.center,
              child: isLoading
                  ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2, color: Colors.white))
                  : Text(label, style: GoogleFonts.inter(fontWeight: FontWeight.bold, fontSize: 15, color: Colors.white)),
            ),
          ),
        ),
      );
    }

    Widget buildCoreSettingsPanel() {
      return buildPanelWrapper(
        'Queue Parameters & General Settings',
        [
          Row(
            children: [
              Expanded(
                child: TextField(
                  controller: _startingTokenController,
                  style: const TextStyle(color: Colors.white),
                  keyboardType: TextInputType.number,
                  decoration: InputDecoration(
                    labelText: 'Starting Token Number',
                    labelStyle: const TextStyle(color: Colors.white60),
                    filled: true,
                    fillColor: const Color(0xFF090D1A),
                    enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
                    focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
                  ),
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: TextField(
                  controller: _avgServiceTimeController,
                  style: const TextStyle(color: Colors.white),
                  keyboardType: TextInputType.number,
                  decoration: InputDecoration(
                    labelText: 'Average Service Time (mins)',
                    labelStyle: const TextStyle(color: Colors.white60),
                    filled: true,
                    fillColor: const Color(0xFF090D1A),
                    enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
                    focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
                  ),
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          TextField(
            controller: _orgNameController,
            style: const TextStyle(color: Colors.white),
            decoration: InputDecoration(
              labelText: 'Organization Name',
              labelStyle: const TextStyle(color: Colors.white60),
              filled: true,
              fillColor: const Color(0xFF090D1A),
              enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
              focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
            ),
          ),
          const SizedBox(height: 20),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('Audio Buzz Announcement', style: GoogleFonts.inter(color: Colors.white, fontSize: 14, fontWeight: FontWeight.w600)),
                  Text('Play chime sound at consoles when calling tokens.', style: GoogleFonts.inter(color: Colors.white60, fontSize: 12)),
                ],
              ),
              Switch(
                value: _enableBuzzer,
                activeColor: const Color(0xFF818CF8),
                activeTrackColor: const Color(0xFF6366F1).withOpacity(0.5),
                onChanged: (val) => setState(() => _enableBuzzer = val),
              )
            ],
          ),
          const SizedBox(height: 20),
          DropdownButtonFormField<String>(
            value: _printerConnection,
            dropdownColor: const Color(0xFF0F1426),
            style: const TextStyle(color: Colors.white),
            decoration: InputDecoration(
              labelText: 'Thermal Ticket Printer Interface',
              labelStyle: const TextStyle(color: Colors.white60),
              filled: true,
              fillColor: const Color(0xFF090D1A),
              enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
              focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
            ),
            items: const [
              DropdownMenuItem(value: 'wire', child: Text('USB Serial Port Connection')),
              DropdownMenuItem(value: 'bluetooth', child: Text('Bluetooth Wireless SPP Connection')),
            ],
            onChanged: (val) {
              if (val != null) setState(() => _printerConnection = val);
            },
          ),
          if (_printerConnection == 'bluetooth') ...[
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: DropdownButtonFormField<String>(
                    value: _selectedPrinterMac.isEmpty ? null : _selectedPrinterMac,
                    dropdownColor: const Color(0xFF0F1426),
                    style: const TextStyle(color: Colors.white),
                    decoration: InputDecoration(
                      labelText: 'Select Discovered Printer',
                      labelStyle: const TextStyle(color: Colors.white60),
                      filled: true,
                      fillColor: const Color(0xFF090D1A),
                      enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
                      focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
                    ),
                    items: [
                      const DropdownMenuItem(value: '', child: Text('-- No Devices Discovered --')),
                      ..._discoveredPrinters.map((p) {
                        final address = p['address'] ?? '';
                        final name = p['name'] ?? 'Unknown Device';
                        return DropdownMenuItem(value: address, child: Text('$name ($address)'));
                      })
                    ],
                    onChanged: (val) {
                      if (val != null) setState(() => _selectedPrinterMac = val);
                    },
                  ),
                ),
                const SizedBox(width: 12),
                ElevatedButton(
                  onPressed: _isBluetoothScanning ? null : () => _triggerBluetoothScan(state),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: const Color(0xFF6366F1),
                    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
                    padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 18),
                  ),
                  child: _isBluetoothScanning
                      ? const SizedBox(width: 16, height: 16, child: CircularProgressIndicator(strokeWidth: 2, color: Colors.white))
                      : const Text('Scan Devices'),
                )
              ],
            )
          ],
          const SizedBox(height: 24),
          buildSaveButton(
            'Save General Settings',
            () => _saveCoreSettings(state),
            _isSavingCore,
          ),
        ],
      );
    }

    Widget buildCredentialsPanel() {
      return buildPanelWrapper(
        'Admin Username & Recovery Account Settings',
        [
          TextField(
            controller: _adminUsernameController,
            style: const TextStyle(color: Colors.white),
            decoration: InputDecoration(
              labelText: 'Admin Login Username',
              labelStyle: const TextStyle(color: Colors.white60),
              filled: true,
              fillColor: const Color(0xFF090D1A),
              enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
              focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
            ),
          ),
          const SizedBox(height: 16),
          TextField(
            controller: _adminEmailController,
            style: const TextStyle(color: Colors.white),
            decoration: InputDecoration(
              labelText: 'OTP Recovery Email Address',
              labelStyle: const TextStyle(color: Colors.white60),
              filled: true,
              fillColor: const Color(0xFF090D1A),
              enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
              focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
            ),
          ),
          const SizedBox(height: 16),
          TextField(
            controller: _adminPasswordController,
            style: const TextStyle(color: Colors.white),
            obscureText: true,
            decoration: InputDecoration(
              labelText: 'Admin Password',
              labelStyle: const TextStyle(color: Colors.white60),
              filled: true,
              fillColor: const Color(0xFF090D1A),
              enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
              focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
            ),
          ),
          const SizedBox(height: 24),
          buildSaveButton(
            'Save Account Details',
            () => _saveCredentials(state),
            _isSavingCreds,
          ),
        ],
      );
    }

    Widget buildEmailDispatcherPanel() {
      return buildPanelWrapper(
        'Forgot Password Notification Service Settings',
        [
          DropdownButtonFormField<String>(
            value: _emailProvider,
            dropdownColor: const Color(0xFF0F1426),
            style: const TextStyle(color: Colors.white),
            decoration: InputDecoration(
              labelText: 'OTP Dispatcher Provider',
              labelStyle: const TextStyle(color: Colors.white60),
              filled: true,
              fillColor: const Color(0xFF090D1A),
              enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
              focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
            ),
            items: const [
              DropdownMenuItem(value: 'demo', child: Text('Console Mode (Log/Debug Print Only)')),
              DropdownMenuItem(value: 'resend', child: Text('Resend email API Gateway')),
              DropdownMenuItem(value: 'smtp', child: Text('SMTP Server Connection')),
            ],
            onChanged: (val) {
              if (val != null) setState(() => _emailProvider = val);
            },
          ),
          const SizedBox(height: 16),
          if (_emailProvider == 'resend') ...[
            TextField(
              controller: _resendApiKeyController,
              style: const TextStyle(color: Colors.white),
              decoration: InputDecoration(
                labelText: 'Resend API Authorization Key (re_...)',
                labelStyle: const TextStyle(color: Colors.white60),
                filled: true,
                fillColor: const Color(0xFF090D1A),
                enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
                focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
              ),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: _resendSenderController,
              style: const TextStyle(color: Colors.white),
              decoration: InputDecoration(
                labelText: 'Sender Email Address (e.g. name@resend.dev)',
                labelStyle: const TextStyle(color: Colors.white60),
                filled: true,
                fillColor: const Color(0xFF090D1A),
                enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
                focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
              ),
            ),
          ] else if (_emailProvider == 'smtp') ...[
            Row(
              children: [
                Expanded(
                  flex: 3,
                  child: TextField(
                    controller: _smtpHostController,
                    style: const TextStyle(color: Colors.white),
                    decoration: InputDecoration(
                      labelText: 'SMTP Server Hostname',
                      labelStyle: const TextStyle(color: Colors.white60),
                      filled: true,
                      fillColor: const Color(0xFF090D1A),
                      enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
                      focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
                    ),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: TextField(
                    controller: _smtpPortController,
                    style: const TextStyle(color: Colors.white),
                    keyboardType: TextInputType.number,
                    decoration: InputDecoration(
                      labelText: 'Port',
                      labelStyle: const TextStyle(color: Colors.white60),
                      filled: true,
                      fillColor: const Color(0xFF090D1A),
                      enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
                      focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            TextField(
              controller: _smtpUsernameController,
              style: const TextStyle(color: Colors.white),
              decoration: InputDecoration(
                labelText: 'SMTP Login Username',
                labelStyle: const TextStyle(color: Colors.white60),
                filled: true,
                fillColor: const Color(0xFF090D1A),
                enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
                focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
              ),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: _smtpPasswordController,
              style: const TextStyle(color: Colors.white),
              obscureText: true,
              decoration: InputDecoration(
                labelText: 'SMTP Login Password',
                labelStyle: const TextStyle(color: Colors.white60),
                filled: true,
                fillColor: const Color(0xFF090D1A),
                enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
                focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
              ),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: _smtpSenderController,
              style: const TextStyle(color: Colors.white),
              decoration: InputDecoration(
                labelText: 'SMTP Sender Email (From)',
                labelStyle: const TextStyle(color: Colors.white60),
                filled: true,
                fillColor: const Color(0xFF090D1A),
                enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(10)),
                focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
              ),
            ),
            const SizedBox(height: 16),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                const Text('Force SSL Connection (Secure TLS)', style: TextStyle(color: Colors.white70, fontSize: 14)),
                Switch(
                  value: _smtpSsl,
                  activeColor: const Color(0xFF818CF8),
                  activeTrackColor: const Color(0xFF6366F1).withOpacity(0.5),
                  onChanged: (val) => setState(() => _smtpSsl = val),
                ),
              ],
            ),
          ] else ...[
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
              decoration: BoxDecoration(
                color: Colors.blue.withOpacity(0.08),
                borderRadius: BorderRadius.circular(10),
                border: Border.all(color: Colors.blue.withOpacity(0.2)),
              ),
              child: Text(
                'Demo Mode is selected. Reset OTP codes will print in your terminal logs directly. No setup required.',
                style: GoogleFonts.inter(color: Colors.blueAccent[100], fontSize: 13),
              ),
            ),
          ],
          const SizedBox(height: 24),
          buildSaveButton(
            'Save Notification Settings',
            () => _saveEmailSettings(state),
            _isSavingEmail,
          ),
        ],
      );
    }

    return Scaffold(
      backgroundColor: const Color(0xFF090D1A),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'System Settings',
                  style: GoogleFonts.outfit(color: Colors.white, fontSize: 24, fontWeight: FontWeight.bold),
                ),
                const SizedBox(height: 4),
                Text(
                  'Customize token printer configurations, recovery notification gateways, and credentials.',
                  style: GoogleFonts.inter(color: Colors.white60, fontSize: 13),
                ),
              ],
            ),
            const SizedBox(height: 32),
            // Responsive layout
            isDesktop
                ? Row(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      // Left column
                      Expanded(
                        flex: 1,
                        child: buildCoreSettingsPanel(),
                      ),
                      const SizedBox(width: 24),
                      // Right column
                      Expanded(
                        flex: 1,
                        child: Column(
                          children: [
                            buildCredentialsPanel(),
                            buildEmailDispatcherPanel(),
                          ],
                        ),
                      ),
                    ],
                  )
                : Column(
                    children: [
                      buildCoreSettingsPanel(),
                      buildCredentialsPanel(),
                      buildEmailDispatcherPanel(),
                    ],
                  ),
          ],
        ),
      ),
    );
  }
}
