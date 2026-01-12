#ifndef __FIRMWARE_UPDATE_H__
#define __FIRMWARE_UPDATE_H__

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <SD.h>
#include <MD5Builder.h>

// Global variables for firmware update state
extern bool firmwareUpdateAvailable;
extern String pendingFirmwareFile;
extern uint32_t firmwareUpdateSize;
extern String firmwareUpdateMD5;
extern bool firmwareUpdatesEnabled;

// Function declarations
void initFirmwareUpdate();
void startFirmwareUpdate();
void handleFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void handleFirmwareFlash(AsyncWebServerRequest *request);
void handleFirmwareStatus(AsyncWebServerRequest *request);
void checkPendingFirmware();
void clearPendingFirmware();
void scheduleFirmwareUpdate(const String& filename, uint32_t size, const String& md5);
bool verifyFirmwareFile(const String& filepath, const String& expectedMD5);
bool flashFirmware(const String& filepath);

// Settings functions
void setFirmwareUpdateSettings();
void enableFirmwareUpdates();
void disableFirmwareUpdates();
bool isFirmwareUpdatesEnabled();

// Utility functions
String calculateMD5(const String& filepath);
uint32_t getFileSize(const String& filepath);
String formatBytes(uint32_t bytes);

#endif // __FIRMWARE_UPDATE_H__