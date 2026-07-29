/**
 * Smart Token Management System - Reports and Analytics Controller
 */

document.addEventListener("DOMContentLoaded", () => {
  // 1. Auth Guard
  if (!SmartTokenAPI.isLoggedIn()) {
    window.location.href = "login.html";
    return;
  }

  // 2. State Management
  let reportLogs = [];
  let reportSummary = {};
  let distributions = { byService: {}, byHour: {} };
  
  let serviceChartInstance = null;
  let trafficChartInstance = null;

  // Search/Filters
  let searchQuery = "";
  let statusFilter = "";
  let currentPage = 1;
  const rowsPerPage = 10;

  // DOM Elements
  const spinnerLoader = document.getElementById("spinner-loader");
  const orgTitleDisplay = document.getElementById("org-title-display");
  
  // Cards
  const repStatTotal = document.getElementById("rep-stat-total");
  const repStatCompleted = document.getElementById("rep-stat-completed");
  const repStatWaiting = document.getElementById("rep-stat-waiting");
  const repStatRatio = document.getElementById("rep-stat-ratio");
  
  // Table
  const logSearchInput = document.getElementById("log-search");
  const logStatusFilter = document.getElementById("log-status-filter");
  const logTableBody = document.getElementById("log-table-body");
  const logPaginationInfo = document.getElementById("log-pagination-info");
  const logPagination = document.getElementById("log-pagination");
  
  // Buttons
  const btnExportCSV = document.getElementById("btn-export-csv");
  const btnExportPDF = document.getElementById("btn-export-pdf");
  
  // Sidebar Mobile Trigger
  const sidebar = document.getElementById("sidebar");
  const sidebarToggle = document.getElementById("sidebarToggle");
  const logoutBtn = document.getElementById("logout-btn");

  /**
   * Page Initialization
   */
  async function init() {
    setupSidebarToggle();
    setupLogout();
    setupEventHandlers();
    
    await loadSettings();
    await fetchReports();
  }

  /**
   * Sidebar Drawer control
   */
  function setupSidebarToggle() {
    if (sidebarToggle && sidebar) {
      sidebarToggle.addEventListener("click", () => {
        sidebar.classList.toggle("show");
      });
    }
  }

  /**
   * Terminate Staff session
   */
  function setupLogout() {
    if (logoutBtn) {
      logoutBtn.addEventListener("click", () => {
        SmartTokenAPI.logout();
        window.location.href = "login.html";
      });
    }
  }

  /**
   * Load System Settings Configuration
   */
  async function loadSettings() {
    if (!SmartTokenAPI.isConfigured()) return;
    const response = await SmartTokenAPI.getSettings();
    if (response.success && response.settings) {
      orgTitleDisplay.textContent = response.settings["Organization Name"] || "Smart Token System";
    }
  }

  /**
   * Query backend REST endpoint for stats
   */
  async function fetchReports() {
    spinnerLoader.classList.add("show");
    
    if (SmartTokenAPI.isConfigured()) {
      const response = await SmartTokenAPI.getReports();
      spinnerLoader.classList.remove("show");

      if (response.success) {
        reportSummary = response.summary;
        reportLogs = response.data || [];
        distributions = response.distributions || { byService: {}, byHour: {} };
      } else {
        alert(response.error);
        loadMockData();
      }
    } else {
      // Offline fallback
      spinnerLoader.classList.remove("show");
      loadMockData();
    }

    updateReportsUI();
    renderCharts();
  }

  /**
   * Bind DOM Actions
   */
  function setupEventHandlers() {
    // Search Filter
    logSearchInput.addEventListener("input", (e) => {
      searchQuery = e.target.value;
      currentPage = 1;
      renderLogTable();
    });

    // Status Select Filter
    logStatusFilter.addEventListener("change", (e) => {
      statusFilter = e.target.value;
      currentPage = 1;
      renderLogTable();
    });

    // Export CSV Trigger
    btnExportCSV.addEventListener("click", () => {
      exportCSV();
    });

    // Export PDF Print Trigger
    btnExportPDF.addEventListener("click", () => {
      window.print();
    });
  }

  /**
   * Map summary metrics and lists onto table
   */
  function updateReportsUI() {
    repStatTotal.textContent = reportSummary.totalTokens || "0";
    repStatCompleted.textContent = reportSummary.completedTokens || "0";
    repStatWaiting.textContent = (reportSummary.averageWaitingTimeMinutes || "0") + "m";
    repStatRatio.textContent = `${reportSummary.onlineTokens || '0'} / ${reportSummary.manualTokens || '0'}`;

    renderLogTable();
  }

  /**
   * Render logs database table rows
   */
  function renderLogTable() {
    let filteredList = [...reportLogs];

    // Status category filter
    if (statusFilter) {
      filteredList = filteredList.filter(l => l.status === statusFilter);
    }

    // Name match filter
    if (searchQuery) {
      const q = searchQuery.toLowerCase().trim();
      filteredList = filteredList.filter(l => 
        (l.customerName && l.customerName.toLowerCase().includes(q)) || 
        l.tokenNumber.toString().includes(q)
      );
    }

    logTableBody.innerHTML = "";

    if (filteredList.length === 0) {
      logTableBody.innerHTML = `
        <tr>
          <td colspan="8" class="text-center py-4 text-muted">No transactions logged matching filters.</td>
        </tr>
      `;
      logPaginationInfo.textContent = "Showing 0 to 0 of 0 entries";
      logPagination.innerHTML = "";
      return;
    }

    // Pagination calculations
    const totalEntries = filteredList.length;
    const totalPages = Math.ceil(totalEntries / rowsPerPage);
    if (currentPage > totalPages) currentPage = Math.max(1, totalPages);

    const startIndex = (currentPage - 1) * rowsPerPage;
    const endIndex = Math.min(startIndex + rowsPerPage, totalEntries);
    const paginatedList = filteredList.slice(startIndex, endIndex);

    logPaginationInfo.textContent = `Showing ${startIndex + 1} to ${endIndex} of ${totalEntries} entries`;

    paginatedList.forEach(log => {
      const row = document.createElement("tr");
      
      // Status badge builder
      let badgeClass = "badge-waiting";
      if (log.status === "Completed") badgeClass = "badge-completed";
      else if (log.status === "Serving") badgeClass = "badge-serving";
      else if (log.status === "Skipped") badgeClass = "badge-skipped";

      const sourceBadge = log.source === "Online" 
        ? `<span class="badge bg-info-subtle text-info border border-info-subtle"><i class="fa-solid fa-globe me-1"></i>Online</span>`
        : `<span class="badge bg-light text-dark border"><i class="fa-solid fa-user me-1"></i>Walk-in</span>`;

      row.innerHTML = `
        <td class="fw-bold">${log.tokenNumber}</td>
        <td class="fw-semibold">${log.customerName || "Walk-In"}</td>
        <td>${log.phoneNumber || "-"}</td>
        <td>${log.serviceType || "General Service"}</td>
        <td>${sourceBadge}</td>
        <td><span class="badge ${badgeClass}">${log.status}</span></td>
        <td>${log.time || "-"}</td>
        <td class="text-muted text-truncate" style="max-width: 150px;" title="${log.remarks || ''}">${log.remarks || "-"}</td>
      `;
      logTableBody.appendChild(row);
    });

    renderPaginationControls(totalPages);
  }

  /**
   * Render Table Pagination Elements
   */
  function renderPaginationControls(totalPages) {
    logPagination.innerHTML = "";

    const prevLi = document.createElement("li");
    prevLi.className = `page-item ${currentPage === 1 ? 'disabled' : ''}`;
    prevLi.innerHTML = `<a class="page-link" href="#"><i class="fa-solid fa-angle-left"></i></a>`;
    prevLi.addEventListener("click", (e) => {
      e.preventDefault();
      if (currentPage > 1) {
        currentPage--;
        renderLogTable();
      }
    });
    logPagination.appendChild(prevLi);

    for (let i = 1; i <= totalPages; i++) {
      const pageLi = document.createElement("li");
      pageLi.className = `page-item ${currentPage === i ? 'active' : ''}`;
      pageLi.innerHTML = `<a class="page-link" href="#">${i}</a>`;
      pageLi.addEventListener("click", (e) => {
        e.preventDefault();
        currentPage = i;
        renderLogTable();
      });
      logPagination.appendChild(pageLi);
    }

    const nextLi = document.createElement("li");
    nextLi.className = `page-item ${currentPage === totalPages ? 'disabled' : ''}`;
    nextLi.innerHTML = `<a class="page-link" href="#"><i class="fa-solid fa-angle-right"></i></a>`;
    nextLi.addEventListener("click", (e) => {
      e.preventDefault();
      if (currentPage < totalPages) {
        currentPage++;
        renderLogTable();
      }
    });
    logPagination.appendChild(nextLi);
  }

  /**
   * Instantiate visual Chart JS canvasses
   */
  function renderCharts() {
    // Destroy previous instances to avoid redraw overlays
    if (serviceChartInstance) serviceChartInstance.destroy();
    if (trafficChartInstance) trafficChartInstance.destroy();

    // Chart 1: Service Categories Doughnut
    const serviceLabels = Object.keys(distributions.byService || {});
    const serviceValues = Object.values(distributions.byService || {});

    const serviceCtx = document.getElementById("serviceChart").getContext("2d");
    serviceChartInstance = new Chart(serviceCtx, {
      type: "doughnut",
      data: {
        labels: serviceLabels.length > 0 ? serviceLabels : ["No Records"],
        datasets: [{
          data: serviceValues.length > 0 ? serviceValues : [1],
          backgroundColor: ["#1a73e8", "#34a853", "#fbbc05", "#ea4335", "#9333ea"],
          borderWidth: 2,
          borderColor: "#ffffff"
        }]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
          legend: { position: "bottom" }
        }
      }
    });

    // Chart 2: Hourly traffic analysis Line graph
    const hourLabels = Object.keys(distributions.byHour || {}).sort();
    const hourValues = hourLabels.map(h => distributions.byHour[h]);

    const trafficCtx = document.getElementById("trafficChart").getContext("2d");
    trafficChartInstance = new Chart(trafficCtx, {
      type: "line",
      data: {
        labels: hourLabels.length > 0 ? hourLabels : ["09:00", "12:00", "15:00", "18:00"],
        datasets: [{
          label: "Volume called",
          data: hourValues.length > 0 ? hourValues : [0, 0, 0, 0],
          borderColor: "#1a73e8",
          backgroundColor: "rgba(26, 115, 232, 0.1)",
          fill: true,
          tension: 0.35,
          borderWidth: 3
        }]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
          legend: { display: false }
        },
        scales: {
          y: { beginAtZero: true, ticks: { stepSize: 1 } }
        }
      }
    });
  }

  /**
   * CSV compilation and download simulator
   */
  function exportCSV() {
    if (reportLogs.length === 0) {
      alert("No log records available to export.");
      return;
    }

    let csvContent = "data:text/csv;charset=utf-8,";
    csvContent += "Token Number,Customer Name,Phone,Email,Service Type,Source,Status,Remarks\n";

    reportLogs.forEach(log => {
      const row = [
        `"${log.tokenNumber}"`,
        `"${log.customerName || 'Walk-In'}"`,
        `"${log.phoneNumber || '-'}"`,
        `"${log.email || '-'}"`,
        `"${log.serviceType || 'General Service'}"`,
        `"${log.source}"`,
        `"${log.status}"`,
        `"${(log.remarks || '').replace(/"/g, '""')}"`
      ].join(",");
      csvContent += row + "\n";
    });

    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", `queue_report_${new Date().toISOString().split('T')[0]}.csv`);
    document.body.appendChild(link); // Required for FF
    link.click();
    document.body.removeChild(link);
  }

  /**
   * Load mock data for layout configuration
   */
  function loadMockData() {
    reportSummary = {
      totalTokens: 0,
      completedTokens: 0,
      onlineTokens: 0,
      manualTokens: 0,
      averageWaitingTimeMinutes: 0
    };
    distributions = { byService: {}, byHour: {} };
    reportLogs = [];
    updateReportUI();
  }

  // Run
  init();
});
