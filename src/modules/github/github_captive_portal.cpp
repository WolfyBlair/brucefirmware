#include "github_captive_portal.h"
#include "core/display.h"
#include "core/utils.h"
#include "core/config.h"
#include <ESPmDNS.h>

// Global instance
GitHubCaptivePortal githubPortal;

GitHubCaptivePortal::GitHubCaptivePortal() 
    : webServer(80) {
    _portalActive = false;
    _accessPointActive = false;
    _apSSID = "Bruce-GitHub-Setup";
    _storedToken = "";
    _redirectUrl = "http://example.com";
    _portalToken = "";
}

GitHubCaptivePortal::~GitHubCaptivePortal() {
    stopPortal();
    stopAccessPoint();
}

bool GitHubCaptivePortal::startPortal() {
    if (_portalActive) {
        Serial.println("Captive portal already active");
        return false;
    }
    
    _portalToken = generateToken();
    _portalActive = true;
    
    Serial.println("Starting GitHub Captive Portal");
    Serial.println("Token: " + _portalToken);
    
    return true;
}

void GitHubCaptivePortal::stopPortal() {
    _portalActive = false;
    _storedToken = "";
    _portalToken = "";
    webServer.end();
    
    if (_dnsServer.isStarted()) {
        _dnsServer.stop();
    }
}

bool GitHubCaptivePortal::isPortalActive() {
    return _portalActive;
}

void GitHubCaptivePortal::startAccessPoint(const String& ssid) {
    if (_accessPointActive) {
        stopAccessPoint();
    }
    
    _apSSID = ssid;
    _accessPointActive = true;
    
    // Start WiFi access point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str());
    
    IPAddress IP = WiFi.softAPIP();
    Serial.println("GitHub Captive Portal Access Point started");
    Serial.println("SSID: " + ssid);
    Serial.println("IP: " + IP.toString());
    
    // Start DNS server to redirect all requests to the access point
    _dnsServer.start(53, "*", IP);
}

void GitHubCaptivePortal::stopAccessPoint() {
    if (_accessPointActive) {
        WiFi.softAPdisconnect(true);
        _dnsServer.stop();
        _accessPointActive = false;
        Serial.println("GitHub Captive Portal Access Point stopped");
    }
}

bool GitHubCaptivePortal::isAccessPointActive() {
    return _accessPointActive;
}

void GitHubCaptivePortal::setupPortalRoutes() {
    if (!_portalActive) return;
    
    // Root page - Token setup form
    webServer.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRoot(request);
    });
    
    // Handle token submission
    webServer.on("/setup", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleTokenSubmit(request);
    });
    
    // Success page
    webServer.on("/success", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleSuccess(request);
    });
    
    // Status endpoint
    webServer.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleStatus(request);
    });
    
    // Captive portal detection routes
    webServer.on("/generate_204", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRedirect(request);
    });
    
    webServer.on("/fwlink", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRedirect(request);
    });
    
    // Android captive portal detection
    webServer.on("/ncsi.txt", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRedirect(request);
    });
    
    // Windows captive portal detection  
    webServer.on("/connecttest.txt", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRedirect(request);
    });
    
    // iOS captive portal detection
    webServer.on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRedirect(request);
    });
    
    // Apple captive portal detection
    webServer.on("/library/test/success", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRedirect(request);
    });
    
    // Microsoft NCSI detection
    webServer.on("/ncsi", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRedirect(request);
    });
    
    webServer.begin();
}

void GitHubCaptivePortal::handleRoot(AsyncWebServerRequest *request) {
    String html = getSetupPageHtml();
    request->send(200, "text/html", html);
}

void GitHubCaptivePortal::handleTokenSubmit(AsyncWebServerRequest *request) {
    if (request->hasParam("token", true)) {
        String token = request->getParam("token", true)->value();
        token.trim();
        
        if (token.length() > 0) {
            // Basic token validation
            if (isValidGitHubToken(token)) {
                saveTokenToConfig(token);
                _storedToken = token;
                
                // Redirect to success page
                request->redirect("/success");
                return;
            } else {
                // Invalid token - show error
                String html = getSetupPageHtml();
                html.replace("{{ERROR_MESSAGE}}", 
                    "<div style='background: #fee; border: 1px solid #fcc; padding: 10px; margin: 10px 0; border-radius: 4px; color: #c33;'>"
                    "Invalid GitHub token format. Please check your token and try again."
                    "</div>");
                request->send(400, "text/html", html);
                return;
            }
        }
    }
    
    // No token provided - redirect back
    request->redirect("/");
}

void GitHubCaptivePortal::handleSuccess(AsyncWebServerRequest *request) {
    String html = getSuccessPageHtml();
    request->send(200, "text/html", html);
}

void GitHubCaptivePortal::handleStatus(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(512);
    doc["portal_active"] = _portalActive;
    doc["token_set"] = _storedToken.length() > 0;
    doc["access_point_active"] = _accessPointActive;
    doc["ssid"] = _apSSID;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void GitHubCaptivePortal::handleRedirect(AsyncWebServerRequest *request) {
    request->redirect("/");
}

void GitHubCaptivePortal::setToken(const String& token) {
    _storedToken = token;
}

String GitHubCaptivePortal::getToken() {
    return _storedToken;
}

bool GitHubCaptivePortal::isTokenSet() {
    return _storedToken.length() > 0;
}

void GitHubCaptivePortal::clearToken() {
    _storedToken = "";
}

void GitHubCaptivePortal::setRedirectUrl(const String& url) {
    _redirectUrl = url;
}

bool GitHubCaptivePortal::isTokenConfigured() {
    return bruceConfig.githubToken.length() > 0;
}

String GitHubCaptivePortal::generateToken(int length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    String result = "";
    for (int i = 0; i < length; i++) {
        result += charset[random(0, sizeof(charset) - 1)];
    }
    return result;
}

String GitHubCaptivePortal::buildRedirectUrl() {
    return _redirectUrl;
}

void GitHubCaptivePortal::saveTokenToConfig(const String& token) {
    bruceConfig.setGitHubToken(token);
    Serial.println("GitHub token saved to configuration");
}

bool GitHubCaptivePortal::isValidGitHubToken(const String& token) {
    // Basic GitHub token validation
    // GitHub tokens typically start with 'ghp_' or 'github_pat_' or are 40-character hex strings
    if (token.length() < 10) return false;
    
    // Check for common GitHub token prefixes
    if (token.startsWith("ghp_") && token.length() >= 40) return true;
    if (token.startsWith("gho_") && token.length() >= 40) return true;
    if (token.startsWith("ghu_") && token.length() >= 40) return true;
    if (token.startsWith("ghs_") && token.length() >= 40) return true;
    if (token.startsWith("ghr_") && token.length() >= 40) return true;
    
    // Check for 40-character hex string (classic personal access tokens)
    if (token.length() == 40) {
        for (int i = 0; i < token.length(); i++) {
            char c = token.charAt(i);
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                return false;
            }
        }
        return true;
    }
    
    return false;
}

String GitHubCaptivePortal::getSetupPageHtml() {
    String html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>GitHub Token Setup - Bruce ESP32</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh; display: flex; align-items: center; justify-content: center;
        }
        .container { 
            background: white; padding: 40px; border-radius: 12px; 
            box-shadow: 0 20px 40px rgba(0,0,0,0.1); max-width: 500px; width: 90%;
            text-align: center;
        }
        .logo { font-size: 64px; margin-bottom: 20px; }
        h1 { color: #24292e; margin-bottom: 10px; font-size: 28px; }
        .subtitle { color: #586069; margin-bottom: 30px; font-size: 16px; }
        .form-group { margin-bottom: 20px; text-align: left; }
        label { display: block; margin-bottom: 8px; font-weight: 600; color: #24292e; }
        input[type="password"], input[type="text"] { 
            width: 100%; padding: 12px; border: 2px solid #d1d5da; 
            border-radius: 6px; font-size: 16px; transition: border-color 0.3s;
        }
        input:focus { outline: none; border-color: #0366d6; }
        .btn { 
            background: #28a745; color: white; border: none; padding: 12px 24px; 
            border-radius: 6px; font-size: 16px; font-weight: 600; cursor: pointer; 
            transition: background-color 0.3s; width: 100%; margin-top: 10px;
        }
        .btn:hover { background: #218838; }
        .btn:disabled { background: #6c757d; cursor: not-allowed; }
        .info-box { 
            background: #f6f8fa; border: 1px solid #e1e4e8; border-radius: 6px; 
            padding: 16px; margin: 20px 0; text-align: left;
        }
        .info-title { font-weight: 600; margin-bottom: 8px; color: #24292e; }
        .info-text { color: #586069; font-size: 14px; line-height: 1.5; }
        .link { color: #0366d6; text-decoration: none; }
        .link:hover { text-decoration: underline; }
        .error-placeholder {{ERROR_MESSAGE}}
    </style>
</head>
<body>
    <div class="container">
        <div class="logo">🐙</div>
        <h1>GitHub Token Setup</h1>
        <p class="subtitle">Configure your GitHub Personal Access Token</p>
        
        {{ERROR_MESSAGE}}
        
        <form action="/setup" method="POST">
            <div class="form-group">
                <label for="token">GitHub Personal Access Token:</label>
                <input type="password" id="token" name="token" placeholder="ghp_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" required>
                <div style="font-size: 12px; color: #6a737d; margin-top: 4px;">
                    <a href="https://github.com/settings/tokens" class="link" target="_blank">
                        Generate a new token →
                    </a>
                </div>
            </div>
            
            <div class="info-box">
                <div class="info-title">Required Scopes:</div>
                <div class="info-text">
                    • <strong>repo</strong> - Full control of private repositories<br>
                    • <strong>user</strong> - Update ALL user data<br>
                    • <strong>gist</strong> - Create gists<br>
                    • <strong>admin:repo_hook</strong> - Full control of repository hooks
                </div>
            </div>
            
            <button type="submit" class="btn">Save Token</button>
        </form>
        
        <div class="info-box">
            <div class="info-title">How to get a token:</div>
            <div class="info-text">
                1. Go to <a href="https://github.com/settings/tokens" class="link" target="_blank">GitHub Settings → Developer settings → Personal access tokens</a><br>
                2. Click "Generate new token (classic)"<br>
                3. Select the required scopes above<br>
                4. Copy and paste the token here
            </div>
        </div>
        
        <div style="margin-top: 30px; padding-top: 20px; border-top: 1px solid #e1e4e8;">
            <small style="color: #6a737d;">Bruce ESP32 GitHub Setup Portal</small>
        </div>
    </div>
    
    <script>
        // Auto-focus on token input
        document.getElementById('token').focus();
        
        // Show/hide token
        document.getElementById('token').addEventListener('click', function() {
            this.type = 'text';
        });
        
        document.getElementById('token').addEventListener('blur', function() {
            this.type = 'password';
        });
    </script>
</body>
</html>
    )";
    
    return html;
}

String GitHubCaptivePortal::getSuccessPageHtml() {
    String html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Setup Complete - Bruce ESP32</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; 
            background: linear-gradient(135deg, #28a745 0%, #20c997 100%);
            min-height: 100vh; display: flex; align-items: center; justify-content: center;
        }
        .container { 
            background: white; padding: 40px; border-radius: 12px; 
            box-shadow: 0 20px 40px rgba(0,0,0,0.1); max-width: 500px; width: 90%;
            text-align: center;
        }
        .success-icon { font-size: 80px; margin-bottom: 20px; }
        h1 { color: #24292e; margin-bottom: 10px; font-size: 28px; }
        .message { color: #586069; margin-bottom: 30px; font-size: 16px; line-height: 1.5; }
        .btn { 
            background: #0366d6; color: white; border: none; padding: 12px 24px; 
            border-radius: 6px; font-size: 16px; font-weight: 600; cursor: pointer; 
            transition: background-color 0.3s; text-decoration: none; display: inline-block;
        }
        .btn:hover { background: #0256cc; }
    </style>
</head>
<body>
    <div class="container">
        <div class="success-icon">✅</div>
        <h1>Setup Complete!</h1>
        <p class="message">
            Your GitHub Personal Access Token has been successfully configured and saved to your Bruce ESP32 device.
            <br><br>
            You can now use all GitHub features from your device's menu.
        </p>
        <a href="/" class="btn">Setup Another Device</a>
    </div>
    
    <script>
        // Auto-close window after 5 seconds
        setTimeout(function() {
            if (window.opener) {
                window.close();
            }
        }, 5000);
    </script>
</body>
</html>
    )";
    
    return html;
}

String GitHubCaptivePortal::getStatusPageHtml() {
    String html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Portal Status - Bruce ESP32</title>
    <style>
        body { font-family: Arial, sans-serif; padding: 20px; background: #f6f8fa; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .status { padding: 15px; border-radius: 6px; margin: 15px 0; }
        .active { background: #d4edda; border: 1px solid #c3e6cb; color: #155724; }
        .inactive { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }
        h1 { color: #24292e; text-align: center; margin-bottom: 30px; }
        .info { background: #e7f3ff; padding: 15px; border-radius: 6px; margin: 15px 0; border-left: 4px solid #0366d6; }
    </style>
</head>
<body>
    <div class="container">
        <h1>GitHub Portal Status</h1>
        <div class="info">
            <strong>Portal Status:</strong> <span id="portalStatus">Checking...</span><br>
            <strong>Access Point:</strong> <span id="apStatus">Checking...</span><br>
            <strong>Token Status:</strong> <span id="tokenStatus">Checking...</span><br>
            <strong>SSID:</strong> <span id="ssid">-</span>
        </div>
        <div style="text-align: center; margin-top: 30px;">
            <a href="/" style="background: #0366d6; color: white; padding: 10px 20px; text-decoration: none; border-radius: 6px;">Go to Setup</a>
        </div>
    </div>
    
    <script>
        fetch('/status')
            .then(response => response.json())
            .then(data => {
                document.getElementById('portalStatus').textContent = data.portal_active ? 'Active' : 'Inactive';
                document.getElementById('portalStatus').parentElement.className = 'status ' + (data.portal_active ? 'active' : 'inactive');
                
                document.getElementById('apStatus').textContent = data.access_point_active ? 'Active' : 'Inactive';
                document.getElementById('apStatus').parentElement.className = 'status ' + (data.access_point_active ? 'active' : 'inactive');
                
                document.getElementById('tokenStatus').textContent = data.token_set ? 'Configured' : 'Not Set';
                document.getElementById('tokenStatus').parentElement.className = 'status ' + (data.token_set ? 'active' : 'inactive');
                
                document.getElementById('ssid').textContent = data.ssid || '-';
            })
            .catch(error => {
                console.error('Error:', error);
            });
    </script>
</body>
</html>
    )";
    
    return html;
}