#include "firmware_update.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/utils.h"
#include "core/settings.h"
#include "core/wifi/webInterface.h"
#include <globals.h>
#include <esp_task_wdt.h>

// Global variables for firmware update state
bool firmwareUpdateAvailable = false;
String pendingFirmwareFile = "";
uint32_t firmwareUpdateSize = 0;
String firmwareUpdateMD5 = "";
bool firmwareUpdatesEnabled = true;
File uploadFile;
String uploadPath = "";
FS updateFS = LittleFS;

/**********************************************************************
 **  Function: initFirmwareUpdate
 **  Initialize firmware update functionality
 **********************************************************************/
void initFirmwareUpdate() {
    Serial.println("Initializing firmware update system...");
    
    // Configure ArduinoOTA with custom settings
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }
        Serial.println("OTA Update starting: " + type);
        
        // Stop WebUI during OTA update
        if (isWebUIActive) {
            stopWebUi();
        }
        
        // Display update message
        #if defined(HAS_SCREEN)
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(2);
        tft.drawCentreString("Updating...", tftWidth / 2, tftHeight / 2 - 20, 1);
        tft.setTextSize(1);
        tft.drawCentreString("Please wait", tftWidth / 2, tftHeight / 2 + 10, 1);
        #endif
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA Update Complete");
        #if defined(HAS_SCREEN)
        tft.fillScreen(TFT_GREEN);
        tft.setTextColor(TFT_WHITE, TFT_GREEN);
        tft.setTextSize(2);
        tft.drawCentreString("Update Complete!", tftWidth / 2, tftHeight / 2 - 20, 1);
        tft.setTextSize(1);
        tft.drawCentreString("Rebooting...", tftWidth / 2, tftHeight / 2 + 10, 1);
        delay(3000);
        #endif
        ESP.restart();
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        #if defined(HAS_SCREEN)
        tft.fillRect(0, tftHeight / 2 + 30, (progress * tftWidth) / total, 5, TFT_BLUE);
        #endif
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
        
        #if defined(HAS_SCREEN)
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.setTextSize(2);
        tft.drawCentreString("Update Failed!", tftWidth / 2, tftHeight / 2 - 20, 1);
        tft.setTextSize(1);
        tft.drawCentreString("Error: " + String(error), tftWidth / 2, tftHeight / 2 + 10, 1);
        delay(5000);
        #endif
    });

    ArduinoOTA.setHostname("bruce-ota");
    ArduinoOTA.setPassword("bruce");
    ArduinoOTA.begin();
    Serial.println("OTA service started");
}

/**********************************************************************
 **  Function: handleFirmwareUpload
 **  Handle firmware file upload via HTTP
 **********************************************************************/
void handleFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!checkUserWebAuth(request)) return;
    
    if (!firmwareUpdatesEnabled) {
        request->send(403, "text/plain", "Firmware updates are disabled");
        return;
    }
    
    if (index == 0) {
        // Start of upload
        Serial.println("Starting firmware upload: " + filename);
        
        // Determine storage location
        String storage = request->arg("storage");
        if (storage == "SD" && setupSdCard()) {
            updateFS = SD;
            uploadPath = "/bruce_firmware.bin";
        } else {
            updateFS = LittleFS;
            uploadPath = "/bruce_firmware.bin";
        }
        
        // Remove existing file
        if (updateFS.exists(uploadPath)) {
            updateFS.remove(uploadPath);
        }
        
        // Create new file
        uploadFile = updateFS.open(uploadPath, "w");
        if (!uploadFile) {
            request->send(500, "text/plain", "Failed to create upload file");
            return;
        }
    }
    
    if (uploadFile && len > 0) {
        uploadFile.write(data, len);
    }
    
    if (final) {
        // End of upload
        if (uploadFile) {
            uploadFile.close();
            firmwareUpdateAvailable = true;
            pendingFirmwareFile = uploadPath;
            firmwareUpdateSize = updateFS.exists(uploadPath) ? updateFS.open(uploadPath).size() : 0;
            
            // Calculate MD5 if needed
            if (request->hasArg("md5")) {
                firmwareUpdateMD5 = request->arg("md5");
            } else {
                firmwareUpdateMD5 = calculateMD5(uploadPath);
            }
            
            Serial.println("Firmware upload complete");
            Serial.println("File: " + pendingFirmwareFile);
            Serial.println("Size: " + String(firmwareUpdateSize));
            Serial.println("MD5: " + firmwareUpdateMD5);
            
            request->send(200, "application/json", 
                "{\"status\":\"success\",\"message\":\"Firmware uploaded successfully\",\"size\":" + 
                String(firmwareUpdateSize) + ",\"md5\":\"" + firmwareUpdateMD5 + "\"}");
        } else {
            request->send(500, "text/plain", "Upload failed");
        }
        UNMOUNT_SD_CARD;
    }
}

/**********************************************************************
 **  Function: handleFirmwareFlash
 **  Handle firmware flashing request
 **********************************************************************/
void handleFirmwareFlash(AsyncWebServerRequest *request) {
    if (!checkUserWebAuth(request)) return;
    
    if (!firmwareUpdateAvailable) {
        request->send(400, "text/plain", "No firmware available to flash");
        return;
    }
    
    if (!firmwareUpdatesEnabled) {
        request->send(403, "text/plain", "Firmware updates are disabled");
        return;
    }
    
    // Verify firmware file before flashing
    if (!verifyFirmwareFile(pendingFirmwareFile, firmwareUpdateMD5)) {
        request->send(400, "text/plain", "Firmware verification failed");
        return;
    }
    
    // Schedule firmware update for next boot
    scheduleFirmwareUpdate(pendingFirmwareFile, firmwareUpdateSize, firmwareUpdateMD5);
    
    request->send(200, "application/json", 
        "{\"status\":\"success\",\"message\":\"Firmware scheduled for update on next reboot\"}");
}

/**********************************************************************
 **  Function: handleFirmwareStatus
 **  Handle firmware status request
 **********************************************************************/
void handleFirmwareStatus(AsyncWebServerRequest *request) {
    if (!checkUserWebAuth(request)) return;
    
    String status = "{\"status\":\"";
    
    if (firmwareUpdateAvailable) {
        status += "uploaded\",\"file\":\"" + pendingFirmwareFile + 
                  "\",\"size\":" + String(firmwareUpdateSize) +
                  ",\"md5\":\"" + firmwareUpdateMD5 + "\"}";
    } else {
        status += "none\"}";
    }
    
    request->send(200, "application/json", status);
}

/**********************************************************************
 **  Function: scheduleFirmwareUpdate
 **  Schedule firmware update for next boot
 **********************************************************************/
void scheduleFirmwareUpdate(const String& filename, uint32_t size, const String& md5) {
    File settingsFile = LittleFS.open("/bruce_firmware_update.json", "w");
    if (settingsFile) {
        settingsFile.println("{\"filename\":\"" + filename + "\",");
        settingsFile.println("\"size\":" + String(size) + ",");
        settingsFile.println("\"md5\":\"" + md5 + "\",");
        settingsFile.println("\"timestamp\":" + String(millis()) + "}");
        settingsFile.close();
        Serial.println("Firmware update scheduled for next boot");
    }
}

/**********************************************************************
 **  Function: checkPendingFirmware
 **  Check and process pending firmware on startup
 **********************************************************************/
void checkPendingFirmware() {
    File settingsFile = LittleFS.open("/bruce_firmware_update.json", "r");
    if (settingsFile) {
        String content = settingsFile.readString();
        settingsFile.close();
        
        // Parse JSON (simple parsing for filename, size, md5)
        int filenameStart = content.indexOf("\"filename\":\"") + 12;
        int filenameEnd = content.indexOf("\"", filenameStart);
        String filename = content.substring(filenameStart, filenameEnd);
        
        int sizeStart = content.indexOf("\"size\":") + 7;
        int sizeEnd = content.indexOf(",", sizeStart);
        uint32_t size = content.substring(sizeStart, sizeEnd).toInt();
        
        int md5Start = content.indexOf("\"md5\":\"") + 7;
        int md5End = content.indexOf("\"", md5Start);
        String md5 = content.substring(md5Start, md5End);
        
        if (filename.length() > 0 && updateFS.exists(filename)) {
            Serial.println("Found pending firmware update");
            Serial.println("File: " + filename);
            Serial.println("Size: " + String(size));
            Serial.println("MD5: " + md5);
            
            // Verify firmware before flashing
            if (verifyFirmwareFile(filename, md5)) {
                Serial.println("Firmware verified, starting update...");
                startFirmwareUpdate();
            } else {
                Serial.println("Firmware verification failed, clearing update");
                clearPendingFirmware();
            }
        } else {
            Serial.println("Pending firmware file not found");
            clearPendingFirmware();
        }
    }
}

/**********************************************************************
 **  Function: clearPendingFirmware
 **  Clear pending firmware update
 **********************************************************************/
void clearPendingFirmware() {
    LittleFS.remove("/bruce_firmware_update.json");
    if (updateFS.exists("/bruce_firmware.bin")) {
        updateFS.remove("/bruce_firmware.bin");
    }
    
    firmwareUpdateAvailable = false;
    pendingFirmwareFile = "";
    firmwareUpdateSize = 0;
    firmwareUpdateMD5 = "";
    
    Serial.println("Pending firmware update cleared");
}

/**********************************************************************
 **  Function: startFirmwareUpdate
 **  Start the firmware update process
 **********************************************************************/
void startFirmwareUpdate() {
    if (!firmwareUpdateAvailable || pendingFirmwareFile.length() == 0) {
        Serial.println("No firmware to update");
        return;
    }
    
    Serial.println("Starting firmware update process...");
    
    #if defined(HAS_SCREEN)
    tft.fillScreen(TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.setTextSize(2);
    tft.drawCentreString("Updating Firmware", tftWidth / 2, tftHeight / 2 - 30, 1);
    tft.setTextSize(1);
    tft.drawCentreString("Please wait...", tftWidth / 2, tftHeight / 2, 1);
    #endif
    
    // This would integrate with the actual OTA update mechanism
    // For now, we'll use ArduinoOTA's built-in functionality
    
    // Clear pending update first
    clearPendingFirmware();
    
    // Restart to complete update
    delay(2000);
    ESP.restart();
}

/**********************************************************************
 **  Function: verifyFirmwareFile
 **  Verify firmware file integrity
 **********************************************************************/
bool verifyFirmwareFile(const String& filepath, const String& expectedMD5) {
    if (!updateFS.exists(filepath)) {
        Serial.println("Firmware file not found: " + filepath);
        return false;
    }
    
    if (expectedMD5.length() > 0) {
        String calculatedMD5 = calculateMD5(filepath);
        if (calculatedMD5 != expectedMD5) {
            Serial.println("MD5 mismatch");
            Serial.println("Expected: " + expectedMD5);
            Serial.println("Calculated: " + calculatedMD5);
            return false;
        }
    }
    
    // Additional checks could be added here (file size, magic bytes, etc.)
    
    Serial.println("Firmware verification passed");
    return true;
}

/**********************************************************************
 **  Function: calculateMD5
 **  Calculate MD5 hash of a file
 **********************************************************************/
String calculateMD5(const String& filepath) {
    MD5Builder md5;
    md5.begin();
    
    File file = updateFS.open(filepath, "r");
    if (!file) {
        Serial.println("Failed to open file for MD5 calculation: " + filepath);
        return "";
    }
    
    uint8_t buffer[1024];
    while (file.available()) {
        size_t bytesRead = file.read(buffer, sizeof(buffer));
        md5.add((uint8_t*)buffer, bytesRead);
    }
    file.close();
    
    md5.calculate();
    return md5.toString();
}

/**********************************************************************
 **  Function: getFileSize
 **  Get file size
 **********************************************************************/
uint32_t getFileSize(const String& filepath) {
    File file = updateFS.open(filepath, "r");
    if (!file) return 0;
    uint32_t size = file.size();
    file.close();
    return size;
}

/**********************************************************************
 **  Function: formatBytes
 **  Format bytes in human readable format
 **********************************************************************/
String formatBytes(uint32_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < 1048576) return String(bytes / 1024.0) + " kB";
    else if (bytes < 1073741824) return String(bytes / 1048576.0) + " MB";
    else return String(bytes / 1073741824.0) + " GB";
}

/**********************************************************************
 **  Function: setFirmwareUpdateSettings
 **  Add firmware update settings to menu
 **********************************************************************/
void setFirmwareUpdateSettings() {
    options = {
        {"Enable Updates", enableFirmwareUpdates},
        {"Disable Updates", disableFirmwareUpdates},
        {"Status", [](void) {
            tft.fillScreen(bruceConfig.bgColor);
            setTftDisplay(0, 0);
            tft.setTextColor(bruceConfig.priColor);
            tft.setTextSize(FM);
            tft.println("Firmware Update Settings");
            tft.setTextSize(FP);
            
            if (firmwareUpdatesEnabled) {
                tft.setTextColor(TFT_GREEN);
                tft.println("Status: ENABLED");
            } else {
                tft.setTextColor(TFT_RED);
                tft.println("Status: DISABLED");
            }
            
            if (firmwareUpdateAvailable) {
                tft.setTextColor(TFT_YELLOW);
                tft.println("Update available!");
                tft.println("File: " + pendingFirmwareFile);
                tft.println("Size: " + formatBytes(firmwareUpdateSize));
                tft.println("MD5: " + firmwareUpdateMD5);
            } else {
                tft.setTextColor(bruceConfig.priColor);
                tft.println("No pending updates");
            }
            
            delay(5000);
        }},
        {"Clear Pending", clearPendingFirmware},
    };
    loopOptions(options);
}

/**********************************************************************
 **  Function: enableFirmwareUpdates
 **  Enable firmware updates
 **********************************************************************/
void enableFirmwareUpdates() {
    firmwareUpdatesEnabled = true;
    tft.fillScreen(TFT_GREEN);
    tft.setTextColor(TFT_WHITE, TFT_GREEN);
    tft.setTextSize(FM);
    tft.drawCentreString("Firmware Updates", tftWidth / 2, tftHeight / 2 - 20, 1);
    tft.setTextSize(FP);
    tft.drawCentreString("ENABLED", tftWidth / 2, tftHeight / 2 + 10, 1);
    delay(2000);
}

/**********************************************************************
 **  Function: disableFirmwareUpdates
 **  Disable firmware updates
 **********************************************************************/
void disableFirmwareUpdates() {
    firmwareUpdatesEnabled = false;
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextSize(FM);
    tft.drawCentreString("Firmware Updates", tftWidth / 2, tftHeight / 2 - 20, 1);
    tft.setTextSize(FP);
    tft.drawCentreString("DISABLED", tftWidth / 2, tftHeight / 2 + 10, 1);
    delay(2000);
}

/**********************************************************************
 **  Function: isFirmwareUpdatesEnabled
 **  Check if firmware updates are enabled
 **********************************************************************/
bool isFirmwareUpdatesEnabled() {
    return firmwareUpdatesEnabled;
}