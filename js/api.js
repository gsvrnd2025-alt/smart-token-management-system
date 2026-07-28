/**
 * Smart Token Management System - Client API Wrapper (Supabase Version)
 * 
 * Communicates directly with the Supabase Postgres database.
 */

const SmartTokenAPI = (function() {
  const STORAGE_KEY_SESSION = "smart_token_session";
  
  // Set your Supabase details here:
  const SUPABASE_URL = "https://lziwnwdiyfdgyznngcma.supabase.co";
  const SUPABASE_ANON_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imx6aXdud2RpeWZkZ3l6bm5nY21hIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODQ3MDQ2OTYsImV4cCI6MjEwMDI4MDY5Nn0.Gpe-dZfVIjLhFT_VP__uawVjwPMbDciUfmFzWNz5hpc";
  
  let supabase = null;

  async function getClient() {
    if (supabase) return supabase;
    if (!window.supabase) {
        // Dynamically load the Supabase library if not present in HTML
        await new Promise((resolve, reject) => {
            const script = document.createElement("script");
            script.src = "https://cdn.jsdelivr.net/npm/@supabase/supabase-js@2";
            script.onload = resolve;
            script.onerror = reject;
            document.head.appendChild(script);
        });
    }
    supabase = window.supabase.createClient(SUPABASE_URL, SUPABASE_ANON_KEY);
    return supabase;
  }

  // --- API Endpoints ---

  async function verifyLogin(username, password) {
    try {
      const db = await getClient();
      
      // Try to get both username and password from settings (using regular select, not single)
      // Handle case where these settings might not exist yet (initialize to defaults)
      const { data: usernameArray, error: usernameError } = await db.from('settings').select('value').eq('key', 'Admin Username');
      const { data: passwordArray, error: passwordError } = await db.from('settings').select('value').eq('key', 'Admin Password');
      
      // Extract values from arrays with fallbacks
      // If no results, fall back to defaults
      const dbUsername = (usernameArray && usernameArray.length > 0) ? usernameArray[0].value : 'admin';
      const dbPassword = (passwordArray && passwordArray.length > 0) ? passwordArray[0].value : 'admin123';
      
      console.log("✓ Database query completed. Username:", dbUsername, "Password length:", dbPassword?.length);

      // If DB error occurred (e.g. TypeError: Failed to fetch) OR query failed:
      if (usernameError || passwordError) {
        console.warn("DB fetch warning during login:", usernameError || passwordError);
        if (username.toLowerCase() === "admin" && password === "admin123") {
          const token = "session_offline_" + Math.random().toString(36).substr(2);
          localStorage.setItem(STORAGE_KEY_SESSION, token);
          console.log("✓ Offline Authentication successful for user:", username);
          return { success: true, message: "Offline Authentication Successful", token: token };
        }
      }
      
      // Verify credentials against DB values
      if (username.toLowerCase() === dbUsername.toLowerCase() && password === dbPassword) {
        const token = "session_" + Math.random().toString(36).substr(2);
        localStorage.setItem(STORAGE_KEY_SESSION, token);
        console.log("✓ Authentication successful for user:", username);
        return { success: true, message: "Authentication successful", token: token };
      }
      
      // Fallback for admin / admin123
      if (username.toLowerCase() === "admin" && password === "admin123") {
        const token = "session_offline_" + Math.random().toString(36).substr(2);
        localStorage.setItem(STORAGE_KEY_SESSION, token);
        console.log("✓ Fallback Authentication successful for admin");
        return { success: true, message: "Authentication successful (admin fallback)", token: token };
      }

      console.warn("✗ Authentication failed for user:", username);
      console.warn(`  Expected: ${dbUsername} / ${dbPassword}`);
      console.warn(`  Got: ${username} / ${password}`);
      return { success: false, error: "Invalid username or password" };
    } catch (error) {
      console.error("✗ Login verification error:", error);
      // Offline fallback: Allow default admin / admin123 if network or Supabase is unreachable
      if (username.toLowerCase() === "admin" && password === "admin123") {
        const token = "session_offline_" + Math.random().toString(36).substr(2);
        localStorage.setItem(STORAGE_KEY_SESSION, token);
        console.log("✓ Offline Authentication successful for user:", username);
        return { success: true, message: "Offline Authentication Successful", token: token };
      }
      return { success: false, error: "Database Connection Error: " + error.message + ". Try using admin / admin123." };
    }
  }

  function isLoggedIn() {
    return localStorage.getItem(STORAGE_KEY_SESSION) !== null;
  }

  function logout() {
    localStorage.removeItem(STORAGE_KEY_SESSION);
  }

  async function generateToken(details) {
    try {
      const db = await getClient();
      
      // Get settings parameters
      const { data: set1, error: e1 } = await db.from('settings').select('value').eq('key', 'Last Generated Token').single();
      const { data: set2, error: e2 } = await db.from('settings').select('value').eq('key', 'Starting Token Number').single();
      const { data: set3, error: e3 } = await db.from('settings').select('value').eq('key', 'Average Service Time').single();
      
      if (e1 || e2 || e3) {
        console.error("✗ Failed to fetch settings:", { e1, e2, e3 });
        return { success: false, error: "Failed to fetch system settings" };
      }
      
      let lastToken = parseInt(set1?.value || "0");
      let startingToken = parseInt(set2?.value || "1");
      let avgServiceTime = parseInt(set3?.value || "10");
      
      let newTokenNum = lastToken + 1;
      if (newTokenNum < startingToken) newTokenNum = startingToken;

      // Insert new token
      const { error: insErr } = await db.from('tokens').insert([{
        token_number: newTokenNum,
        customer_name: details.name || "Walk-In",
        phone_number: details.phone || "-",
        email: details.email || "-",
        service_type: details.serviceType || "General",
        source: details.source || "Manual",
        status: "Waiting",
        remarks: details.remarks || ""
      }]);

      if (insErr) {
        console.error("✗ Token insertion error:", insErr);
        return { success: false, error: insErr.message };
      }

      // Update settings last token
      const { error: updateErr } = await db.from('settings').update({ value: newTokenNum.toString() }).eq('key', 'Last Generated Token');
      if (updateErr) {
        console.error("✗ Failed to update last generated token:", updateErr);
      }

      // Calculate wait time
      const { count, error: countErr } = await db.from('tokens')
          .select('*', { count: 'exact', head: true })
          .in('status', ['Waiting', 'Serving'])
          .lt('token_number', newTokenNum);

      if (countErr) {
        console.error("✗ Failed to calculate queue count:", countErr);
      }

      console.log("✓ Token generated:", newTokenNum);
      return {
        success: true,
        tokenNumber: newTokenNum,
        customerName: details.name || "Walk-In",
        serviceType: details.serviceType || "General",
        source: details.source || "Manual",
        estimatedWaitingTimeMinutes: (count || 0) * avgServiceTime,
        timeGenerated: new Date().toLocaleTimeString(),
        dateGenerated: new Date().toLocaleDateString()
      };
    } catch (error) {
      console.error("✗ Token generation error:", error);
      return { success: false, error: "Token generation error: " + error.message };
    }
  }

  async function getQueue() {
    const db = await getClient();
    const { data, error } = await db.from('tokens')
        .select('*')
        .in('status', ['Waiting', 'Serving'])
        .order('token_number', { ascending: true });
    
    if (error) return { success: false, queue: [] };
    
    return {
      success: true,
      queue: data.map(d => ({
        tokenNumber: d.token_number,
        customerName: d.customer_name,
        phoneNumber: d.phone_number,
        email: d.email,
        serviceType: d.service_type,
        source: d.source,
        status: d.status,
        date: new Date(d.created_at).toLocaleDateString(),
        time: new Date(d.created_at).toLocaleTimeString(),
        remarks: d.remarks
      }))
    };
  }

  async function nextToken() {
    const db = await getClient();
    // Complete current serving token first
    await db.from('tokens').update({ status: 'Completed' }).eq('status', 'Serving');
    
    // Find next waiting
    const { data: waitingData } = await db.from('tokens')
        .select('*').eq('status', 'Waiting')
        .order('token_number', { ascending: true })
        .limit(1);
        
    if (waitingData && waitingData.length > 0) {
        const next = waitingData[0];
        await db.from('tokens').update({ status: 'Serving' }).eq('id', next.id);
        await db.from('settings').update({ value: next.token_number.toString() }).eq('key', 'Current Serving Token');
        return { success: true, message: "Serving next token", serving: {
            tokenNumber: next.token_number,
            customerName: next.customer_name,
            serviceType: next.service_type,
            source: next.source,
            status: "Serving"
        }};
    } else {
        await db.from('settings').update({ value: "0" }).eq('key', 'Current Serving Token');
        return { success: true, message: "No waiting tokens in queue", serving: null };
    }
  }

  async function completeToken(tokenNumber) {
    const db = await getClient();
    await db.from('tokens').update({ status: 'Completed' }).eq('token_number', tokenNumber);
    const { data } = await db.from('settings').select('value').eq('key', 'Current Serving Token').single();
    if (data && data.value === tokenNumber.toString()) {
        await db.from('settings').update({ value: "0" }).eq('key', 'Current Serving Token');
    }
    return { success: true };
  }

  async function skipToken(tokenNumber) {
    const db = await getClient();
    await db.from('tokens').update({ status: 'Skipped' }).eq('token_number', tokenNumber);
    const { data } = await db.from('settings').select('value').eq('key', 'Current Serving Token').single();
    if (data && data.value === tokenNumber.toString()) {
        await db.from('settings').update({ value: "0" }).eq('key', 'Current Serving Token');
    }
    return { success: true };
  }

  async function getCurrentToken() {
    const db = await getClient();
    const { data, error } = await db.from('tokens').select('*').eq('status', 'Serving').single();
    if (error || !data) return { success: true, serving: null };
    
    return { success: true, serving: {
        tokenNumber: data.token_number,
        customerName: data.customer_name,
        serviceType: data.service_type,
        source: data.source,
        status: data.status
    }};
  }

  async function getReports() {
    const db = await getClient();
    const today = new Date();
    today.setHours(0,0,0,0);
    const { data, error } = await db.from('tokens').select('*').gte('created_at', today.toISOString());
    
    if (error) return { success: false };
    
    let stats = {
        totalTokens: data.length,
        manualTokens: data.filter(d => d.source === 'Manual').length,
        onlineTokens: data.filter(d => d.source === 'Online').length,
        completedTokens: data.filter(d => d.status === 'Completed').length,
        skippedTokens: data.filter(d => d.status === 'Skipped').length,
        averageWaitingTimeMinutes: 0
    };
    
    return { success: true, summary: stats, distributions: { byService: {}, byHour: {} }, data: data };
  }

  async function getSettings() {
    const db = await getClient();
    const { data, error } = await db.from('settings').select('*');
    if (error) return { success: false, settings: {} };
    
    let settings = {};
    data.forEach(d => {
        if (d.key !== 'Admin Password') settings[d.key] = d.value;
    });
    return { success: true, settings: settings };
  }

  async function updateSettings(settingsData) {
    const db = await getClient();
    for (const [key, value] of Object.entries(settingsData)) {
       if (value !== undefined && value !== null) {
           let mapKey = key;
           if (key === 'startingToken') mapKey = 'Starting Token Number';
           else if (key === 'avgServiceTime') mapKey = 'Average Service Time';
           else if (key === 'orgName') mapKey = 'Organization Name';
           else if (key === 'enableBuzzer') mapKey = 'Enable Buzzer';
           else if (key === 'thermalPrinterSettings') mapKey = 'Thermal Printer Settings';
           else if (key === 'newPassword') mapKey = 'Admin Password';
           
           await db.from('settings').upsert({ key: mapKey, value: value.toString() });
       }
    }
    return await getSettings();
  }

  async function getTokenDetails(tokenNumber) {
    if (!isConfigured() || SUPABASE_URL.includes("offline-setup-placeholder")) {
      return {
        success: false,
        error: "Database not configured"
      };
    }
    
    const db = await getClient();
    const cleanTokenNum = parseInt(tokenNumber.toString().replace(/\D/g, ''), 10);
    
    const { data, error } = await db.from('tokens')
        .select('*')
        .eq('token_number', isNaN(cleanTokenNum) ? tokenNumber : cleanTokenNum)
        .order('created_at', { ascending: false })
        .limit(1);
        
    if (error) return { success: false, error: error.message };
    if (!data || data.length === 0) return { success: false, error: "Token not found" };
    
    const token = data[0];
    
    // Calculate waiting time estimate if status is Waiting
    let estimatedWait = 0;
    if (token.status === 'Waiting') {
      const { count } = await db.from('tokens')
          .select('*', { count: 'exact', head: true })
          .in('status', ['Waiting', 'Serving'])
          .lt('token_number', token.token_number);
          
      const { data: set3 } = await db.from('settings').select('value').eq('key', 'Average Service Time').single();
      const avgServiceTime = parseInt(set3?.value || "10");
      estimatedWait = (count || 0) * avgServiceTime;
    }
    
    return {
      success: true,
      token: {
        tokenNumber: token.token_number,
        customerName: token.customer_name,
        phoneNumber: token.phone_number,
        email: token.email,
        serviceType: token.service_type,
        source: token.source,
        status: token.status,
        date: new Date(token.created_at).toLocaleDateString(),
        time: new Date(token.created_at).toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', hour12: true }),
        remarks: token.remarks,
        estimatedWaitingTimeMinutes: estimatedWait
      }
    };
  }

  // Backwards compatibility stubs for UI
  function setBaseURL(url) { return true; }
  function getBaseURL() { return SUPABASE_URL; }
  function isConfigured() { return SUPABASE_ANON_KEY !== "YOUR_SUPABASE_ANON_KEY_HERE"; }

  return {
    setBaseURL, getBaseURL, isConfigured, isLoggedIn, logout, verifyLogin,
    generateToken, getQueue, nextToken, completeToken, skipToken,
    getCurrentToken, getReports, getSettings, updateSettings, getTokenDetails
  };
})();
