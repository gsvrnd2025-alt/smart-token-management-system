import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:google_fonts/google_fonts.dart';
import 'providers/app_state.dart';
import 'views/login_view.dart';
import 'views/dashboard_shell.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(
    ChangeNotifierProvider(
      create: (_) => AppState(),
      child: const MyApp(),
    ),
  );
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Smart Token Management Dashboard',
      debugShowCheckedModeBanner: false,
      themeMode: ThemeMode.dark, // Default to premium dark theme
      darkTheme: ThemeData(
        brightness: Brightness.dark,
        primaryColor: const Color(0xFF6366F1),
        scaffoldBackgroundColor: const Color(0xFF0F172A),
        cardColor: const Color(0xFF1E293B),
        colorScheme: const ColorScheme.dark(
          primary: Color(0xFF6366F1),
          secondary: Color(0xFF818CF8),
          surface: Color(0xFF1E293B),
          background: Color(0xFF0F172A),
          error: Colors.redAccent,
        ),
        textTheme: GoogleFonts.interTextTheme(ThemeData.dark().textTheme).copyWith(
          titleLarge: GoogleFonts.outfit(color: Colors.white, fontWeight: FontWeight.bold),
          headlineMedium: GoogleFonts.outfit(color: Colors.white, fontWeight: FontWeight.bold),
        ),
        inputDecorationTheme: InputDecorationTheme(
          labelStyle: const TextStyle(color: Colors.white60),
          enabledBorder: OutlineInputBorder(
            borderSide: const BorderSide(color: Color(0xFF334155)),
            borderRadius: BorderRadius.circular(10),
          ),
          focusedBorder: OutlineInputBorder(
            borderSide: const BorderSide(color: Color(0xFF6366F1), width: 1.5),
            borderRadius: BorderRadius.circular(10),
          ),
          filled: true,
          fillColor: const Color(0xFF0F172A).withOpacity(0.5),
        ),
      ),
      home: const MainRouter(),
    );
  }
}

class MainRouter extends StatelessWidget {
  const MainRouter({super.key});

  @override
  Widget build(BuildContext context) {
    final state = Provider.of<AppState>(context);

    // Show custom loading screen while AppState is initializing database
    if (!state.isInitialized) {
      return const Scaffold(
        backgroundColor: Color(0xFF0F172A),
        body: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(Icons.layers, color: Color(0xFF818CF8), size: 48),
              SizedBox(height: 24),
              SizedBox(
                width: 32,
                height: 32,
                child: CircularProgressIndicator(
                  strokeWidth: 3,
                  color: Color(0xFF6366F1),
                ),
              ),
              SizedBox(height: 16),
              Text(
                'Connecting to Supabase Database...',
                style: TextStyle(color: Colors.white60, fontSize: 14),
              )
            ],
          ),
        ),
      );
    }

    // Direct routing based on login session state
    if (state.isLoggedIn) {
      return const DashboardShell();
    } else {
      return const LoginView();
    }
  }
}
