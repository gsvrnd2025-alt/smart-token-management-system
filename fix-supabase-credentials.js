#!/usr/bin/env node

/**
 * Smart Token Management System - Supabase Settings Fix
 * This script fixes the database credential synchronization issue
 */

const SUPABASE_URL = "https://swqgfhtyfudkwvyuulzz.supabase.co";
const SUPABASE_ANON_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InN3cWdmaHR5ZnVka3d2eXV1bHp6Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODE2MDE4ODIsImV4cCI6MjA5NzE3Nzg4Mn0.qbjAR4I8NfCFusutfws4I4oZJsbCx4TGeaYtfSyA1fc";

const settingsToUpdate = [
    { key: 'Admin Username', value: 'admin' },
    { key: 'Admin Password', value: 'admin123' },
    { key: 'Admin Email', value: 'admin@example.com' },
    { key: 'Starting Token Number', value: '1' },
    { key: 'Average Service Time', value: '10' },
    { key: 'Organization Name', value: 'Smart Token Management System' },
    { key: 'Enable Buzzer', value: 'true' },
];

async function updateSettings() {
    console.log('🔄 Updating Supabase settings...\n');
    
    let successCount = 0;
    let errorCount = 0;
    
    for (const setting of settingsToUpdate) {
        try {
            // Try to update first (if key exists)
            const updateResponse = await fetch(
                `${SUPABASE_URL}/rest/v1/settings?key=eq.${encodeURIComponent(setting.key)}`,
                {
                    method: 'PATCH',
                    headers: {
                        'Content-Type': 'application/json',
                        'Authorization': `Bearer ${SUPABASE_ANON_KEY}`,
                        'apikey': SUPABASE_ANON_KEY,
                        'Prefer': 'return=representation',
                    },
                    body: JSON.stringify({ value: setting.value }),
                }
            );
            
            if (updateResponse.ok) {
                console.log(`✅ Updated: ${setting.key} = ${setting.value}`);
                successCount++;
            } else if (updateResponse.status === 404) {
                // Key doesn't exist, insert it
                console.log(`  └─ Key not found, inserting...`);
                
                const insertResponse = await fetch(
                    `${SUPABASE_URL}/rest/v1/settings`,
                    {
                        method: 'POST',
                        headers: {
                            'Content-Type': 'application/json',
                            'Authorization': `Bearer ${SUPABASE_ANON_KEY}`,
                            'apikey': SUPABASE_ANON_KEY,
                            'Prefer': 'return=representation',
                        },
                        body: JSON.stringify({ key: setting.key, value: setting.value }),
                    }
                );
                
                if (insertResponse.ok) {
                    console.log(`✅ Inserted: ${setting.key} = ${setting.value}`);
                    successCount++;
                } else {
                    console.log(`❌ Failed to insert: ${setting.key}`);
                    errorCount++;
                }
            } else {
                console.log(`❌ Failed to update: ${setting.key}`);
                errorCount++;
            }
        } catch (error) {
            console.log(`❌ Error: ${setting.key} - ${error.message}`);
            errorCount++;
        }
    }
    
    console.log(`\n📊 Results: ${successCount} updated, ${errorCount} errors\n`);
    
    if (errorCount === 0) {
        console.log('✅ All settings synchronized successfully!');
        console.log('🔑 Default credentials:');
        console.log('   Username: admin');
        console.log('   Password: admin123');
        console.log('\nYou can now login to the dashboard.\n');
    } else {
        console.log('⚠️  Some updates failed. Check your Supabase connection.\n');
    }
}

updateSettings().catch(console.error);
