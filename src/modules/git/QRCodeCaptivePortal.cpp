#include "QRCodeCaptivePortal.h"
#include "core/display.h"
#include "core/wifi/wifi_common.h"
#include "core/utils.h"
#include "core/settings.h"
#include <qrcode.h>

QRCodeCaptivePortal::QRCodeCaptivePortal(GitProvider* provider, const String& name) {
    gitProvider = provider;
    providerName = name;
    server = nullptr;
    isAccessPointActive = false;
    tokenReceived = false;
    
    // Set default URLs based on provider
    if (providerName == "GitLab") {
        authCallbackURL = "https://gitlab.com/oauth/authorize";
        tokenEndpointURL = "https://gitlab.com/oauth/token";
    } else if (providerName == "Gitee") {
        authCallbackURL = "https://gitee.com/oauth/authorize";
        tokenEndpointURL = "https://gitee.com/oauth/token";
    } else {
        authCallbackURL = "";
        tokenEndpointURL = "";
    }
}

QRCodeCaptivePortal::~QRCodeCaptivePortal() {
    stopPortal();
}

String QRCodeCaptivePortal::getPortalPage() {
    return R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Bruce Git Authentication</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0;
            padding: 20px;
            min-height: 100vh;
            color: #333;
        }
        .container { 
            max-width: 800px; 
            margin: 0 auto; 
            background: white;
            border-radius: 12px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.2);
            overflow: hidden;
        }
        .header {
            background: #2c3e50;
            color: white;
            padding: 20px;
            text-align: center;
        }
        .header h1 { 
            margin: 0; 
            font-size: 24px;
            font-weight: bold;
        }
        .header .provider { 
            margin-top: 5px; 
            font-size: 16px;
            opacity: 0.9;
        }
        .content { padding: 30px; }
        .section { 
            margin-bottom: 30px; 
            padding: 20px;
            background: #f8f9fa;
            border-radius: 8px;
            border-left: 4px solid #667eea;
        }
        .section h2 {
            color: #667eea;
            margin-top: 0;
            font-size: 18px;
        }
        .qr-container {
            text-align: center;
            padding: 20px;
            background: white;
            border-radius: 8px;
            margin: 20px 0;
        }
        .qr-code {
            display: inline-block;
            padding: 10px;
            background: white;
            border: 2px solid #ddd;
            border-radius: 8px;
        }
        .form-group {
            margin-bottom: 15px;
        }
        .form-group label {
            display: block;
            margin-bottom: 5px;
            font-weight: 600;
            color: #555;
        }
        .form-group input {
            width: 100%;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 6px;
            font-size: 16px;
            box-sizing: border-box;
        }
        .form-group input:focus {
            outline: none;
            border-color: #667eea;
        }
        .btn {
            background: #667eea;
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 16px;
            font-weight: 600;
            transition: background 0.3s;
        }
        .btn:hover {
            background: #5568d3;
        }
        .btn-secondary {
            background: #6c757d;
        }
        .btn-success {
            background: #28a745;
        }
        .instructions {
            background: #e3f2fd;
            padding: 15px;
            border-radius: 6px;
            margin-bottom: 20px;
        }
        .instructions ol {
            margin: 10px 0;
            padding-left: 20px;
        }
        .instructions li {
            margin-bottom: 8px;
        }
        .info-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin: 20px 0;
        }
        .info-item {
            background: white;
            padding: 15px;
            border-radius: 6px;
            border: 1px solid #ddd;
        }
        .info-item strong {
            color: #667eea;
        }
        .status {
            padding: 15px;
            border-radius: 6px;
            margin: 15px 0;
            font-weight: 600;
        }
        .status.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .status.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        @media (max-width: 768px) {
            .info-grid {
                grid-template-columns: 1fr;
            }
            .container {
                margin: 10px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Bruce ESP32 Git Authentication</h1>
            <div class="provider">)" + providerName + R"( Configuration</div>
        </div>
        <div class="content">
            <div class="section">
                <h2>📱 Access Point Information</h2>
                <div class="info-grid">
                    <div class="info-item">
                        <strong>WiFi Network:</strong><br>
                        <span id="ssid">)" + portalSSID + R"(</span>
                    </div>
                    <div class="info-item">
                        <strong>IP Address:</strong><br>
                        <span id="ip">)" + getAccessPointIP() + R"(</span>
                    </div>
                </div>
            </div>
            
            <div class="section">
                <h2>🖼️ Authentication Method 1: Manual Token</h2>
                <div class="instructions">
                    <strong>Steps:</strong>
                    <ol>
                        <li>Generate a Personal Access Token on )" + providerName + R"(</li>
                        <li>Copy the token (usually starts with 'glpat-' or similar)</li>
                        <li>Enter it in the form below and click Submit</li>
                    </ol>
                </div>
                <form action="/submit-token" method="POST">
                    <div class="form-group">
                        <label for="token">Personal Access Token:</label>
                        <input type="text" id="token" name="token" placeholder="Enter your " + providerName + " token" required>
                    </div>
                    <button type="submit" class="btn btn-success">Submit Token</button>
                </form>
            </div>
            
            <div class="section">
                <h2>🗽 Authentication Method 2: QR Code OAuth</h2>
                <div class="instructions">
                    <strong>Steps:</strong>
                    <ol>
                        <li>Scan the QR code below with your mobile device</li>
                        <li>Tap the link to open in your browser</li>
                        <li>Approve the OAuth application on )" + providerName + R"(</li>
                        <li>Copy the authorization code and return to this page</li>
                    </ol>
                </div>
                <div class="qr-container">
                    <div class="qr-code" id="qrcode">
                        <!-- QR Code will be inserted here -->
                    </div>
                </div>
                <div style="text-align: center; margin-top: 10px;">
                    <a href="/start-oauth" class="btn">Start OAuth Flow</a>
                </div>
            </div>
            
            <div class="section">
                <h2>🔗 Configuration Notes</h2>
                <div class="info-item">
                    <strong>Provider:</strong> )" + providerName + R"(<br>
                    <strong>Status:</strong> <span id="status">Waiting for authentication...</span>
                </div>
            </div>
        </div>
    </div>
</body>
</html>)";
}

String QRCodeCaptivePortal::getSuccessPage() {
    return R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Success - Bruce Git Authentication</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0;
            padding: 20px;
            min-height: 100vh;
        }
        .success-container {
            max-width: 600px;
            margin: 50px auto;
            background: white;
            border-radius: 12px;
            padding: 40px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.2);
            text-align: center;
        }
        .success-icon {
            font-size: 64px;
            color: #28a745;
            margin-bottom: 20px;
        }
        .success-title {
            color: #333;
            font-size: 28px;
            margin-bottom: 20px;
        }
        .success-message {
            color: #666;
            font-size: 18px;
            line-height: 1.6;
            margin-bottom: 30px;
        }
        .details {
            background: #f8f9fa;
            padding: 20px;
            border-radius: 8px;
            margin: 20px 0;
            text-align: left;
        }
        .details h3 {
            margin-top: 0;
            color: #667eea;
        }
    </style>
    <script>
        setTimeout(function() {
            window.location.href = '/';
        }, 5000);
    </script>
</head>
<body>
    <div class="success-container">
        <div class="success-icon">✔️</div>
        <h1 class="success-title">Authentication Successful!</h1>
        <div class="success-message">
            <p>Your " + providerName + " token has been successfully configured.</p>
            <p>You can now close this page and return to your Bruce ESP32 device.</p>
        </div>
        <div class="details">
            <h3>✅ What happens next:</h3>
            <p>1. The ESP32 will automatically disconnect this access point</p>
            <p>2. You will be returned to the main menu on the device</p>
            <p>3. All Git operations will use your new authentication</p>
        </div>
        <p style="color: #999; font-size: 14px;">This page will automatically redirect in 5 seconds...</p>
    </div>
</body>
</html>)";
}