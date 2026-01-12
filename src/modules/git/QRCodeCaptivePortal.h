#ifndef QR_CODE_CAPTIVE_PORTAL_H
#define QR_CODE_CAPTIVE_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "modules/git/GitProvider.h"

class QRCodeCaptivePortal {
private:
    DNSServer dnsServer;
    AsyncWebServer* server;
    GitProvider* gitProvider;
    String providerName;
    
    // Portal configuration
    String portalSSID;
    String authCallbackURL;
    String tokenEndpointURL;
    
    // QR and WiFi management
    void setupDNS();
    void setupWebServer();
    String generateQRCodeURL();
    bool isAccessPointActive;
    
    // State tracking
    String authToken;
    bool tokenReceived;
    
    // HTML templates
    String getPortalPage();
    String getSuccessPage();
    String getQRPage();
    String getTokenForm();
    
    // Authentication handling
    void handleTokenSubmit(AsyncWebServerRequest* request);
    void handleOAuthCallback(AsyncWebServerRequest* request);
    
public:
    QRCodeCaptivePortal(GitProvider* provider, const String& name);
    ~QRCodeCaptivePortal();
    
    // Portal control
    bool startPortal(const String& ssidPrefix = "Bruce-Git-Auth");
    void stopPortal();
    void loop();
    
    // Access Point management
    bool startAccessPoint(const String& ssid);
    void stopAccessPoint();
    String getAccessPointIP();
    
    // State checks
    bool isPortalRunning();
    bool hasTokenReceived();
    String getReceivedToken();
    String getAuthURL();
    
    // QR Code features
    String generateQRData();
    void displayQRCode(const String& data);
    
    // Provider settings
    void setProvider(GitProvider* provider, const String& name);
    void setAuthURL(const String& url);
    void setTokenEndpoint(const String& url);
};

#endif // QR_CODE_CAPTIVE_PORTAL_H