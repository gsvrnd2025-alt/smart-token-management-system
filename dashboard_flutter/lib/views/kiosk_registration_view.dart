import 'dart:ui';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:qr_flutter/qr_flutter.dart';
import '../providers/app_state.dart';

class KioskRegistrationView extends StatefulWidget {
  const KioskRegistrationView({super.key});

  @override
  State<KioskRegistrationView> createState() => _KioskRegistrationViewState();
}

class _KioskRegistrationViewState extends State<KioskRegistrationView> {
  final _formKey = GlobalKey<FormState>();
  final _nameController = TextEditingController();
  final _phoneController = TextEditingController();
  final _emailController = TextEditingController();
  
  String _selectedServiceType = 'General Service';
  bool _isLoading = false;
  Map<String, dynamic>? _generatedTicket;

  final List<String> _serviceTypes = [
    'General Service',
    'Consultation',
    'Enquiry',
    'Premium Service',
  ];

  @override
  void dispose() {
    _nameController.dispose();
    _phoneController.dispose();
    _emailController.dispose();
    super.dispose();
  }

  Future<void> _handleRegister(AppState state) async {
    if (!_formKey.currentState!.validate()) return;

    setState(() {
      _isLoading = true;
      _generatedTicket = null;
    });

    try {
      final details = {
        'name': _nameController.text.trim(),
        'phone': _phoneController.text.trim(),
        'email': _emailController.text.trim(),
        'serviceType': _selectedServiceType,
        'source': 'Online',
        'remarks': 'Registered at Self-Service Kiosk',
      };

      // Call database generate token
      final result = await state.generateWalkinToken(
        details['name'] ?? 'Guest',
        _selectedServiceType,
      );

      // Fetch base URL or fallback to GitHub pages
      String hostUrl = 'https://monaskumar.github.io/smart-token-management-system';
      if (kIsWeb) {
        final uri = Uri.parse(Uri.base.toString());
        hostUrl = "${uri.scheme}://${uri.host}:${uri.port}";
      }

      final tokenNum = result['tokenNumber'];
      final qrUrl = "$hostUrl/token-status.html?token=$tokenNum";

      setState(() {
        _generatedTicket = {
          'tokenNumber': tokenNum,
          'customerName': result['customerName'],
          'serviceType': result['serviceType'],
          'estimatedWaitingTimeMinutes': result['estimatedWaitingTimeMinutes'],
          'qrUrl': qrUrl,
        };
        // Reset inputs
        _nameController.clear();
        _phoneController.clear();
        _emailController.clear();
        _selectedServiceType = 'General Service';
      });
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Registration failed: $e'), backgroundColor: Colors.redAccent),
        );
      }
    } finally {
      if (mounted) setState(() => _isLoading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final state = Provider.of<AppState>(context);
    final size = MediaQuery.of(context).size;
    final isDesktop = size.width > 900;
    
    final orgName = state.settings['Organization Name'] ?? 'Smart Token Kiosk';

    Widget buildFormCard() {
      return Container(
        padding: const EdgeInsets.all(32),
        decoration: BoxDecoration(
          color: const Color(0xFF1E293B).withOpacity(0.4),
          borderRadius: BorderRadius.circular(24),
          border: Border.all(color: Colors.white.withOpacity(0.08)),
          boxShadow: [
            BoxShadow(color: Colors.black.withOpacity(0.3), blurRadius: 30, offset: const Offset(0, 15)),
          ],
        ),
        child: Form(
          key: _formKey,
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                'New Queue Ticket',
                style: GoogleFonts.outfit(color: Colors.white, fontSize: 22, fontWeight: FontWeight.bold),
              ),
              const SizedBox(height: 6),
              Text(
                'Please fill in details to join waitlist.',
                style: GoogleFonts.inter(color: Colors.white60, fontSize: 13),
              ),
              const Divider(color: Color(0xFF334155), height: 32),
              
              // Name Input
              TextFormField(
                controller: _nameController,
                style: const TextStyle(color: Colors.white),
                validator: (val) => val == null || val.trim().isEmpty ? 'Enter your name' : null,
                decoration: InputDecoration(
                  prefixIcon: const Icon(Icons.person_outline, color: Colors.white60),
                  labelText: 'Customer Name',
                  labelStyle: const TextStyle(color: Colors.white60),
                  enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF334155)), borderRadius: BorderRadius.circular(10)),
                  focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
                ),
              ),
              const SizedBox(height: 16),
              
              // Phone Input
              TextFormField(
                controller: _phoneController,
                style: const TextStyle(color: Colors.white),
                keyboardType: TextInputType.phone,
                validator: (val) => val == null || val.trim().isEmpty ? 'Enter phone number' : null,
                decoration: InputDecoration(
                  prefixIcon: const Icon(Icons.phone_outlined, color: Colors.white60),
                  labelText: 'Phone Number',
                  labelStyle: const TextStyle(color: Colors.white60),
                  enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF334155)), borderRadius: BorderRadius.circular(10)),
                  focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
                ),
              ),
              const SizedBox(height: 16),

              // Email Input
              TextFormField(
                controller: _emailController,
                style: const TextStyle(color: Colors.white),
                keyboardType: TextInputType.emailAddress,
                decoration: InputDecoration(
                  prefixIcon: const Icon(Icons.email_outlined, color: Colors.white60),
                  labelText: 'Email Address (Optional)',
                  labelStyle: const TextStyle(color: Colors.white60),
                  enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF334155)), borderRadius: BorderRadius.circular(10)),
                  focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
                ),
              ),
              const SizedBox(height: 16),

              // Dropdown
              DropdownButtonFormField<String>(
                value: _selectedServiceType,
                style: const TextStyle(color: Colors.white),
                dropdownColor: const Color(0xFF1E293B),
                decoration: InputDecoration(
                  prefixIcon: const Icon(Icons.design_services_outlined, color: Colors.white60),
                  labelText: 'Select Service Counters',
                  labelStyle: const TextStyle(color: Colors.white60),
                  enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF334155)), borderRadius: BorderRadius.circular(10)),
                  focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(10)),
                ),
                items: _serviceTypes.map((type) {
                  return DropdownMenuItem(value: type, child: Text(type));
                }).toList(),
                onChanged: (val) {
                  if (val != null) setState(() => _selectedServiceType = val);
                },
              ),
              const SizedBox(height: 32),

              ElevatedButton(
                onPressed: _isLoading ? null : () => _handleRegister(state),
                style: ElevatedButton.styleFrom(
                  backgroundColor: const Color(0xFF6366F1),
                  minimumSize: const Size(double.infinity, 54),
                  shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                ),
                child: _isLoading
                    ? const SizedBox(width: 24, height: 24, child: CircularProgressIndicator(strokeWidth: 2, color: Colors.white))
                    : Text('Register Waitlist', style: GoogleFonts.inter(color: Colors.white, fontSize: 16, fontWeight: FontWeight.bold)),
              ),
            ],
          ),
        ),
      );
    }

    Widget buildTicketCard() {
      final ticket = _generatedTicket!;
      final num = ticket['tokenNumber'];
      final type = ticket['serviceType'];
      final mins = ticket['estimatedWaitingTimeMinutes'];
      final qrUrl = ticket['qrUrl'];

      return Container(
        padding: const EdgeInsets.all(32),
        decoration: BoxDecoration(
          color: const Color(0xFF1E293B),
          borderRadius: BorderRadius.circular(24),
          border: Border.all(color: const Color(0xFF10B981).withOpacity(0.3)),
          boxShadow: [
            BoxShadow(color: Colors.black.withOpacity(0.3), blurRadius: 30, offset: const Offset(0, 15)),
          ],
        ),
        child: Column(
          children: [
            const Icon(Icons.check_circle, color: Color(0xFF10B981), size: 54),
            const SizedBox(height: 16),
            Text(
              'Registration Successful!',
              style: GoogleFonts.outfit(color: Colors.white, fontSize: 22, fontWeight: FontWeight.bold),
            ),
            const Divider(color: Color(0xFF334155), height: 32),
            Text(
              'YOUR TOKEN NUMBER IS',
              style: GoogleFonts.inter(color: Colors.white60, fontSize: 12, letterSpacing: 1),
            ),
            const SizedBox(height: 8),
            Text(
              '$num',
              style: GoogleFonts.outfit(color: Colors.white, fontSize: 64, fontWeight: FontWeight.w900),
            ),
            Text(
              type,
              style: GoogleFonts.inter(color: const Color(0xFF818CF8), fontSize: 15, fontWeight: FontWeight.w600),
            ),
            const SizedBox(height: 24),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(color: Colors.white, borderRadius: BorderRadius.circular(16)),
              child: QrImageView(
                data: qrUrl,
                version: QrVersions.auto,
                size: 160.0,
                gapless: false,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              'Scan to track queue live status',
              style: GoogleFonts.inter(color: Colors.white38, fontSize: 11),
            ),
            const SizedBox(height: 20),
            Text(
              'Estimated Waiting Time: $mins mins',
              style: GoogleFonts.inter(color: Colors.white70, fontSize: 14, fontWeight: FontWeight.w500),
            ),
            const SizedBox(height: 32),
            ElevatedButton(
              onPressed: () => setState(() => _generatedTicket = null),
              style: ElevatedButton.styleFrom(
                backgroundColor: const Color(0xFF0F172A),
                foregroundColor: Colors.white70,
                minimumSize: const Size(double.infinity, 48),
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
              ),
              child: const Text('Back to Home'),
            )
          ],
        ),
      );
    }

    return Scaffold(
      body: Container(
        width: double.infinity,
        height: double.infinity,
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            colors: [Color(0xFF0F172A), Color(0xFF1E1B4B), Color(0xFF311042)],
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
          ),
        ),
        child: SafeArea(
          child: Column(
            children: [
              // Header
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Row(
                      children: [
                        const Icon(Icons.layers, color: Color(0xFF818CF8), size: 28),
                        const SizedBox(width: 8),
                        Text(orgName, style: GoogleFonts.outfit(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold)),
                      ],
                    ),
                    IconButton(
                      icon: const Icon(Icons.close, color: Colors.white70),
                      onPressed: () => Navigator.pop(context),
                    )
                  ],
                ),
              ),
              // Body forms or ticket
              Expanded(
                child: Center(
                  child: SingleChildScrollView(
                    padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 20),
                    child: SizedBox(
                      width: isDesktop ? 460 : double.infinity,
                      child: ClipRRect(
                        borderRadius: BorderRadius.circular(24),
                        child: BackdropFilter(
                          filter: ImageFilter.blur(sigmaX: 12, sigmaY: 12),
                          child: _generatedTicket == null ? buildFormCard() : buildTicketCard(),
                        ),
                      ),
                    ),
                  ),
                ),
              )
            ],
          ),
        ),
      ),
    );
  }
}
