#!/usr/bin/env node

/**
 * Fix missing Admin Username setting in Supabase
 */

const SUPABASE_URL = "https://lziwnwdiyfdgyznngcma.supabase.co";
const SUPABASE_ANON_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imx6aXdud2RpeWZkZ3l6bm5nY21hIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODQ3MDQ2OTYsImV4cCI6MjEwMDI4MDY5Nn0.Gpe-dZfVIjLhFT_VP__uawVjwPMbDciUfmFzWNz5hpc";

async function insertMissingSettings() {
    console.log('🔧 Inserting missing settings...\n');
    
    const settingsToInsert = [
        { key: 'Admin Username', value: 'admin' },
        { key: 'Admin Email', value: 'admin@example.com' },
    ];
    
    for (const setting of settingsToInsert) {
        try {
            console.log(`Inserting: ${setting.key}...`);
            
            const response = await fetch(
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
            
            const data = await response.json();
            
            if (response.ok) {
                console.log(`✅ Inserted: ${setting.key} = ${setting.value}\n`);
            } else if (response.status === 409) {
                console.log(`⚠️  Already exists: ${setting.key}\n`);
            } else {
                console.log(`❌ Failed: ${setting.key}`);
                console.log(`   Status: ${response.status}`);
                console.log(`   Response: ${JSON.stringify(data, null, 2)}\n`);
            }
        } catch (error) {
            console.log(`❌ Error inserting ${setting.key}: ${error.message}\n`);
        }
    }
    
    console.log('✅ Done! All missing settings have been inserted.\n');
}

insertMissingSettings().catch(console.error);
