import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:google_fonts/google_fonts.dart';
import '../providers/app_state.dart';

class QueueConsoleView extends StatefulWidget {
  const QueueConsoleView({super.key});

  @override
  State<QueueConsoleView> createState() => _QueueConsoleViewState();
}

class _QueueConsoleViewState extends State<QueueConsoleView> {
  final _walkinNameController = TextEditingController();
  String _selectedServiceType = 'General Service';
  
  bool _isActionLoading = false;
  bool _isGenerateLoading = false;

  final List<String> _serviceTypes = [
    'General Service',
    'Consultation',
    'Enquiry',
    'Premium Service',
  ];

  @override
  void dispose() {
    _walkinNameController.dispose();
    super.dispose();
  }

  Future<void> _handleNextToken(AppState state) async {
    setState(() => _isActionLoading = true);
    try {
      await state.callNextToken();
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Called next token successfully!'), backgroundColor: Color(0xFF6366F1)),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error: $e'), backgroundColor: Colors.redAccent),
        );
      }
    } finally {
      if (mounted) setState(() => _isActionLoading = false);
    }
  }

  Future<void> _handleSkipToken(AppState state, int currentNum) async {
    setState(() => _isActionLoading = true);
    try {
      await state.skipToken(currentNum);
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Skipped token $currentNum.'), backgroundColor: Colors.amber),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error skipping: $e'), backgroundColor: Colors.redAccent),
        );
      }
    } finally {
      if (mounted) setState(() => _isActionLoading = false);
    }
  }

  Future<void> _handleCompleteToken(AppState state, int currentNum) async {
    setState(() => _isActionLoading = true);
    try {
      await state.completeToken(currentNum);
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Completed token $currentNum.'), backgroundColor: Colors.green),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error completing: $e'), backgroundColor: Colors.redAccent),
        );
      }
    } finally {
      if (mounted) setState(() => _isActionLoading = false);
    }
  }

  Future<void> _handleGenerateWalkin(AppState state) async {
    setState(() => _isGenerateLoading = true);
    try {
      final name = _walkinNameController.text.trim();
      final result = await state.generateWalkinToken(name, _selectedServiceType);
      
      _walkinNameController.clear();
      
      if (mounted) {
        final num = result['tokenNumber'];
        showDialog(
          context: context,
          builder: (context) => AlertDialog(
            backgroundColor: const Color(0xFF1E293B),
            shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
            title: Row(
              children: [
                const Icon(Icons.print, color: Color(0xFF818CF8)),
                const SizedBox(width: 12),
                Text('Token Printed', style: GoogleFonts.outfit(color: Colors.white, fontWeight: FontWeight.bold)),
              ],
            ),
            content: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Text('Your physical walk-in token has been generated:', style: GoogleFonts.inter(color: Colors.white70)),
                const SizedBox(height: 16),
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
                  decoration: BoxDecoration(
                    color: const Color(0xFF0F172A),
                    borderRadius: BorderRadius.circular(12),
                    border: Border.all(color: const Color(0xFF6366F1).withOpacity(0.3)),
                  ),
                  child: Column(
                    children: [
                      Text('TOKEN NUMBER', style: GoogleFonts.inter(color: Colors.white60, fontSize: 11, letterSpacing: 1)),
                      Text(
                        '$num',
                        style: GoogleFonts.outfit(color: Colors.white, fontSize: 48, fontWeight: FontWeight.bold),
                      ),
                      const SizedBox(height: 4),
                      Text(_selectedServiceType, style: GoogleFonts.inter(color: const Color(0xFF818CF8), fontSize: 13, fontWeight: FontWeight.w600)),
                    ],
                  ),
                ),
                const SizedBox(height: 12),
                Text('Estimated wait time: ${result['estimatedWaitingTimeMinutes']} mins', style: GoogleFonts.inter(color: Colors.white60, fontSize: 12)),
              ],
            ),
            actions: [
              TextButton(
                onPressed: () => Navigator.pop(context),
                child: Text('Close', style: GoogleFonts.inter(color: const Color(0xFF818CF8), fontWeight: FontWeight.bold)),
              ),
            ],
          ),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Failed to generate: $e'), backgroundColor: Colors.redAccent),
        );
      }
    } finally {
      if (mounted) setState(() => _isGenerateLoading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final state = Provider.of<AppState>(context);
    final size = MediaQuery.of(context).size;
    final isDesktop = size.width > 1100;

    // Retrieve active serving token details from stream if matching settings value
    Map<String, dynamic>? activeServingToken;
    final servingNum = state.currentlyServingNum;
    if (servingNum > 0) {
      final matches = state.activeTokens.where((t) => t['token_number'] == servingNum && t['status'] == 'Serving');
      if (matches.isNotEmpty) {
        activeServingToken = matches.first;
      }
    }

    // Dynamic sizing helper
    double gridAspectRatio = size.width > 1400 ? 2.0 : (size.width > 900 ? 1.5 : (size.width > 600 ? 1.3 : 1.1));

    Widget buildStatCard(String title, String value, IconData icon, Color color) {
      return Container(
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(16),
          boxShadow: [
            BoxShadow(
              color: color.withOpacity(0.04),
              blurRadius: 16,
              offset: const Offset(0, 8),
            ),
          ],
        ),
        child: ClipRRect(
          borderRadius: BorderRadius.circular(16),
          child: Stack(
            children: [
              // Faint watermark icon in background
              Positioned(
                right: -10,
                bottom: -10,
                child: Icon(
                  icon,
                  color: color.withOpacity(0.05),
                  size: 80,
                ),
              ),
              Container(
                padding: const EdgeInsets.all(20),
                decoration: BoxDecoration(
                  color: const Color(0xFF0F1426),
                  borderRadius: BorderRadius.circular(16),
                  border: Border.all(color: color.withOpacity(0.2), width: 1),
                ),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Text(
                            title,
                            overflow: TextOverflow.ellipsis,
                            style: GoogleFonts.inter(
                              color: Colors.white60,
                              fontSize: 13,
                              fontWeight: FontWeight.w500,
                            ),
                          ),
                          const SizedBox(height: 8),
                          Text(
                            value,
                            style: GoogleFonts.outfit(
                              color: Colors.white,
                              fontSize: 26,
                              fontWeight: FontWeight.bold,
                              shadows: [
                                Shadow(
                                  color: color.withOpacity(0.4),
                                  blurRadius: 8,
                                  offset: const Offset(0, 2),
                                ),
                              ],
                            ),
                          ),
                        ],
                      ),
                    ),
                    Container(
                      padding: const EdgeInsets.all(10),
                      decoration: BoxDecoration(
                        gradient: LinearGradient(
                          colors: [color, color.withOpacity(0.7)],
                          begin: Alignment.topLeft,
                          end: Alignment.bottomRight,
                        ),
                        borderRadius: BorderRadius.circular(12),
                        boxShadow: [
                          BoxShadow(
                            color: color.withOpacity(0.2),
                            blurRadius: 8,
                            offset: const Offset(0, 3),
                          ),
                        ],
                      ),
                      child: Icon(icon, color: Colors.white, size: 20),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      );
    }

    Widget buildServingConsole() {
      final isServing = activeServingToken != null;
      final currentNum = servingNum;
      final name = activeServingToken?['customer_name'] ?? 'No Active Customer';
      final type = activeServingToken?['service_type'] ?? 'Please call the next token';
      final source = activeServingToken?['source'] ?? '-';

      return Container(
        padding: const EdgeInsets.all(28),
        decoration: BoxDecoration(
          gradient: const LinearGradient(
            colors: [Color(0xFF161B33), Color(0xFF0E1122)],
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
          ),
          borderRadius: BorderRadius.circular(20),
          border: Border.all(color: const Color(0xFF6366F1).withOpacity(0.35), width: 1.5),
          boxShadow: [
            BoxShadow(color: const Color(0xFF6366F1).withOpacity(0.12), blurRadius: 24, offset: const Offset(0, 8)),
          ],
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Queue Console',
                      style: GoogleFonts.outfit(color: Colors.white, fontSize: 20, fontWeight: FontWeight.bold),
                    ),
                    const SizedBox(height: 4),
                    Text(
                      'ACTIVE DESK CONTROL',
                      style: GoogleFonts.inter(color: const Color(0xFF818CF8), fontSize: 11, fontWeight: FontWeight.bold, letterSpacing: 1),
                    ),
                  ],
                ),
                if (isServing)
                  Container(
                    padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
                    decoration: BoxDecoration(
                      color: const Color(0xFF6366F1).withOpacity(0.15),
                      borderRadius: BorderRadius.circular(12),
                      border: Border.all(color: const Color(0xFF6366F1).withOpacity(0.3)),
                    ),
                    child: Text(
                      'Source: $source',
                      style: GoogleFonts.inter(color: const Color(0xFF818CF8), fontWeight: FontWeight.bold, fontSize: 12),
                    ),
                  ),
              ],
            ),
            const SizedBox(height: 32),
            Center(
              child: Column(
                children: [
                  Text(
                    'NOW SERVING',
                    style: GoogleFonts.inter(color: Colors.white.withOpacity(0.5), fontSize: 13, fontWeight: FontWeight.bold, letterSpacing: 1.5),
                  ),
                  const SizedBox(height: 8),
                  Text(
                    isServing ? '$currentNum' : '--',
                    style: GoogleFonts.outfit(
                      color: isServing ? const Color(0xFF818CF8) : Colors.white24,
                      fontSize: 76,
                      fontWeight: FontWeight.bold,
                      height: 1.1,
                      shadows: isServing
                          ? [
                              Shadow(
                                color: const Color(0xFF818CF8).withOpacity(0.5),
                                blurRadius: 20,
                              ),
                              Shadow(
                                color: const Color(0xFF6366F1).withOpacity(0.3),
                                blurRadius: 10,
                              ),
                            ]
                          : null,
                    ),
                  ),
                  const SizedBox(height: 12),
                  Text(
                    name,
                    style: GoogleFonts.outfit(color: Colors.white, fontSize: 22, fontWeight: FontWeight.bold),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    type,
                    style: GoogleFonts.inter(color: Colors.white.withOpacity(0.7), fontSize: 14),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 36),
            Row(
              children: [
                Expanded(
                  flex: 2,
                  child: ElevatedButton(
                    onPressed: _isActionLoading ? null : () => _handleNextToken(state),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.transparent,
                      foregroundColor: Colors.white,
                      padding: EdgeInsets.zero,
                      elevation: 0,
                      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                    ),
                    child: Ink(
                      decoration: BoxDecoration(
                        gradient: const LinearGradient(
                          colors: [Color(0xFF6366F1), Color(0xFF8B5CF6)],
                          begin: Alignment.topLeft,
                          end: Alignment.bottomRight,
                        ),
                        borderRadius: BorderRadius.circular(12),
                        boxShadow: [
                          BoxShadow(
                            color: const Color(0xFF6366F1).withOpacity(0.3),
                            blurRadius: 12,
                            offset: const Offset(0, 4),
                          ),
                        ],
                      ),
                      child: Container(
                        alignment: Alignment.center,
                        height: 54,
                        child: _isActionLoading
                            ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2, color: Colors.white))
                            : Row(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  const Icon(Icons.skip_next, size: 20),
                                  const SizedBox(width: 8),
                                  Text('Next Token', style: GoogleFonts.inter(fontWeight: FontWeight.bold, fontSize: 15)),
                                ],
                              ),
                      ),
                    ),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: OutlinedButton(
                    onPressed: (_isActionLoading || !isServing) ? null : () => _handleSkipToken(state, currentNum),
                    style: OutlinedButton.styleFrom(
                      foregroundColor: Colors.amberAccent,
                      side: BorderSide(color: Colors.amberAccent.withOpacity(0.4)),
                      disabledForegroundColor: Colors.white24,
                      minimumSize: const Size(0, 54),
                      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                    ),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        const Icon(Icons.forward_to_inbox, size: 18),
                        const SizedBox(width: 6),
                        Text('Skip', style: GoogleFonts.inter(fontWeight: FontWeight.bold, fontSize: 14)),
                      ],
                    ),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: OutlinedButton(
                    onPressed: (_isActionLoading || !isServing) ? null : () => _handleCompleteToken(state, currentNum),
                    style: OutlinedButton.styleFrom(
                      foregroundColor: const Color(0xFF34D399),
                      side: BorderSide(color: const Color(0xFF34D399).withOpacity(0.4)),
                      disabledForegroundColor: Colors.white24,
                      minimumSize: const Size(0, 54),
                      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                    ),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        const Icon(Icons.check_circle_outline, size: 18),
                        const SizedBox(width: 6),
                        Text('Done', style: GoogleFonts.inter(fontWeight: FontWeight.bold, fontSize: 14)),
                      ],
                    ),
                  ),
                ),
              ],
            )
          ],
        ),
      );
    }

    Widget buildWalkinRegistration() {
      final lastGen = state.settings['Last Generated Token'] ?? '--';

      return Container(
        padding: const EdgeInsets.all(28),
        decoration: BoxDecoration(
          color: const Color(0xFF0F1426),
          borderRadius: BorderRadius.circular(20),
          border: Border.all(color: const Color(0xFF1E293B)),
          boxShadow: [
            BoxShadow(color: Colors.black.withOpacity(0.2), blurRadius: 16, offset: const Offset(0, 8)),
          ],
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.confirmation_num_outlined, color: Color(0xFF818CF8)),
                const SizedBox(width: 12),
                Text(
                  'Walk-In Registration',
                  style: GoogleFonts.outfit(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const SizedBox(height: 6),
            Text(
              'Generate and print physical queue tickets for client counters.',
              style: GoogleFonts.inter(color: Colors.white60, fontSize: 12),
            ),
            const SizedBox(height: 24),
            TextField(
              controller: _walkinNameController,
              style: const TextStyle(color: Colors.white),
              decoration: InputDecoration(
                labelText: 'Customer Name (Optional)',
                labelStyle: const TextStyle(color: Colors.white60),
                prefixIcon: const Icon(Icons.person_outline, color: Colors.white60),
                filled: true,
                fillColor: const Color(0xFF090D1A),
                enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(12)),
                focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(12)),
              ),
            ),
            const SizedBox(height: 16),
            DropdownButtonFormField<String>(
              value: _selectedServiceType,
              style: const TextStyle(color: Colors.white),
              dropdownColor: const Color(0xFF0F1426),
              decoration: InputDecoration(
                labelText: 'Service Type',
                labelStyle: const TextStyle(color: Colors.white60),
                prefixIcon: const Icon(Icons.miscellaneous_services_outlined, color: Colors.white60),
                filled: true,
                fillColor: const Color(0xFF090D1A),
                enabledBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF1E293B)), borderRadius: BorderRadius.circular(12)),
                focusedBorder: OutlineInputBorder(borderSide: const BorderSide(color: Color(0xFF6366F1)), borderRadius: BorderRadius.circular(12)),
              ),
              items: _serviceTypes.map((type) {
                return DropdownMenuItem(value: type, child: Text(type));
              }).toList(),
              onChanged: (val) {
                if (val != null) setState(() => _selectedServiceType = val);
              },
            ),
            const Spacer(),
            const Divider(color: Color(0xFF1E293B)),
            const SizedBox(height: 8),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text('Last Ticket Generated:', style: GoogleFonts.inter(color: Colors.white60, fontSize: 13)),
                Text(
                  lastGen,
                  style: GoogleFonts.outfit(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const SizedBox(height: 16),
            ElevatedButton(
              onPressed: _isGenerateLoading ? null : () => _handleGenerateWalkin(state),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.transparent,
                foregroundColor: Colors.white,
                padding: EdgeInsets.zero,
                elevation: 0,
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
              ),
              child: Ink(
                decoration: BoxDecoration(
                  gradient: const LinearGradient(
                    colors: [Color(0xFF6366F1), Color(0xFF8B5CF6)],
                    begin: Alignment.topLeft,
                    end: Alignment.bottomRight,
                  ),
                  borderRadius: BorderRadius.circular(12),
                  boxShadow: [
                    BoxShadow(
                      color: const Color(0xFF6366F1).withOpacity(0.3),
                      blurRadius: 12,
                      offset: const Offset(0, 4),
                    ),
                  ],
                ),
                child: Container(
                  alignment: Alignment.center,
                  height: 50,
                  child: _isGenerateLoading
                      ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2, color: Colors.white))
                      : Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            const Icon(Icons.add_circle_outline, color: Colors.white, size: 20),
                            const SizedBox(width: 8),
                            Text('Generate Ticket', style: GoogleFonts.inter(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 15)),
                          ],
                        ),
                ),
              ),
            ),
          ],
        ),
      );
    }

    return SingleChildScrollView(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Row 1: Statistics Grid
          GridView.count(
            crossAxisCount: size.width > 1200 ? 5 : (size.width > 700 ? 3 : 2),
            crossAxisSpacing: 16,
            mainAxisSpacing: 16,
            shrinkWrap: true,
            childAspectRatio: gridAspectRatio,
            physics: const NeverScrollableScrollPhysics(),
            children: [
              buildStatCard('Currently Serving', servingNum > 0 ? '$servingNum' : 'None', Icons.support_agent_rounded, const Color(0xFF818CF8)),
              buildStatCard('Waiting Queue', '${state.waitingQueueCount}', Icons.people_outline, Colors.amber),
              buildStatCard('Completed Today', '${state.completedTodayCount}', Icons.check_circle_outlined, const Color(0xFF34D399)),
              buildStatCard('Online Bookings', '${state.onlineBookingsCount}', Icons.language, Colors.cyan),
              buildStatCard('Walk-In (Manual)', '${state.walkInCount}', Icons.confirmation_num_outlined, Colors.deepOrangeAccent),
            ],
          ),
          const SizedBox(height: 24),
          
          // Row 2: Console and Registration side-by-side
          if (isDesktop)
            Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Expanded(flex: 7, child: buildServingConsole()),
                const SizedBox(width: 24),
                Expanded(flex: 5, child: SizedBox(height: 400, child: buildWalkinRegistration())),
              ],
            )
          else ...[
            buildServingConsole(),
            const SizedBox(height: 24),
            SizedBox(height: 420, child: buildWalkinRegistration()),
          ],
        ],
      ),
    );
  }
}

