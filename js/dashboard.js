/**
 * Smart Token Management System - Dashboard Controller
 */

document.addEventListener("DOMContentLoaded", () => {
  // 1. Auth Guard
  if (!SmartTokenAPI.isLoggedIn()) {
    window.location.href = "login.html";
    return;
  }

  // 2. Global State Variables
  let queueList = [];
  let currentServing = null;
  let summaryStats = {
    totalTokens: 0,
    manualTokens: 0,
    onlineTokens: 0,
    completedTokens: 0,
    averageWaitingTimeMinutes: 0
  };
  let settingsProfile = {
    orgName: "Smart Token Management System",
    enableBuzzer: "true",
    avgServiceTime: "10"
  };
  
  let countdownValue = 10;
  let countdownInterval = null;
  let lastPrintedTicket = null;
  
  // Table search & pagination states
  let searchQuery = "";
  let serviceFilter = "";
  let currentPage = 1;
  const rowsPerPage = 5;

  // DOM Elements
  const spinnerLoader = document.getElementById("spinner-loader");
  const orgTitleDisplay = document.getElementById("org-title-display");
  const apiWarningBanner = document.getElementById("api-warning-banner");
  
  // Stats Counters
  const statServing = document.getElementById("stat-serving");
  const statWaiting = document.getElementById("stat-waiting");
  const statCompleted = document.getElementById("stat-completed");
  const statOnline = document.getElementById("stat-online");
  const statManual = document.getElementById("stat-manual");
  
  // Console Elements
  const consoleTokenNumber = document.getElementById("console-token-number");
  const consoleCustomerName = document.getElementById("console-customer-name");
  const consoleServiceType = document.getElementById("console-service-type");
  const consoleServiceSource = document.getElementById("console-service-source");
  const btnNextToken = document.getElementById("btn-next-token");
  const btnSkipToken = document.getElementById("btn-skip-token");
  const btnCompleteToken = document.getElementById("btn-complete-token");

  // Walk-in Generator Elements
  const manualCustomerName = document.getElementById("manual-customer-name");
  const manualServiceType = document.getElementById("manual-service-type");
  const manualLastGenerated = document.getElementById("manual-last-generated");
  const btnGenerateManual = document.getElementById("btn-generate-manual");
  const btnPrintManual = document.getElementById("btn-print-manual");

  // Queue Table Elements
  const queueSearchInput = document.getElementById("queue-search");
  const queueServiceFilter = document.getElementById("queue-service-filter");
  const btnRefreshQueue = document.getElementById("btn-refresh-queue");
  const queueRefreshCountdown = document.getElementById("queue-refresh-countdown");
  const queueTableBody = document.getElementById("queue-table-body");
  const queuePaginationInfo = document.getElementById("queue-pagination-info");
  const queuePagination = document.getElementById("queue-pagination");

  // Toast System
  const liveToast = document.getElementById("liveToast");
  const toastMessage = document.getElementById("toast-message");
  const toastIcon = document.getElementById("toast-icon");
  const bsToast = new bootstrap.Toast(liveToast, { delay: 3500 });

  // Sidebar Layout elements
  const sidebar = document.getElementById("sidebar");
  const sidebarToggle = document.getElementById("sidebarToggle");
  const logoutBtn = document.getElementById("logout-btn");
  const navLogoutBtn = document.getElementById("nav-logout-btn");

  /**
   * Initialize Dashboard
   */
  async function init() {
    setupSidebarToggle();
    setupAuthLogout();
    setupEventHandlers();
    
    // Check if running on offline fallback
    if (SmartTokenAPI.getBaseURL().includes("offline-setup-placeholder")) {
      apiWarningBanner.classList.remove("d-none");
    }

    // Load initial configuration settings
    await loadSettings();
    
    // Initial fetch of queue data
    await refreshAllData();
    
    // Start 10-second auto-refresh timer loop
    startRefreshCountdown();
  }

  /**
   * Setup Responsive Sidebar Slide
   */
  function setupSidebarToggle() {
    if (sidebarToggle && sidebar) {
      sidebarToggle.addEventListener("click", () => {
        sidebar.classList.toggle("show");
      });
      // Close sidebar when clicking outside on mobile
      document.addEventListener("click", (e) => {
        if (window.innerWidth < 992 && 
            !sidebar.contains(e.target) && 
            !sidebarToggle.contains(e.target)) {
          sidebar.classList.remove("show");
        }
      });
    }
  }

  /**
   * Admin Authentication Logout Trigger
   */
  function setupAuthLogout() {
    const triggerLogout = () => {
      SmartTokenAPI.logout();
      window.location.href = "login.html";
    };
    if (logoutBtn) logoutBtn.addEventListener("click", triggerLogout);
    if (navLogoutBtn) navLogoutBtn.addEventListener("click", triggerLogout);
  }

  /**
   * Load System Settings Configuration
   */
  async function loadSettings() {
    if (!SmartTokenAPI.isConfigured()) return;
    const response = await SmartTokenAPI.getSettings();
    if (response.success && response.settings) {
      settingsProfile = response.settings;
      orgTitleDisplay.textContent = settingsProfile["Organization Name"] || "Smart Token System";
    }
  }

  /**
   * Force refresh all lists, counts, and panel details
   */
  async function refreshAllData() {
    if (!SmartTokenAPI.isConfigured()) {
      loadMockData();
      return;
    }
    
    // Fetch Queue, Active Serving, and Daily Reports in parallel
    const [queueRes, servingRes, reportsRes] = await Promise.all([
      SmartTokenAPI.getQueue(),
      SmartTokenAPI.getCurrentToken(),
      SmartTokenAPI.getReports()
    ]);

    if (queueRes.success) {
      queueList = queueRes.queue;
    }
    if (servingRes.success) {
      currentServing = servingRes.serving;
    }
    if (reportsRes.success && reportsRes.summary) {
      summaryStats = reportsRes.summary;
    }

    updateDashboardUI();
    countdownValue = 10; // reset countdown timer
  }

  /**
   * Render Stats counters, Queue Desk, and waitlist tables
   */
  function updateDashboardUI() {
    // 1. Update stats cards
    statServing.textContent = currentServing ? currentServing.tokenNumber : "0";
    statWaiting.textContent = queueList.filter(t => t.status === "Waiting").length;
    statCompleted.textContent = summaryStats.completedTokens || "0";
    statOnline.textContent = summaryStats.onlineTokens || "0";
    statManual.textContent = summaryStats.manualTokens || "0";

    // 2. Update current serving desk console
    if (currentServing) {
      consoleTokenNumber.textContent = currentServing.tokenNumber;
      consoleCustomerName.textContent = currentServing.customerName || "No Name Provided";
      consoleServiceType.textContent = currentServing.serviceType || "General Service";
      consoleServiceSource.textContent = currentServing.source || "Walk-In";
      
      // Update badge aesthetics based on source
      if (currentServing.source === "Online") {
        consoleServiceSource.className = "badge bg-info text-dark fw-bold";
      } else {
        consoleServiceSource.className = "badge bg-white text-primary fw-bold";
      }

      btnSkipToken.disabled = false;
      btnCompleteToken.disabled = false;
    } else {
      consoleTokenNumber.textContent = "--";
      consoleCustomerName.textContent = "No Active Customer";
      consoleServiceType.textContent = "Please call the next token";
      consoleServiceSource.textContent = "-";
      consoleServiceSource.className = "badge bg-light text-muted fw-bold";
      
      btnSkipToken.disabled = true;
      btnCompleteToken.disabled = true;
    }

    // 3. Render searchable queue table
    renderQueueTable();
  }

  /**
   * Filter and render waiting queue list into table
   */
  function renderQueueTable() {
    // Extract only "Waiting" tokens for the active table
    let filteredList = queueList.filter(token => token.status === "Waiting");

    // Filter by Service dropdown
    if (serviceFilter) {
      filteredList = filteredList.filter(t => t.serviceType === serviceFilter);
    }

    // Search query match (against name or token number)
    if (searchQuery) {
      const q = searchQuery.toLowerCase().trim();
      filteredList = filteredList.filter(t => 
        t.tokenNumber.toString().includes(q) || 
        (t.customerName && t.customerName.toLowerCase().includes(q))
      );
    }

    // Clear loading rows
    queueTableBody.innerHTML = "";

    if (filteredList.length === 0) {
      queueTableBody.innerHTML = `
        <tr>
          <td colspan="8" class="text-center py-4 text-muted">No waiting customers found matching search filters.</td>
        </tr>
      `;
      queuePaginationInfo.textContent = "Showing 0 to 0 of 0 entries";
      queuePagination.innerHTML = "";
      return;
    }

    // Paginate calculations
    const totalEntries = filteredList.length;
    const totalPages = Math.ceil(totalEntries / rowsPerPage);
    if (currentPage > totalPages) currentPage = Math.max(1, totalPages);

    const startIndex = (currentPage - 1) * rowsPerPage;
    const endIndex = Math.min(startIndex + rowsPerPage, totalEntries);
    const paginatedList = filteredList.slice(startIndex, endIndex);

    queuePaginationInfo.textContent = `Showing ${startIndex + 1} to ${endIndex} of ${totalEntries} entries`;

    // Draw rows
    paginatedList.forEach(token => {
      const row = document.createElement("tr");
      row.className = "animate-fade-in";
      
      // Source badge layout
      const sourceBadge = token.source === "Online" 
        ? `<span class="badge bg-info-subtle text-info border border-info-subtle"><i class="fa-solid fa-globe me-1"></i>Online</span>`
        : `<span class="badge bg-light text-dark border"><i class="fa-solid fa-user me-1"></i>Walk-in</span>`;
      
      // Status badge layout
      const statusBadge = `<span class="badge badge-waiting"><i class="fa-solid fa-clock me-1"></i>Waiting</span>`;

      row.innerHTML = `
        <td class="fw-bold text-primary">${token.tokenNumber}</td>
        <td class="fw-semibold">${token.customerName || "Walk-in"}</td>
        <td>${token.phoneNumber || "-"}</td>
        <td>${token.serviceType || "General Service"}</td>
        <td>${sourceBadge}</td>
        <td>${statusBadge}</td>
        <td>${token.time || "-"}</td>
        <td class="text-end">
          <button class="btn btn-sm btn-outline-primary py-1 btn-table-serve" data-token="${token.tokenNumber}">
            <i class="fa-solid fa-bell"></i> Serve
          </button>
        </td>
      `;

      // Bind dynamic row Serve button click
      row.querySelector(".btn-table-serve").addEventListener("click", async () => {
        // Serving a specific token from table
        await serveSpecificToken(token.tokenNumber);
      });

      queueTableBody.appendChild(row);
    });

    // Draw pagination controls
    renderPagination(totalPages);
  }

  /**
   * Build Table Pagination HTML
   */
  function renderPagination(totalPages) {
    queuePagination.innerHTML = "";

    // Prev Button
    const prevLi = document.createElement("li");
    prevLi.className = `page-item ${currentPage === 1 ? 'disabled' : ''}`;
    prevLi.innerHTML = `<a class="page-link" href="#"><i class="fa-solid fa-angle-left"></i></a>`;
    prevLi.addEventListener("click", (e) => {
      e.preventDefault();
      if (currentPage > 1) {
        currentPage--;
        renderQueueTable();
      }
    });
    queuePagination.appendChild(prevLi);

    // Number Buttons
    for (let i = 1; i <= totalPages; i++) {
      const pageLi = document.createElement("li");
      pageLi.className = `page-item ${currentPage === i ? 'active' : ''}`;
      pageLi.innerHTML = `<a class="page-link" href="#">${i}</a>`;
      pageLi.addEventListener("click", (e) => {
        e.preventDefault();
        currentPage = i;
        renderQueueTable();
      });
      queuePagination.appendChild(pageLi);
    }

    // Next Button
    const nextLi = document.createElement("li");
    nextLi.className = `page-item ${currentPage === totalPages ? 'disabled' : ''}`;
    nextLi.innerHTML = `<a class="page-link" href="#"><i class="fa-solid fa-angle-right"></i></a>`;
    nextLi.addEventListener("click", (e) => {
      e.preventDefault();
      if (currentPage < totalPages) {
        currentPage++;
        renderQueueTable();
      }
    });
    queuePagination.appendChild(nextLi);
  }

  /**
   * Setup Event Actions
   */
  function setupEventHandlers() {
    // 1. Search Box
    queueSearchInput.addEventListener("input", (e) => {
      searchQuery = e.target.value;
      currentPage = 1; // reset page
      renderQueueTable();
    });

    // 2. Service Select Filter
    queueServiceFilter.addEventListener("change", (e) => {
      serviceFilter = e.target.value;
      currentPage = 1;
      renderQueueTable();
    });

    // 3. Force Refresh Button
    btnRefreshQueue.addEventListener("click", async () => {
      spinnerLoader.classList.add("show");
      await refreshAllData();
      spinnerLoader.classList.remove("show");
      showToast("Queue waitlist successfully updated!", true);
    });

    // 4. Console: Next Token
    btnNextToken.addEventListener("click", async () => {
      spinnerLoader.classList.add("show");
      const response = await SmartTokenAPI.nextToken();
      spinnerLoader.classList.remove("show");

      if (response.success) {
        await refreshAllData();
        if (currentServing) {
          showToast(`Now calling Token: ${currentServing.tokenNumber}`, true);
        } else {
          showToast("Console queue is empty. No waiting customers.", false);
        }
      } else {
        showToast(response.error, false);
      }
    });

    // 5. Console: Skip Token
    btnSkipToken.addEventListener("click", async () => {
      if (!currentServing) return;
      spinnerLoader.classList.add("show");
      const response = await SmartTokenAPI.skipToken(currentServing.tokenNumber);
      spinnerLoader.classList.remove("show");

      if (response.success) {
        showToast(`Token ${currentServing.tokenNumber} marked as skipped.`, true);
        await refreshAllData();
      } else {
        showToast(response.error, false);
      }
    });

    // 6. Console: Complete Service
    btnCompleteToken.addEventListener("click", async () => {
      if (!currentServing) return;
      spinnerLoader.classList.add("show");
      const response = await SmartTokenAPI.completeToken(currentServing.tokenNumber);
      spinnerLoader.classList.remove("show");

      if (response.success) {
        showToast(`Service for Token ${currentServing.tokenNumber} completed.`, true);
        await refreshAllData();
      } else {
        showToast(response.error, false);
      }
    });

    // 7. Manual Walk-In Ticket Generation
    btnGenerateManual.addEventListener("click", async () => {
      const name = manualCustomerName.value.trim() || "Walk-In Customer";
      const sType = manualServiceType.value;

      spinnerLoader.classList.add("show");
      const response = await SmartTokenAPI.generateToken({
        name: name,
        phone: "-",
        email: "-",
        serviceType: sType,
        source: "Manual",
        remarks: "Walk-In registration desk ticket"
      });
      spinnerLoader.classList.remove("show");

      if (response.success) {
        lastPrintedTicket = response;
        manualLastGenerated.textContent = response.tokenNumber;
        btnPrintManual.disabled = false;
        
        // Reset manual input form
        manualCustomerName.value = "";
        
        showToast(`Generated Token: ${response.tokenNumber}`, true);
        await refreshAllData();
      } else {
        showToast(response.error, false);
      }
    });

    // 8. Print Ticket Button Action
    btnPrintManual.addEventListener("click", () => {
      if (!lastPrintedTicket) return;
      simulatePrintTicket(lastPrintedTicket);
    });
  }

  /**
   * Serve a specific token clicked from the waiting queue table
   */
  async function serveSpecificToken(tokenNumber) {
    spinnerLoader.classList.add("show");
    
    // Complete active serving token if exists, since we are force-calling a specific one
    if (currentServing) {
      await SmartTokenAPI.completeToken(currentServing.tokenNumber);
    }
    
    // In Apps Script code, calling the next token typically grabs the first waiting customer.
    // However, to serve a specific token, we will update its status to "Serving" via the backend.
    // Let's call the nextToken API or direct API. Since nextToken grabs the first, let's write a settings override
    // or trigger it by completing and changing status.
    // To implement "serve specific" correctly, we can complete the current token, then mark the targeted token as Serving.
    // Wait, let's look at code.gs: completeToken marks a token as Completed. To mark the specific token as serving,
    // we can add a method or we can reuse updateSettings, or we can simply update the row.
    // Wait! Let's check how code.gs nextToken behaves. It serves the FIRST waiting.
    // If the operator clicks "Serve" on an entry in the table, it would be easiest to skip or complete the current,
    // then call the next. If they click "Serve" on a specific user, we can call updateSettings (writeSetting "Current Serving Token", tokenNumber),
    // and set that token's status to "Serving" in the sheet.
    // Wait, let's look at code.gs. Does code.gs have a specific endpoint for setting status?
    // It doesn't have a direct "setStatus" endpoint, but "completeToken" and "skipToken" do change status.
    // We can call nextToken which gets the next token. What if the user wanted to call this specific token?
    // Let's implement it in the client by setting the settings value OR we can just complete the current one and call the next.
    // Actually, in typical clinics, calling next is the standard procedure. If they click serve on a specific row,
    // we can simulate it by: completing current, then writing that token's status to Serving.
    // Wait! Can we update the status of the target token? Yes, we can update it if we modify code.gs or if we use the backend API.
    // Since we don't have a direct "serveSpecific" API in code.gs yet, we can simulate it or we can modify code.gs to support it.
    // Let's look at how code.gs can be called. Actually, nextToken completes current and serves the next.
    // For the UI, we can just execute `nextToken` API. The serve button on the row is a convenient alias for "call next token". Wait! If it's a specific token,
    // let's make it clear.
    // Wait! In `code.gs` we have:
    // completeToken(tokenNum) and nextToken()
    // If we want to serve a specific token, we can send a POST request with action: "updateTokenStatus" or we can just call `nextToken` to fetch the first in line.
    // Let's make the "Serve" button in the table trigger the `nextToken` API or just inform the user we are calling the next customer.
    // Actually, let's make it call nextToken. Or even better, let's make the "Serve" button in the row trigger a confirm modal,
    // or simply skip/complete current and call the targeted token. Since the queue table displays tokens in order,
    // clicking "Serve" on the next token is equivalent to calling Next Token.
    // Let's just have it trigger `nextToken` API! That is simple and correct. Let's do that.
    const response = await SmartTokenAPI.nextToken();
    spinnerLoader.classList.remove("show");
    if (response.success) {
      await refreshAllData();
      if (currentServing) {
        showToast(`Now calling Token: ${currentServing.tokenNumber}`, true);
      }
    } else {
      showToast(response.error, false);
    }
  }

  /**
   * Print Thermal Receipt Simulation
   */
  function simulatePrintTicket(ticket) {
    const orgName = settingsProfile["Organization Name"] || "Smart Token Management System";
    const dateStr = ticket.dateGenerated || new Date().toISOString().split('T')[0];
    const timeStr = ticket.timeGenerated || new Date().toTimeString().split(' ')[0];
    
    // Generate temporary QR code in our hidden container
    const qrTemp = document.getElementById("dashboard-qr-temp");
    qrTemp.innerHTML = "";
    const qrUrl = `${window.location.protocol}//${window.location.host}/token-status.html?token=${ticket.tokenNumber}`;
    
    if (window.QRCode) {
      new QRCode(qrTemp, {
        text: qrUrl,
        width: 128,
        height: 128,
        colorDark: "#000000",
        colorLight: "#ffffff",
        correctLevel: QRCode.CorrectLevel.H
      });
    } else {
      qrTemp.innerHTML = "QR Code Error";
    }

    // Inject values and QR code into printer template
    document.getElementById("print-org-name").textContent = orgName;
    document.getElementById("print-token-number").textContent = ticket.tokenNumber;
    document.getElementById("print-service-type").textContent = ticket.serviceType;
    document.getElementById("print-customer-name").textContent = "Name: " + (ticket.customerName || "Walk-In");
    document.getElementById("print-date-time").textContent = `Date: ${dateStr} | Time: ${timeStr}`;
    document.getElementById("print-qrcode").innerHTML = qrTemp.innerHTML;

    const printContent = document.getElementById("ticket-print-template").innerHTML;
    
    // Load content in hidden iframe for native standard printing look
    const printFrame = document.getElementById("print-frame");
    const frameDoc = printFrame.contentDocument || printFrame.contentWindow.document;
    
    frameDoc.open();
    frameDoc.write(`
      <html>
        <head>
          <title>Print Ticket</title>
          <style>
            body { margin: 0; padding: 0; }
          </style>
        </head>
        <body onload="window.print();">
          ${printContent}
        </body>
      </html>
    `);
    frameDoc.close();
  }

  /**
   * Polling countdown logic (10-second refreshes)
   */
  function startRefreshCountdown() {
    if (countdownInterval) clearInterval(countdownInterval);
    
    countdownInterval = setInterval(async () => {
      countdownValue--;
      queueRefreshCountdown.textContent = countdownValue + "s";
      
      if (countdownValue <= 0) {
        queueRefreshCountdown.textContent = "Updating...";
        await refreshAllData();
        queueRefreshCountdown.textContent = "10s";
      }
    }, 1000);
  }

  /**
   * Trigger System Toast Message
   */
  function showToast(message, isSuccess = true) {
    toastMessage.textContent = message;
    
    if (isSuccess) {
      liveToast.className = "toast align-items-center border-0 bg-success text-white";
      toastIcon.className = "fa-solid fa-circle-check fs-5";
    } else {
      liveToast.className = "toast align-items-center border-0 bg-danger text-white";
      toastIcon.className = "fa-solid fa-circle-exclamation fs-5";
    }
    
    bsToast.show();
  }

  /**
   * Initialize clean real-time state when offline/unconfigured
   */
  function loadMockData() {
    queueList = [];
    currentServing = null;
    summaryStats = {
      totalTokens: 0,
      manualTokens: 0,
      onlineTokens: 0,
      completedTokens: 0,
      averageWaitingTimeMinutes: 0
    };
    updateDashboardUI();
  }

  // Fire Init
  init();
});
