/**
 * Smart Token Management System - Lobby TV Monitor Controller
 */

document.addEventListener("DOMContentLoaded", () => {
  // DOM Elements
  const displayOrgName = document.getElementById("display-org-name");
  const clockDisplay = document.getElementById("clock-display");
  
  const servingTokenNumber = document.getElementById("serving-token-number");
  const servingCustomerName = document.getElementById("serving-customer-name");
  const servingServiceType = document.getElementById("serving-service-type");
  const servingCardGlow = document.getElementById("serving-card-glow");
  
  const nextTokensContainer = document.getElementById("next-tokens-container");
  const buzzerSound = document.getElementById("buzzer-sound");

  // State
  let lastServingToken = null;
  let settingsProfile = {
    orgName: "Smart Token Management System",
    enableBuzzer: "true"
  };

  /**
   * Initialize TV Display
   */
  async function init() {
    // Start clock ticking
    updateClock();
    setInterval(updateClock, 1000);

    // Fetch org configuration
    await loadSettings();

    // Initial load
    await refreshDisplayData();

    // Poll display data every 5 seconds
    setInterval(refreshDisplayData, 5000);
  }

  /**
   * Update clock text
   */
  function updateClock() {
    const now = new Date();
    clockDisplay.textContent = now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
  }

  /**
   * Load System Settings Configuration
   */
  async function loadSettings() {
    if (!SmartTokenAPI.isConfigured()) return;
    const response = await SmartTokenAPI.getSettings();
    if (response.success && response.settings) {
      settingsProfile = response.settings;
      displayOrgName.textContent = settingsProfile["Organization Name"] || "Smart Token System";
    }
  }

  /**
   * Fetch serving status and waiting queue
   */
  async function refreshDisplayData() {
    if (!SmartTokenAPI.isConfigured()) {
      simulateOfflineDisplay();
      return;
    }

    const [servingRes, queueRes] = await Promise.all([
      SmartTokenAPI.getCurrentToken(),
      SmartTokenAPI.getQueue()
    ]);

    let activeServing = null;
    let waitingQueue = [];

    if (servingRes.success && servingRes.serving) {
      activeServing = servingRes.serving;
    }
    if (queueRes.success && queueRes.queue) {
      waitingQueue = queueRes.queue.filter(t => t.status === "Waiting");
    }

    // Update UI elements
    renderActiveServing(activeServing);
    renderNextQueue(waitingQueue);
  }

  /**
   * Render the central NOW SERVING token
   */
  function renderActiveServing(token) {
    if (token) {
      servingTokenNumber.textContent = token.tokenNumber;
      servingCustomerName.textContent = token.customerName || "Walk-In";
      servingServiceType.textContent = token.serviceType || "General Service";
      servingServiceType.classList.remove("d-none");

      // Check if token number has changed to trigger voice alert
      if (lastServingToken !== token.tokenNumber) {
        triggerCallNotification(token);
        lastServingToken = token.tokenNumber;
      }
    } else {
      servingTokenNumber.textContent = "---";
      servingCustomerName.textContent = "Please wait for your number to be called";
      servingServiceType.textContent = "";
      servingServiceType.classList.add("d-none");
      lastServingToken = null;
    }
  }

  /**
   * Render the list cards for the next 3 waiting tokens
   */
  function renderNextQueue(queue) {
    nextTokensContainer.innerHTML = "";
    
    // Grab the first 3 waiting items in queue
    const nextThree = queue.slice(0, 3);

    // Fill empty slots if less than 3
    while (nextThree.length < 3) {
      nextThree.push(null);
    }

    nextThree.forEach((token, index) => {
      const col = document.createElement("div");
      col.className = "col-md-4";
      
      if (token) {
        col.innerHTML = `
          <div class="next-token-card animate-fade-in">
            <div class="next-token-number">${token.tokenNumber}</div>
            <div class="next-token-label">${token.serviceType}</div>
            <div class="text-white-50 mt-1" style="font-size: 0.8rem;">${token.customerName || 'Customer'}</div>
          </div>
        `;
      } else {
        col.innerHTML = `
          <div class="next-token-card" style="opacity: 0.3;">
            <div class="next-token-number">--</div>
            <div class="next-token-label">Lobby Empty</div>
          </div>
        `;
      }
      nextTokensContainer.appendChild(col);
    });
  }

  /**
   * Sound and Vocal Announcement Alerts
   */
  function triggerCallNotification(token) {
    // 1. Play Buzzer Sound (if enabled in settings)
    if (settingsProfile.enableBuzzer !== "false") {
      buzzerSound.play().catch(e => console.log("Audio autoplay blocked by browser policy"));
    }

    // 2. Flash display border
    servingCardGlow.classList.add("buzzer-alert");
    setTimeout(() => {
      servingCardGlow.classList.remove("buzzer-alert");
    }, 4500);

    // 3. Text-to-Speech Vocal Calling
    announceTokenVocally(token.tokenNumber);
  }

  /**
   * Vocal Text to Speech Synthesis
   */
  function announceTokenVocally(tokenNumber) {
    if ('speechSynthesis' in window) {
      // Format number digits clearly (e.g. "1 0 2" instead of "one hundred and two")
      const digitsSpoken = tokenNumber.toString().split('').join(' ');
      const textToSpeak = `Token number, ${digitsSpoken}, please proceed to the counter.`;
      
      const utterance = new SpeechSynthesisUtterance(textToSpeak);
      utterance.rate = 0.85; // slower speech for clarity
      utterance.pitch = 1.0;
      
      // Select appropriate English voice if available
      const voices = window.speechSynthesis.getVoices();
      const englishVoice = voices.find(v => v.lang.includes("en-US") || v.lang.includes("en-GB"));
      if (englishVoice) utterance.voice = englishVoice;

      window.speechSynthesis.speak(utterance);
    }
  }

  /**
   * Offline demo data simulation
   */
  function simulateOfflineDisplay() {
    renderActiveServing(null);
    renderNextQueue([]);
  }

  // Load Speech Voice list pre-emptively for browser compatibility
  if ('speechSynthesis' in window) {
    window.speechSynthesis.getVoices();
  }

  // Run
  init();
});
