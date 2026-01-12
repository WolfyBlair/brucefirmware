#ifndef GITHUB_CAPTIVE_PORTAL_H
#define GITHUB_CAPTIVE_PORTAL_H

#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "github_app.h"

class GitHubCaptivePortal {
public:
    GitHubCaptivePortal();
    ~GitHubCaptivePortal();
    
    // Portal Management
    bool startPortal();
    void stopPortal();
    bool isPortalActive();
    
    // Web Server Integration
    void setupPortalRoutes();
    
    // Access Point Management
    void startAccessPoint(const String& ssid = "Bruce-GitHub-Setup");
    void stopAccessPoint();
    bool isAccessPointActive();
    
    // Token Management
    void setToken(const String& token);
    String getToken();
    bool isTokenSet();
    void clearToken();
    
    // Configuration
    void setRedirectUrl(const String& url);
    
    // Status
    bool isTokenConfigured();
    
private:
    // Web Handlers
    void handleRoot(AsyncWebServerRequest *request);
    void handleTokenSubmit(AsyncWebServerRequest *request);
    void handleSuccess(AsyncWebServerRequest *request);
    void handleStatus(AsyncWebServerRequest *request);
    void handleRedirect(AsyncWebServerRequest *request);
    
    // Utility Functions
    String generateToken(int length = 32);
    String buildRedirectUrl();
    void saveTokenToConfig(const String& token);
    
    // Member variables
    AsyncWebServer webServer;
    DNSServer _dnsServer;
    bool _portalActive;
    bool _accessPointActive;
    String _apSSID;
    String _storedToken;
    String _redirectUrl;
    String _portalToken;
    
    // Captive Portal HTML Templates
    String getSetupPageHtml();
    String getSuccessPageHtml();
    String getStatusPageHtml();
};

extern GitHubCaptivePortal githubPortal;

#endif // GITHUB_CAPTIVE_PORTAL_H