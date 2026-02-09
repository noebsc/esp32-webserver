#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

// ===== CONFIG WIFI =====
const char* SSID = "Noe";
const char* PASSWORD = "noebsc41";
const char* AP_SSID = "AERA";
const char* AP_PASSWORD = "noebsc41";

// ===== CONFIG FIREBASE =====
#define API_KEY "AIzaSyDeSFTWEKGrydz1LQbsZCZJLRyX2uG7cYA"
#define DATABASE_URL "https://aera-projectid-default-rtdb.europe-west1.firebasedatabase.app"
#define USER_EMAIL "besancon.noe@gmail.com"
#define USER_PASSWORD "Besancon203+*"

// ===== PINS =====
#define DHT_PIN 8
#define DHT_TYPE DHT22
#define PIR1_PIN 10
#define PIR2_PIN 11
#define POT_PIN 6
#define SERVO1_PIN 4
#define SERVO2_PIN 5
#define SERVO3_PIN 7
#define SERVO4_PIN 9

// ===== OBJETS =====
DHT dht(DHT_PIN, DHT_TYPE);
AsyncWebServer server(80);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ===== VARIABLES GLOBALES =====
float temperature = 0.0;
float humidity = 0.0;
int potValue = 0;
boolean pir1Detected = false;
boolean pir2Detected = false;

int servo1Pos = 0;
int servo2Pos = 0;
int servo3Pos = 0;
int servo4Pos = 0;

float tempMin = 19.0;
float tempMax = 26.0;
String currentMode = "Auto";
boolean systemActive = true;

unsigned long lastRead = 0;
const unsigned long READ_INTERVAL = 2000;

// ===== HTML PAGE COMPLÈTE =====
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AERA - Système Intelligent</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        :root {
            --bg-primary: #121212;
            --bg-secondary: #1E1E1E;
            --bg-tertiary: #2A2A2A;
            --text-primary: #FFFFFF;
            --text-secondary: #B0B0B0;
            --accent: #1E88E5;
            --success: #4CAF50;
            --danger: #FF5252;
            --warning: #FFC107;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, var(--bg-primary) 0%, var(--bg-secondary) 100%);
            color: var(--text-primary);
            min-height: 100vh;
        }
        #loginPage {
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            padding: 20px;
        }
        .login-container {
            background: var(--bg-secondary);
            padding: 40px;
            border-radius: 15px;
            max-width: 400px;
            width: 100%;
            border: 1px solid rgba(255,255,255,0.1);
            box-shadow: 0 8px 32px rgba(0,0,0,0.3);
            animation: slideUp 0.5s ease;
        }
        @keyframes slideUp {
            from { opacity: 0; transform: translateY(20px); }
            to { opacity: 1; transform: translateY(0); }
        }
        .logo { font-size: 48px; font-weight: bold; text-align: center; margin-bottom: 10px; color: var(--accent); }
        .login-title { text-align: center; margin-bottom: 30px; }
        .login-title h1 { font-size: 28px; margin-bottom: 5px; }
        .login-title p { color: var(--text-secondary); font-size: 14px; }
        .form-group { margin-bottom: 20px; }
        label { display: block; margin-bottom: 8px; font-size: 14px; color: var(--text-secondary); }
        select, input[type="password"] { 
            width: 100%; padding: 12px; background: var(--bg-tertiary); border: 1px solid rgba(255,255,255,0.1);
            color: var(--text-primary); border-radius: 8px; font-size: 14px;
        }
        select:focus, input[type="password"]:focus { outline: none; border-color: var(--accent); }
        .login-btn { 
            width: 100%; padding: 12px; background: var(--accent); color: white; border: none;
            border-radius: 8px; cursor: pointer; font-weight: 600; margin-top: 10px; transition: all 0.3s;
        }
        .login-btn:hover { background: #1565c0; transform: translateY(-2px); box-shadow: 0 8px 16px rgba(30,136,229,0.3); }
        .error { color: var(--danger); text-align: center; margin-top: 15px; display: none; }
        
        #mainPage { display: none; }
        header { background: var(--bg-secondary); padding: 20px; border-radius: 10px; margin: 20px;
                 display: flex; justify-content: space-between; align-items: center; border: 1px solid rgba(255,255,255,0.1); }
        .header-left { display: flex; align-items: center; gap: 20px; }
        .header-logo { font-size: 32px; font-weight: bold; color: var(--accent); }
        .header-info { display: flex; flex-direction: column; gap: 5px; }
        .header-info span { font-size: 14px; color: var(--text-secondary); }
        .logout-btn { padding: 10px 20px; background: var(--danger); color: white; border: none; border-radius: 8px; cursor: pointer; }
        .logout-btn:hover { background: #d32f2f; }
        
        .content { max-width: 1200px; margin: 0 auto; padding: 0 20px 20px; }
        .tabs { display: flex; gap: 10px; margin-bottom: 20px; border-bottom: 1px solid rgba(255,255,255,0.1); padding-bottom: 10px; }
        .tab-btn { padding: 10px 20px; background: transparent; color: var(--text-secondary); border: none;
                  cursor: pointer; font-weight: 600; border-bottom: 2px solid transparent; margin-bottom: -11px; }
        .tab-btn.active { color: var(--accent); border-bottom-color: var(--accent); }
        .tab-content { display: none; }
        .tab-content.active { display: block; }
        
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin-bottom: 20px; }
        .card { background: var(--bg-secondary); padding: 20px; border-radius: 10px; border: 1px solid rgba(255,255,255,0.1);
               transition: all 0.3s; }
        .card:hover { background: var(--bg-tertiary); border-color: var(--accent); }
        .card-title { font-size: 13px; color: var(--text-secondary); text-transform: uppercase; margin-bottom: 10px; }
        .card-value { font-size: 32px; font-weight: bold; margin-bottom: 5px; }
        .card-unit { font-size: 14px; color: var(--text-secondary); }
        .status { display: inline-block; padding: 6px 12px; border-radius: 4px; font-size: 12px; font-weight: 600; margin-top: 8px; }
        .status.ok { background: rgba(76,175,80,0.2); color: var(--success); }
        .status.error { background: rgba(255,82,82,0.2); color: var(--danger); }
        
        .section { margin-bottom: 30px; }
        .section h2 { font-size: 20px; margin-bottom: 20px; color: var(--accent); }
        
        .slider-group { margin-bottom: 20px; }
        .slider-label { display: flex; justify-content: space-between; margin-bottom: 8px; font-size: 14px; }
        input[type="range"] { width: 100%; height: 6px; border-radius: 3px; background: var(--bg-tertiary); outline: none; -webkit-appearance: none; }
        input[type="range"]::-webkit-slider-thumb { -webkit-appearance: none; width: 18px; height: 18px; border-radius: 50%;
                                                     background: var(--accent); cursor: pointer; }
        input[type="range"]::-moz-range-thumb { width: 18px; height: 18px; border-radius: 50%; background: var(--accent); border: none; cursor: pointer; }
        
        .buttons { display: flex; gap: 10px; flex-wrap: wrap; margin-top: 15px; }
        .btn { padding: 10px 20px; background: var(--accent); color: white; border: none; border-radius: 8px;
              cursor: pointer; font-weight: 600; transition: all 0.3s; }
        .btn:hover { background: #1565c0; transform: translateY(-2px); }
        .btn.secondary { background: var(--bg-tertiary); color: var(--accent); border: 1px solid var(--accent); }
        
        .windows { display: grid; grid-template-columns: repeat(2, 1fr); gap: 15px; margin-top: 20px; }
        .window-card { background: var(--bg-tertiary); padding: 20px; border-radius: 8px; text-align: center; }
        .window-card h3 { font-size: 14px; margin-bottom: 10px; color: var(--text-secondary); }
        .window-value { font-size: 28px; font-weight: bold; color: var(--accent); margin-bottom: 10px; }
        
        .notification { position: fixed; bottom: 20px; right: 20px; background: var(--bg-secondary); padding: 15px 20px;
                       border-radius: 8px; border-left: 4px solid var(--success); animation: slideIn 0.3s; z-index: 1000; }
        @keyframes slideIn { from { transform: translateX(400px); opacity: 0; } to { transform: translateX(0); opacity: 1; } }
        .notification.error { border-left-color: var(--danger); }
        
        @media (max-width: 768px) {
            .grid { grid-template-columns: 1fr; }
            .windows { grid-template-columns: 1fr; }
            header { flex-direction: column; text-align: center; }
            .tabs { flex-wrap: wrap; }
            .card-value { font-size: 24px; }
        }
    </style>
</head>
<body>
    <!-- LOGIN PAGE -->
    <div id="loginPage">
        <div class="login-container">
            <div class="logo">A</div>
            <div class="login-title">
                <h1>AERA</h1>
                <p>Système de Gestion Intelligent</p>
            </div>
            <form id="loginForm" onsubmit="login(event)">
                <div class="form-group">
                    <label>Rôle</label>
                    <select id="role" required>
                        <option value="">-- Sélectionnez --</option>
                        <option value="Administrateur">Administrateur</option>
                        <option value="Technicien">Technicien</option>
                        <option value="Consultant">Consultant</option>
                        <option value="Invité">Invité</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Mot de passe</label>
                    <input type="password" id="password" required>
                </div>
                <button type="submit" class="login-btn">Connexion</button>
                <div class="error" id="loginError"></div>
            </form>
        </div>
    </div>

    <!-- MAIN PAGE -->
        <div id="mainPage">
        <header>
            <div class="header-left">
                <div class="header-logo">AERA</div>
                <div class="header-info">
                    <div style="font-weight: 600;" id="userRole"></div>
                    <span id="statusText">En ligne</span>
                </div>
            </div>
            <button class="logout-btn" onclick="logout()">Déconnexion</button>
        </header>

        <div class="content">
            <!-- TABS -->
            <div class="tabs">
                <button class="tab-btn active" onclick="switchTab('dashboard')">Tableau de bord</button>
                <button class="tab-btn" onclick="switchTab('controls')">Contrôles</button>
                <button class="tab-btn" onclick="switchTab('settings')">Paramètres</button>
            </div>

            <!-- DASHBOARD TAB -->
            <div id="dashboard" class="tab-content active">
                <h2>Tableau de bord</h2>
                <div class="grid">
                    <div class="card">
                        <div class="card-title">Température</div>
                        <div class="card-value" id="tempDisplay">--</div>
                        <div class="card-unit">°C</div>
                        <span class="status ok" id="tempStatus">OK</span>
                    </div>
                    <div class="card">
                        <div class="card-title">Humidité</div>
                        <div class="card-value" id="humDisplay">--</div>
                        <div class="card-unit">%</div>
                        <span class="status ok" id="humStatus">OK</span>
                    </div>
                    <div class="card">
                        <div class="card-title">Potentiomètre</div>
                        <div class="card-value" id="potDisplay">--</div>
                        <div class="card-unit">%</div>
                    </div>
                    <div class="card">
                        <div class="card-title">PIR 1</div>
                        <div class="card-value" id="pir1Display">Non</div>
                        <span class="status ok" id="pir1Status">Inactif</span>
                    </div>
                    <div class="card">
                        <div class="card-title">PIR 2</div>
                        <div class="card-value" id="pir2Display">Non</div>
                        <span class="status ok" id="pir2Status">Inactif</span>
                    </div>
                    <div class="card">
                        <div class="card-title">État Système</div>
                        <div class="card-value" style="color: var(--success);">Actif</div>
                        <span class="status ok">En ligne</span>
                    </div>
                </div>

                <div class="section">
                    <h2>État des Fenêtres</h2>
                    <div class="windows">
                        <div class="window-card">
                            <h3>Fenêtre 1</h3>
                            <div class="window-value" id="servo1Display">0%</div>
                            <span class="status ok">Fermée</span>
                        </div>
                        <div class="window-card">
                            <h3>Fenêtre 2</h3>
                            <div class="window-value" id="servo2Display">0%</div>
                            <span class="status ok">Fermée</span>
                        </div>
                        <div class="window-card">
                            <h3>Fenêtre 3</h3>
                            <div class="window-value" id="servo3Display">0%</div>
                            <span class="status ok">Fermée</span>
                        </div>
                        <div class="window-card">
                            <h3>Fenêtre 4</h3>
                            <div class="window-value" id="servo4Display">0%</div>
                            <span class="status ok">Fermée</span>
                        </div>
                    </div>
                </div>
            </div>

            <!-- CONTROLS TAB -->
            <div id="controls" class="tab-content">
                <h2>Gestion des Fenêtres</h2>
                <div class="section">
                    <div class="slider-group">
                        <div class="slider-label">
                            <span>Fenêtre 1</span>
                            <span id="s1Val">0%</span>
                        </div>
                        <input type="range" id="s1" min="0" max="100" value="0" oninput="updateServo(1, this.value)">
                    </div>

                    <div class="slider-group">
                        <div class="slider-label">
                            <span>Fenêtre 2</span>
                            <span id="s2Val">0%</span>
                        </div>
                        <input type="range" id="s2" min="0" max="100" value="0" oninput="updateServo(2, this.value)">
                    </div>

                    <div class="slider-group">
                        <div class="slider-label">
                            <span>Fenêtre 3</span>
                            <span id="s3Val">0%</span>
                        </div>
                        <input type="range" id="s3" min="0" max="100" value="0" oninput="updateServo(3, this.value)">
                    </div>

                    <div class="slider-group">
                        <div class="slider-label">
                            <span>Fenêtre 4</span>
                            <span id="s4Val">0%</span>
                        </div>
                        <input type="range" id="s4" min="0" max="100" value="0" oninput="updateServo(4, this.value)">
                    </div>

                    <div class="buttons">
                        <button class="btn" onclick="setAllServos(0)">Tout Fermer</button>
                        <button class="btn" onclick="setAllServos(50)">Mi-ouvert</button>
                        <button class="btn" onclick="setAllServos(100)">Tout Ouvrir</button>
                    </div>
                </div>

                <h2 style="margin-top: 40px;">Gestion Système</h2>
                <div class="section">
                    <div class="slider-group">
                        <label>Mode Fonctionnement</label>
                        <select id="modeSelect" onchange="updateMode()">
                            <option value="Auto">Auto</option>
                            <option value="Manuel">Manuel</option>
                            <option value="Programmé">Programmé</option>
                        </select>
                    </div>
                </div>
            </div>

            <!-- SETTINGS TAB -->
            <div id="settings" class="tab-content">
                <h2>Paramètres Système</h2>
                
                <div class="section">
                    <h2>Configuration Température</h2>
                    <div class="slider-group">
                        <div class="slider-label">
                            <span>Température Minimum</span>
                            <span id="tempMinVal">19°C</span>
                        </div>
                        <input type="range" id="tempMin" min="10" max="30" value="19" oninput="updateTempMin()">
                    </div>

                    <div class="slider-group">
                        <div class="slider-label">
                            <span>Température Maximum</span>
                            <span id="tempMaxVal">26°C</span>
                        </div>
                        <input type="range" id="tempMax" min="15" max="35" value="26" oninput="updateTempMax()">
                    </div>

                    <div class="buttons">
                        <button class="btn" onclick="saveSettings()">Sauvegarder</button>
                    </div>
                </div>

                <div class="section">
                    <h2>Informations Système</h2>
                    <div class="grid">
                        <div class="card">
                            <div class="card-title">IP ESP32</div>
                            <div class="card-value" id="espIP">--</div>
                        </div>
                        <div class="card">
                            <div class="card-title">SSID Connecté</div>
                            <div class="card-value" style="font-size: 18px;" id="connSSID">--</div>
                        </div>
                        <div class="card">
                            <div class="card-title">Uptime</div>
                            <div class="card-value" style="font-size: 18px;" id="uptime">--</div>
                        </div>
                        <div class="card">
                            <div class="card-title">Version</div>
                            <div class="card-value" style="font-size: 18px;">AERA 1.0</div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <div class="notification" id="notification" style="display: none;"></div>

    <script>
        // ===== VARIABLES LOCALES =====
        let currentUser = null;
        let currentRole = null;
        let autoRefreshInterval = null;

        // ===== PASSWORDS =====
        const passwords = {
            "Administrateur": "admin1234",
            "Technicien": "tech1234",
            "Consultant": "consultant1234",
            "Invité": "invite1234"
        };

        // ===== LOGIN =====
        function login(e) {
            e.preventDefault();
            const role = document.getElementById('role').value;
            const pwd = document.getElementById('password').value;
            const errorDiv = document.getElementById('loginError');

            if (!role) {
                errorDiv.textContent = 'Sélectionnez un rôle';
                errorDiv.style.display = 'block';
                return;
            }

            if (passwords[role] !== pwd) {
                errorDiv.textContent = 'Mot de passe incorrect';
                errorDiv.style.display = 'block';
                return;
            }

            currentUser = role;
            currentRole = role;
            document.getElementById('loginPage').style.display = 'none';
            document.getElementById('mainPage').style.display = 'block';
            document.getElementById('userRole').textContent = role;
            
            loadConfig();
            startAutoRefresh();
            showNotification('Connecté avec succès', false);
        }

        // ===== LOGOUT =====
        function logout() {
            currentUser = null;
            currentRole = null;
            clearInterval(autoRefreshInterval);
            document.getElementById('loginPage').style.display = 'flex';
            document.getElementById('mainPage').style.display = 'none';
            document.getElementById('loginForm').reset();
        }

        // ===== TAB SWITCHING =====
        function switchTab(tabName) {
            const contents = document.querySelectorAll('.tab-content');
            const buttons = document.querySelectorAll('.tab-btn');
            
            contents.forEach(c => c.classList.remove('active'));
            buttons.forEach(b => b.classList.remove('active'));
            
            document.getElementById(tabName).classList.add('active');
            event.target.classList.add('active');
        }

        // ===== FETCH SENSOR DATA =====
        async function fetchSensorData() {
            try {
                const response = await fetch('/api/sensors');
                if (!response.ok) {
                    updateDisplayError();
                    return;
                }
                const data = await response.json();
                updateDisplay(data);
            } catch (e) {
                updateDisplayError();
            }
        }

        // ===== UPDATE DISPLAY =====
        function updateDisplay(data) {
            if (data.temp !== null) {
                document.getElementById('tempDisplay').textContent = data.temp.toFixed(1);
                document.getElementById('tempStatus').textContent = (data.temp >= 19 && data.temp <= 26) ? 'OK' : 'ALERTE';
            } else {
                document.getElementById('tempDisplay').textContent = 'Inaccessible';
            }

            if (data.hum !== null) {
                document.getElementById('humDisplay').textContent = data.hum.toFixed(1);
            } else {
                document.getElementById('humDisplay').textContent = 'Inaccessible';
            }

            if (data.pot !== null) {
                document.getElementById('potDisplay').textContent = data.pot;
            } else {
                document.getElementById('potDisplay').textContent = 'Inaccessible';
            }

            document.getElementById('pir1Display').textContent = data.pir1 ? 'Oui' : 'Non';
            document.getElementById('pir2Display').textContent = data.pir2 ? 'Oui' : 'Non';

            document.getElementById('servo1Display').textContent = data.s1 + '%';
            document.getElementById('servo2Display').textContent = data.s2 + '%';
            document.getElementById('servo3Display').textContent = data.s3 + '%';
            document.getElementById('servo4Display').textContent = data.s4 + '%';

            document.getElementById('espIP').textContent = data.ip || '--';
            document.getElementById('connSSID').textContent = data.ssid || 'Noe';
            document.getElementById('uptime').textContent = formatUptime(data.uptime);
        }

        function updateDisplayError() {
            document.getElementById('tempDisplay').textContent = 'Inaccessible';
            document.getElementById('humDisplay').textContent = 'Inaccessible';
            document.getElementById('potDisplay').textContent = 'Inaccessible';
            document.getElementById('statusText').textContent = 'Déconnecté';
        }

        function formatUptime(ms) {
            if (!ms) return '--';
            const days = Math.floor(ms / 86400000);
            const hours = Math.floor((ms % 86400000) / 3600000);
            const mins = Math.floor((ms % 3600000) / 60000);
            return days + 'j ' + hours + 'h ' + mins + 'm';
        }

        // ===== SERVO CONTROL =====
        async function updateServo(num, value) {
            document.getElementById('s' + num + 'Val').textContent = value + '%';
            
            try {
                const response = await fetch('/api/servo', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ servo: num, position: parseInt(value) })
                });
                
                if (response.ok) {
                    showNotification('Fenêtre ' + num + ' mise à jour', false);
                }
            } catch (e) {
                showNotification('Erreur mise à jour', true);
            }
        }

        function setAllServos(value) {
            document.getElementById('s1').value = value;
            document.getElementById('s2').value = value;
            document.getElementById('s3').value = value;
            document.getElementById('s4').value = value;
            updateServo(1, value);
            updateServo(2, value);
            updateServo(3, value);
            updateServo(4, value);
        }

        // ===== MODE CONTROL =====
        async function updateMode() {
            const mode = document.getElementById('modeSelect').value;
            try {
                const response = await fetch('/api/mode', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ mode: mode })
                });
                if (response.ok) {
                    showNotification('Mode changé en: ' + mode, false);
                }
            } catch (e) {
                showNotification('Erreur changement mode', true);
            }
        }

        // ===== TEMPERATURE SETTINGS =====
        function updateTempMin() {
            const val = document.getElementById('tempMin').value;
            document.getElementById('tempMinVal').textContent = val + '°C';
        }

        function updateTempMax() {
            const val = document.getElementById('tempMax').value;
            document.getElementById('tempMaxVal').textContent = val + '°C';
        }

        async function saveSettings() {
            const tempMin = parseFloat(document.getElementById('tempMin').value);
            const tempMax = parseFloat(document.getElementById('tempMax').value);

            if (tempMin >= tempMax) {
                showNotification('T° Min doit être < T° Max', true);
                return;
            }

            try {
                const response = await fetch('/api/settings', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ 
                        tempMin: tempMin,
                        tempMax: tempMax
                    })
                });

                if (response.ok) {
                    showNotification('Paramètres sauvegardés', false);
                }
            } catch (e) {
                showNotification('Erreur sauvegarde', true);
            }
        }

        // ===== LOAD CONFIG =====
        async function loadConfig() {
            try {
                const response = await fetch('/api/config');
                if (response.ok) {
                    const config = await response.json();
                    document.getElementById('tempMin').value = config.tempMin;
                    document.getElementById('tempMinVal').textContent = config.tempMin + '°C';
                    document.getElementById('tempMax').value = config.tempMax;
                    document.getElementById('tempMaxVal').textContent = config.tempMax + '°C';
                    document.getElementById('modeSelect').value = config.mode;
                }
            } catch (e) {
                console.log('Erreur chargement config');
            }
        }

        // ===== AUTO REFRESH =====
        function startAutoRefresh() {
            fetchSensorData();
            autoRefreshInterval = setInterval(fetchSensorData, 2000);
        }

        // ===== NOTIFICATION =====
        function showNotification(message, isError) {
            const notif = document.getElementById('notification');
            notif.textContent = message;
            notif.className = isError ? 'notification error' : 'notification';
            notif.style.display = 'block';
            
            setTimeout(() => {
                notif.style.display = 'none';
            }, 3000);
        }
    </script>
</body>
</html>
)rawliteral";

// ===== SETUP =====
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // Initialiser capteurs
    dht.begin();
    pinMode(PIR1_PIN, INPUT);
    pinMode(PIR2_PIN, INPUT);
    pinMode(POT_PIN, INPUT);
    pinMode(SERVO1_PIN, OUTPUT);
    pinMode(SERVO2_PIN, OUTPUT);
    pinMode(SERVO3_PIN, OUTPUT);
    pinMode(SERVO4_PIN, OUTPUT);
    
    // Initialiser servos à 0
    analogWrite(SERVO1_PIN, 0);
    analogWrite(SERVO2_PIN, 0);
    analogWrite(SERVO3_PIN, 0);
    analogWrite(SERVO4_PIN, 0);
    
    Serial.println("\n========================================");
    Serial.println("AERA - Démarrage système");
    Serial.println("========================================\n");
    
    // Connexion WiFi
    connectToWiFi();
    
    // Configuration Firebase
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    config.token_status_callback = tokenStatusCallback;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    
    // Routes Web Server
    setupWebServer();
    
    Serial.println("Système prêt!");
    Serial.println("Accédez à: http://" + WiFi.localIP().toString());
}

// ===== CONNEXION WIFI =====
void connectToWiFi() {
    Serial.print("Connexion WiFi: ");
    Serial.println(SSID);
    
    WiFi.begin(SSID, PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connecté!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nConnexion WiFi échouée!");
    }
}

// ===== SETUP WEB SERVER =====
void setupWebServer() {
    // Page principale
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", INDEX_HTML);
    });
    
    // API: Récupérer données capteurs
    server.on("/api/sensors", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"temp\":" + String(temperature, 1) + ",";
        json += "\"hum\":" + String(humidity, 1) + ",";
        json += "\"pot\":" + String(potValue) + ",";
        json += "\"pir1\":" + String(pir1Detected ? "true" : "false") + ",";
        json += "\"pir2\":" + String(pir2Detected ? "true" : "false") + ",";
        json += "\"s1\":" + String(servo1Pos) + ",";
        json += "\"s2\":" + String(servo2Pos) + ",";
        json += "\"s3\":" + String(servo3Pos) + ",";
        json += "\"s4\":" + String(servo4Pos) + ",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"ssid\":\"" + String(WiFi.SSID()) + "\",";
        json += "\"uptime\":" + String(millis());
        json += "}";
        
        request->send(200, "application/json", json);
    });
    
    // API: Contrôle servo
    server.on("/api/servo", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t upload) {
        String body = "";
        for (size_t i = 0; i < len; i++) {
            body += (char)data[i];
        }
        
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, body);
        
        int servo = doc["servo"];
        int position = doc["position"];
        
        setServoPosition(servo, position);
        
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    // API: Changer mode
    server.on("/api/mode", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t upload) {
        String body = "";
        for (size_t i = 0; i < len; i++) {
            body += (char)data[i];
        }
        
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, body);
        
        currentMode = doc["mode"].as<String>();
        saveToFirebase("system/mode", currentMode);
        
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    // API: Récupérer config
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"tempMin\":" + String(tempMin, 1) + ",";
        json += "\"tempMax\":" + String(tempMax, 1) + ",";
        json += "\"mode\":\"" + currentMode + "\"";
        json += "}";
        
        request->send(200, "application/json", json);
    });
    
    // API: Sauvegarder settings
    server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t upload) {
        String body = "";
        for (size_t i = 0; i < len; i++) {
            body += (char)data[i];
        }
        
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, body);
        
        tempMin = doc["tempMin"];
        tempMax = doc["tempMax"];
        
        saveToFirebase("system/tempMin", tempMin);
        saveToFirebase("system/tempMax", tempMax);
        
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    server.begin();
    Serial.println("Serveur Web démarré");
}

// ===== SERVO CONTROL =====
void setServoPosition(int servo, int position) {
    int pwmValue = map(position, 0, 100, 0, 255);
    
    switch(servo) {
        case 1:
            servo1Pos = position;
            analogWrite(SERVO1_PIN, pwmValue);
            break;
        case 2:
            servo2Pos = position;
            analogWrite(SERVO2_PIN, pwmValue);
            break;
        case 3:
            servo3Pos = position;
            analogWrite(SERVO3_PIN, pwmValue);
            break;
        case 4:
            servo4Pos = position;
            analogWrite(SERVO4_PIN, pwmValue);
            break;
    }
    
    saveToFirebase("servos/servo" + String(servo), position);
}

// ===== READ SENSORS =====
void readSensors() {
    if (millis() - lastRead < READ_INTERVAL) return;
    lastRead = millis();
    
    // DHT22
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    
    if (isnan(temperature)) temperature = 0;
    if (isnan(humidity)) humidity = 0;
    
    // PIR
    pir1Detected = digitalRead(PIR1_PIN);
    pir2Detected = digitalRead(PIR2_PIN);
    
    // Potentiomètre
    potValue = map(analogRead(POT_PIN), 0, 4095, 0, 100);
    
    // Affichage série
    Serial.print("T°: ");
    Serial.print(temperature);
    Serial.print("°C | H: ");
    Serial.print(humidity);
    Serial.print("% | Pot: ");
    Serial.print(potValue);
    Serial.print("% | PIR1: ");
    Serial.print(pir1Detected ? "Oui" : "Non");
    Serial.print(" | PIR2: ");
    Serial.println(pir2Detected ? "Oui" : "Non");
    
    // Envoyer à Firebase
    saveToFirebase("sensors/temperature", temperature);
    saveToFirebase("sensors/humidity", humidity);
    saveToFirebase("sensors/potentiometer", potValue);
    saveToFirebase("sensors/pir1", pir1Detected);
    saveToFirebase("sensors/pir2", pir2Detected);
}

// ===== FIREBASE =====
// Version pour les nombres entiers (AJOUTÉE)
void saveToFirebase(String path, int value) {
    if (Firebase.ready() && WiFi.status() == WL_CONNECTED) {
        Firebase.RTDB.setInt(&fbdo, path, value);
    }
}

// Version pour les nombres décimaux
void saveToFirebase(String path, float value) {
    if (Firebase.ready() && WiFi.status() == WL_CONNECTED) {
        Firebase.RTDB.setFloat(&fbdo, path, value);
    }
}

// Version pour les booléens
void saveToFirebase(String path, boolean value) {
    if (Firebase.ready() && WiFi.status() == WL_CONNECTED) {
        Firebase.RTDB.setBool(&fbdo, path, value);
    }
}

// Version pour le texte
void saveToFirebase(String path, String value) {
    if (Firebase.ready() && WiFi.status() == WL_CONNECTED) {
        Firebase.RTDB.setString(&fbdo, path, value);
    }
}

// ===== LOOP =====
void loop() {
    readSensors();
    delay(100);
}
