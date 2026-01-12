#include "github_oauth.h"
#include "core/display.h"
#include "core/utils.h"
#include "core/config.h"
#include <ESPmDNS.h>
#include <HTTPClient.h>

// OAuth URLs for GitHub
const char* GitHubOAuth::OAUTH_AUTHORIZE_URL = "https://github.com/login/oauth/authorize";
const char* GitHubOAuth::OAUTH_TOKEN_URL = "https://github.com/login/oauth/access_token";
const char* GitHubOAuth::OAUTH_API_URL = "https://api.github.com";

// Global instance
GitHubOAuth githubOAuth;

GitHubOAuth::GitHubOAuth() {
    _server = nullptr;
    _oauthActive = false;
    _accessPointActive = false;
    _clientId = "";
    _clientSecret = "";
    _redirectUri = "http://172.0.0.1:80/github/callback";
    _scope = "repo user gist";
    _state = "";
    _tempToken = "";
    _apSSID = "Bruce-GitHub-Auth";
    _lastError = "";
}

GitHubOAuth::~GitHubOAuth() {
    stopOAuthFlow();
    stopAccessPoint();
}

bool GitHubOAuth::startOAuthFlow(AsyncWebServer *server) {
    if (_oauthActive) {
        Serial.println("OAuth flow already active");
        return false;
    }
    
    _server = server;
    _state = generateState();
    _tempToken = "";
    _oauthActive = true;
    
    Serial.println("Starting GitHub OAuth flow");
    Serial.println("State: " + _state);
    
    return true;
}

void GitHubOAuth::stopOAuthFlow() {
    _oauthActive = false;
    _state = "";
    _tempToken = "";
    _server = nullptr;
    
    if (_dnsServer.isStarted()) {
        _dnsServer.stop();
    }
}

bool GitHubOAuth::isOAuthActive() {
    return _oauthActive;
}

void GitHubOAuth::setupOAuthRoutes(AsyncWebServer *server) {
    if (!_oauthActive) return;
    
    // OAuth start page
    server->on("/github/auth", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleAuthGet(request);
    });
    
    // OAuth start authorization
    server->on("/github/start", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleStartAuth(request);
    });
    
    // OAuth callback
    server->on("/github/callback", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleCallbackGet(request);
    });
    
    // OAuth success page
    server->on("/github/success", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleSuccess(request);
    });
    
    // OAuth error page
    server->on("/github/error", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String error = "Unknown error";
        if (request->hasParam("error")) {
            error = request->getParam("error")->value();
        }
        handleError(request, error);
    });
    
    // API endpoint to check OAuth status
    server->on("/github/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
        doc["oauth_active"] = _oauthActive;
        doc["state"] = _state;
        doc["has_temp_token"] = _tempToken.length() > 0;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
}

void GitHubOAuth::handleAuthGet(AsyncWebServerRequest *request) {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>GitHub OAuth - Bruce ESP32</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f6f8fa; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 40px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #24292e; text-align: center; }
        .github-btn { display: block; width: 100%; padding: 12px 16px; background: #2ea44f; color: white; text-decoration: none; text-align: center; border-radius: 6px; font-weight: bold; margin: 20px 0; }
        .github-btn:hover { background: #2c974b; }
        .info { background: #f3f4f6; padding: 15px; border-radius: 6px; margin: 20px 0; }
        .logo { text-align: center; font-size: 48px; margin-bottom: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="logo">🐙</div>
        <h1>GitHub OAuth Authentication</h1>
        <div class="info">
            <h3>About this authentication:</h3>
            <ul>
                <li>This will authenticate you with GitHub</li>
                <li>You'll be redirected to GitHub to authorize the app</li>
                <li>After authorization, you'll return here automatically</li>
                <li>Your access token will be securely stored on your device</li>
            </ul>
        </div>
        <a href="/github/start" class="github-btn">Authorize with GitHub</a>
        <div class="info">
            <strong>Note:</strong> Make sure you're connected to the internet and have a GitHub account.
        </div>
    </div>
</body>
</html>
    )";
    
    request->send(200, "text/html", html);
}

void GitHubOAuth::handleSuccess(AsyncWebServerRequest *request) {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Authentication Successful - Bruce ESP32</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f6f8fa; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 40px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; }
        .success { color: #28a745; font-size: 48px; margin-bottom: 20px; }
        h1 { color: #24292e; }
        .info { background: #f3f4f6; padding: 15px; border-radius: 6px; margin: 20px 0; }
        .close-btn { background: #0366d6; color: white; padding: 12px 24px; text-decoration: none; border-radius: 6px; display: inline-block; margin-top: 20px; }
        .close-btn:hover { background: #0256cc; }
    </style>
    <script>
        // Close window after 3 seconds
        setTimeout(function() {
            window.close();
        }, 3000);
    </script>
</head>
<body>
    <div class="container">
        <div class="success">✓</div>
        <h1>Authentication Successful!</h1>
        <div class="info">
            <p>Your GitHub access token has been successfully obtained and stored.</p>
            <p>You can now close this window and return to your device.</p>
            <p>This window will close automatically in 3 seconds.</p>
        </div>
        <a href="javascript:window.close()" class="close-btn">Close Window</a>
    </div>
</body>
</html>
    )";
    
    request->send(200, "text/html", html);
}

void GitHubOAuth::handleError(AsyncWebServerRequest *request, const String& error) {
    String errorDescription = "An unknown error occurred";
    if (error == "access_denied") {
        errorDescription = "You cancelled the authorization process";
    } else if (error == "invalid_request") {
        errorDescription = "Invalid request parameters";
    } else if (error == "unauthorized_client") {
        errorDescription = "Unauthorized client";
    } else if (error == "unsupported_response_type") {
        errorDescription = "Unsupported response type";
    } else if (error == "invalid_scope") {
        errorDescription = "Invalid scope requested";
    }
    
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Authentication Error - Bruce ESP32</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f6f8fa; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 40px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; }
        .error { color: #d73a49; font-size: 48px; margin-bottom: 20px; }
        h1 { color: #24292e; }
        .error-info { background: #ffeef0; padding: 15px; border-radius: 6px; margin: 20px 0; border-left: 4px solid #d73a49; }
        .retry-btn { background: #0366d6; color: white; padding: 12px 24px; text-decoration: none; border-radius: 6px; display: inline-block; margin-top: 20px; }
        .retry-btn:hover { background: #0256cc; }
    </style>
</head>
<body>
    <div class="container">
        <div class="error">✗</div>
        <h1>Authentication Error</h1>
        <div class="error-info">
            <p><strong>Error:</strong> )" + error + R"(</p>
            <p><strong>Description:</strong> )" + errorDescription + R"(</p>
        </div>
        <a href="/github/auth" class="retry-btn">Try Again</a>
    </div>
</body>
</html>
    )";
    
    request->send(200, "text/html", html);
}

void GitHubOAuth::handleCallbackGet(AsyncWebServerRequest *request) {
    Serial.println("GitHub OAuth callback received");
    
    // Check for errors
    if (request->hasParam("error")) {
        String error = request->getParam("error")->value();
        Serial.println("OAuth error: " + error);
        request->redirect("/github/error?error=" + urlEncode(error));
        return;
    }
    
    // Check for authorization code
    if (!request->hasParam("code")) {
        Serial.println("No authorization code received");
        request->redirect("/github/error?error=no_code");
        return;
    }
    
    // Check state parameter
    if (!request->hasParam("state") || request->getParam("state")->value() != _state) {
        Serial.println("Invalid state parameter");
        request->redirect("/github/error?error=invalid_state");
        return;
    }
    
    String authCode = request->getParam("code")->value();
    Serial.println("Authorization code received: " + authCode);
    
    // Exchange code for access token
    String tokenResponse = exchangeCodeForToken(authCode);
    
    if (tokenResponse.length() > 0) {
        // Parse token response
        String accessToken = extractParam(tokenResponse, "access_token");
        String tokenType = extractParam(tokenResponse, "token_type");
        String scope = extractParam(tokenResponse, "scope");
        
        if (accessToken.length() > 0) {
            _tempToken = accessToken;
            Serial.println("Access token obtained successfully");
            
            // Test the token by getting user info
            if (testAccessToken(accessToken)) {
                // Store the token permanently
                bruceConfig.setGitHubToken(accessToken);
                
                // Redirect to success page
                request->redirect("/github/success");
                return;
            } else {
                Serial.println("Token validation failed");
                request->redirect("/github/error?error=token_validation_failed");
                return;
            }
        } else {
            Serial.println("No access token in response");
            request->redirect("/github/error?error=no_token");
            return;
        }
    } else {
        Serial.println("Token exchange failed");
        request->redirect("/github/error?error=exchange_failed");
        return;
    }
}

String GitHubOAuth::exchangeCodeForToken(const String& code) {
    Serial.println("Exchanging authorization code for access token...");
    
    WiFiClient client;
    HTTPClient http;
    
    http.begin(client, "https://github.com/login/oauth/access_token");
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "Bruce-ESP32/1.0");
    
    // Create JSON payload
    DynamicJsonDocument doc(512);
    doc["client_id"] = _clientId;
    doc["client_secret"] = "YOUR_CLIENT_SECRET"; // This should be configured
    doc["code"] = code;
    doc["redirect_uri"] = _redirectUri;
    
    String payload;
    serializeJson(doc, payload);
    
    Serial.println("Making token exchange request...");
    int httpResponseCode = http.POST(payload);
    
    if (httpResponseCode == 200) {
        String response = http.getString();
        Serial.println("Token exchange response: " + response);
        http.end();
        return response;
    } else {
        Serial.println("Token exchange failed with HTTP code: " + String(httpResponseCode));
        http.end();
        return "";
    }
}

bool GitHubOAuth::testAccessToken(const String& token) {
    Serial.println("Testing access token...");
    
    // Test token by making API request to get user info
    WiFiClient client;
    
    if (!client.connect("api.github.com", 443)) {
        Serial.println("Failed to connect to GitHub API");
        return false;
    }
    
    // Make request to GitHub API
    client.println("GET /user HTTP/1.1");
    client.println("Host: api.github.com");
    client.println("Authorization: token " + token);
    client.println("User-Agent: Bruce-ESP32/1.0");
    client.println("Accept: application/json");
    client.println("Connection: close");
    client.println();
    
    // Read response
    String response = "";
    while (client.connected() || client.available()) {
        if (client.available()) {
            response += (char)client.read();
        }
    }
    client.stop();
    
    Serial.println("API Response length: " + String(response.length()));
    
    // Simple check for successful response
    if (response.indexOf("\"login\"") > 0) {
        Serial.println("Token validation successful");
        return true;
    } else {
        Serial.println("Token validation failed");
        Serial.println("Response: " + response.substring(0, 200));
        return false;
    }
}

String GitHubOAuth::generateRandomString(int length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    String result = "";
    for (int i = 0; i < length; i++) {
        result += charset[random(0, sizeof(charset) - 1)];
    }
    return result;
}

String GitHubOAuth::generateState() {
    return generateRandomString(32);
}

String GitHubOAuth::buildAuthUrl(const String& state) {
    String url = OAUTH_AUTHORIZE_URL;
    url += "?client_id=" + urlEncode(_clientId);
    url += "&redirect_uri=" + urlEncode(_redirectUri);
    url += "&scope=" + urlEncode(_scope);
    url += "&state=" + urlEncode(state);
    url += "&allow_signup=true";
    
    return url;
}

String GitHubOAuth::urlEncode(const String& str) {
    String encoded = "";
    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            encoded += '%' + String(c, HEX);
        }
    }
    return encoded;
}

String GitHubOAuth::extractParam(const String& response, const String& param) {
    int start = response.indexOf(param + "=");
    if (start < 0) return "";
    
    start += param.length() + 1;
    int end = response.indexOf("&", start);
    if (end < 0) end = response.indexOf("\n", start);
    if (end < 0) end = response.indexOf("\r", start);
    if (end < 0) end = response.length();
    
    return response.substring(start, end);
}

void GitHubOAuth::setState(const String& state) {
    _state = state;
}

String GitHubOAuth::getState() {
    return _state;
}

bool GitHubOAuth::validateState(const String& state) {
    return state == _state;
}

void GitHubOAuth::setClientId(const String& clientId) {
    _clientId = clientId;
}

void GitHubOAuth::setClientSecret(const String& clientSecret) {
    _clientSecret = clientSecret;
}

void GitHubOAuth::setRedirectUri(const String& redirectUri) {
    _redirectUri = redirectUri;
}

void GitHubOAuth::setScope(const String& scope) {
    _scope = scope;
}

void GitHubOAuth::startAccessPoint(const String& ssid) {
    if (_accessPointActive) {
        stopAccessPoint();
    }
    
    _apSSID = ssid;
    _accessPointActive = true;
    
    // Start WiFi access point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str());
    
    IPAddress IP = WiFi.softAPIP();
    Serial.println("GitHub OAuth Access Point started");
    Serial.println("SSID: " + ssid);
    Serial.println("IP: " + IP.toString());
    
    // Start DNS server to redirect all requests to the access point
    _dnsServer.start(53, "*", IP);
}

void GitHubOAuth::stopAccessPoint() {
    if (_accessPointActive) {
        WiFi.softAPdisconnect(true);
        _dnsServer.stop();
        _accessPointActive = false;
        Serial.println("GitHub OAuth Access Point stopped");
    }
}

bool GitHubOAuth::isAccessPointActive() {
    return _accessPointActive;
}

// Handle the start authorization request
void GitHubOAuth::handleStartAuth(AsyncWebServerRequest *request) {
    if (!_oauthActive) {
        request->send(400, "text/plain", "OAuth flow not active");
        return;
    }
    
    // Check if WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
        request->send(400, "text/plain", "WiFi not connected");
        return;
    }
    
    // Generate new state
    _state = generateState();
    
    // Build authorization URL
    String authUrl = buildAuthUrl(_state);
    
    Serial.println("Redirecting to GitHub authorization: " + authUrl);
    
    // Redirect to GitHub
    request->redirect(authUrl);
}