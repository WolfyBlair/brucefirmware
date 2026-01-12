#include"QRCodeCaptivePortal.h"#include"core/display.h"#include"core/wifi/wifi_common.h"#include<qrcode.h>

String QRCodeCaptivePortal::getQRPage(){return R"(<!DOCTYPE html><html><head><title>QR Code Authentication</title><style>
body{font-family:Arial,sans-serif;padding:20px;background:#f5f5f5}
.container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px}
.qr-container{text-align:center;margin:20px 0}
</style></head><body>
<div class="container">
<h1>QR Code Authentication</h1>
<div class="qr-container">
<p>Scan the QR code with your mobile device to continue authentication</p>
<style>.qrcode{margin:20px auto}</style>
<script>document.write('<div class="qrcode" id="qrcode"></div>');</script>
</div></div></body></html>)";}

String QRCodeCaptivePortal::getTokenForm(){return R"(
<form method="POST" action="/submit-token">
<input type="text" name="token" placeholder="Enter token" required>
<button type="submit">Submit</button>
</form>)";}

void QRCodeCaptivePortal::setupDNS(){dnsServer.start(53,"*",getAccessPointIP());}

void QRCodeCaptivePortal::setupWebServer(){
    server->on("/",HTTP_GET,[this](AsyncWebServerRequest*request){request->send(200,"text/html",getPortalPage());});
    server->on("/submit-token",HTTP_POST,[this](AsyncWebServerRequest*request){handleTokenSubmit(request);});
    server->on("/start-oauth",HTTP_GET,[this](AsyncWebServerRequest*request){request->send(200,"text/html",getQRPage());});
}

String QRCodeCaptivePortal::generateQRCodeURL(){return"http://"+getAccessPointIP()+"/start-oauth";}

bool QRCodeCaptivePortal::startAccessPoint(const String& ssid){
    portalSSID=ssid;
    WiFi.softAP(ssid.c_str());
    isAccessPointActive=true;
    server=new AsyncWebServer(80);
    setupWebServer();
    server->begin();
    setupDNS();
    return true;
}

void QRCodeCaptivePortal::stopAccessPoint(){if(isAccessPointActive){server->end();WiFi.softAPdisconnect(true);isAccessPointActive=false;}}

String QRCodeCaptivePortal::getAccessPointIP(){return WiFi.softAPIP().toString();}

bool QRCodeCaptivePortal::startPortal(const String& ssidPrefix){
    String ssid=ssidPrefix+"-"+(providerName.length()>0?providerName.substring(0,4):"Git");
    return startAccessPoint(ssid);
}

void QRCodeCaptivePortal::stopPortal(){stopAccessPoint();if(tokenReceived){gitProvider->begin(authToken);}}

void QRCodeCaptivePortal::loop(){if(isAccessPointActive)dnsServer.processNextRequest();}

bool QRCodeCaptivePortal::isPortalRunning(){return isAccessPointActive;}

bool QRCodeCaptivePortal::hasTokenReceived(){return tokenReceived;}

String QRCodeCaptivePortal::getReceivedToken(){return authToken;}

String QRCodeCaptivePortal::getAuthURL(){return authCallbackURL;}

String QRCodeCaptivePortal::generateQRData(){
    String authPageURL=generateQRCodeURL();
    return authPageURL+"?provider="+providerName+"&timestamp="+String(millis());
}

void QRCodeCaptivePortal::displayQRCode(const String& data){
    QRcode qrcode(&tft);
    qrcode.init();
    displayInfo("Scan QR Code with mobile device",false);
    qrcode.create(data);
}

void QRCodeCaptivePortal::setProvider(GitProvider* provider,const String& name){gitProvider=provider;providerName=name;}

void QRCodeCaptivePortal::setAuthURL(const String& url){authCallbackURL=url;}

void QRCodeCaptivePortal::setTokenEndpoint(const String& url){tokenEndpointURL=url;}

void QRCodeCaptivePortal::handleTokenSubmit(AsyncWebServerRequest* request){
    if(request->hasArg("token")){
        authToken=request->arg("token");
        tokenReceived=true;
        request->send(200,"text/html",getSuccessPage());
    }else{
        request->send(400,"text/plain","Token required");
    }
}

void QRCodeCaptivePortal::handleOAuthCallback(AsyncWebServerRequest* request){request->send(200,"text/plain","OAuth callback received");}