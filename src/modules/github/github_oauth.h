#ifndef GITHUB_OAUTH_H
#define GITHUB_OAUTH_H

#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "github_app.h"

class GitHubOAuth {
public:
    GitHubOAuth();
    ~GitHubOAuth();
    
    // OAuth Flow Management
    bool startOAuthFlow(AsyncWebServer *server);
    void stopOAuthFlow();
    bool isOAuthActive();
    
    // Web Server Integration
    void setupOAuthRoutes(AsyncWebServer *server);
    
    // Callback Handling
    void handleCallbackGet(AsyncWebServerRequest *request);
    void handleAuthPage(AsyncWebServerRequest *request);
    void handleStartAuth(AsyncWebServerRequest *request);
    void handleSuccess(AsyncWebServerRequest *request);
    void handleError(AsyncWebServerRequest *request, const String& error);
    
    // State Management
    void setState(const String& state);
    String getState();
    bool validateState(const String& state);
    
    // Configuration
    void setClientId(const String& clientId);
    void setClientSecret(const String& clientSecret);
    void setRedirectUri(const String& redirectUri);
    void setScope(const String& scope);
    
    // Access Point Management
    void startAccessPoint(const String& ssid = "Bruce-GitHub-Auth");
    void stopAccessPoint();
    bool isAccessPointActive();
    
    // Utility
    String getLastError() { return _lastError; }
    
private:
    String generateRandomString(int length = 32);
    String generateState();
    String buildAuthUrl(const String& state);
    String exchangeCodeForToken(const String& code);
    bool testAccessToken(const String& token);
    
    // HTTP Request Handlers
    void handleAuthGet(AsyncWebServerRequest *request);
    void handleCallbackGetInternal(AsyncWebServerRequest *request, const String& code, const String& state);
    
    // Utility Functions
    String urlEncode(const String& str);
    String extractParam(const String& response, const String& param);
    
    // Member variables
    AsyncWebServer *_server;
    DNSServer _dnsServer;
    bool _oauthActive;
    String _clientId;
    String _clientSecret;
    String _redirectUri;
    String _scope;
    String _state;
    String _tempToken;
    bool _accessPointActive;
    String _apSSID;
    String _lastError;
    
    // OAuth Configuration
    static const char* OAUTH_AUTHORIZE_URL;
    static const char* OAUTH_TOKEN_URL;
    static const char* OAUTH_API_URL;
};

extern GitHubOAuth githubOAuth;

#endif // GITHUB_OAUTH_H