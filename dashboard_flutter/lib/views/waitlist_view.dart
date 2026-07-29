import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:google_fonts/google_fonts.dart';
import '../providers/app_state.dart';

class WaitlistView extends StatefulWidget {
  const WaitlistView({super.key});

  @override
  State<WaitlistView> createState() => _WaitlistViewState();
}

class _WaitlistViewState extends State<WaitlistView> {
  final _searchController = TextEditingController();
  String _selectedFilter = '';

  final List<String> _filters = [
    'General Service',
    'Consultation',
    'Enquiry',
    'Premium Service',
  ];

  @override
  void dispose() {
    _searchController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final state = Provider.of<AppState>(context);
    final size = MediaQuery.of(context).size;
    final isDesktop = size.width > 900;

    // Filter and search logic
    final query = _searchController.text.trim().toLowerCase();
    final list = state.activeTokens.where((t) {
      final name = (t['customer_name'] ?? '').toString().toLowerCase();
      final num = t['token_number'].toString();
      final phone = (t['phone_number'] ?? '').toString();
      final type = t['service_type'] ?? '';

      final matchesQuery = name.contains(query) || num.contains(query) || phone.contains(query);
      final matchesFilter = _selectedFilter.isEmpty || type == _selectedFilter;

      return matchesQuery && matchesFilter;
    }).toList();

    Widget buildSearchAndFilters() {
      return Container(
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: const Color(0xFF0F1426),
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: const Color(0xFF1E293B)),
          boxShadow: [
            BoxShadow(color: Colors.black.withOpacity(0.1), blurRadius: 10, offset: const Offset(0, 4)),
          ],
        ),
        child: isDesktop
            ? Row(
                children: [
                  Expanded(
                    child: TextField(
                      controller: _searchController,
                      style: const TextStyle(color: Colors.white),
                      decoration: InputDecoration(
                        prefixIcon: const Icon(Icons.search, color: Color(0xFF818CF8)),
                        hintText: 'Search token, name, or phone...',
                        hintStyle: const TextStyle(color: Colors.white30),
                        filled: true,
                        fillColor: const Color(0xFF090D1A),
                        contentPadding: const EdgeInsets.symmetric(vertical: 16),
                        enabledBorder: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(10),
                          borderSide: const BorderSide(color: Color(0xFF1E293B)),
                        ),
                        focusedBorder: OutlineInputBorder(
                          borderRadius: BorderRadius.circular(10),
                          borderSide: const BorderSide(color: Color(0xFF6366F1)),
                        ),
                      ),
                      onChanged: (_) => setState(() {}),
                    ),
                  ),
                  const SizedBox(width: 16),
                  Container(
                    padding: const EdgeInsets.symmetric(horizontal: 16),
                    decoration: BoxDecoration(
                      color: const Color(0xFF090D1A),
                      borderRadius: BorderRadius.circular(10),
                      border: Border.all(color: const Color(0xFF1E293B)),
                    ),
                    child: DropdownButtonHideUnderline(
                      child: DropdownButton<String>(
                        value: _selectedFilter.isEmpty ? null : _selectedFilter,
                        hint: const Text('All Services', style: TextStyle(color: Colors.white60, fontSize: 14)),
                        dropdownColor: const Color(0xFF0F1426),
                        style: const TextStyle(color: Colors.white),
                        icon: const Icon(Icons.keyboard_arrow_down, color: Color(0xFF818CF8)),
                        items: [
                          const DropdownMenuItem(value: '', child: Text('All Services')),
                          ..._filters.map((f) => DropdownMenuItem(value: f, child: Text(f)))
                        ],
                        onChanged: (val) {
                          setState(() {
                            _selectedFilter = val ?? '';
                          });
                        },
                      ),
                    ),
                  ),
                ],
              )
            : Column(
                children: [
                  TextField(
                    controller: _searchController,
                    style: const TextStyle(color: Colors.white),
                    decoration: InputDecoration(
                      prefixIcon: const Icon(Icons.search, color: Color(0xFF818CF8)),
                      hintText: 'Search token, name...',
                      hintStyle: const TextStyle(color: Colors.white30),
                      filled: true,
                      fillColor: const Color(0xFF090D1A),
                      contentPadding: const EdgeInsets.symmetric(vertical: 16),
                      enabledBorder: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(10),
                        borderSide: const BorderSide(color: Color(0xFF1E293B)),
                      ),
                      focusedBorder: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(10),
                        borderSide: const BorderSide(color: Color(0xFF6366F1)),
                      ),
                    ),
                    onChanged: (_) => setState(() {}),
                  ),
                  const SizedBox(height: 12),
                  DropdownButtonFormField<String>(
                    value: _selectedFilter.isEmpty ? null : _selectedFilter,
                    dropdownColor: const Color(0xFF0F1426),
                    style: const TextStyle(color: Colors.white),
                    icon: const Icon(Icons.keyboard_arrow_down, color: Color(0xFF818CF8)),
                    decoration: InputDecoration(
                      filled: true,
                      fillColor: const Color(0xFF090D1A),
                      contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                      enabledBorder: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(10),
                        borderSide: const BorderSide(color: Color(0xFF1E293B)),
                      ),
                      focusedBorder: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(10),
                        borderSide: const BorderSide(color: Color(0xFF6366F1)),
                      ),
                    ),
                    items: [
                      const DropdownMenuItem(value: '', child: Text('All Services')),
                      ..._filters.map((f) => DropdownMenuItem(value: f, child: Text(f)))
                    ],
                    onChanged: (val) {
                      setState(() {
                        _selectedFilter = val ?? '';
                      });
                    },
                  ),
                ],
              ),
      );
    }

    Widget buildListView() {
      if (list.isEmpty) {
        return Container(
          padding: const EdgeInsets.symmetric(vertical: 60),
          alignment: Alignment.center,
          child: Column(
            children: [
              const Icon(Icons.hourglass_empty, color: Colors.white24, size: 48),
              const SizedBox(height: 16),
              Text(
                'No active waitlist tokens.',
                style: GoogleFonts.inter(color: Colors.white38, fontSize: 15),
              ),
            ],
          ),
        );
      }

      return ListView.builder(
        shrinkWrap: true,
        physics: const NeverScrollableScrollPhysics(),
        itemCount: list.length,
        itemBuilder: (context, index) {
          final t = list[index];
          final num = t['token_number'];
          final name = t['customer_name'] ?? 'Walk-In';
          final phone = t['phone_number'] ?? '-';
          final type = t['service_type'] ?? 'General Service';
          final source = t['source'] ?? 'Manual';
          final status = t['status'] ?? 'Waiting';
          final isServing = status == 'Serving';

          return Container(
            margin: const EdgeInsets.only(bottom: 14),
            padding: const EdgeInsets.all(18),
            decoration: BoxDecoration(
              color: const Color(0xFF0F1426),
              borderRadius: BorderRadius.circular(16),
              border: Border.all(
                color: isServing ? const Color(0xFF6366F1) : const Color(0xFF1E293B),
                width: isServing ? 1.5 : 1,
              ),
              boxShadow: [
                BoxShadow(
                  color: isServing ? const Color(0xFF6366F1).withOpacity(0.08) : Colors.black.withOpacity(0.05),
                  blurRadius: 10,
                  offset: const Offset(0, 4),
                ),
              ],
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Container(
                      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                      decoration: BoxDecoration(
                        color: isServing ? const Color(0xFF6366F1).withOpacity(0.15) : const Color(0xFF090D1A),
                        borderRadius: BorderRadius.circular(8),
                        border: Border.all(
                          color: isServing ? const Color(0xFF6366F1).withOpacity(0.3) : const Color(0xFF1E293B),
                        ),
                      ),
                      child: Text(
                        '#$num',
                        style: GoogleFonts.outfit(
                          color: isServing ? const Color(0xFF818CF8) : Colors.white70,
                          fontSize: 16,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                    ),
                    Container(
                      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                      decoration: BoxDecoration(
                        color: isServing ? const Color(0xFF10B981).withOpacity(0.12) : const Color(0xFFF59E0B).withOpacity(0.12),
                        borderRadius: BorderRadius.circular(6),
                        border: Border.all(
                          color: isServing ? const Color(0xFF10B981).withOpacity(0.3) : const Color(0xFFF59E0B).withOpacity(0.3),
                        ),
                      ),
                      child: Text(
                        status.toUpperCase(),
                        style: TextStyle(
                          color: isServing ? const Color(0xFF34D399) : const Color(0xFFFBBF24),
                          fontSize: 11,
                          fontWeight: FontWeight.bold,
                          letterSpacing: 0.5,
                        ),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 12),
                Text(
                  name,
                  style: GoogleFonts.outfit(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold),
                ),
                const SizedBox(height: 8),
                Text('Phone: $phone', style: GoogleFonts.inter(color: Colors.white54, fontSize: 13)),
                Text('Service: $type', style: GoogleFonts.inter(color: Colors.white54, fontSize: 13)),
                Text('Source: $source', style: GoogleFonts.inter(color: Colors.white54, fontSize: 13)),
                const SizedBox(height: 12),
                Row(
                  mainAxisAlignment: MainAxisAlignment.end,
                  children: [
                    _buildWaitlistIconButton(
                      icon: Icons.support_agent,
                      color: const Color(0xFF6366F1),
                      tooltip: 'Call / Serve',
                      onPressed: () => state.saveSettings({'Current Serving Token': num.toString()}),
                    ),
                    const SizedBox(width: 8),
                    _buildWaitlistIconButton(
                      icon: Icons.check_circle_outline,
                      color: const Color(0xFF10B981),
                      tooltip: 'Complete',
                      onPressed: () => state.completeToken(num),
                    ),
                    const SizedBox(width: 8),
                    _buildWaitlistIconButton(
                      icon: Icons.forward_to_inbox,
                      color: const Color(0xFFF59E0B),
                      tooltip: 'Skip',
                      onPressed: () => state.skipToken(num),
                    ),
                  ],
                )
              ],
            ),
          );
        },
      );
    }

    Widget buildTableView() {
      if (list.isEmpty) {
        return Container(
          padding: const EdgeInsets.symmetric(vertical: 60),
          alignment: Alignment.center,
          child: Column(
            children: [
              const Icon(Icons.hourglass_empty, color: Colors.white24, size: 48),
              const SizedBox(height: 16),
              Text(
                'No active waitlist tokens.',
                style: GoogleFonts.inter(color: Colors.white38, fontSize: 15),
              ),
            ],
          ),
        );
      }

      return Container(
        width: double.infinity,
        decoration: BoxDecoration(
          color: const Color(0xFF0F1426),
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: const Color(0xFF1E293B)),
          boxShadow: [
            BoxShadow(color: Colors.black.withOpacity(0.1), blurRadius: 10, offset: const Offset(0, 4)),
          ],
        ),
        child: ClipRRect(
          borderRadius: BorderRadius.circular(16),
          child: SingleChildScrollView(
            scrollDirection: Axis.horizontal,
            child: DataTable(
              headingRowColor: MaterialStateProperty.all(const Color(0xFF090D1A)),
              dataRowColor: MaterialStateProperty.resolveWith<Color?>((Set<MaterialState> states) {
                if (states.contains(MaterialState.hovered)) {
                  return const Color(0xFF161B33);
                }
                return null;
              }),
              horizontalMargin: 24,
              columnSpacing: 36,
              columns: [
                DataColumn(label: Text('Token', style: GoogleFonts.outfit(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 14))),
                DataColumn(label: Text('Customer Name', style: GoogleFonts.outfit(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 14))),
                DataColumn(label: Text('Phone Number', style: GoogleFonts.outfit(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 14))),
                DataColumn(label: Text('Service Type', style: GoogleFonts.outfit(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 14))),
                DataColumn(label: Text('Source', style: GoogleFonts.outfit(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 14))),
                DataColumn(label: Text('Status', style: GoogleFonts.outfit(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 14))),
                DataColumn(label: Text('Actions', style: GoogleFonts.outfit(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 14))),
              ],
              rows: list.map((t) {
                final num = t['token_number'];
                final name = t['customer_name'] ?? 'Walk-In';
                final phone = t['phone_number'] ?? '-';
                final type = t['service_type'] ?? 'General Service';
                final source = t['source'] ?? 'Manual';
                final status = t['status'] ?? 'Waiting';
                final isServing = status == 'Serving';

                return DataRow(
                  color: isServing ? MaterialStateProperty.all(const Color(0xFF6366F1).withOpacity(0.05)) : null,
                  cells: [
                    DataCell(
                      Container(
                        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                        decoration: BoxDecoration(
                          color: isServing ? const Color(0xFF6366F1).withOpacity(0.15) : const Color(0xFF090D1A),
                          borderRadius: BorderRadius.circular(6),
                          border: Border.all(
                            color: isServing ? const Color(0xFF6366F1).withOpacity(0.3) : const Color(0xFF1E293B),
                          ),
                        ),
                        child: Text(
                          '#$num',
                          style: GoogleFonts.outfit(
                            color: isServing ? const Color(0xFF818CF8) : Colors.white70,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                      ),
                    ),
                    DataCell(Text(name, style: GoogleFonts.inter(color: Colors.white70, fontWeight: FontWeight.w600))),
                    DataCell(Text(phone, style: GoogleFonts.inter(color: Colors.white54))),
                    DataCell(Text(type, style: GoogleFonts.inter(color: Colors.white54))),
                    DataCell(Text(source, style: GoogleFonts.inter(color: Colors.white54))),
                    DataCell(
                      Container(
                        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                        decoration: BoxDecoration(
                          color: isServing ? const Color(0xFF10B981).withOpacity(0.12) : const Color(0xFFF59E0B).withOpacity(0.12),
                          borderRadius: BorderRadius.circular(6),
                          border: Border.all(
                            color: isServing ? const Color(0xFF10B981).withOpacity(0.3) : const Color(0xFFF59E0B).withOpacity(0.3),
                          ),
                        ),
                        child: Text(
                          status.toUpperCase(),
                          style: TextStyle(
                            color: isServing ? const Color(0xFF34D399) : const Color(0xFFFBBF24),
                            fontSize: 11,
                            fontWeight: FontWeight.bold,
                            letterSpacing: 0.5,
                          ),
                        ),
                      ),
                    ),
                    DataCell(
                      Row(
                        children: [
                          TextButton.icon(
                            onPressed: () => state.saveSettings({'Current Serving Token': num.toString()}),
                            icon: const Icon(Icons.support_agent, size: 16, color: Color(0xFF818CF8)),
                            label: Text('Call', style: GoogleFonts.inter(color: const Color(0xFF818CF8), fontSize: 13, fontWeight: FontWeight.w600)),
                          ),
                          const SizedBox(width: 8),
                          TextButton.icon(
                            onPressed: () => state.completeToken(num),
                            icon: const Icon(Icons.check_circle_outline, size: 16, color: Color(0xFF34D399)),
                            label: Text('Done', style: GoogleFonts.inter(color: const Color(0xFF34D399), fontSize: 13, fontWeight: FontWeight.w600)),
                          ),
                          const SizedBox(width: 8),
                          TextButton.icon(
                            onPressed: () => state.skipToken(num),
                            icon: const Icon(Icons.forward_to_inbox, size: 16, color: Color(0xFFFBBF24)),
                            label: Text('Skip', style: GoogleFonts.inter(color: const Color(0xFFFBBF24), fontSize: 13, fontWeight: FontWeight.w600)),
                          ),
                        ],
                      ),
                    ),
                  ],
                );
              }).toList(),
            ),
          ),
        ),
      );
    }

    return Scaffold(
      backgroundColor: const Color(0xFF090D1A),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(24),
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
                      'Waiting Queue',
                      style: GoogleFonts.outfit(color: Colors.white, fontSize: 24, fontWeight: FontWeight.bold),
                    ),
                    const SizedBox(height: 4),
                    Text(
                      'Real-time customer status waitlist logs.',
                      style: GoogleFonts.inter(color: Colors.white60, fontSize: 13),
                    ),
                  ],
                ),
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
                  decoration: BoxDecoration(
                    color: const Color(0xFF6366F1).withOpacity(0.1),
                    borderRadius: BorderRadius.circular(20),
                    border: Border.all(color: const Color(0xFF6366F1).withOpacity(0.2)),
                  ),
                  child: Text(
                    '${list.length} Records',
                    style: GoogleFonts.inter(color: const Color(0xFF818CF8), fontWeight: FontWeight.bold, fontSize: 13),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 24),
            buildSearchAndFilters(),
            const SizedBox(height: 20),
            isDesktop ? buildTableView() : buildListView(),
          ],
        ),
      ),
    );
  }

  Widget _buildWaitlistIconButton({
    required IconData icon,
    required Color color,
    required String tooltip,
    required VoidCallback onPressed,
  }) {
    return Tooltip(
      message: tooltip,
      child: Container(
        decoration: BoxDecoration(
          color: color.withOpacity(0.08),
          borderRadius: BorderRadius.circular(10),
          border: Border.all(color: color.withOpacity(0.2)),
        ),
        child: IconButton(
          icon: Icon(icon, color: color, size: 18),
          onPressed: onPressed,
        ),
      ),
    );
  }
}
