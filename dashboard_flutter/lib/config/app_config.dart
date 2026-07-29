// Configuration file for Smart Token Management System - Flutter
// This file contains environment-specific settings that should be customized for your deployment

class AppConfig {
  // Supabase Configuration
  static const String supabaseUrl = String.fromEnvironment(
    'SUPABASE_URL',
    defaultValue: 'https://swqgfhtyfudkwvyuulzz.supabase.co',
  );

  static const String supabaseAnonKey = String.fromEnvironment(
    'SUPABASE_ANON_KEY',
    defaultValue: 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InN3cWdmaHR5ZnVka3d2eXV1bHp6Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODE2MDE4ODIsImV4cCI6MjA5NzE3Nzg4Mn0.qbjAR4I8NfCFusutfws4I4oZJsbCx4TGeaYtfSyA1fc',
  );

  // Default Admin Credentials (MUST BE CHANGED IN PRODUCTION)
  static const String defaultAdminUsername = 'admin';
  static const String defaultAdminPassword = 'admin123';
  static const String defaultAdminEmail = 'admin@example.com';

  // Token System Defaults
  static const int defaultStartingTokenNumber = 1;
  static const int defaultAverageServiceTime = 10;
  static const String defaultOrganizationName = 'Smart Token Management System';

  // Email Service Defaults
  static const String defaultEmailProvider = 'demo'; // 'demo', 'resend', or 'smtp'

  // Validation Methods
  static bool isValidSupabaseConfig() {
    return supabaseUrl.isNotEmpty && 
           supabaseAnonKey.isNotEmpty &&
           supabaseUrl.startsWith('https://');
  }

  // Get environment info for debugging
  static Map<String, String> getEnvironmentInfo() {
    return {
      'supabase_url': supabaseUrl,
      'admin_username': defaultAdminUsername,
      'organization': defaultOrganizationName,
    };
  }
}
