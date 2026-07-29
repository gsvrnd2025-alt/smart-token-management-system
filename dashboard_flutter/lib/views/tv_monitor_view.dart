import 'dart:async';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:flutter_tts/flutter_tts.dart';
import 'package:intl/intl.dart';
import '../providers/app_state.dart';

class TvMonitorView extends StatefulWidget {
  const TvMonitorView({super.key});

  @override
  State<TvMonitorView> createState() => _TvMonitorViewState();
}

class _TvMonitorViewState extends State<TvMonitorView> {
  final FlutterTts _flutterTts = FlutterTts();
  int _lastSpokenToken = 0;
  String _currentTime = '';
  late Timer _timer;

  @override
  void initState() {
    super.initState();
    _initTts();
    _updateClock();
    _timer = Timer.periodic(const Duration(seconds: 1), (timer) => _updateClock());
  }

  void _initTts() async {
    await _flutterTts.setLanguage("en-US");
    await _flutterTts.setPitch(1.0);
    await _flutterTts.setSpeechRate(0.45); // Sleek, clear speech speed
  }

  void _updateClock() {
    final now = DateTime.now();
    setState(() {
      _currentTime = DateFormat('hh:mm:ss a').format(now);
    });
  }

  Future<void> _announceToken(int tokenNum, String serviceType) async {
    if (tokenNum <= 0) return;
    _lastSpokenToken = tokenNum;
    
    final announcement = "Token number $tokenNum, please proceed to the console for $serviceType.";
    await _flutterTts.speak(announcement);
  }

  @override
  void dispose() {
    _timer.cancel();
    _flutterTts.stop();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final state = Provider.of<AppState>(context);
    final size = MediaQuery.of(context).size;
    final isDesktop = size.width > 900;

    final servingNum = state.currentlyServingNum;
    final orgName = state.settings['Organization Name'] ?? 'Queue Management System';

    // Retrieve details for currently serving
    Map<String, dynamic>? activeServingToken;
    if (servingNum > 0) {
      final matches = state.activeTokens.where((t) => t['token_number'] == servingNum);
      if (matches.isNotEmpty) {
        activeServingToken = matches.first;
      }
    }

    final currentName = activeServingToken?['customer_name'] ?? 'Walk-In Customer';
    final currentService = activeServingToken?['service_type'] ?? 'General Queue';

    // Detect if a new token has been called and speak it
    if (servingNum > 0 && servingNum != _lastSpokenToken) {
      _announceToken(servingNum, currentService);
    }

    // Get recently called/completed tokens (history)
    final recentTokens = state.todayTokens
        .where((t) => t['status'] == 'Completed' || t['status'] == 'Serving')
        .take(4)
        .toList();

    Widget buildLobbyHeader() {
      return Container(
        padding: const EdgeInsets.symmetric(horizontal: 32, vertical: 20),
        color: const Color(0xFF1E293B),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Row(
              children: [
                const Icon(Icons.layers, color: Color(0xFF6366F1), size: 36),
                const SizedBox(width: 16),
                Text(
                  orgName,
                  style: GoogleFonts.outfit(color: Colors.white, fontSize: 24, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 10),
              decoration: BoxDecoration(color: const Color(0xFF0F172A), borderRadius: BorderRadius.circular(12)),
              child: Text(
                _currentTime,
                style: GoogleFonts.shareTechMono(color: Colors.cyanAccent, fontSize: 24, fontWeight: FontWeight.bold),
              ),
            )
          ],
        ),
      );
    }

    Widget buildMainDisplay() {
      return Expanded(
        flex: 7,
        child: Container(
          margin: const EdgeInsets.all(24),
          padding: const EdgeInsets.all(40),
          decoration: BoxDecoration(
            color: const Color(0xFF1E293B),
            borderRadius: BorderRadius.circular(28),
            border: Border.all(color: const Color(0xFF334155), width: 1.5),
            boxShadow: [
              BoxShadow(color: Colors.black.withOpacity(0.4), blurRadius: 40, offset: const Offset(0, 10)),
            ],
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text(
                'NOW SERVING',
                style: GoogleFonts.inter(
                  color: const Color(0xFF818CF8),
                  fontSize: 20,
                  fontWeight: FontWeight.w800,
                  letterSpacing: 3,
                ),
              ),
              const SizedBox(height: 16),
              Container(
                width: double.infinity,
                padding: const EdgeInsets.symmetric(vertical: 40),
                decoration: BoxDecoration(
                  color: const Color(0xFF0F172A),
                  borderRadius: BorderRadius.circular(24),
                  border: Border.all(color: const Color(0xFF6366F1).withOpacity(0.2)),
                ),
                child: Center(
                  child: Text(
                    servingNum > 0 ? '$servingNum' : 'WAITING',
                    style: GoogleFonts.outfit(
                      color: servingNum > 0 ? Colors.white : Colors.white38,
                      fontSize: 160,
                      fontWeight: FontWeight.w900,
                      height: 1,
                      shadows: servingNum > 0
                          ? [
                              BoxShadow(color: const Color(0xFF6366F1).withOpacity(0.5), blurRadius: 50),
                            ]
                          : [],
                    ),
                  ),
                ),
              ),
              const SizedBox(height: 32),
              Text(
                currentName,
                style: GoogleFonts.outfit(color: Colors.white, fontSize: 36, fontWeight: FontWeight.bold),
              ),
              const SizedBox(height: 8),
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
                decoration: BoxDecoration(
                  color: const Color(0xFF6366F1).withOpacity(0.15),
                  borderRadius: BorderRadius.circular(30),
                  border: Border.all(color: const Color(0xFF6366F1).withOpacity(0.3)),
                ),
                child: Text(
                  currentService,
                  style: GoogleFonts.inter(color: const Color(0xFF818CF8), fontSize: 18, fontWeight: FontWeight.bold),
                ),
              )
            ],
          ),
        ),
      );
    }

    Widget buildRecentTokensPanel() {
      return Container(
        width: isDesktop ? 360 : double.infinity,
        margin: EdgeInsets.only(right: 24, top: 24, bottom: 24, left: isDesktop ? 0 : 24),
        padding: const EdgeInsets.all(28),
        decoration: BoxDecoration(
          color: const Color(0xFF1E293B),
          borderRadius: BorderRadius.circular(28),
          border: Border.all(color: const Color(0xFF334155)),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Recently Called',
              style: GoogleFonts.outfit(color: Colors.white, fontSize: 20, fontWeight: FontWeight.bold),
            ),
            const Divider(color: Color(0xFF334155), height: 32),
            Expanded(
              child: recentTokens.isEmpty
                  ? Center(child: Text('No called tokens yet.', style: GoogleFonts.inter(color: Colors.white30)))
                  : ListView.builder(
                      itemCount: recentTokens.length,
                      itemBuilder: (context, index) {
                        final t = recentTokens[index];
                        final num = t['token_number'];
                        final name = t['customer_name'] ?? 'Walk-In';
                        final status = t['status'] ?? 'Waiting';
                        final type = t['service_type'] ?? 'General';
                        final isServing = status == 'Serving';

                        return Container(
                          margin: const EdgeInsets.only(bottom: 16),
                          padding: const EdgeInsets.all(16),
                          decoration: BoxDecoration(
                            color: isServing ? const Color(0xFF6366F1).withOpacity(0.08) : const Color(0xFF0F172A).withOpacity(0.4),
                            borderRadius: BorderRadius.circular(16),
                            border: Border.all(color: isServing ? const Color(0xFF6366F1) : const Color(0xFF334155)),
                          ),
                          child: Row(
                            children: [
                              Container(
                                width: 50,
                                height: 50,
                                decoration: BoxDecoration(
                                  color: isServing ? const Color(0xFF6366F1) : const Color(0xFF1E293B),
                                  shape: BoxShape.circle,
                                ),
                                child: Center(
                                  child: Text(
                                    '$num',
                                    style: GoogleFonts.outfit(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 18),
                                  ),
                                ),
                              ),
                              const SizedBox(width: 16),
                              Expanded(
                                child: Column(
                                  crossAxisAlignment: CrossAxisAlignment.start,
                                  children: [
                                    Text(name, style: GoogleFonts.outfit(color: Colors.white, fontSize: 15, fontWeight: FontWeight.bold)),
                                    const SizedBox(height: 2),
                                    Text(type, style: GoogleFonts.inter(color: Colors.white60, fontSize: 12)),
                                  ],
                                ),
                              ),
                              Container(
                                padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                                decoration: BoxDecoration(
                                  color: isServing ? Colors.green.withOpacity(0.1) : Colors.white10,
                                  borderRadius: BorderRadius.circular(6),
                                ),
                                child: Text(
                                  isServing ? 'Calling' : 'Done',
                                  style: TextStyle(color: isServing ? Colors.green : Colors.white60, fontSize: 10, fontWeight: FontWeight.bold),
                                ),
                              )
                            ],
                          ),
                        );
                      },
                    ),
            ),
            // Help/Close Button
            ElevatedButton(
              onPressed: () => Navigator.pop(context),
              style: ElevatedButton.styleFrom(
                backgroundColor: const Color(0xFF0F172A),
                foregroundColor: Colors.white60,
                minimumSize: const Size(double.infinity, 48),
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
              ),
              child: const Text('Exit Fullscreen Display'),
            )
          ],
        ),
      );
    }

    return Scaffold(
      backgroundColor: const Color(0xFF0F172A),
      body: SafeArea(
        child: SizedBox(
          width: double.infinity,
          height: double.infinity,
          child: Column(
            children: [
              buildLobbyHeader(),
              Expanded(
                child: isDesktop
                    ? Row(
                        children: [
                          buildMainDisplay(),
                          buildRecentTokensPanel(),
                        ],
                      )
                    : Column(
                        children: [
                          buildMainDisplay(),
                          Expanded(child: buildRecentTokensPanel()),
                        ],
                      ),
              )
            ],
          ),
        ),
      ),
    );
  }
}
